cmake_minimum_required (VERSION 2.6)
project(Peerspeak)

find_package (PkgConfig REQUIRED)
pkg_check_modules (GTKMM REQUIRED gtkmm-3.0)

execute_process (
    COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=glib_compile_resources gio-2.0
    OUTPUT_VARIABLE GLIB_COMPILE_RESOURCES
    OUTPUT_STRIP_TRAILING_WHITESPACE)

add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/resources.c
    DEPENDS res/peerspeak.gresources.xml res/peerspeak_ui.glade
    COMMAND ${GLIB_COMPILE_RESOURCES} res/peerspeak.gresources.xml --target=${CMAKE_CURRENT_BINARY_DIR}/resources.c --sourcedir=${CMAKE_CURRENT_SOURCE_DIR}/res --generate-source)

include_directories (${GTKMM_INCLUDE_DIRS})
link_directories (${GTKMM_LIBRARY_DIRS})
add_definitions (${GTKMM_CFLAGS_OTHER})

set (
    SOURCES
    src/main.cc
    src/peerspeak_window.cc
    ${CMAKE_CURRENT_BINARY_DIR}/resources.c)

add_executable (peerspeak ${SOURCES})

target_link_libraries (peerspeak ${GTKMM_LIBRARIES})
set_property (TARGET peerspeak PROPERTY CXX_STANDARD 14)

install (TARGETS peerspeak DESTINATION bin)

#file (COPY TypingTest.desktop DESTINATION ${CMAKE_BINARY_DIR})
#install (FILES ${CMAKE_BINARY_DIR}/TypingTest.desktop DESTINATION share/applications)
