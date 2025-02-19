# CMakeList.txt : CMake project for vulkan_guide, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

add_subdirectory(bulletml)


file(GLOB SOURCE_FILES
"${CMAKE_CURRENT_SOURCE_DIR}/*.c"
"${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/*.h" )


add_executable(rrootage-ex ${SOURCE_FILES})
target_include_directories(rrootage-ex PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")



if(NOT MSVC)
    target_link_libraries(rrootage-ex SDLlibs bulletML GL GLU)
else()
    target_link_libraries(rrootage-ex SDLlibs bulletML opengl32 glu32)
endif()
set_property(TARGET rrootage-ex PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/..")

# copy exe to the root folder, both windows and linux
add_custom_command(TARGET rrootage-ex POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:rrootage-ex> "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

# only copy the DLLs on windows
if( WIN32 )
    # copy dlls to the root folder
    add_custom_command(TARGET rrootage-ex POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${SDL_PATH}/lib/x64/SDL2.dll" "${CMAKE_CURRENT_SOURCE_DIR}/.."
    )

    # copy dlls to the bin folder
    add_custom_command(TARGET rrootage-ex POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${SDL_PATH}/lib/x64/SDL2.dll"  $<TARGET_FILE_DIR:rrootage-ex>
    )
endif()


if(NOT MSVC)
    set_target_properties(rrootage-ex PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-deprecated-declarations")
endif()

