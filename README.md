# ESPLogger

ESP32 based IoT Device for air quality logging featuring an MQTT client and REST API access. 

It works in conjunction with the VINDRIKTNING air sensor from IKEA which can be hacked to become an IoT by this or by an SHT1x sensor which reports temperature and humidity.

Other sensors can be added by implementing a sensor driver class.

It features a bootstrapping mechanism opening a local wireless hotspot for connecting it to your
favorite wireless lan and has also an MQTT provider build in, which you can use to log the data.

You also can leverage the API calls to configure the device and get the sensor data to your favorite application.

This app supersedes the ESPTemplogger and ESPDustLogger app as it joins their functionality.
    
## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. 

### Prerequisites

You will need a running copy of the ESP IDF SDK. Please follow these instructions:

https://docs.espressif.com/projects/esp-idf/en/latest/get-started/

Be sure that you have the `idf.py` application in your path and that all environment variables are setup, e.g. by providing a `.zshrc` file (example for macOS Catalina):

```
#ESP32 stuff

export IDF_PATH=<your IDF path>/esp-idf

source $IDF_PATH/export.sh
```

Currently (Jan 2024) the code is based on 5.2-beta of the IDF.

### Installing and Compiling

Download or clone the repository to your system

```
https://github.com/timbocgn/esplogger
```

Firstly you should install all the dependencies for the vue based web application

```
cd front/webapp
npm install
```

When this is done, you can build the vue app

```
npm run build
```

This will "compile" the app into the build directory where the ESP toolchain will pick it up to store it onto the ESP32 fat flash filesystem, where it is then served by a http server.

When the build is done, you can configure your IDF app

```
cd ../..
idf.py menuconfig
```

Under "ESP Logger Configuration" you will be able to define bootstrap switch pin and the LED pin. Sensor configuration (including pins) can be specified in `sensor_config.h`.

When configured, compile and flash to your device:

```
idf.py build
...
idf.py -p <your serial device> flash
```

My WEMOS mini board is equipped with a CP2104 USB/UART converter and in my case it defaults to /dev/tty.usbserial-00E3A8A2. 

Please use 
```
ls /dev/tty.*
```
to find the device name of your programming port.

### Monitor your device

A lot of debugging output will be generated which you can see if you monitor the ESP:

```
idf.py -p /dev/tty.usbserial-00E3A8A2 monitor
```

## How to use

### Bootstrap

* Close the bootstrap switch (which means pulling the GPIO pin to H" on system boot). The system reboots into bootstrap mode...
* The LED will blink in a 500ms on - 2500ms off sequence, which indicated that the build in access point is up and running
* Connect your system to this AP's IP-Address - the password is "let-me-in-1234". Typically the IP address of the ESP in this mode is 192.168.4.1
* Go to the configuration page, provide your WLAN access point SSID (press 'Scan' to get a list) and provide the password
* Reboot the system (power off and on)
* When the LED blinks in a 100ms on - 100ms off - 100ms on - 2700ms off fashion, the system is connecting to your AP
* When the LED blinks in a 100ms on - 2900ms off fashion, the system is connected to your AP

### Access the web interface

Please take a look at your router, DHCP server or the monitor output to get the IP address of the ESP32. Point your browser to the IP address to check if the sensors are working. 

It might take a while until the Vindriktning sensor receives its first measurement.

### Push the sensor data to MQTT

Just provide the necessary data in the MQTT section and enable the MQTT client. The sensors will provide the data as JSON struct.

## Development

### Changing the UI

To change the UI it is very handy to you the build in web server of vite:

```
npm run dev
```

It will start a local web server on your machine which forwards the API calls to your device. Please check the `vite.config.js` file in `front/webapp` and enter the IP address the requests should be forwarded to (to the address of your ESP).

Please follow the vue.js guides and how to's on how to change the front end code.

## Adding more sensors

The sensors are added to the code using a macro based factory. Take a look at `sensor_config.h` to see the three devices I build with this framework.

You can specify parameters such as GPIO or UART id by adding config variables.

## Adding new sensors

All sensor drivers are a derived class if the CSensor abstract base class, which provides a common interface for all sensor types.

Once you have implemented this class, you can use the factory in sensor_config.h to install your sensor object to the code.

Everything else is created dynamically: UI of the homepage, MQTT push formats, etc.

The system is not limited to any specific type of sensor.

## Over the air update

The app is fully implementing the ESP-IDF facilities for providing OTA updated when the system is running. 

The respective function is in the About page - just build a new `ESPLogger.bin` file and upload this to the device. Once the flash is ready, the device will reboot.

Just refresh you browser (reload the web app) to access the interface again.

## Easier Development

If your are using VS.Code I recommend to install a couple of plugins:

- Espressif IDF for building esp idf projects
- Volar for vue.js language features

## Built With

* [ESP IDF](https://www.espressif.com/en/products/software/esp-sdk/overview) - The SDK used for development
* [vue.js](https://vuejs.org) - Javascript frontend framework used for the SPA
* [vuetify.js](https://vuetifyjs.com/) - Javascript component framework based on vue.js

* [vue-the-mask](https://github.com/vuejs-tips/vue-the-mask) - A vue.js component for more fancy input masking 

* [Material Design Icons](https://pictogrammers.com/library/mdi/) - An extensive library of SVG based icons for the web app

* [vite plugin compression](https://github.com/vbenjs/vite-plugin-compression) - A plugin to statically gzip the css and js files in the flash file system

* [node.js](https://nodejs.org/en/) - Used to compile the vue app

* [pm1006 on github](https://github.com/bertrik/pm1006) - code for receiving the Vindriktning sensor data

* [Raspberry PI SHT1x Library](https://www.john.geek.nz/2012/08/reading-data-from-a-sensirion-sht1x-with-a-raspberry-pi/) - A full implementation of the comms protocol for the Sensirion SHT1x sensor

* [Bosch BME280 Driver](https://github.com/BoschSensortec/BME280_driver) - manufacturer provided driver for the alternative temp and rH sensor

## Authors

* **Tim Hagemann** - *Initial work* - https://github.com/timbocgn

## License

This project is licensed under the MIT License.

## Acknowledgments

* The ESP IDF authors for the perfect source code examples they provide within their SDK
* Bertrik Sikken for his excellent PM1006 class available on GitHub
* John Burns (www.john.geek.nz) for parts of the SHT1x code
* Daesung Kim for the initial work on the SHT1x code base


