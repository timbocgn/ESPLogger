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

#ifndef CSENSOR_H_
#define	CSENSOR_H_

////////////////////////////////////////////////////////////////////////////////////////

#include <string>
#include "sdkconfig.h"
#include "cJSON.h"


////////////////////////////////////////////////////////////////////////////////////////

// --- this is an abstract base class forming the interface for all sensors

class CSensor
{
public:
    virtual std::string GetSensorValueString(void) = 0;
    virtual std::string GetSensorDescriptionString(void) = 0;

 	virtual bool PerformMeasurement(void) = 0;
 	virtual bool SetupSensor(gpio_num_t *f_pins,int *f_data) = 0;
    virtual void AddValuesToJSON_MQTT(cJSON *f_root) = 0;
    virtual void AddValuesToJSON_API(cJSON *f_root) = 0;


};

#endif