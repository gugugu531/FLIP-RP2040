# Included only by desktop/CMakeLists.txt when BUILD_TESTING=ON.
add_executable(fluid_test test_fluid.c)
target_link_libraries(fluid_test fluid m)
add_test(NAME fluid_stability COMMAND fluid_test)
add_executable(render_test test_render.c)
target_link_libraries(render_test renderer)
add_test(NAME render_depth_and_bezel COMMAND render_test)
