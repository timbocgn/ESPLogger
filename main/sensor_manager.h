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

#ifndef SENSOR_MANAGER_H_
#define	SENSOR_MANAGER_H_

////////////////////////////////////////////////////////////////////////////////////////

#include <string>

#include "sensor_config.h"


#include "sdkconfig.h"
#include "csensor.h"

////////////////////////////////////////////////////////////////////////////////////////

class SensorManager
{
public:

    // --- action functions

    void ProcessMeasurements(void);
    void InitSensors(void);

    // --- low level getters

    CSensor *GetSensor(int f_idx)
    {
        assert(f_idx < SENSOR_CONFIG_SENSOR_CNT);
        return m_Sensors[f_idx];
    }

    int GetSensorCount(void) const
    {
        return SENSOR_CONFIG_SENSOR_CNT;
    }

private:
    CSensor *m_Sensors[SENSOR_CONFIG_SENSOR_CNT];
};

////////////////////////////////////////////////////////////////////////////////////////


extern SensorManager g_SensorManager;


#endif