cmake_minimum_required (VERSION 2.6)
project(Peerspeak)

find_package (PkgConfig REQUIRED)
pkg_check_modules (GTKMM REQUIRED gtkmm-3.0)

include_directories (${GTKMM_INCLUDE_DIRS})
link_directories (${GTKMM_LIBRARY_DIRS})
add_definitions (${GTKMM_CFLAGS_OTHER})

set (
	SOURCES
	src/main.cpp)

add_executable (peerspeak ${SOURCES})

target_link_libraries (peerspeak ${GTKMM_LIBRARIES})
set_property (TARGET peerspeak PROPERTY CXX_STANDARD 11)

install (TARGETS peerspeak DESTINATION bin)

#file (COPY TypingTest.desktop DESTINATION ${CMAKE_BINARY_DIR})
#install (FILES ${CMAKE_BINARY_DIR}/TypingTest.desktop DESTINATION share/applications)
