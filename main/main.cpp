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

#include <stdio.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "driver/sdmmc_host.h"
#include "driver/gpio.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include "vindriktning.h"
#include "sensor_manager.h"
#include "config_manager.h"
#include "infomanager.h"
#include "config_manager_defines.h"
#include "mqtt_manager.h"
#include "applogger.h"
#include "esp_partition.h"
#include "ota_manager.h"

#include "timestamp.h"

#define CONFIG_EXAMPLE_WEB_MOUNT_POINT "/www"

////////////////////////////////////////////////////////////////////////////////////////

// --- these are our sensors (wrapped by a manager doing some calculation)

///SensorManager m_Sensors[CONFIG_TEMP_SENSOR_CNT];

////////////////////////////////////////////////////////////////////////////////////////

static const char *TAG = "ESPLoggerMain";

////////////////////////////////////////////////////////////////////////////////////////

#define GOT_IPV4_BIT BIT(0)
#define GOT_IPV6_BIT BIT(1)

//static EventGroupHandle_t s_connect_event_group;
//static ip4_addr_t s_ip_addr;
//static ip6_addr_t s_ipv6_addr;

////////////////////////////////////////////////////////////////////////////////////////

static void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi got ip...");
    
    // --- now we are connected

    g_InfoManager.SetMode(InfoMode_Connected);
}

////////////////////////////////////////////////////////////////////////////////////////

static void on_got_ipv6(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi got ipv6...");

    //ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
    //memcpy(&s_ipv6_addr, &event->ip6_info.ip, sizeof(s_ipv6_addr));
    //xEventGroupSetBits(s_connect_event_group, GOT_IPV6_BIT);
}

////////////////////////////////////////////////////////////////////////////////////////

static void on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    ESP_ERROR_CHECK(esp_wifi_connect());
}

////////////////////////////////////////////////////////////////////////////////////////

static void on_wifi_connect(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "on_wifi_connect...");
    //tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
}

////////////////////////////////////////////////////////////////////////////////////////

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) 
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);

    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) 
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}


////////////////////////////////////////////////////////////////////////////////////////

// --- note: at least 8 chars!

#define DEFAULT_WIFI_PASS       "let-me-in-1234"

static void start_wifi_client()
{
    if (g_ConfigManager.GetIntValue(CFMGR_BOOTSTRAP_DONE) == 0)
    {   
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        uint8_t l_mac[6];

        // --- device NOT configured: create AP

        ESP_ERROR_CHECK(esp_read_mac(l_mac, ESP_MAC_WIFI_SOFTAP));

        char l_macstr[64];
        char l_prodname[32];

        strncpy(l_prodname,CONFIG_PRODUCT_NAME,32);

        snprintf(l_macstr,64,"%s-%02x%02x%02x%02x%02x%02x",l_prodname,MAC2STR(l_mac));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

        wifi_config_t wifi_config;
        memset(&wifi_config,0,sizeof(wifi_config_t));
        
        strncpy((char *)wifi_config.ap.ssid,l_macstr,32);
        strncpy((char *)wifi_config.ap.password,DEFAULT_WIFI_PASS,64);
        
        wifi_config.ap.ssid_len = (uint8_t)strlen(l_macstr);
        wifi_config.ap.max_connection = 5;
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

        if (strlen(DEFAULT_WIFI_PASS) == 0) 
        {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }
        
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s",(char *)wifi_config.ap.ssid);
    }
    else
    {
        // --- device configured: connecting to WLAN

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connect, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_GOT_IP6, &on_got_ipv6, NULL));

        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

        wifi_config_t wifi_config;
        memset(&wifi_config,0,sizeof(wifi_config_t));

        // --- get config from config manager

        std::string l_ssid      = g_ConfigManager.GetStringValue(CFMGR_WIFI_SSID);
        std::string l_wlanpwd   = g_ConfigManager.GetStringValue(CFMGR_WIFI_PASSWORD);

        strncpy((char *)wifi_config.sta.ssid,l_ssid.c_str(),32);
        strncpy((char *)wifi_config.sta.password,l_wlanpwd.c_str(),64);

        ESP_LOGI(TAG, "Connecting to '%s'...", wifi_config.sta.ssid);

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
}

////////////////////////////////////////////////////////////////////////////////////////

/*

static void stop_wifi_client()
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_GOT_IP6, &on_got_ipv6));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &on_wifi_connect));

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}
*/

////////////////////////////////////////////////////////////////////////////////////////

esp_err_t init_fs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_EXAMPLE_WEB_MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}


////////////////////////////////////////////////////////////////////////////////////////

void init_sensors(void)
{
    g_SensorManager.InitSensors();
}

////////////////////////////////////////////////////////////////////////////////////////

void ProcessMeasurements(void)
{
    g_SensorManager.ProcessMeasurements();
}

////////////////////////////////////////////////////////////////////////////////////////

esp_err_t start_rest_server(const char *base_path);

// --- need this now since it is a cpp code file

extern "C" 
{
    void app_main();
}

// --- the main starting point

void app_main()
{
     //vTaskDelay(5000 / portTICK_PERIOD_MS);
     
    // ---- init flash lib

    ESP_ERROR_CHECK(nvs_flash_init());

    // ---- setup SPIFFS  (as early as possible for the app logger)

    ESP_LOGI(TAG, "Initialize file system");
    ESP_ERROR_CHECK(init_fs());

    // ---- init app logger (which needs the SPIFFS)

    ESP_ERROR_CHECK(g_AppLogger.InitAppLogger());    

    // --- start the info manager

    g_InfoManager.InitManager();

    // ---- print some OTA stuff

    g_OTAManager.logOTAInfo();

    // ---- init config storage

    ESP_LOGI(TAG,"Initialize ConfigManager");

    ESP_ERROR_CHECK(g_ConfigManager.InitConfigManager());

    //ESP_LOGI(TAG,"Config %s -> %d",CFMGR_BOOTSTRAP_DONE, g_ConfigManager.GetIntValue(CFMGR_BOOTSTRAP_DONE));

    // --- check for the user pressing the bootstrap key (on startup)

    if (g_InfoManager.IsBootstrapActivated())  
    {
        g_AppLogger.Log("Bootstrap activated by user. Reset bootstrap flag and reboot!");

        if (g_ConfigManager.GetIntValue(CFMGR_BOOTSTRAP_DONE) == 0)
        {
            g_AppLogger.Log("Device not bootstrapped - ignore!");
        }
        else
        {
            g_AppLogger.Log("Reset bootstrap flag and reboot!");

            // --- set the flash flag to "not configured"

            g_ConfigManager.SetIntValue(CFMGR_BOOTSTRAP_DONE,0);

            // --- be sure to let the flash write the stuff

            nvs_flash_deinit();

            // --- and reboot

            esp_restart();
        }
        
    }     

    // ---- use our new cmake hack to get a more precise compile time. Good for OTA testing. 

    g_AppLogger.Log("App compile time: " _TIMEZ_);

    // ---- init netif lib

    esp_netif_init();
  
    // ---- create default event loop

    ESP_ERROR_CHECK(esp_event_loop_create_default());


    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
 
    // ---- check for bootstrap mode

    if (g_ConfigManager.GetIntValue(CFMGR_BOOTSTRAP_DONE) == 0)
    {
        g_AppLogger.Log("Device not bootstrapped - enter bootstrap mode");

        g_InfoManager.SetMode(InfoMode_Bootstrap);

        // ---- default values

        if (g_ConfigManager.GetStringValue(CFMGR_DEVICE_NAME).length() == 0)
            g_ConfigManager.SetStringValue(CFMGR_DEVICE_NAME,"IoTDevice");

        if (g_ConfigManager.GetStringValue(CFMGR_MQTT_SERVER).length() == 0)
            g_ConfigManager.SetStringValue(CFMGR_MQTT_SERVER,"mqtt://192.168.1.60");
        
        if (g_ConfigManager.GetStringValue(CFMGR_MQTT_TOPIC).length() == 0)
            g_ConfigManager.SetStringValue(CFMGR_MQTT_TOPIC,"mytopic/templogger");

        if (g_ConfigManager.GetIntValue(CFMGR_MQTT_TIME) == 0)
            g_ConfigManager.SetIntValue(CFMGR_MQTT_TIME,60);
    }
    else
    {
        g_InfoManager.SetMode(InfoMode_WaitToConnect);
        g_AppLogger.Log("Device configuration found. Let's rock it.");
    }

    // ---- setup wifi client or hotspot

    g_AppLogger.Log("Start WIFI client");
    start_wifi_client();
  
    // ---- initialize all the sensors

    g_AppLogger.Log("Initialize sensor system");
    init_sensors();

    // ---- now start the web server

    g_AppLogger.Log("Start web server");
    start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT);

    // --- start the mqtt manager

    g_AppLogger.Log("Start MQTT manager");
    g_MqttManager.InitManager();

    // --- just before we enter the main loop - get all GPIO states to the console

    gpio_dump_io_configuration(stdout, SOC_GPIO_VALID_GPIO_MASK);

	// ---- main measurement loop

    ESP_LOGI(TAG, "Enter the main program loop");
    while(1) 
    {
        
        heap_caps_check_integrity_all(true);

		ProcessMeasurements();
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
