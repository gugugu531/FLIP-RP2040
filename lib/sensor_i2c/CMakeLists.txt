add_library(sensor_i2c sensor_i2c.c)
target_include_directories(sensor_i2c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(sensor_i2c PRIVATE -Wall -Wextra)
target_link_libraries(sensor_i2c PUBLIC hardware_i2c)
