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

////////////////////////////////////////////////////////////////////////////////////////

#ifndef ESP32_hm3300_sensor_H_
#define	ESP32_hm3300_sensor_H_

////////////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>

#include "driver/i2c.h"
#include "csensor.h"

#include "bme280.h"

////////////////////////////////////////////////////////////////////////////////////////
/*

Data1~Data2 reserved
Data3~Data4 Sensor number
Data5~Data6 PM1.0 concentration（CF=1 ，Standard particulate）unit μg/ m3
Data7~Data8 PM2.5 concentration（CF=1 ，Standard particulate）unit μg/ m3
Data9~Data10 PM10 concentration（CF=1 ，Standard particulate）unit μg/ m3
Data11~Data12 PM1.0 concentration （Atmospheric environment）unit μg/ m3
Data13~Data14 PM2.5 concentration （Atmospheric environment）unit μg/ m3
Data15~Data16 PM10 concentration （Atmospheric environment）unit μg/ m3
Data17~Data18 the number of particles with diameter 0.3um or above in 1 liter of air
Data19~Data20 the number of particles with diameter 0.5um or above in 1 liter of air
Data21~Data22 the number of particles with diameter 1.0um or above in 1 liter of air
Data23~Data24 the number of particles with diameter 2.5um or above in 1 liter of air
Data25~Data26 the number of particles with diameter 5.0um or above in 1 liter of air
Data27~Data28 the number of particles with diameter 10um or above in 1 liter of air
Data29 Data0~Data28 Checksum

*/

typedef struct HM3300_Sensor_DataGram_s
{
	uint16_t reserved;
	uint16_t sensornum;
	uint16_t pm1_spm;
	uint16_t pm25_spm;
	uint16_t pm10_spm;
	uint16_t pm1_ae;
	uint16_t pm25_ae;
	uint16_t pm10_ae;
	uint16_t pm03_air;
	uint16_t pm05_air;
	uint16_t pm1_air;
	uint16_t pm25_air;
	uint16_t pm5_air;
	uint16_t pm10_air;
	uint8_t  checksum;
} HM3300_Sensor_DataGram;


////////////////////////////////////////////////////////////////////////////////////////

class CHM3300Sensor : public CSensor
{

public:

	// --- construct 

	CHM3300Sensor(void);

	// --- actions

	bool SetupSensor(gpio_num_t f_sda,gpio_num_t f_scl,i2c_port_t f_i2c_port,int f_dev_address);

	// --- internal functions do not use

	virtual std::string GetSensorValueString(void);
    virtual std::string GetSensorDescriptionString(void);
 	virtual bool PerformMeasurement(void);
    virtual void AddValuesToJSON_MQTT(cJSON *f_root);
    virtual void AddValuesToJSON_API(cJSON *f_root);
 	virtual bool SetupSensor(gpio_num_t *f_pins,int *f_data);	

private:

	inline uint16_t byteswap(uint16_t i)
	{
		return i >> 8 | (i & 0xFF) << 8;
	}
	
	// --- our last measurements

	uint m_pm1_ae;
	uint m_pm25_ae;
	uint m_pm10_ae;

	uint m_pm1_spm;
	uint m_pm25_spm;
	uint m_pm10_spm;	

	// --- our configuration

	gpio_num_t 		m_pin_sda;
	gpio_num_t 		m_pin_scl;
	i2c_port_t	 	m_i2c_port;
	int				m_bme280_i2c_adr;
	int 			m_dev_address;

	// --- I2C initialization tracking

	static bool m_i2c_initialized[I2C_NUM_MAX];
	static bool m_i2c_array_initialized;
	
	// --- general handling stuff
	
	bool m_Initialized;
};

////////////////////////////////////////////////////////////////////////////////////////

#endif

