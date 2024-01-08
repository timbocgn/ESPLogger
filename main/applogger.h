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

#ifndef APPLOGGER_H
#define	APPLOGGER_H

///////////////////////////////////////////////////////////////////////////////////////

#include "sdkconfig.h"
#include <string>

////////////////////////////////////////////////////////////////////////////////////////

#define APPLOGGER_MAX_LINE_LEN 80
#define APPLOGGER_MAX_NUMLINES 100

////////////////////////////////////////////////////////////////////////////////////////

class CAppLogger
{
public:

    CAppLogger()
    {
        m_NumLines  = 0;
        m_NextLine  = 0;

        m_NextLogIdx = 1;

        for (int i = 0;i < APPLOGGER_MAX_NUMLINES;++i)
        {
            m_IdLogBuffer[i].m_LogIndex = 0;
            m_IdLogBuffer[i].m_Text = NULL;
        }
    }

    // --- init functions

    esp_err_t InitAppLogger(void);
    void ShutdownAppLogger(void);

    // --- logger functions

    void Log(const std::string& sMessage);
    void Log(const char *format, ... );

    CAppLogger& operator<<(const std::string& sMessage );

    const char *GetLineText(int f_line);
    uint32_t GetLineId(int f_line);
    int FindLineNumber(uint32_t f_id);

    int  GetLineCount(void);

private:

    typedef struct LogEntry
    {
        uint32_t    m_LogIndex;
        char        *m_Text;
    } LogEntry_t;

    // --- don't copy this object

    CAppLogger& operator=(const CAppLogger& )
    { 
    	return *this;
    };
    
    // --- simple helper function

    void SetLine(int f_line,uint32_t f_idx, const char *f_s);
    void AddLine(const char *f_s); 
    void RawAddLine(const char *f_s); 

    // --- our buffer 

    int         m_NumLines;
    int         m_NextLine;

    uint32_t    m_NextLogIdx;

    LogEntry_t m_IdLogBuffer[APPLOGGER_MAX_NUMLINES];
};

////////////////////////////////////////////////////////////////////////////////////////


extern CAppLogger g_AppLogger;

#endif