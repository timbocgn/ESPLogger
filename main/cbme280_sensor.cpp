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


#ifdef ALTERNATIVE_FUCK

////////////////////////////////////////////////////////////////////////////////////////

BME280_INTF_RET_TYPE CBme280Sensor::bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	CBme280Sensor *l_this = (CBme280Sensor *)intf_ptr;

	ESP_LOGI(TAG,"bme280_i2c_read reg_addr %02x length %d",(int)reg_addr,(int)length);

	// --- write register address

ESP_LOGE(TAG,"----->Before transmit-----");

	esp_err_t l_retcode = i2c_master_transmit(l_this->m_i2c_dev_handle, &reg_addr, sizeof(uint8_t), 1000);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"bme280_i2c_read / i2c_master_transmit failed with %d", l_retcode);
		return BME280_E_COMM_FAIL;
	}
ESP_LOGE(TAG,"----->after transmit-----");

	// --- write data bytes 
ESP_LOGE(TAG,"----->Before receive-----");
	l_retcode = i2c_master_receive(l_this->m_i2c_dev_handle, reg_data, length, 1000);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"bme280_i2c_read / i2c_master_receive failed with %d", l_retcode);
		return BME280_E_COMM_FAIL;
	}
ESP_LOGE(TAG,"----->after receive-----");

	return BME280_INTF_RET_SUCCESS;
}
#endif


BME280_INTF_RET_TYPE CBme280Sensor::bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	CBme280Sensor *l_this = (CBme280Sensor *)intf_ptr;

	ESP_LOGI(TAG,"bme280_i2c_read reg_addr %02x length %d",(int)reg_addr,(int)length);

	// --- write register address

ESP_LOGE(TAG,"----->Before i2c_master_transmit_receive-----");

	esp_err_t l_retcode = i2c_master_transmit_receive(l_this->m_i2c_dev_handle,  &reg_addr, sizeof(uint8_t), reg_data, length, 1000);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"bme280_i2c_read / i2c_master_transmit_receive failed with %d", l_retcode);
		return BME280_E_COMM_FAIL;
	}
ESP_LOGE(TAG,"----->after i2c_master_transmit_receive-----");


	return BME280_INTF_RET_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////

BME280_INTF_RET_TYPE CBme280Sensor::bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	CBme280Sensor *l_this = (CBme280Sensor *)intf_ptr;

	ESP_LOGI(TAG,"bme280_i2c_write reg_addr %02x",(int)reg_addr);

	// --- write register address

	esp_err_t l_retcode = i2c_master_transmit(l_this->m_i2c_dev_handle, &reg_addr, sizeof(uint8_t), 100);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"bme280_i2c_write / i2c_master_transmit (1) failed with %d", l_retcode);
		return BME280_E_COMM_FAIL;
	}

	// --- write data bytes 

	l_retcode = i2c_master_transmit(l_this->m_i2c_dev_handle, reg_data, length, 100);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"bme280_i2c_write / i2c_master_transmit (2) failed with %d", l_retcode);
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

CBme280Sensor::bus_handle_map_t CBme280Sensor::m_bus_handle_map;

////////////////////////////////////////////////////////////////////////////////////////

CBme280Sensor::CBme280Sensor(void)
{
	m_Initialized		= false;

	m_pin_sda			= (gpio_num_t)0;
	m_pin_scl			= (gpio_num_t)0;
	m_i2c_port 			= (i2c_port_num_t)0;
	
	m_temp				= 0;
	m_rh				= 0;
	m_pressure			= 0;
}



////////////////////////////////////////////////////////////////////////////////////////

bool CBme280Sensor::SetupSensor(gpio_num_t f_sda,gpio_num_t f_scl,i2c_port_num_t f_i2c_port,int f_dev_address)
{
	m_pin_sda			= f_sda;
	m_pin_scl			= f_scl;
	m_i2c_port 			= f_i2c_port;
	m_dev_address		= f_dev_address;

	ESP_LOGI(TAG,"Setting up sensor on i2c %d on sda pin %d scl pin %d i2c %d addr %d", (int)m_i2c_port,(int)m_pin_sda,(int)m_pin_scl,(int)f_i2c_port,f_dev_address);

	// --- I2C handling is a bit more complicated. Since two sensors can be on the same I2C bus 
	//     the bus must only initalized once. But the CSensor instances are independent.
	//     So the code below keeps track of the initialized busses and reuses them

	if (m_bus_handle_map.contains(m_i2c_port))
	{
		// --- so the bus already has been initialized - reuse the existing handle

		m_i2c_bus_handle = m_bus_handle_map[m_i2c_port];
		
		ESP_LOGI(TAG,"Reuse I2C bus handle %x", (int)m_i2c_bus_handle);
	}
	else
	{
		ESP_LOGI(TAG,"Initialize I2C bus for port %d", (int)f_i2c_port);

		// --- we need to initialize the bus / configure I2C master

		i2c_master_bus_config_t i2c_mst_config;

		i2c_mst_config.clk_source 			= I2C_CLK_SRC_DEFAULT;
		i2c_mst_config.i2c_port 			= m_i2c_port;
		i2c_mst_config.scl_io_num 			= m_pin_scl;
		i2c_mst_config.sda_io_num 			= m_pin_sda;
		i2c_mst_config.intr_priority		= 0;
		i2c_mst_config.glitch_ignore_cnt 	= 7;
		i2c_mst_config.trans_queue_depth	= 0;	// --- this has to be zero, otherwise the interface becomes async!

		i2c_mst_config.flags.enable_internal_pullup = true;

		// --- now do it

		esp_err_t l_retcode = i2c_new_master_bus(&i2c_mst_config, &m_i2c_bus_handle);
		if (l_retcode != ESP_OK)
		{
			ESP_LOGE(TAG,"bme280_init / i2c_new_master_bus failed with %d", l_retcode);
			return false;
		}

		ESP_LOGI(TAG,"New I2C bus handle %x", (int)m_i2c_bus_handle);

		// --- and save the handle for potential reuse

		m_bus_handle_map[m_i2c_port] = m_i2c_bus_handle;
	}

	// --- calculate address (BMW280 only supports 0x76 and 0x77)

	assert(f_dev_address == 0 ||  f_dev_address == 1);
	uint16_t l_realaddr = 0x76 + f_dev_address;

	ESP_LOGI(TAG,"Use device address %02X", (int)l_realaddr);

	// --- now configure our sensor device

	i2c_device_config_t dev_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = l_realaddr,
		.scl_speed_hz = 100000,
	};

	esp_err_t l_retcode = i2c_master_bus_add_device(m_i2c_bus_handle, &dev_cfg, &m_i2c_dev_handle);
	if (l_retcode != ESP_OK)
	{
		ESP_LOGE(TAG,"bme280_init / i2c_master_bus_add_device failed with %d", l_retcode);
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

	// --- and initialize the low level API. Since this call reads the chip version, I2C has to be initialized already

    int8_t rslt; 
    rslt = bme280_init(&m_bme280_dev);
	if (rslt != BME280_OK)
	{
		ESP_LOGE(TAG,"bme280_init failed with %d", rslt);
		return false;
	}
  
#ifdef XXXXX
    /* Always read the current settings before writing, especially when all the configuration is not modified */
    rslt = bme280_get_sensor_settings(&settings, &dev);
  
    /* Configuring the over-sampling rate, filter coefficient and standby time */
    /* Overwrite the desired settings */
    settings.filter = BME280_FILTER_COEFF_2;

    /* Over-sampling rate for humidity, temperature and pressure */
    settings.osr_h = BME280_OVERSAMPLING_1X;
    settings.osr_p = BME280_OVERSAMPLING_1X;
    settings.osr_t = BME280_OVERSAMPLING_1X;

    /* Setting the standby time */
    settings.standby_time = BME280_STANDBY_TIME_0_5_MS;

    rslt = bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &settings, &dev);
    bme280_error_codes_print_result("bme280_set_sensor_settings", rslt);

    /* Always set the power mode after setting the configuration */
    rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &dev);

    /* Calculate measurement time in microseconds */
    rslt = bme280_cal_meas_delay(&period, &settings);
    bme280_error_codes_print_result("bme280_cal_meas_delay", rslt);

    printf("\nTemperature calculation (Data displayed are compensated values)\n");
    printf("Measurement time : %lu us\n\n", (long unsigned int)period);

    rslt = get_temperature(period, &dev);
    bme280_error_codes_print_result("get_temperature", rslt);



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

	return SetupSensor(f_pins[0],f_pins[1],(i2c_port_num_t)f_data[0],f_data[1]);

#endif
}
