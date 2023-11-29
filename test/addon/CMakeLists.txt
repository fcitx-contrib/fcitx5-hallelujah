add_custom_target(copy-addon DEPENDS hallelujah-addon.conf.in-fmt)
add_custom_command(TARGET copy-addon COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/src/hallelujah-addon.conf ${CMAKE_CURRENT_BINARY_DIR}/hallelujah.conf)
