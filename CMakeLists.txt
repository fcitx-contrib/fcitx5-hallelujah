cmake_minimum_required(VERSION 3.6.0)

project(fcitx5-hallelujah VERSION 5.0.3)

find_package(ECM 1.0.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})
include(FeatureSummary)
include(GNUInstallDirs)
include(ECMUninstallTarget)

option(ENABLE_TEST "Build Test" On)
option(BUILD_DATA "Build data" On)

find_package(Gettext REQUIRED)
find_package(Fcitx5Core 5.1.13 REQUIRED)
find_package(Fcitx5Module REQUIRED COMPONENTS Spell TestFrontend)
find_package(PkgConfig REQUIRED)

if (NOT TARGET fmt::fmt)
    find_package(fmt REQUIRED)
endif()

if (NOT DEFINED MARISA_TARGET)
    pkg_check_modules(Marisa REQUIRED IMPORTED_TARGET marisa)
    set(MARISA_TARGET PkgConfig::Marisa)
endif()

find_package(nlohmann_json)

add_definitions(-DFCITX_GETTEXT_DOMAIN=\"fcitx5-hallelujah\")
fcitx5_add_i18n_definition()

include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

add_subdirectory(src)
add_subdirectory(po)

if (ENABLE_TEST)
  enable_testing()
  add_subdirectory(test)
endif()
if (BUILD_DATA)
  add_subdirectory(data)
endif()

fcitx5_translate_desktop_file(org.fcitx.Fcitx5.Addon.Hallelujah.metainfo.xml.in
                              org.fcitx.Fcitx5.Addon.Hallelujah.metainfo.xml XML)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.Addon.Hallelujah.metainfo.xml" DESTINATION ${FCITX_INSTALL_DATADIR}/metainfo)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
