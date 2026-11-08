add_library(cst816 cst816.c)
target_include_directories(cst816 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(cst816 PRIVATE -Wall -Wextra)
target_link_libraries(cst816 PUBLIC sensor_i2c hardware_gpio pico_time)
