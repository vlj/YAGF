project(YAGF)
cmake_minimum_required(VERSION 3.6)

add_definitions(-DGLEW_STATIC)
find_package(GLEW REQUIRED)
#find_package(glfw3 3.2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype)

option(BUILD_OPENCL "Build Sample using OpenCL" OFF)
option(BUILD_DX12 "Build Sample using DX12" OFF)

include_directories(
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
   "${FREETYPE_INCLUDE_DIRS}/freetype2"
   "deps/d3d12"
   "include"
)

add_subdirectory(src)

#add_executable(ssao examples/ssao.cpp)
#target_link_libraries(ssao ${PROJECT_NAME})

#add_executable(gaussianblur examples/gaussianblur.cpp)
#target_link_libraries(gaussianblur ${PROJECT_NAME})

#add_executable(skybox examples/skybox.cpp src/glapi.cpp src/glshaders.cpp)
#target_link_libraries(skybox ${PROJECT_NAME})

#add_executable(ibl examples/ibl.cpp src/glapi.cpp src/glshaders.cpp src/ibl.cpp)
#target_link_libraries(ibl ${PROJECT_NAME})
#set_target_properties(ibl PROPERTIES COMPILE_DEFINITIONS "GLBUILD")

#add_executable(text examples/text.cpp)
#target_link_libraries(text ${PROJECT_NAME})

#add_executable(oit examples/oit.cpp)
#target_link_libraries(oit ${PROJECT_NAME})

#add_executable(mesh examples/mesh.cpp)
#target_link_libraries(mesh ${PROJECT_NAME})

#add_executable(tfx examples/TressFX/tfx.cpp examples/TressFX/common.cpp)
#target_link_libraries(tfx ${PROJECT_NAME})

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
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/skyboxvert.hlsl PROPERTIES VS_SHADER_TYPE Vertex VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/sunlight.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/tonemap.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/linearize.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/skybox.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/importance_sampling_specular.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/computesh.hlsl PROPERTIES VS_SHADER_TYPE Compute VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/ibl.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )

  SET_SOURCE_FILES_PROPERTIES(examples/shaders/ssao_vtx.hlsl PROPERTIES VS_SHADER_TYPE Vertex VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/ssao_pix.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
  SET_SOURCE_FILES_PROPERTIES(examples/shaders/pix.hlsl PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main )
endif()

if(BUILD_DX12)
  add_executable(meshdx12 src/d3dapi.cpp src/d3dshaders.cpp examples/meshdx12.cpp examples/shaders/skinnedobject.hlsl examples/shaders/object_gbuffer.hlsl)
  set_target_properties(meshdx12 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
  target_link_libraries(meshdx12 ${PROJECT_NAME})
  add_executable(ssaodx12 src/d3dapi.cpp examples/ssaodx12.cpp examples/shaders/ssao_vtx.hlsl examples/shaders/ssao_pix.hlsl examples/shaders/screenquad.hlsl examples/shaders/linearize.hlsl)
  set_target_properties(ssaodx12 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
  target_link_libraries(ssaodx12 ${PROJECT_NAME})
  add_executable(skyboxdx12 src/d3dapi.cpp src/d3dshaders.cpp examples/skyboxdx12.cpp examples/shaders/screenquad.hlsl examples/shaders/skybox.hlsl)
  set_target_properties(skyboxdx12 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
  target_link_libraries(skyboxdx12 ${PROJECT_NAME})
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/examples/Scene/include")
#add_executable(scene
#  examples/Scene/main.cpp
#  examples/Scene/scene.cpp
#  examples/Scene/material.cpp
#  examples/Scene/meshscenenode.cpp
#  examples/Scene/texturemanager.cpp
#  src/glshaders.cpp
#  src/ibl.cpp
#  src/glapi.cpp)
#set_target_properties(scene PROPERTIES COMPILE_DEFINITIONS "GLBUILD")
#target_link_libraries(scene ${PROJECT_NAME})

if(BUILD_DX12)
  add_executable(scenedx12
    src/d3dapi.cpp
    src/ibl.cpp
    examples/Scene/main.cpp
    examples/Scene/scene.cpp
    examples/Scene/material.cpp
    examples/Scene/meshscenenode.cpp
    examples/Scene/texturemanager.cpp
    examples/shaders/sunlight.hlsl
    src/d3dshaders.cpp
    examples/shaders/object.hlsl
    examples/shaders/object_gbuffer.hlsl
    examples/shaders/skybox.hlsl
    examples/shaders/skyboxvert.hlsl
    examples/shaders/importance_sampling_specular.hlsl
    examples/shaders/computesh.hlsl
    examples/shaders/ibl.hlsl
    examples/shaders/tonemap.hlsl)
  set_target_properties(scenedx12 PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
  set_target_properties(scenedx12 PROPERTIES COMPILE_DEFINITIONS "DXBUILD")
  target_link_libraries(scenedx12 ${PROJECT_NAME})
endif()