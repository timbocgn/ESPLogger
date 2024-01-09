
# ----- register all source files here

idf_component_register(SRCS "ESP32_SHT1x.cpp" "vindriktning.cpp" "main.cpp" 
                            "rest_server.cpp" "sensor_manager.cpp" "config_manager.cpp" 
                            "infomanager.cpp" "mqtt_manager.cpp" "applogger.cpp" "bme280.c"
                            "cbme280_sensor.cpp"
                       INCLUDE_DIRS ".")


# ----- specify the root of the webapp directory here

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front/webapp")

# ----- package the dist dictory to the "www" partition of the ESP image

if(EXISTS ${WEB_SRC_DIR}/dist)
    spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()