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

#ifndef MQTT_MANAGER_H_
#define	MQTT_MANAGER_H_

////////////////////////////////////////////////////////////////////////////////////////

#include "sdkconfig.h"
#include "freertos/timers.h"
#include "mqtt_client.h"

////////////////////////////////////////////////////////////////////////////////////////

class MqttManager
{

public:
    esp_err_t InitManager(void);

    void UpdateConfig(void);
    void ProcessCallback(void);

private:

    esp_err_t SetupMqtt(void);
    void Shutdown(void);
    void ReadConfig(void);

    TimerHandle_t   m_timer;
    bool            m_mqtt_enabled;
    int             m_mqtt_delay;
    int             m_delay_current;

    esp_mqtt_client_handle_t m_mqtt_hdl = NULL;

};

////////////////////////////////////////////////////////////////////////////////////////


extern MqttManager g_MqttManager;


#endif
