add_library(LCD LCD_1in28.c)
target_include_directories(LCD PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(LCD PUBLIC Config)
