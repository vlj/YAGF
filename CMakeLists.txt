project(YAGF)
cmake_minimum_required(VERSION 2.8)

add_definitions(-DGLEW_STATIC)
option(BUILD_OPENCL "Build Sample using OpenCL" OFF)

option(BUILD_DX12 "Build Sample using DX12" OFF)

if (WIN32)
  # GLEW
  add_library(glew STATIC "${CMAKE_CURRENT_SOURCE_DIR}/deps/glew-1.12.0/src/glew.c" "${CMAKE_CURRENT_SOURCE_DIR}/deps/glew-1.12.0/src/glewinfo.c")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/glew-1.12.0/include")
  # Freetype 2
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/freetype-2.5.5")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/freetype-2.5.5/include")
  # GLFW
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw-3.1.1")
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/deps/glfw-3.1.1/include")
else ()
  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLFW REQUIRED glfw3)
  find_package(GLEW REQUIRED)
  find_package(Freetype REQUIRED)

  include_directories(
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    "${FREETYPE_INCLUDE_DIRS}/freetype2")

  ADD_DEFINITIONS(-std=c++11)
endif()

find_package(OpenGL REQUIRED)

include_directories(
  ${OPENGL_INCLUDE_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB_RECURSE HEADERS_LIST include "*.h")
add_library(${PROJECT_NAME} runtime.cpp ${HEADERS_LIST})
if (WIN32)
  target_link_libraries(${PROJECT_NAME} glew ${GLEW_LIBRARY} glfw ${GLFW_LIBRARIES} freetype ${FREETYPE_LIBRARY} ${OPENGL_LIBRARY})
else()
  target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${FREETYPE_LIBRARY} ${OPENGL_LIBRARY})
endif()

add_executable(ssao examples/ssao.cpp)
target_link_libraries(ssao ${PROJECT_NAME})

add_executable(gaussianblur examples/gaussianblur.cpp)
target_link_libraries(gaussianblur ${PROJECT_NAME})

add_executable(text examples/text.cpp)
target_link_libraries(text ${PROJECT_NAME})

add_executable(oit examples/oit.cpp)
target_link_libraries(oit ${PROJECT_NAME})

add_executable(mesh examples/mesh.cpp)
target_link_libraries(mesh ${PROJECT_NAME})

add_executable(tfx examples/TressFX/tfx.cpp examples/TressFX/common.cpp)
target_link_libraries(tfx ${PROJECT_NAME})

if(BUILD_OPENCL)
  include_directories("C:/Program Files (x86)/AMD APP SDK/3.0-0-Beta/include")
  add_executable(tfx-ocl examples/TressFX/tfx-ocl.cpp examples/TressFX/common.cpp)
  target_link_libraries(tfx-ocl ${PROJECT_NAME} "C:/Program Files (x86)/AMD APP SDK/3.0-0-Beta/lib/x86/OpenCL.lib")
endif()

if(BUILD_DX12)
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/object.hlsl PROPERTIES VS_SHADER_TYPE Vertex VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/skinnedobject.hlsl PROPERTIES VS_SHADER_TYPE Vertex VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/object_gbuffer.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )

  SET_SOURCE_FILES_PROPERTIES(examples/shaders/screenquad.hlsl PROPERTIES VS_SHADER_TYPE Vertex VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/sunlight.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/tonemap.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/linearize.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )

  SET_SOURCE_FILES_PROPERTIES(examples/shaders/ssao_vtx.hlsl PROPERTIES VS_SHADER_TYPE Vertex VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/ssao_pix.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/pix.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
endif()

if(BUILD_DX12)
  add_executable(meshdx12 d3dapi.cpp d3dshaders.cpp examples/meshdx12.cpp examples/shaders/skinnedobject.hlsl examples/shaders/object_gbuffer.hlsl)
  set_target_properties(meshdx12 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
  target_link_libraries(meshdx12 ${PROJECT_NAME})
  add_executable(ssaodx12 d3dapi.cpp examples/ssaodx12.cpp examples/shaders/ssao_vtx.hlsl examples/shaders/ssao_pix.hlsl examples/shaders/screenquad.hlsl examples/shaders/linearize.hlsl)
  set_target_properties(ssaodx12 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
  target_link_libraries(ssaodx12 ${PROJECT_NAME})
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/examples/Scene/include")
add_executable(scene
  examples/Scene/main.cpp
  examples/Scene/scene.cpp
  examples/Scene/material.cpp
  examples/Scene/meshscenenode.cpp
  examples/Scene/rendertargets.cpp
  examples/Scene/fullscreen.cpp
  glshaders.cpp
  examples/Scene/glapi.cpp)
set_target_properties(scene PROPERTIES COMPILE_DEFINITIONS "GLBUILD")
target_link_libraries(scene ${PROJECT_NAME})

if(BUILD_DX12)
  add_executable(scenedx12
    d3dapi.cpp
    examples/Scene/main.cpp
    examples/Scene/scene.cpp
    examples/Scene/material.cpp
    examples/Scene/meshscenenode.cpp
    examples/Scene/rendertargets.cpp
    examples/Scene/fullscreen.cpp
    examples/shaders/sunlight.hlsl
    d3dshaders.cpp
    examples/shaders/object.hlsl
    examples/shaders/object_gbuffer.hlsl
        examples/shaders/tonemap.hlsl)
  set_target_properties(scenedx12 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
  set_target_properties(scenedx12 PROPERTIES COMPILE_DEFINITIONS "DXBUILD")
  target_link_libraries(scenedx12 ${PROJECT_NAME})
endif()