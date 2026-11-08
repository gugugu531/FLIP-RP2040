# Keep the hardware adapter's source list explicit; vendor demos are not built.
add_library(Config DEV_Config.c)
target_include_directories(Config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Config PUBLIC
    pico_stdlib hardware_spi hardware_i2c hardware_pwm hardware_adc hardware_dma)
