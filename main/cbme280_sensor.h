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

#ifndef ESP32_cbme280_sensor_H_
#define	ESP32_cbme280_sensor_H_

////////////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>

#include "driver/i2c_master.h"

#include "csensor.h"

#include "bme280.h"

////////////////////////////////////////////////////////////////////////////////////////

class CBme280Sensor : public CSensor
{

public:

	// --- construct 

	CBme280Sensor(void);

	// --- actions

	bool SetupSensor(gpio_num_t f_sda,gpio_num_t f_scl,i2c_port_num_t f_i2c_port);

	// --- getter

	float GetTemp(void) const
	{
		return m_temp;
	}

	float GetRH(void) const
	{
		return m_rh;
	}

	float GetPressure(void) const
	{
		return m_pressure;
	}

	// --- internal functions do not use

/*
	gpio_num_t GetDataPin(void) { return m_pin_data; }
	uart_port_t GetUart(void) { return m_uart; }

*/

	virtual std::string GetSensorValueString(void);
    virtual std::string GetSensorDescriptionString(void);
 	virtual bool PerformMeasurement(void);
    virtual void AddValuesToJSON_MQTT(cJSON *f_root);
    virtual void AddValuesToJSON_API(cJSON *f_root);
 	virtual bool SetupSensor(gpio_num_t *f_pins,int *f_data);	

private:

	static BME280_INTF_RET_TYPE bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr);
	static BME280_INTF_RET_TYPE bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr);
	static void bme280_delay_us(uint32_t period_us, void *intf_ptr);


	float m_temp;
	float m_rh;
	float m_pressure;

	
	gpio_num_t 		m_pin_sda;
	gpio_num_t 		m_pin_scl;
	i2c_port_num_t 	m_i2c_port;
	int				m_bme280_i2c_adr;

	struct bme280_dev m_bme280_dev;
	
	bool m_Initialized;
};

////////////////////////////////////////////////////////////////////////////////////////

#endif

