project(YAGF)
cmake_minimum_required(VERSION 2.8)

if (WIN32)
  set(GLEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/glew-1.12.0/include")
  set(GLEW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/deps/glew-1.12.0/lib/Release/x64/glew32s.lib")
  set(GLFW_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw/include")
  set(GLFW_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw/lib-vc2013/glfw3.lib")
  set(FREETYPE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/deps/freetype2/include")
  set(FREETYPE_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/deps/freetype2/lib/freetype.lib")
else ()
  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLFW REQUIRED glfw3)
endif()

find_package(Freetype REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(
  ${GLEW_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${OPENGL_INCLUDE_DIR}
  ${FREETYPE_INCLUDE_DIRS}
  "${FREETYPE_INCLUDE_DIRS}/freetype2"
  "${CMAKE_CURRENT_SOURCE_DIR}/include")

ADD_DEFINITIONS(-std=c++11)

file(GLOB_RECURSE HEADERS_LIST include "*.h")
add_library(${PROJECT_NAME} runtime.cpp ${HEADERS_LIST})
target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARY} ${GLFW_LIBRARY} ${OPENGL_LIBRARY})

add_executable(ssao examples/ssao.cpp)
target_link_libraries(ssao ${PROJECT_NAME})

add_executable(gaussianblur examples/gaussianblur.cpp)
target_link_libraries(gaussianblur ${PROJECT_NAME})

add_executable(text examples/text.cpp)
target_link_libraries(text ${PROJECT_NAME} ${FREETYPE_LIBRARY})


