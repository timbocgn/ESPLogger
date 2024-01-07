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

#ifndef SENSOR_CONFIG_H
#define	SENSOR_CONFIG_H

// --- debug setting to stub the sensor readings

#define SENSOR_CONFIG_STUB_SENSORS 1

// --- how many sensors do we have?

#define SENSOR_CONFIG_SENSOR_CNT    2

// --- for each sensor, specify the class, the pin params and the data params. Meaning of these parameter
//     is defined in the sensor class implementation

#define SENSOR_CONFIG_SENSOR1_CLASS CVindriktning
#define SENSOR_CONFIG_SENSOR1_PINS  {GPIO_NUM_0,GPIO_NUM_NC,GPIO_NUM_NC,GPIO_NUM_NC,GPIO_NUM_NC,GPIO_NUM_NC}
#define SENSOR_CONFIG_SENSOR1_DATA  {0,1,2,3,4,5}

#define SENSOR_CONFIG_SENSOR2_CLASS SHT1x
#define SENSOR_CONFIG_SENSOR2_PINS  {GPIO_NUM_26,GPIO_NUM_25,GPIO_NUM_NC,GPIO_NUM_NC,GPIO_NUM_NC,GPIO_NUM_NC}
#define SENSOR_CONFIG_SENSOR2_DATA  {0,0,0,0,0,0}

// --- as of now up to 4 sensors are possible, just add some #if statements in the sensor manager if you need more

#endif