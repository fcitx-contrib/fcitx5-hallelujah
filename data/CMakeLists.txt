set(DEST_DIR "${FCITX_INSTALL_DATADIR}/hallelujah")
add_custom_target(google ALL
    COMMAND marisa-build -b google_227800_words.txt -o "${CMAKE_CURRENT_BINARY_DIR}/google_227800_words.bin"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/google_227800_words.bin" DESTINATION ${DEST_DIR} COMPONENT config)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/words.json" DESTINATION ${DEST_DIR} COMPONENT config)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cedict.json" DESTINATION ${DEST_DIR} COMPONENT config)
