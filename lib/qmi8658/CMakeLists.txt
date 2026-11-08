add_library(qmi8658 qmi8658.c)
target_include_directories(qmi8658 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(qmi8658 PRIVATE -Wall -Wextra)
target_link_libraries(qmi8658 PUBLIC sensor_i2c pico_time)
