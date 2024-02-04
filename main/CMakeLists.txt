
# ----- register all source files here

idf_component_register(SRCS "hm3300_sensor.cpp" "ESP32_SHT1x.cpp" "vindriktning.cpp" "main.cpp" 
                            "rest_server.cpp" "sensor_manager.cpp" "config_manager.cpp" 
                            "infomanager.cpp" "mqtt_manager.cpp" "applogger.cpp" "bme280.c"
                            "cbme280_sensor.cpp" "ota_manager.cpp" "hm3300_sensor.cpp"
                       INCLUDE_DIRS "." 
                       )


# ---- this is cmake stuff to create a real compile time. idf compile time give you the last
#      cmake run, not the last compile time
#      Find the source of this here:
#      https://stackoverflow.com/questions/24292898/compile-date-and-time-using-cmake

FILE (WRITE ${CMAKE_BINARY_DIR}/timestamp.cmake "STRING(TIMESTAMP TIMEZ)\n")
FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "FILE(WRITE timestamp.h \"#ifndef TIMESTAMP_H\\n\")\n")
FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "FILE(APPEND timestamp.h \"#define TIMESTAMP_H\\n\\n\")\n")
FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "FILE(APPEND timestamp.h \"#define _TIMEZ_ \\\"\${TIMEZ}\\\"\\n\\n\")\n")
FILE (APPEND ${CMAKE_BINARY_DIR}/timestamp.cmake "FILE(APPEND timestamp.h \"#endif // TIMESTAMP_H\\n\")\n")

add_custom_target(timestamp
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/timestamp.cmake
    ADD_DEPENDENCIES ${CMAKE_BINARY_DIR}/timestamp.cmake )

ADD_DEPENDENCIES (${COMPONENT_LIB} timestamp)

INCLUDE_DIRECTORIES (${CMAKE_BINARY_DIR}/esp-idf/main)

# ----- specify the root of the webapp directory here

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front/webapp")

# ----- package the dist dictory to the "www" partition of the ESP image

if(EXISTS ${WEB_SRC_DIR}/dist)
    spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()
