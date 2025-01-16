# CMakeList.txt : CMake project for vulkan_guide, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("rRootage")

set (CMAKE_CXX_STANDARD 17)


include(FetchContent)

# setup SDL library
#set(SDL_PATH "/usr/include/SDL" CACHE  PATH "path to SDL installation")
set(DOWNLOAD_DEPS true CACHE BOOL "Automatic download SDL")

find_package(SDL2 REQUIRED)
message(STATUS "SDL2 header path: ${SDL2_INCLUDE_DIRS}")
message(STATUS "SDL2 library: ${SDL2_LIBRARIES}")

add_library(SDLlibs INTERFACE)

if(NOT MSVC)
	#find_path(SDL_PATH SDL.h
	#HINTS
	#	ENV SDLDIR
	#PATH_SUFFIXES SDL SDL12 SDL11
	#				# path suffixes to search inside ENV{SDLDIR}
	#				include/SDL include/SDL2 include
	#)
		
	#message(STATUS "Found SDL At ${SDL_PATH}" )
	target_include_directories(SDLlibs INTERFACE "${SDL2_INCLUDE_DIRS}")
else()

	if(${DOWNLOAD_DEPS})
		FetchContent_Declare(
		ext_sdllib
		URL https://www.libsdl.org/release/SDL2-devel-2.0.16-VC.zip
		SOURCE_DIR "libs/SDL"
		)
		
		FetchContent_MakeAvailable(ext_sdllib)
		
	
		
		set(SDL_PATH "${CMAKE_BINARY_DIR}/libs/SDL/")

		message(STATUS "Found SDL At ${SDL_PATH}" )
	endif()
endif()






#target_link_directories(SDLlibs INTERFACE "${SDL_PATH}/lib/x64")
target_link_libraries(SDLlibs INTERFACE "${SDL2_LIBRARIES}")
#target_include_directories(SDLlibs INTERFACE "${SDL_PATH}/include")

add_subdirectory(src)
