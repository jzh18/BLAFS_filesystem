cmake_minimum_required(VERSION 3.10)

project(simple_file_reader)

option(STATIC "build static library" ON)

add_executable(simple_file_reader main.cpp)
# build the executable staticly
if(STATIC)
	message("build static library")
	target_link_libraries(simple_file_reader PUBLIC "-static")
endif()
