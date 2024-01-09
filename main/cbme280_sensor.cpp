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

////////////////////////////////////////////////////////////////////////////////////////

BME280_INTF_RET_TYPE CBme280Sensor::bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	CBme280Sensor *l_this = (CBme280Sensor *)intf_ptr;
	return BME280_E_COMM_FAIL;
}

////////////////////////////////////////////////////////////////////////////////////////

BME280_INTF_RET_TYPE CBme280Sensor::bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr)
{
	CBme280Sensor *l_this = (CBme280Sensor *)intf_ptr;
	return BME280_E_COMM_FAIL;
}

////////////////////////////////////////////////////////////////////////////////////////

void CBme280Sensor::bme280_delay_us(uint32_t period_us, void *intf_ptr)
{
	ets_delay_us(period_us);
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
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

bool CBme280Sensor::SetupSensor(gpio_num_t f_sda,gpio_num_t f_scl,i2c_port_num_t f_i2c_port)
{
	m_pin_sda			= f_sda;
	m_pin_scl			= f_scl;
	m_i2c_port 			= f_i2c_port;

	ESP_LOGI(TAG,"Setting up sensor on i2c %d on sda pin %d scl pin %d", (int)m_i2c_port,(int)m_pin_sda,(int)m_pin_scl);

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
	// --- Pins: 0: SDA 1: SCL   Data: 0: I2C port

	return SetupSensor(f_pins[0],f_pins[1],(i2c_port_num_t)f_data[0]);

#endif
}
