set(DEST_DIR "${FCITX_INSTALL_DATADIR}/hallelujah")

set(GOOGLE_SRC google_227800_words.txt)
set(GOOGLE_BIN "${CMAKE_CURRENT_BINARY_DIR}/google_227800_words.bin")

add_custom_command(
    OUTPUT "${GOOGLE_BIN}"
    COMMAND marisa-build -b "${GOOGLE_SRC}" -o "${GOOGLE_BIN}"
    DEPENDS "${GOOGLE_SRC}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating ${GOOGLE_BIN}"
)
add_custom_target(google ALL DEPENDS "${GOOGLE_BIN}")

install(FILES "${GOOGLE_BIN}" DESTINATION ${DEST_DIR} COMPONENT config)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/words.json" DESTINATION ${DEST_DIR} COMPONENT config)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cedict.json" DESTINATION ${DEST_DIR} COMPONENT config)
