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

// Sensor Data Sheet: https://files.seeedstudio.com/wiki/Grove-Laser_PM2.5_Sensor-HM3301/res/HM-3300%263600_V2.1.pdf

////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <ctime>
#include "freertos/FreeRTOS.h"
#include "rom/ets_sys.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"


#include "sensor_config.h"
#include "hm3300_sensor.h"

////////////////////////////////////////////////////////////////////////////////////////

bool CHM3300Sensor::m_i2c_initialized[I2C_NUM_MAX];
bool CHM3300Sensor::m_i2c_array_initialized = false;

////////////////////////////////////////////////////////////////////////////////////////

static const char *TAG = "CHM3300Sensor";

////////////////////////////////////////////////////////////////////////////////////////

CHM3300Sensor::CHM3300Sensor(void)
{
	m_Initialized		= false;

	m_pin_sda			= (gpio_num_t)0;
	m_pin_scl			= (gpio_num_t)0;
	m_i2c_port 			= (i2c_port_t)0;
	
	m_pm1_ae			= 0;
	m_pm25_ae			= 0;
	m_pm10_ae			= 0;

	m_pm1_spm			= 0;
	m_pm25_spm			= 0;
	m_pm10_spm			= 0;

	if (!m_i2c_array_initialized)
	{
		for (int i=0;i<I2C_NUM_MAX;++i) m_i2c_initialized[i] = false;
		m_i2c_array_initialized = true;
	}
}

////////////////////////////////////////////////////////////////////////////////////////

bool CHM3300Sensor::SetupSensor(gpio_num_t f_sda,gpio_num_t f_scl,i2c_port_t f_i2c_port,int f_dev_address)
{
	m_pin_sda			= f_sda;
	m_pin_scl			= f_scl;
	m_i2c_port 			= f_i2c_port;
	m_dev_address		= f_dev_address;

	ESP_LOGI(TAG,"Setting up sensor on i2c %d on sda pin %d scl pin %d i2c %d addr %d", (int)m_i2c_port,(int)m_pin_sda,(int)m_pin_scl,(int)f_i2c_port,f_dev_address);

	// ---- be sure the port is valid

	assert(m_i2c_port < I2C_NUM_MAX);

	// ---- initialize i2c port

	if (!m_i2c_initialized[m_i2c_port])
	{
		ESP_LOGI(TAG,"Setup i2c port %d",m_i2c_port);

		i2c_config_t conf;

		conf.mode 				= I2C_MODE_MASTER;
		conf.sda_io_num 		= m_pin_sda;
		conf.scl_io_num 		= m_pin_scl;
		conf.sda_pullup_en 		= GPIO_PULLUP_ENABLE;
		conf.scl_pullup_en 		= GPIO_PULLUP_ENABLE;
		conf.master.clk_speed 	= 100000;
		conf.clk_flags 			= 0;

		i2c_param_config(m_i2c_port, &conf);
		
		esp_err_t l_retcode = i2c_driver_install(m_i2c_port,conf.mode, 0, 0, 0);
		if (l_retcode != ESP_OK)
		{
			ESP_LOGE(TAG,"CHM3300Sensor / i2c_driver_install failed with %d", l_retcode);
			return false;
		}

		// --- remember that this I2C is up and ready

		m_i2c_initialized[m_i2c_port] = true;
	}
	else
	{
		ESP_LOGI(TAG,"Reuse i2c port %d - new pin settings etc are ignored!",m_i2c_port);
	}

	// --- switch sensor to I2C mode

	uint8_t reg_addr = 0x88;

	esp_err_t l_retcode = i2c_master_write_to_device(m_i2c_port, m_dev_address, &reg_addr, 1, 1000 / portTICK_PERIOD_MS);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"CHM3300Sensor::SetupSensor / command 0x88 failed with %d", l_retcode);
		return false;
	}

    // --- initialize the device structure for the low level Bosch API

	m_Initialized = true;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////

bool CHM3300Sensor::PerformMeasurement(void)
{
#ifdef SENSOR_CONFIG_STUB_SENSORS
	// ---- do some random magic to generate some values

	m_pm1_ae			= rand();
	m_pm25_ae			= rand();
	m_pm10_ae			= rand();

	m_pm1_spm			= rand();
	m_pm25_spm			= rand();
	m_pm10_spm			= rand();

	return true;
#else

	if (!m_Initialized)
	{
		ESP_LOGE(TAG,"Error in PerformMeasurement() - using an uninitialized sensor");
		return false;
	}

	// ---- read sensor data

	uint8_t l_data[29];

	esp_err_t l_retcode = i2c_master_read_from_device(m_i2c_port, m_dev_address, l_data, 29, 1000 / portTICK_PERIOD_MS);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"CHM3300Sensor::PerformMeasurement / read failed with %d", l_retcode);
		return false;
	}

	// --- calculate checksum and verify it
	uint8_t sum = 0;
    for (int i = 0; i < 28; i++) 
	{
        sum += l_data[i];
    }
    
	if (sum != l_data[28]) 
	{
		ESP_LOGE(TAG,"CHM3300Sensor::PerformMeasurement / checksum wrong");
		return false;
	}

	// --- now decode the datagram

	HM3300_Sensor_DataGram *l_dgm = (HM3300_Sensor_DataGram *)l_data;	

	m_pm1_spm 	= byteswap(l_dgm->pm1_spm);
	m_pm25_spm 	= byteswap(l_dgm->pm25_spm);
	m_pm10_spm 	= byteswap(l_dgm->pm10_spm);
	m_pm1_ae 	= byteswap(l_dgm->pm1_ae);
	m_pm25_ae 	= byteswap(l_dgm->pm25_ae);
	m_pm10_ae 	= byteswap(l_dgm->pm10_ae);

	return true;

#endif
}

////////////////////////////////////////////////////////////////////////////////////////

std::string CHM3300Sensor::GetSensorValueString(void)
{
	char l_buf[200];
	snprintf(l_buf,200,"CHM3300Sensor: spm pm1 %d pm25 %d pm10 %d / ae pm1 %d pm25 %d pm10 %d",m_pm1_spm,m_pm25_spm,m_pm10_spm,
																							   m_pm1_ae,m_pm25_ae,m_pm10_ae);

	return std::string(l_buf);
}

////////////////////////////////////////////////////////////////////////////////////////

std::string CHM3300Sensor::GetSensorDescriptionString(void)
{
	char l_buf[200];
	snprintf(l_buf,200,"BME280 Sensor / i2c %d on sda pin %d scl pin %d adr %d", (int)m_i2c_port,(int)m_pin_sda,(int)m_pin_scl,(int)m_dev_address);

	return std::string(l_buf);
}

////////////////////////////////////////////////////////////////////////////////////////

void CHM3300Sensor::AddValuesToJSON_MQTT(cJSON *f_root)
{
	cJSON_AddNumberToObject(f_root, "pm1_ae", m_pm1_ae);
	cJSON_AddNumberToObject(f_root, "pm25_ae", m_pm25_ae);
	cJSON_AddNumberToObject(f_root, "pm10_ae", m_pm10_ae);

	cJSON_AddNumberToObject(f_root, "pm1_spm", m_pm1_spm);
	cJSON_AddNumberToObject(f_root, "pm25_spm", m_pm25_spm);
	cJSON_AddNumberToObject(f_root, "pm10_spm", m_pm10_spm);
}

////////////////////////////////////////////////////////////////////////////////////////

void CHM3300Sensor::AddValuesToJSON_API(cJSON *f_root)
{
	cJSON *pm25_spm 	= cJSON_CreateObject();
    cJSON *pm10_spm 	= cJSON_CreateObject();
    cJSON *pm1_spm 		= cJSON_CreateObject();

	cJSON *pm25_ae 		= cJSON_CreateObject();
    cJSON *pm10_ae 		= cJSON_CreateObject();
    cJSON *pm1_ae 		= cJSON_CreateObject();


	cJSON_AddStringToObject(pm1_spm, "unit", "ug/m3");
	cJSON_AddStringToObject(pm1_spm, "value", uint_2_string("%d",m_pm1_spm));
	cJSON_AddStringToObject(pm1_spm, "text", "PM1.0 concentration (Standard particulate matter)");

	cJSON_AddStringToObject(pm25_spm, "unit", "ug/m3");
	cJSON_AddStringToObject(pm25_spm, "value", uint_2_string("%d",m_pm25_spm));
	cJSON_AddStringToObject(pm25_spm, "text", "PM2.5 concentration (Standard particulate matter)");

	cJSON_AddStringToObject(pm10_spm, "unit", "ug/m3");
	cJSON_AddStringToObject(pm10_spm, "value", uint_2_string("%d",m_pm10_spm));
	cJSON_AddStringToObject(pm10_spm, "text", "PM10 concentration (Standard particulate matter)");

	cJSON_AddStringToObject(pm1_ae, "unit", "ug/m3");
	cJSON_AddStringToObject(pm1_ae, "value", uint_2_string("%d",m_pm1_ae));
	cJSON_AddStringToObject(pm1_ae, "text", "PM1.0 concentration (Atmospheric environment)");

	cJSON_AddStringToObject(pm25_ae, "unit", "ug/m3");
	cJSON_AddStringToObject(pm25_ae, "value", uint_2_string("%d",m_pm25_ae));
	cJSON_AddStringToObject(pm25_ae, "text", "PM2.5 concentration (Atmospheric environment)");

	cJSON_AddStringToObject(pm10_ae, "unit", "ug/m3");
	cJSON_AddStringToObject(pm10_ae, "value", uint_2_string("%d",m_pm10_ae));
	cJSON_AddStringToObject(pm10_ae, "text", "PM10 concentration (Atmospheric environment)");

	cJSON_AddItemToObject(f_root,"pm1_spm",pm1_spm);
	cJSON_AddItemToObject(f_root,"pm25_spm",pm25_spm);
	cJSON_AddItemToObject(f_root,"pm10_spm",pm10_spm);

	cJSON_AddItemToObject(f_root,"pm1_ae",pm1_ae);
	cJSON_AddItemToObject(f_root,"pm25_ae",pm25_ae);
	cJSON_AddItemToObject(f_root,"pm10_ae",pm10_ae);

	cJSON_AddStringToObject(f_root, "SensorType", "HM3300 Dust Sensor");
}

////////////////////////////////////////////////////////////////////////////////////////

bool CHM3300Sensor::SetupSensor(gpio_num_t *f_pins,int *f_data)
{
#ifdef SENSOR_CONFIG_STUB_SENSORS
	
	srand((unsigned)time(0)); 	
	return true;

#else
	// --- Pins: 0: SDA 1: SCL / Data: 0: I2C port 1: I2C address (use 0x40)

	return SetupSensor(f_pins[0],f_pins[1],(i2c_port_t)f_data[0],f_data[1]);

#endif
}
