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

#ifndef OTA_MANAGER_H_
#define	OTA_MANAGER_H_

////////////////////////////////////////////////////////////////////////////////////////

#include <string>
#include "sdkconfig.h"
#include "esp_ota_ops.h"

#define MAX_IMAGE_HEADER 2048

////////////////////////////////////////////////////////////////////////////////////////

class OTAManager
{
public:
    void logOTAInfo(void);

    bool StartOTATransfer(void);
    bool AddOTAChunk(char *f_bytes, int f_Length);
    bool FinishOTATransfer(void);


private:
    int         m_DataRead;
    bool        m_image_header_was_checked;
    
    const esp_partition_t *m_running;
    const esp_partition_t *m_update_partition;
    esp_ota_handle_t       m_update_handle;

    char                   m_imageheader[MAX_IMAGE_HEADER];
    char                  *m_imageheader_ptr;


};

////////////////////////////////////////////////////////////////////////////////////////


extern OTAManager g_OTAManager;

#endif