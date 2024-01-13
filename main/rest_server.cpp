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

#include <string.h>
#include <fcntl.h>
#include <string>

#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "sensor_manager.h"
#include "sensor_config.h"
#include "config_manager.h"
#include "config_manager_defines.h"
#include "mqtt_manager.h"
#include "applogger.h"

////////////////////////////////////////////////////////////////////////////////////////

static const char *REST_TAG = "esp-rest";

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (100*1024)

#define DEFAULT_SCAN_LIST_SIZE 128

////////////////////////////////////////////////////////////////////////////////////////

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

////////////////////////////////////////////////////////////////////////////////////////

std::string SanetizedString(const char *f_s)
{
    std::string html(f_s);
    std::string text;

    for(;;)
    {
        std::string::size_type  startpos;

        startpos = html.find('<');
        if(startpos == std::string::npos)
        {
            // no tags left only text!
            text += html;
            break;
        }

        // handle the text before the tag    
        if(0 != startpos)
        {
            text += html.substr(0, startpos);
            html = html.substr(startpos, html.size() - startpos);
            startpos = 0;
        }

        //  skip all the text in the html tag

        std::string::size_type endpos;
        for(endpos = startpos; endpos < html.size() && html[endpos] != '>'; ++endpos)
        {
            // since '>' can appear inside of an attribute string we need
            // to make sure we process it properly.
            if(html[endpos] == '"')
            {
                endpos++;
                while(endpos < html.size() && html[endpos] != '"')
                {
                    endpos++;
                }
            }
        }

        //  Handle text and end of html that has beginning of tag but not the end
        if(endpos == html.size())
        {
            html = html.substr(endpos, html.size() - endpos);
            break;
        }
        else
        {
            //  handle the entire tag
            endpos++;
            html = html.substr(endpos, html.size() - endpos);
        }
    }

    return text;
}

////////////////////////////////////////////////////////////////////////////////////////

inline bool CheckFileExtension(const char *filename, const char *ext)
{
    return strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0;
}

////////////////////////////////////////////////////////////////////////////////////////

// ---- Set HTTP response content type according to file extension 

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type        = "text/plain";
    const char *encoding    = NULL;

    if (CheckFileExtension(filepath, ".html")) 
    {
        type = "text/html";
    } 
    
    if (CheckFileExtension(filepath, ".js")) 
    {
        type     = "application/javascript";
        encoding = "gzip";
    } 
    
    if (CheckFileExtension(filepath, ".css")) 
    {
        type     = "text/css";
        encoding = "gzip";
    }
    
    if (CheckFileExtension(filepath, ".png")) {
        type = "image/png";
    } 
    
    if (CheckFileExtension(filepath, ".ico")) {
        type = "image/x-icon";
    } 
    
    if (CheckFileExtension(filepath, ".svg")) 
    {
        type = "text/xml";
    } 
    
    if (CheckFileExtension(filepath, ".gz")) {
        type = "text/xml";
    }
    
    // --- if an encoding is provide, set the respective header

    if (encoding)
    {
        httpd_resp_set_hdr(req,"Content-Encoding",encoding);
    }
       
    return httpd_resp_set_type(req, type);
}

////////////////////////////////////////////////////////////////////////////////////////

// ---- this is the default handler of the http server 

static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    // --- get the base file path (aka "/www" from the user context to our buffer to be the base of the file path

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    
    // --- check if the user is requesting "/" and append "/index.html" in this case as the root file
    
    if (req->uri[strlen(req->uri) - 1] == '/') 
    {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } 
    else 
    {
        // --- simply add the uri path from the http context to our base path
        strlcat(filepath, req->uri, sizeof(filepath));
    }

    // --- based on the original file name, set content type. Encoding is also set!

    set_content_type_from_file(req, filepath);

    // --- check for css and js files and add ".gz" we are serving those

    if (CheckFileExtension(filepath,".css") || CheckFileExtension(filepath,".js"))
    {
        strcat(filepath,".gz");
    }

    // --- now try to open the file

    int fd;
    fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) 
    {
        // --- this failed. We're assuming that this is a deep link attempt (or a wrong path)
        //     we handle this by defaulting to /index.html 

        strlcpy(filepath, rest_context->base_path, sizeof(filepath));
        strlcat(filepath, "/index.html", sizeof(filepath));
        fd = open(filepath, O_RDONLY, 0);
        if (fd == -1) 
        {
            // --- okay....if we cannot open this, there is something 

            ESP_LOGE(REST_TAG, "Failed to open file : %s. Serious file system issue!", filepath);

            // --- Respond with 500 Internal Server Error

            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file - appliance has a severe issue!");
            return ESP_FAIL;
        }

        set_content_type_from_file(req, filepath);
    }

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////

static esp_err_t sensor_data_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    // ---- find trailing backslash

    char *l_sensorint = strrchr(req->uri,'/');
    if (!l_sensorint)
    {
        ESP_LOGE(REST_TAG, "sensor_data_get_handler: Illegal URI");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Illegal URI");
        return ESP_FAIL;
    }

    // ---- convert to int

    int l_sensor_idx = atoi(l_sensorint+1);

    // ---- check if this is a valid index

    if (l_sensor_idx < 1 || l_sensor_idx > SENSOR_CONFIG_SENSOR_CNT)
    {
        ESP_LOGE(REST_TAG, "sensor_data_get_handler: Illegal sensor index %d",l_sensor_idx);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Illegal sensor index");
        return ESP_FAIL;
    }

    // ---- now ask the sensor for the values and create a JSON from that    

    cJSON *root = cJSON_CreateObject();
    g_SensorManager.GetSensor(l_sensor_idx-1)->AddValuesToJSON_API(root);

    // ---- convert to a string and send the JSON back
    
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    
    free((void *)sys_info);
    cJSON_Delete(root);
    
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////

static esp_err_t sensor_cnt_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    // ---- just return the sensor count
    
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(root, "cnt", SENSOR_CONFIG_SENSOR_CNT);
    
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    
    free((void *)sys_info);
    cJSON_Delete(root);
    
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////

static esp_err_t config_apscan_handler(httpd_req_t *req)
{    
    httpd_resp_set_type(req, "application/json");

    // ---- just return the sensor count
    
    cJSON *root = cJSON_CreateObject();
    
    cJSON *wifiscan_array = cJSON_AddArrayToObject(root,"WiFI_Scan");
    
    // --- initiate a wifi scan 

    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t *ap_info = new wifi_ap_record_t[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset((void *)ap_info, 0, sizeof(wifi_ap_record_t) * DEFAULT_SCAN_LIST_SIZE);

    wifi_scan_config_t l_wscf;

    l_wscf.ssid  = NULL;             
    l_wscf.bssid = NULL;             
    l_wscf.channel = 0;             
    l_wscf.show_hidden = false;           
    l_wscf.scan_type = WIFI_SCAN_TYPE_ACTIVE;  
    l_wscf.scan_time.active.min = 200;  
    l_wscf.scan_time.active.min = 1500;  

    esp_err_t l_err;

    l_err = esp_wifi_scan_start(&l_wscf, true);
    if ( l_err != ESP_OK )
    {
        ESP_LOGE(REST_TAG,"Error on esp_wifi_scan_start: %d",l_err);
        return l_err;
    }

    l_err = esp_wifi_scan_get_ap_records(&number, ap_info);
    if ( l_err != ESP_OK )
    {
        ESP_LOGE(REST_TAG,"Error on esp_wifi_scan_get_ap_records: %d",l_err);
        return l_err;
    }

    l_err = esp_wifi_scan_get_ap_num(&ap_count);
    if ( l_err != ESP_OK )
    {
        ESP_LOGE(REST_TAG,"Error on esp_wifi_scan_get_ap_num: %d",l_err);
        return l_err;
    }

    ESP_LOGI(REST_TAG, "Total APs scanned = %u", ap_count);
 
    for (int i = 0; i < ap_count; i++) 
    {
        cJSON *l_s = cJSON_CreateString((char *)ap_info[i].ssid);
        cJSON_AddItemToArray(wifiscan_array,l_s);

        ESP_LOGI(REST_TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(REST_TAG, "RSSI \t\t%d", ap_info[i].rssi);
    }

    delete [] ap_info;

    // --- now create JSON and send back
    
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    
    free((void *)sys_info);
    cJSON_Delete(root);
    
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////

static esp_err_t config_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    // ---- just return the sensor count
    
    cJSON *root = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(root, CFMGR_BOOTSTRAP_DONE, g_ConfigManager.GetIntValue(CFMGR_BOOTSTRAP_DONE));
    cJSON_AddStringToObject(root, CFMGR_WIFI_SSID,      g_ConfigManager.GetStringValue(CFMGR_WIFI_SSID).c_str());
    cJSON_AddStringToObject(root, CFMGR_WIFI_PASSWORD,  g_ConfigManager.GetStringValue(CFMGR_WIFI_PASSWORD).c_str());
    cJSON_AddStringToObject(root, CFMGR_DEVICE_NAME,    g_ConfigManager.GetStringValue(CFMGR_DEVICE_NAME).c_str());

    cJSON_AddStringToObject(root, CFMGR_MQTT_SERVER,    g_ConfigManager.GetStringValue(CFMGR_MQTT_SERVER).c_str());
    cJSON_AddStringToObject(root, CFMGR_MQTT_TOPIC,     g_ConfigManager.GetStringValue(CFMGR_MQTT_TOPIC).c_str());
    cJSON_AddNumberToObject(root, CFMGR_MQTT_TIME,      g_ConfigManager.GetIntValue(CFMGR_MQTT_TIME));
    cJSON_AddNumberToObject(root, CFMGR_MQTT_ENABLE,    g_ConfigManager.GetIntValue(CFMGR_MQTT_ENABLE));

    // --- now create JSON and send back
    
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    
    free((void *)sys_info);
    cJSON_Delete(root);
    
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////

static esp_err_t config_version_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    // ---- just return the sensor count
    
    cJSON *root = cJSON_CreateObject();
    
    // ---- get some chip infos

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    const esp_app_desc_t *l_appdesc = esp_app_get_description();
    
    cJSON_AddStringToObject(root, "idf_version", IDF_VER);
    cJSON_AddNumberToObject(root, "cpu_cores", chip_info.cores);

    const char *l_model;
    switch (chip_info.model)
    {
        case CHIP_ESP32:        l_model = "ESP32"; break;
        case CHIP_ESP32S2:      l_model = "ESP32-S2"; break;
        case CHIP_ESP32S3:      l_model = "ESP32-S3"; break;
        case CHIP_ESP32C3:      l_model = "ESP32-C3"; break;
        case CHIP_ESP32C2:      l_model = "ESP32-C2"; break;
        case CHIP_ESP32C6:      l_model = "ESP32-C6"; break;
        case CHIP_ESP32H2:      l_model = "ESP32-H2"; break; 
        case CHIP_POSIX_LINUX:  l_model = "Simulator"; break;
        default:                l_model = "unknown"; break;
    };

    cJSON_AddStringToObject(root, "esp_model", l_model);
    cJSON_AddStringToObject(root, "app_compile_time", l_appdesc->time);
    cJSON_AddStringToObject(root, "app_compile_date", l_appdesc->date);
    cJSON_AddStringToObject(root, "app_version", l_appdesc->version);

    uint8_t l_mac[6];
    ESP_ERROR_CHECK(esp_read_mac(l_mac, ESP_MAC_WIFI_STA));

    char l_macstr[32];
    snprintf(l_macstr,32,"%02X:%02X:%02X:%02X:%02X:%02X",MAC2STR(l_mac));
    cJSON_AddStringToObject(root, "mac_address", l_macstr);

    cJSON_AddNumberToObject(root, "free_heap",  esp_get_free_heap_size()/1024);
    cJSON_AddNumberToObject(root, "min_free_heap",  esp_get_minimum_free_heap_size()/1024);

    // --- now create JSON and send back
    
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    
    free((void *)sys_info);
    cJSON_Delete(root);
    
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////

static esp_err_t config_log_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    // ---- find end value 

    char *l_cntint_str = strstr(req->uri,"/cnt-");
    if (!l_cntint_str)
    {
        ESP_LOGE(REST_TAG, "config_log_handler: Illegal URI");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Illegal URI: cnt missing");
        return ESP_FAIL;
    }

    char *l_idxint_str = strstr(req->uri,"/idx-");
    if (!l_idxint_str)
    {
        ESP_LOGE(REST_TAG, "config_log_handler: Illegal URI");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Illegal URI: start index missing");
        return ESP_FAIL;
    }

    // ---- convert to int

    int l_cntint = atoi(l_cntint_str+5);
    int l_idxint = atoi(l_idxint_str+5);

    // ---- calculate the begin line in the AppLogger

    int l_begint;
    if (l_idxint == 0)
    {
        // --- special case: idx 0 means get the first line in the buffer

        l_begint = 0;
    }
    else
    {
        // --- usual case: id specified

        l_begint  = g_AppLogger.FindLineNumber(l_idxint);

        if (l_begint < 0)
        {
            ESP_LOGE(REST_TAG, "config_log_handler: Illegal URI");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Illegal URI: illegal log id specified");
            return ESP_FAIL;
        }
    }

    // --- handle special case for count: 0 means get all lines

    if (l_cntint == 0)
    {
        l_cntint = g_AppLogger.GetLineCount();
    }

    if (l_begint + l_cntint > g_AppLogger.GetLineCount())
    {
        l_cntint = g_AppLogger.GetLineCount()-l_begint;
    }
   
    // ---- some sanity checks

    if (l_cntint > g_AppLogger.GetLineCount())
    {
        ESP_LOGE(REST_TAG, "config_log_handler: Illegal URI");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Illegal URI: cnt exceeds log entries");
        return ESP_FAIL;
    }

    // ---- create the JSON response object
    
    cJSON *root = cJSON_CreateObject();

    // --- add number some interesting numeric values back to the JSON

    cJSON_AddNumberToObject(root, "log_count",      g_AppLogger.GetLineCount());
    cJSON_AddNumberToObject(root, "log_max_count",  APPLOGGER_MAX_NUMLINES);
    cJSON_AddNumberToObject(root, "count",          l_cntint);
    cJSON_AddNumberToObject(root, "startidx",       g_AppLogger.GetLineId(l_begint));

    // --- now add the log lines as an object of (id | text) pairs
   
    cJSON *l_loglines_array = cJSON_AddArrayToObject(root,"log_entries");
   
    for (int l_idx = 0; l_idx < l_cntint; l_idx++)
    {
        cJSON *l_itemObject = cJSON_CreateObject();

        cJSON_AddNumberToObject(l_itemObject, "id",     g_AppLogger.GetLineId(l_begint + l_idx));
        cJSON_AddStringToObject(l_itemObject, "text",   g_AppLogger.GetLineText(l_begint + l_idx));

        cJSON_AddItemToArray(l_loglines_array,l_itemObject);
    } 
    
    // --- now create JSON and send back
    
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    
    free((void *)sys_info);
    cJSON_Delete(root);
    
    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////////////

esp_err_t ProcessJsonString(cJSON *f_root,const char *f_name, bool f_onlysetifnotempty = false)
{
    cJSON *l_js = cJSON_GetObjectItem(f_root, f_name);

    if (!l_js)
    {
        ESP_LOGE(REST_TAG, "Config %s not found", f_name);
        return ESP_FAIL;
    }

    const char *l_s = l_js->valuestring;
    if (!l_s)
    {
        ESP_LOGE(REST_TAG, "Config %s is null", f_name);
        return ESP_FAIL;
        
    }

    // --- if flag is set and string is empty - do noting

    if (f_onlysetifnotempty && strlen(l_s) == 0)
    {
        ESP_LOGI(REST_TAG, "Config %s empty - not set!", f_name);
        return ESP_OK;
    }

    // --- now tell the config mgr

    g_ConfigManager.SetStringValue(f_name,SanetizedString(l_s));
    ESP_LOGI(REST_TAG, "Config %s, value '%s'", f_name,l_s);

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////////////

esp_err_t ProcessJsonInt(cJSON *f_root,const char *f_name)
{
    cJSON *l_js = cJSON_GetObjectItem(f_root, f_name);

    if (!l_js)
    {
        ESP_LOGE(REST_TAG, "Config %s not found", f_name);
        return ESP_FAIL;
    }

    // --- now tell the config mgr

    g_ConfigManager.SetIntValue(f_name,l_js->valueint);
    ESP_LOGI(REST_TAG, "Config %s, value '%d'", f_name,l_js->valueint);

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////////////

static esp_err_t config_post_handler(httpd_req_t *req)
{
    // --- check if we have enough space to process full post request

    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) 
    {
        // --- Respond with 500 Internal Server Error
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    // --- okay, now read the full request

    while (cur_len < total_len) 
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) 
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    // --- convert the JSON string to a JSON object

    cJSON *root = cJSON_Parse(buf);
    
    // --- set config to config mgr

    ProcessJsonString(root,CFMGR_WIFI_SSID);
    ProcessJsonString(root,CFMGR_WIFI_PASSWORD,true);
    ProcessJsonString(root,CFMGR_DEVICE_NAME);  
    ProcessJsonString(root,CFMGR_MQTT_SERVER);
    ProcessJsonString(root,CFMGR_MQTT_TOPIC);

    ProcessJsonInt(root,CFMGR_MQTT_TIME);
    ProcessJsonInt(root,CFMGR_MQTT_ENABLE);

    // --- flag now as bootstrap done
    
    g_ConfigManager.SetIntValue(CFMGR_BOOTSTRAP_DONE,1);

    // ---- tell the mqtt manager that the config might have changed

    g_MqttManager.UpdateConfig();

    // --- free up the JSON object

    cJSON_Delete(root);
    
    // --- send status to server

    httpd_resp_sendstr(req, "Post control value successfully");
    
    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////////////

portMUX_TYPE g_FirmwareUpload_Spinlock = portMUX_INITIALIZER_UNLOCKED;
bool g_FirmwareUpload_in_Progress = false;

static esp_err_t upload_firmware_handler(httpd_req_t *req)
{
    rest_server_context *l_ctx_ptr = (rest_server_context *)req->user_ctx;
    
    ESP_LOGI(REST_TAG,"upload_firmware_handler: content length %d",req->content_len);

    // ---- block the change of g_FirmwareUpload_in_Progress atomically as otherwise users could trick the
    //      code by hitting upload rapidly

    bool l_fault;
    taskENTER_CRITICAL(&g_FirmwareUpload_Spinlock);
    if (g_FirmwareUpload_in_Progress)
    {
        l_fault = true;
    }
    else
    {
        l_fault = false;
        g_FirmwareUpload_in_Progress = true;

    }
    taskEXIT_CRITICAL(&g_FirmwareUpload_Spinlock);

   // --- block multiple concurrent uploads

    if (l_fault)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Firmware upload is in progress");
        return ESP_FAIL;
    }

   // --- check if we have enough space to process full post request

    int total_len   = req->content_len;
    int cur_len     = 0;
    char *buf       = l_ctx_ptr->scratch;


    int received = 0;

    // --- okay, now read the full request

    while (cur_len < total_len) 
    {
        received = httpd_req_recv(req, buf, SCRATCH_BUFSIZE);
        if (received <= 0) 
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to upload firmware image");
            g_FirmwareUpload_in_Progress = false;
            
            return ESP_FAIL;
        }

        // --- update total length

        cur_len += received;

        // --- now do something with the received buffer

        //ESP_LOGI(REST_TAG,"Received chunk len %d total received %d",received,cur_len);
    }

    ESP_LOGI(REST_TAG,"Received total received %d",cur_len);

     // --- we're done now!

  g_FirmwareUpload_in_Progress = false;

    // --- send status to server

    httpd_resp_sendstr(req, "Post control value successfully");
    
    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////

httpd_uri_t apscan_uri_items[] =
{
    { "/api/v1/apscan", HTTP_GET, config_apscan_handler, NULL },
    { "/api/v1/version", HTTP_GET, config_version_handler, NULL },
    { "/api/v1/log/*", HTTP_GET, config_log_handler, NULL },
    { "/api/v1/config", HTTP_GET, config_get_handler, NULL },
    { "/api/v1/config", HTTP_POST, config_post_handler, NULL },
    { "/api/v1/sensorcnt", HTTP_GET, sensor_cnt_get_handler, NULL },
    { "/api/v1/air/*", HTTP_GET, sensor_data_get_handler, NULL },
    { "/upload", HTTP_POST, upload_firmware_handler, NULL },
    {  "/*", HTTP_GET, rest_common_get_handler, NULL },

};

////////////////////////////////////////////////////////////////////////////////////////

esp_err_t start_rest_server(const char *base_path)
{
    assert(base_path);

    rest_server_context_t *rest_context = (rest_server_context_t *)calloc(1, sizeof(rest_server_context_t));
    if (!rest_context)
    {
        ESP_LOGE(REST_TAG, "No memory for rest context");
        return ESP_FAIL;
    }
    
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

        // --- how many URIs to configure?

    const int l_num_config = sizeof(apscan_uri_items) / sizeof(httpd_uri_t);

    httpd_handle_t server = NULL;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_resp_headers = 16;
    config.max_uri_handlers = l_num_config;

    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");

    if (httpd_start(&server, &config) != ESP_OK)
    {
        ESP_LOGE(REST_TAG, "Start server failed");
    }



    ESP_LOGI(REST_TAG,"Found %d URI configurations",l_num_config);

    for (int i=0;i < l_num_config;++i)
    {
        apscan_uri_items[i].user_ctx = rest_context;

        ESP_LOGI(REST_TAG,"Register URI path '%s'",apscan_uri_items[i].uri);
        if (httpd_register_uri_handler(server, &apscan_uri_items[i]) != ESP_OK)
        {
            ESP_LOGE(REST_TAG,"Error registering URI handler '%s'",apscan_uri_items[i].uri);
            return ESP_ERR_NOT_SUPPORTED;
        }
    }

    return ESP_OK;
}
