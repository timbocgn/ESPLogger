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
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "cJSON.h"
#include "esp_event.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
//#include "mdns.h"
#include "mqtt_client.h"

#include "config_manager.h"
#include "config_manager_defines.h"
#include "mqtt_manager.h"
#include "sensor_manager.h"
#include "applogger.h"

////////////////////////////////////////////////////////////////////////////////////////

static const char *TAG = "MqttManager";

////////////////////////////////////////////////////////////////////////////////////////

static void prvMqttTimerCallback( TimerHandle_t xExpiredTimer )
{
    MqttManager *l_mqttmgr;

    // --- Obtain the address of the info manager

    l_mqttmgr = (MqttManager *) pvTimerGetTimerID( xExpiredTimer );

    l_mqttmgr->ProcessCallback();
}

////////////////////////////////////////////////////////////////////////////////////////

void MqttManager::ProcessCallback(void)
{
    // ---- mqtt is off, do nothing

    if (!m_mqtt_enabled) return;

    // ---- decrease the counter and send message, when zero

    --m_delay_current;

    if (!m_delay_current)
    {
        // --- reload our counter

        m_delay_current = m_mqtt_delay;

        std::string l_topic  = g_ConfigManager.GetStringValue(CFMGR_MQTT_TOPIC);
        std::string l_server = g_ConfigManager.GetStringValue(CFMGR_MQTT_SERVER);

        // --- now loop over all sensors and send a message

        for (int l_senidx = 0; l_senidx < g_SensorManager.GetSensorCount(); ++l_senidx)
        {

            // ---- now ask the sensor for the values and create a JSON from that    

            cJSON *root = cJSON_CreateObject();
            g_SensorManager.GetSensor(l_senidx)->AddValuesToJSON_MQTT(root);

            const char *sys_info = cJSON_Print(root);
            
            char l_snum[5];

            std::string l_fulltopic = l_topic;
            l_fulltopic += "/sensor";
            l_fulltopic += itoa(l_senidx+1,l_snum,10);

            int l_err = esp_mqtt_client_publish(m_mqtt_hdl, l_fulltopic.c_str(), sys_info,0, 0,0);
            if (l_err == -1)
            {
                g_AppLogger.Log("Error sending MQTT message to topic '%s' to server '%s'",l_fulltopic.c_str(),l_server.c_str());
            }
            else
            {
                ESP_LOGI(TAG, "Successfully send mqtt message.");
            }

            free((void *)sys_info);
            cJSON_Delete(root);
        }


    }
}

////////////////////////////////////////////////////////////////////////////////////////

esp_err_t MqttManager::SetupMqtt(void)
{
    std::string l_server = g_ConfigManager.GetStringValue(CFMGR_MQTT_SERVER);

    esp_mqtt_client_config_t mqtt_cfg;
    memset(&mqtt_cfg,0,sizeof(esp_mqtt_client_config_t));

    mqtt_cfg.broker.address.uri = l_server.c_str();

    m_mqtt_hdl = esp_mqtt_client_init(&mqtt_cfg);
    if (!m_mqtt_hdl)
    {
        g_AppLogger.Log("Failed to connect to server '%s'",l_server.c_str());

        ESP_LOGE(TAG, "Error on esp_mqtt_client_init (%s)", l_server.c_str());
        return ESP_FAIL;
    }

    esp_err_t l_ee = esp_mqtt_client_start(m_mqtt_hdl);
    if (l_ee != ESP_OK)
    {
        g_AppLogger.Log("Failed to connect to server '%s' (%d)",l_server.c_str(),l_ee);

        ESP_LOGE(TAG, "Error on esp_mqtt_client_start (%s): %d", l_server.c_str(),l_ee);
        return l_ee;
    }

    // ---- timer stuff

    m_timer = xTimerCreate( "T1", 1000 / portTICK_PERIOD_MS, pdTRUE, (void *)this, prvMqttTimerCallback);
    xTimerStart( m_timer, 0 );

    return ESP_OK;

}

////////////////////////////////////////////////////////////////////////////////////////

void MqttManager::Shutdown(void)
{
    // --- stop any timer

    xTimerStop(m_timer,0);
    xTimerDelete(m_timer,0);
    m_timer = NULL;

    // --- stop MQTT client

    esp_mqtt_client_stop(m_mqtt_hdl);
    esp_mqtt_client_destroy(m_mqtt_hdl);
    m_mqtt_hdl = NULL;

}

////////////////////////////////////////////////////////////////////////////////////////

void MqttManager::ReadConfig(void)
{
    // ---- get some flag since we use them frequently

    m_mqtt_enabled = g_ConfigManager.GetIntValue(CFMGR_MQTT_ENABLE) == 1;
    m_mqtt_delay = g_ConfigManager.GetIntValue(CFMGR_MQTT_TIME);

    m_delay_current = m_mqtt_delay;
}

////////////////////////////////////////////////////////////////////////////////////////

esp_err_t MqttManager::InitManager(void)
{
    ESP_LOGI(TAG, "MqttManager::InitManager()");

    // --- read config vars

    ReadConfig();

    // ---- and setup the 

    if (m_mqtt_enabled)
    {
        return SetupMqtt();
    }

    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////

void MqttManager::UpdateConfig(void)
{
    // --- read config vars

    ReadConfig();   

    if (m_mqtt_enabled)
    {
        // --- we want MQTT

        if (m_mqtt_hdl)
        {
            g_AppLogger.Log("Updating MQTT server configuration");

            Shutdown();
            SetupMqtt();
        }
        else
        {
            // --- MQTT was switched on...so do the full party

            SetupMqtt();

            g_AppLogger.Log("Start MQTT client");
        }        
    }
    else
    {
        // --- we DONT want MQTT

        if (m_mqtt_hdl)
        {
            // ---- remove all

            Shutdown();
            g_AppLogger.Log("Shutdown MQTT client");
        }
        else
        {
            // --- do nothing
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////

MqttManager g_MqttManager;

////////////////////////////////////////////////////////////////////////////////////////