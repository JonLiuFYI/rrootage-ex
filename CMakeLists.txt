# CMakeList.txt : CMake project for vulkan_guide, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("rRootage")

set (CMAKE_CXX_STANDARD 17)


include(FetchContent)


# setup SDL library
set(SDL_PATH "Path/To/SDL" CACHE  PATH "path to SDL installation")
set(SDLMIXER_PATH "Path/To/SDLMixer" CACHE  PATH "path to SDL Mixer installation")
set(DOWNLOAD_DEPS true CACHE BOOL "Automatic download SDL")


add_library(SDLlibs INTERFACE)

if(NOT MSVC)
	find_path(SDL_PATH SDL.h
	HINTS
		ENV SDLDIR
	PATH_SUFFIXES SDL SDL12 SDL11
					# path suffixes to search inside ENV{SDLDIR}
					include/SDL include/SDL12 include/SDL11 include
	)
	find_path(SDLMIXER_INCLUDE SDL_mixer.h
	HINTS
		ENV SDLDIR
	PATH_SUFFIXES SDL SDL12 SDL11
					# path suffixes to search inside ENV{SDLDIR}
					include/SDL_mixer include/SDL12 include/SDL11 include
	)
	
	message(STATUS "Found SDL At ${SDL_PATH}" )
	message(STATUS "Found SDLMixer At ${SDLMIXER_INCLUDE}" )
target_include_directories(SDLlibs INTERFACE "${SDLMIXER_INCLUDE}")
else()

	if(${DOWNLOAD_DEPS})
		FetchContent_Declare(
		ext_sdllib
		URL https://www.libsdl.org/release/SDL2-devel-2.0.16-VC.zip
		SOURCE_DIR "libs/SDL"
		)
		
		FetchContent_MakeAvailable(ext_sdllib)
		
		FetchContent_Declare(
		ext_sdlmixer
		URL https://www.libsdl.org/projects/SDL_mixer/release/SDL2_mixer-devel-2.0.4-VC.zip
		SOURCE_DIR "libs/SDL_Mixer"
		)
		
		FetchContent_MakeAvailable(ext_sdlmixer)
		
		set(SDL_PATH "${CMAKE_BINARY_DIR}/libs/SDL/")
		set(SDLMIXER_PATH "${CMAKE_BINARY_DIR}/libs/SDL_Mixer/")

		message(STATUS "Found SDL At ${SDL_PATH}" )
		message(STATUS "Found SDLMixer At ${SDLMIXER_PATH}" )	
	endif()
endif()






target_link_directories(SDLlibs INTERFACE "${SDL_PATH}/lib/x64" "${SDLMIXER_PATH}/lib/x64")
target_link_libraries(SDLlibs INTERFACE SDL2 SDL2main SDL2_mixer)
target_include_directories(SDLlibs INTERFACE "${SDL_PATH}/include" "${SDLMIXER_PATH}/include")



add_subdirectory(src)
