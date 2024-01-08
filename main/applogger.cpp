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
#include <ctime>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_log.h"
#include "applogger.h"

////////////////////////////////////////////////////////////////////////////////////////

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////

static const char *TAG = "AppLogger";

////////////////////////////////////////////////////////////////////////////////////////

CAppLogger g_AppLogger;

////////////////////////////////////////////////////////////////////////////////////////

esp_err_t CAppLogger::InitAppLogger(void)
{
    ESP_LOGI(TAG, "InitAppLogger()");

    return ESP_OK;
}

////////////////////////////////////////////////////////////////////////////////////////

void CAppLogger::ShutdownAppLogger(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////

void CAppLogger::SetLine(int f_line,uint32_t f_idx, const char *f_s)
{
    assert(f_line<APPLOGGER_MAX_NUMLINES);

    m_IdLogBuffer[f_line].m_LogIndex = f_idx;
    if  (m_IdLogBuffer[f_line].m_Text) delete [] m_IdLogBuffer[f_line].m_Text;

    m_IdLogBuffer[f_line].m_Text = new char [strlen(f_s)];
    if (!m_IdLogBuffer[f_line].m_Text)
    {
        ESP_LOGE(TAG, "Failed to allocate log line buffer!");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    
    // --- simply copy the string to the respective buffer position

    strcpy(m_IdLogBuffer[f_line].m_Text,f_s);
}

////////////////////////////////////////////////////////////////////////////////////////

int CAppLogger::GetLineCount(void)
{
    return m_NumLines;
}

////////////////////////////////////////////////////////////////////////////////////////

void CAppLogger::AddLine(const char *f_s)
{
    //char l_buf[APPLOGGER_MAX_LINE_LEN];
    //snprintf(l_buf,APPLOGGER_MAX_LINE_LEN,"%s:%s",CurrentDateTime().c_str(),f_s);

    RawAddLine(f_s);
}

////////////////////////////////////////////////////////////////////////////////////////

void CAppLogger::RawAddLine(const char *f_s)
{
    // --- add string and index to line buffer

    SetLine(m_NextLine,m_NextLogIdx,f_s);

 

    //ESP_LOGI(TAG, "AddLine in: nextline %d numline %d",m_NextLine,m_NumLines);
    
    m_NextLine++;
    if (m_NextLine >= APPLOGGER_MAX_NUMLINES) m_NextLine = 0;
    if (m_NumLines < APPLOGGER_MAX_NUMLINES) m_NumLines++;

   // --- advance log index
    
    m_NextLogIdx++;
    if (m_NextLogIdx > 1000000) m_NextLogIdx = 1;

    //ESP_LOGI(TAG, "AddLine out: nextline %d numline %d",m_NextLine,m_NumLines);
}

////////////////////////////////////////////////////////////////////////////////////////

int CAppLogger::FindLineNumber(uint32_t f_id)
{
    for (int i=0; i < APPLOGGER_MAX_NUMLINES; ++i)
        if (m_IdLogBuffer[i].m_LogIndex == f_id) return i;

    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////

const char *CAppLogger::GetLineText(int f_line)
{
     //ESP_LOGI(TAG, "GetLine in: nextline %d numline %d",m_NextLine,m_NumLines);
  // --- check, if the line is available

    if (f_line >= m_NumLines) return NULL;

    int l_rawidx; 
    if (m_NumLines < APPLOGGER_MAX_NUMLINES)
    {
        l_rawidx = f_line;
    }
    else
    {
        l_rawidx = f_line + m_NextLine; 
        if (l_rawidx >= APPLOGGER_MAX_NUMLINES) l_rawidx -= APPLOGGER_MAX_NUMLINES;
    }

    //ESP_LOGI(TAG, "GetLine out: rawidx %d nextline %d numline %d",l_rawidx,m_NextLine,m_NumLines);

    return m_IdLogBuffer[l_rawidx].m_Text;
}

////////////////////////////////////////////////////////////////////////////////////////

uint32_t CAppLogger::GetLineId(int f_line)
{
     //ESP_LOGI(TAG, "GetLine in: nextline %d numline %d",m_NextLine,m_NumLines);
  // --- check, if the line is available

    if (f_line >= m_NumLines) return 0;

    int l_rawidx; 
    if (m_NumLines < APPLOGGER_MAX_NUMLINES)
    {
        l_rawidx = f_line;
    }
    else
    {
        l_rawidx = f_line + m_NextLine; 
        if (l_rawidx >= APPLOGGER_MAX_NUMLINES) l_rawidx -= APPLOGGER_MAX_NUMLINES;
    }

    //ESP_LOGI(TAG, "GetLine out: rawidx %d nextline %d numline %d",l_rawidx,m_NextLine,m_NumLines);

    return m_IdLogBuffer[l_rawidx].m_LogIndex;
}

////////////////////////////////////////////////////////////////////////////////////////

void CAppLogger::Log( const char * format, ... )
{
    char sMessage[APPLOGGER_MAX_LINE_LEN];
    va_list args;

    va_start (args, format);
    vsprintf (sMessage,format, args);

    AddLine(sMessage);

    va_end (args);
}
 
////////////////////////////////////////////////////////////////////////////////////////

void CAppLogger::Log( const string& sMessage )
{
    AddLine(sMessage.c_str());
}
 
////////////////////////////////////////////////////////////////////////////////////////

CAppLogger& CAppLogger::operator<<(const string& sMessage )
{
    Log(sMessage);
    return *this;
}