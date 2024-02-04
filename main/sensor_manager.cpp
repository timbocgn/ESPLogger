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
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "vindriktning.h"
#include "ESP32_SHT1x.h"
#include "cbme280_sensor.h"
#include "hm3300_sensor.h"

#include "sensor_manager.h"
#include "applogger.h"

////////////////////////////////////////////////////////////////////////////////////////

static const char *TAG = "SensorManager";

////////////////////////////////////////////////////////////////////////////////////////

SensorManager g_SensorManager;

////////////////////////////////////////////////////////////////////////////////////////

void SensorManager::ProcessMeasurements(void)
{
   // --- now measure on all sensors

    for (int i = 0; i < SENSOR_CONFIG_SENSOR_CNT; ++i)
    {
        if (!m_Sensors[i]->PerformMeasurement())
        {
             g_AppLogger.Log("Failed to perform a measurement on sensor %d", i+1);
             g_AppLogger.Log("Sensor Identification: %s", m_Sensors[i]->GetSensorDescriptionString().c_str());

        }
        else
        {
            ESP_LOGI(TAG, "Sensor %d: %s",i+1,m_Sensors[i]->GetSensorValueString().c_str());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////

#define CONFIG_SENS(num) {  gpio_num_t l_pins[] = SENSOR_CONFIG_SENSOR ## num ## _PINS;\
                            int l_data[]        = SENSOR_CONFIG_SENSOR ## num ## _DATA;\
                            ESP_LOGI(TAG, "Configure Sensor %d: %d %d %d %d %d / %d %d %d %d %d", num, l_pins[0],l_pins[1],l_pins[2],l_pins[3],l_pins[4],l_data[0],l_data[1],l_data[2],l_data[3],l_data[4]);\
                            m_Sensors[num-1] = new SENSOR_CONFIG_SENSOR ## num ## _CLASS;\
                            assert(m_Sensors[num-1] != NULL);\
                            ESP_LOGI(TAG, "Sensor pointer %p. Initialize...",m_Sensors[num-1]);\
                            if (!m_Sensors[num-1]->SetupSensor(l_pins,l_data))\
                            {\
                                ESP_LOGE(TAG, "...returned an error!");\
                                g_AppLogger.Log("Error initializing sensor %d (%s)",num,m_Sensors[num-1]->GetSensorDescriptionString().c_str());\
                            }\
                            ESP_LOGI(TAG, "Sensor pointer %p. ...finished.",m_Sensors[num-1]);\
                        }

////////////////////////////////////////////////////////////////////////////////////////

void SensorManager::InitSensors(void)
{
    // ---- this function creates sensor instances and configures them based on the defines
    //      in sensor_config.h - please change the configuration there!

    ESP_LOGI(TAG, "InitSensors");

    // ---- nasty macro magic - see definiton above

    #if SENSOR_CONFIG_SENSOR_CNT >= 1
            CONFIG_SENS(1)     
    #endif

    #if SENSOR_CONFIG_SENSOR_CNT >= 2
            CONFIG_SENS(2)     
    #endif

    #if SENSOR_CONFIG_SENSOR_CNT >= 3
            CONFIG_SENS(3)     
    #endif

    #if SENSOR_CONFIG_SENSOR_CNT >= 4
            CONFIG_SENS(4)     
    #endif

    #if SENSOR_CONFIG_SENSOR_CNT >= 5
        #error You need to add additional lines here!
    #endif

}