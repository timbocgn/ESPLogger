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

// Sensor Data Sheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf

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

#include "bme280.h"

#include "sensor_config.h"
#include "cbme280_sensor.h"

////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////

static const char *TAG = "CBme280Sensor";

BME280_INTF_RET_TYPE CBme280Sensor::bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	CBme280Sensor *l_this = (CBme280Sensor *)intf_ptr;

	//ESP_LOGI(TAG,"bme280_i2c_read reg_addr %02x length %d",(int)reg_addr,(int)length);

	// --- write register address

   	esp_err_t l_retcode = i2c_master_write_read_device(l_this->m_i2c_port, l_this->m_dev_address, &reg_addr, 1, reg_data, length, 1000 / portTICK_PERIOD_MS);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"bme280_i2c_read / i2c_master_transmit_receive failed with %d", l_retcode);
		return BME280_E_COMM_FAIL;
	}

	//ESP_LOG_BUFFER_HEX_LEVEL(TAG,reg_data,length,ESP_LOG_INFO);

	return BME280_INTF_RET_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////

BME280_INTF_RET_TYPE CBme280Sensor::bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	CBme280Sensor *l_this = (CBme280Sensor *)intf_ptr;

	//ESP_LOGI(TAG,"bme280_i2c_write reg_addr %02x len %d",(int)reg_addr,(int)length);
	//ESP_LOG_BUFFER_HEX_LEVEL(TAG,reg_data,length,ESP_LOG_INFO);

	assert(length < 20);

	uint8_t l_writebuf[21];

	l_writebuf[0] = reg_addr;
	memcpy(l_writebuf+1,reg_data,length);



   	esp_err_t l_retcode = i2c_master_write_to_device(l_this->m_i2c_port, l_this->m_dev_address, l_writebuf, length+1, 1000 / portTICK_PERIOD_MS);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"bme280_i2c_read / i2c_master_write_to_device (1) failed with %d", l_retcode);
		return BME280_E_COMM_FAIL;
	}

	return BME280_INTF_RET_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////

void CBme280Sensor::bme280_delay_us(uint32_t period_us, void *intf_ptr)
{
	// --- a very simple forward to a RTOS function

	ets_delay_us(period_us);
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

//Bme280Sensor::bus_handle_map_t CBme280Sensor::m_bus_handle_map;

////////////////////////////////////////////////////////////////////////////////////////

CBme280Sensor::CBme280Sensor(void)
{
	m_Initialized		= false;

	m_pin_sda			= (gpio_num_t)0;
	m_pin_scl			= (gpio_num_t)0;
	m_i2c_port 			= (i2c_port_t)0;
	
	m_temp				= 0;
	m_rh				= 0;
	m_pressure			= 0;
}

////////////////////////////////////////////////////////////////////////////////////////

#define SAMPLE_COUNT  UINT8_C(1)

int8_t CBme280Sensor::get_temperature(uint32_t period)
{
    int8_t rslt = BME280_E_NULL_PTR;
    int8_t idx = 0;
    uint8_t status_reg;
    struct bme280_data comp_data;

    while (idx < SAMPLE_COUNT)
    {
        rslt = bme280_get_regs(BME280_REG_STATUS, &status_reg, 1, &m_bme280_dev);
		if (rslt != BME280_OK)
		{
			ESP_LOGE(TAG,"bme280_get_regs failed with %d", rslt);
			return false;
		}	

ESP_LOGE(TAG,"status ref %0X", status_reg);

     //   if (status_reg & BME280_STATUS_MEAS_DONE)
        {
            /* Measurement time delay given to read sample */
            m_bme280_dev.delay_us(period, m_bme280_dev.intf_ptr);

            /* Read compensated data */
            rslt = bme280_get_sensor_data(BME280_TEMP, &comp_data, &m_bme280_dev);
			if (rslt != BME280_OK)
			{
				ESP_LOGE(TAG,"bme280_get_sensor_data failed with %d", rslt);
				return false;
			}	
			m_temp = comp_data.temperature;
			ESP_LOGI(TAG,"Got temp %f",m_temp);
			
            idx++;
        }
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////////////

bool CBme280Sensor::SetupSensor(gpio_num_t f_sda,gpio_num_t f_scl,i2c_port_t f_i2c_port,int f_dev_address)
{
	m_pin_sda			= f_sda;
	m_pin_scl			= f_scl;
	m_i2c_port 			= f_i2c_port;
	m_dev_address		= f_dev_address + 0x76;

	ESP_LOGI(TAG,"Setting up sensor on i2c %d on sda pin %d scl pin %d i2c %d addr %d", (int)m_i2c_port,(int)m_pin_sda,(int)m_pin_scl,(int)f_i2c_port,f_dev_address);

    i2c_config_t conf;

	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = m_pin_sda;
	conf.scl_io_num = m_pin_scl;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	conf.clk_flags = 0;

    i2c_param_config(m_i2c_port, &conf);
     
	esp_err_t l_retcode = i2c_driver_install(m_i2c_port,conf.mode, 0, 0, 0);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"bme280_init / i2c_driver_install failed with %d", l_retcode);
		return false;
	}

    // --- initialize the device structure for the low level Bosch API

	m_bme280_i2c_adr = BME280_I2C_ADDR_PRIM;
  
	// --- this is used to get our object back in the static member function

    m_bme280_dev.intf_ptr = (void *)this;

	// --- setup all function pointers

	m_bme280_dev.read 		= bme280_i2c_read;
	m_bme280_dev.write 		= bme280_i2c_write;
	m_bme280_dev.delay_us 	= bme280_delay_us;
	m_bme280_dev.intf		= BME280_I2C_INTF;

	// --- and initialize the low level API. Since this call reads the chip version, I2C has to be initialized already

    int8_t rslt; 
    rslt = bme280_init(&m_bme280_dev);
	if (rslt != BME280_OK)
	{
		ESP_LOGE(TAG,"bme280_init failed with %d", rslt);
		return false;
	}
  
    /* Always read the current settings before writing, especially when all the configuration is not modified */
	bme280_settings settings;
    rslt = bme280_get_sensor_settings(&settings, &m_bme280_dev);
	if (rslt != BME280_OK)
	{
		ESP_LOGE(TAG,"bme280_get_sensor_settings failed with %d", rslt);
		return false;
	}	
  
    /* Configuring the over-sampling rate, filter coefficient and standby time */
    /* Overwrite the desired settings */
    settings.filter = BME280_FILTER_COEFF_2;

    /* Over-sampling rate for humidity, temperature and pressure */
    settings.osr_h = BME280_OVERSAMPLING_1X;
    settings.osr_p = BME280_OVERSAMPLING_1X;
    settings.osr_t = BME280_OVERSAMPLING_1X;

    /* Setting the standby time */
    settings.standby_time = BME280_STANDBY_TIME_0_5_MS;

    rslt = bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &settings, &m_bme280_dev);
	if (rslt != BME280_OK)
	{
		ESP_LOGE(TAG,"bme280_set_sensor_settings failed with %d", rslt);
		return false;
	}	

    /* Always set the power mode after setting the configuration */
    rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &m_bme280_dev);
	if (rslt != BME280_OK)
	{
		ESP_LOGE(TAG,"bme280_set_sensor_mode failed with %d", rslt);
		return false;
	}

	uint8_t l_sm;
	rslt = bme280_get_sensor_mode(&l_sm, &m_bme280_dev);
	if (rslt != BME280_OK)
	{
		ESP_LOGE(TAG,"bme280_get_sensor_mode failed with %d", rslt);
		return false;
	}
	else
	{
		ESP_LOGE(TAG,"bme280_get_sensor_mode provided with %d", l_sm);
	}

#ifdef noneed
    /* Calculate measurement time in microseconds */
	uint32_t period;
    rslt = bme280_cal_meas_delay(&period, &settings);
	if (rslt != BME280_OK)
	{
		ESP_LOGE(TAG,"bme280_cal_meas_delay failed with %d", rslt);
		return false;
	}

    ESP_LOGI(TAG,"Measurement time : %lu us",period);

    rslt = get_temperature(period);
#endif

	m_Initialized = true;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////

bool CBme280Sensor::PerformMeasurement(void)
{
#ifdef SENSOR_CONFIG_STUB_SENSORS
		// ---- do some random magic to generate some values

		m_temp 		= rand();
		m_rh 		= rand();
		m_pressure  = rand(); 

		return true;
#else

    int8_t rslt; 
	struct bme280_data comp_data;

	rslt = bme280_get_sensor_data(BME280_ALL , &comp_data, &m_bme280_dev);
	if (rslt != BME280_OK)
	{
		ESP_LOGE(TAG,"bme280_get_sensor_data failed with %d", rslt);
		return false;
	}	
	m_temp 		= comp_data.temperature;
	m_pressure 	= comp_data.pressure / 100;
	m_rh 		= comp_data.humidity;
	
	return true;

#endif
}

////////////////////////////////////////////////////////////////////////////////////////

std::string CBme280Sensor::GetSensorValueString(void)
{
	char l_buf[200];
	snprintf(l_buf,200,"CBme280Sensor: temp: %f C / rH: %f %% / pressure: %f mBar",m_temp,m_rh,m_pressure);

	return std::string(l_buf);
}

////////////////////////////////////////////////////////////////////////////////////////

std::string CBme280Sensor::GetSensorDescriptionString(void)
{
	char l_buf[200];
	snprintf(l_buf,200,"BME280 Sensor / i2c %d on sda pin %d scl pin %d", (int)m_i2c_port,(int)m_pin_sda,(int)m_pin_scl);

	return std::string(l_buf);
}

////////////////////////////////////////////////////////////////////////////////////////

void CBme280Sensor::AddValuesToJSON_MQTT(cJSON *f_root)
{
	cJSON_AddNumberToObject(f_root, "temp", m_temp);
	cJSON_AddNumberToObject(f_root, "rh", m_rh);
	cJSON_AddNumberToObject(f_root, "pressure", m_pressure);	
}

////////////////////////////////////////////////////////////////////////////////////////

void CBme280Sensor::AddValuesToJSON_API(cJSON *f_root)
{
    cJSON *t 	= cJSON_CreateObject();
    cJSON *r 	= cJSON_CreateObject();
    cJSON *p = cJSON_CreateObject();

	cJSON_AddStringToObject(t, "unit", "C");
	cJSON_AddNumberToObject(t, "value", m_temp);
	cJSON_AddStringToObject(t, "text", "Temperature");

	cJSON_AddStringToObject(r, "unit", "%");
	cJSON_AddNumberToObject(r, "value", m_rh);
	cJSON_AddStringToObject(r, "text", "Relative Humidity");

	cJSON_AddStringToObject(p, "unit", "mBar");
	cJSON_AddNumberToObject(p, "value", m_pressure);
	cJSON_AddStringToObject(p, "text", "Pressure");

	cJSON_AddItemToObject(f_root,"temp",t);
	cJSON_AddItemToObject(f_root,"rh",r);
	cJSON_AddItemToObject(f_root,"pressure",p);

	cJSON_AddStringToObject(f_root, "SensorType", "Bosch BME280 Sensor");

}

////////////////////////////////////////////////////////////////////////////////////////

bool CBme280Sensor::SetupSensor(gpio_num_t *f_pins,int *f_data)
{
#ifdef SENSOR_CONFIG_STUB_SENSORS
	
	srand((unsigned)time(0)); 	
	return true;

#else
	// --- Pins: 0: SDA 1: SCL / Data: 0: I2C port 1: I2C address (0 or 1)

	return SetupSensor(f_pins[0],f_pins[1],(i2c_port_t)f_data[0],f_data[1]);

#endif
}
