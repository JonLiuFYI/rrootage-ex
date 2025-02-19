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
file(COPY "${PROJECT_SOURCE_DIR}/resources" DESTINATION ${CMAKE_BINARY_DIR})

# (?) should this be part of emscripten build?
target_include_directories(rrootage-ex PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# flags
if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    message(STATUS "EMSCRIPTEN build")

    set(USE_FLAGS "-s USE_SDL=2 --preload-file ${PROJECT_SOURCE_DIR}/resources/@/resources")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
    file(COPY "${PROJECT_SOURCE_DIR}/index.html" DESTINATION ${CMAKE_BINARY_DIR})
else()
    message(STATUS "LINUX build")
    set_target_properties(rrootage-ex PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-deprecated-declarations")
endif()

# link
if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    target_link_libraries(rrootage-ex bulletML GL GLU) # (?) figure out what to link
else()
    target_link_libraries(rrootage-ex SDLlibs bulletML GL GLU)
endif()
