project(YAGF)
cmake_minimum_required(VERSION 2.8)

if (WIN32)
  # GLEW
  add_library(glew STATIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/glew-1.12.0/src/glew.c" "${CMAKE_CURRENT_SOURCE_DIR}/deps/glew-1.12.0/src/glewinfo.c")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/glew-1.12.0/include")
  set(FREETYPE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/deps/freetype2/include")
  set(FREETYPE_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/deps/freetype2/lib/freetype.lib")
  # GLFW
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw-3.1.1")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw-3.1.1/include")
else ()
  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLFW REQUIRED glfw3)
endif()

find_package(Freetype REQUIRED)
#find_package(GLEW REQUIRED)
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
if (WIN32)
  target_link_libraries(${PROJECT_NAME} glew ${GLEW_LIBRARY} glfw ${GLFW_LIBRARIES} ${OPENGL_LIBRARY})
else()
  target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARY})
endif()

add_executable(ssao examples/ssao.cpp)
target_link_libraries(ssao ${PROJECT_NAME} ${FREETYPE_LIBRARY})
if (WIN32)
add_custom_command(TARGET ssao POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/freetype6.dll"
         $<TARGET_FILE_DIR:ssao>)
 add_custom_command(TARGET ssao POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/zlib1.dll"
         $<TARGET_FILE_DIR:ssao>)
endif()


add_executable(gaussianblur examples/gaussianblur.cpp)
target_link_libraries(gaussianblur ${PROJECT_NAME} ${FREETYPE_LIBRARY})
if (WIN32)
add_custom_command(TARGET gaussianblur POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/freetype6.dll"
         $<TARGET_FILE_DIR:gaussianblur>)
 add_custom_command(TARGET gaussianblur POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/zlib1.dll"
         $<TARGET_FILE_DIR:gaussianblur>)
endif()



add_executable(text examples/text.cpp)
target_link_libraries(text ${PROJECT_NAME} ${FREETYPE_LIBRARY})
if (WIN32)
add_custom_command(TARGET text POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/freetype6.dll"
         $<TARGET_FILE_DIR:text>)
 add_custom_command(TARGET text POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/zlib1.dll"
         $<TARGET_FILE_DIR:text>)
endif()

add_executable(oit examples/oit.cpp)
target_link_libraries(oit ${PROJECT_NAME} ${FREETYPE_LIBRARY})
if (WIN32)
add_custom_command(TARGET oit POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/freetype6.dll"
         $<TARGET_FILE_DIR:oit>)
 add_custom_command(TARGET oit POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/zlib1.dll"
         $<TARGET_FILE_DIR:oit>)
endif()


add_executable(mesh examples/mesh.cpp)
target_link_libraries(mesh ${PROJECT_NAME})

add_executable(tfx examples/TressFX/tfx.cpp examples/TressFX/common.cpp)
target_link_libraries(tfx ${PROJECT_NAME} ${FREETYPE_LIBRARY})
if (WIN32)
add_custom_command(TARGET tfx POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/freetype6.dll"
         $<TARGET_FILE_DIR:tfx>)
 add_custom_command(TARGET tfx POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/zlib1.dll"
         $<TARGET_FILE_DIR:tfx>)
endif()

include_directories("C:/Program Files (x86)/AMD APP SDK/3.0-0-Beta/include")

add_executable(tfx-ocl examples/TressFX/tfx-ocl.cpp examples/TressFX/common.cpp)
target_link_libraries(tfx-ocl ${PROJECT_NAME} "C:/Program Files (x86)/AMD APP SDK/3.0-0-Beta/lib/x86/OpenCL.lib" ${FREETYPE_LIBRARY})
if (WIN32)
add_custom_command(TARGET tfx-ocl POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/freetype6.dll"
         $<TARGET_FILE_DIR:tfx-ocl>)
 add_custom_command(TARGET tfx-ocl POST_BUILD
     COMMAND ${CMAKE_COMMAND} -E copy_if_different
         "${PROJECT_SOURCE_DIR}/deps/freetype2/bin/zlib1.dll"
         $<TARGET_FILE_DIR:tfx-ocl>)
endif()