/*
    --------------------------------------------------------------------------------

    way2.net ESPLogger       
    
    ESP32 based IoT Device for various sensor logging featuring an MQTT client and 
    REST API access. 
    
    --------------------------------------------------------------------------------

    Copyright (c) 2024 Tim Hagemann / way2.net Services

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
    --------------------------------------------------------------------------------
*/

///////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"

#include "OTA_manager.h"
#include "applogger.h"

////////////////////////////////////////////////////////////////////////////////////////

static const char *TAG = "OTAManager";

////////////////////////////////////////////////////////////////////////////////////////

OTAManager g_OTAManager;

////////////////////////////////////////////////////////////////////////////////////////

void OTAManager::logOTAInfo(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    g_AppLogger.Log("Running firmware image from partition %s",running->label);
    g_AppLogger.Log("Next update partition will be %s",update_partition->label);
    
}

////////////////////////////////////////////////////////////////////////////////////////

bool OTAManager::StartOTATransfer(void)
{   
    // --- reset our byte counter

    m_DataRead                 = 0;
    m_image_header_was_checked = false;
    m_imageheader_ptr          = m_imageheader;
    m_update_handle            = 0;

    // --- get some current partition info

    m_running           = esp_ota_get_running_partition();
    m_update_partition  = esp_ota_get_next_update_partition(NULL);

    g_AppLogger.Log("OTA Update: running partition %s",m_running->label);
    g_AppLogger.Log("            update partition %s",m_update_partition->label);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////

bool OTAManager::AddOTAChunk(char *f_bytes, int f_Length)
{
    //ESP_LOG_BUFFER_HEXDUMP(TAG,f_bytes,f_Length,ESP_LOG_INFO);

    // ---- check the header of the image first

    if (m_image_header_was_checked == false) 
    {
        // --- copy the chunk into our image header buffer. It might be that the 
        //     first chunk is smaller than the image header, so we got to assemble it (this is 'wrong' in the IDF
        //     OTA example)

        // --- ensure that we do not write past the image header buffer 

        int l_bytes_left_in_buf = sizeof(m_imageheader) - m_DataRead;

        int l_maxcopy = f_Length;
        if (f_Length>l_bytes_left_in_buf) l_maxcopy = l_bytes_left_in_buf;

        // --- copy the next chunk to the image header buffer

        ESP_LOGI(TAG,"length %d data read %d left in buf %d maxcopy %d m_imageheader_ptr %p",f_Length,m_DataRead,l_bytes_left_in_buf,l_maxcopy,m_imageheader_ptr);

        memcpy(m_imageheader_ptr,f_bytes,l_maxcopy);
        m_imageheader_ptr += l_maxcopy;

        // --- do we have enough bytes for the image header?

        if (m_DataRead+f_Length >= sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
        {

            // --- check if this is a valid image

            if (((esp_image_header_t *)m_imageheader)->magic != ESP_IMAGE_HEADER_MAGIC)
            {
                g_AppLogger.Log("Invalid image provided in firmware upgrade");
                return false;                
            }

            // --- extract the app info from the image

            esp_app_desc_t new_app_info;
            memcpy(&new_app_info, m_imageheader + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t), sizeof(esp_app_desc_t));

            // --- check firmware versions

            ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

            esp_app_desc_t running_app_info;
            if (esp_ota_get_partition_description(m_running, &running_app_info) == ESP_OK) 
            {
                ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
            }

            const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
            esp_app_desc_t invalid_app_info;
            if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) 
            {
                ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
            }

            // --- check provided version with last invalid partition

            if (last_invalid_app != NULL) 
            {
                if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) 
                {
                    ESP_LOGW(TAG, "New version is the same as invalid version.");
                    ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                    ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");

                    return false;
                }
            }

            // --- at this point we can start the real OTA code

            esp_err_t err = esp_ota_begin(m_update_partition, OTA_WITH_SEQUENTIAL_WRITES, &m_update_handle);
            if (err != ESP_OK) 
            {
                ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                if (m_update_handle) esp_ota_abort(m_update_handle);
                return false;
            }

            ESP_LOGI(TAG, "esp_ota_begin succeeded");
                
            /*
            // --- check if provided version and running version are the same

            if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) 
            {
                ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                return false;
            }*/

            m_image_header_was_checked = true;
        }
        
    }

    // ---- write the chunk to the flash

    if (m_update_handle)
    {
        esp_err_t err = esp_ota_write( m_update_handle, (const void *)f_bytes, f_Length);
        if (err != ESP_OK) 
        {
            ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
            if (m_update_handle) esp_ota_abort(m_update_handle);
            return false;
        }

    }
    // ---- remember how much we got already

    m_DataRead += f_Length;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////

bool OTAManager::FinishOTATransfer(void)
{
    esp_err_t err = esp_ota_end(m_update_handle);
    if (err != ESP_OK) 
    {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) 
        {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            return false;
        } 
        else 
        {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
            return false;
        }
    }

    err = esp_ota_set_boot_partition(m_update_partition);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "Prepare to restart system (10 seconds)!");

    vTaskDelay(10000 / portTICK_PERIOD_MS);

    esp_restart();

    // ---- we will never reach this point
    
    while (1);

    return true;
}