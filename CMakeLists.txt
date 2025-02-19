# CMakeList.txt : CMake project for vulkan_guide, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("rRootage-ex")

set (CMAKE_CXX_STANDARD 17)

message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")		# e.g. `embuild`
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")		# TODO: (?) project base dir?

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
	# setup SDL library
	find_package(SDL2 REQUIRED)
	message(STATUS "SDL2 header path: ${SDL2_INCLUDE_DIRS}")
	message(STATUS "SDL2 library: ${SDL2_LIBRARIES}")

	add_library(SDLlibs INTERFACE)

	target_include_directories(SDLlibs INTERFACE "${SDL2_INCLUDE_DIRS}")

	target_link_libraries(SDLlibs INTERFACE "${SDL2_LIBRARIES}")
endif()

add_subdirectory(src)
