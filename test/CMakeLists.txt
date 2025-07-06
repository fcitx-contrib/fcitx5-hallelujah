add_executable(testhallelujah testhallelujah.cpp)
target_link_libraries(testhallelujah Fcitx5::Core Fcitx5::Module::TestFrontend)
add_test(NAME testhallelujah COMMAND testhallelujah)
