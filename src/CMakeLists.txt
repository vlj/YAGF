project(lib)

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

file(GLOB COMP_SHADERS "shaders/glsl/*.comp")
file(GLOB VERT_SHADERS "shaders/glsl/*.comp")
file(GLOB FRAG_SHADERS "shaders/glsl/*.comp")

file(GLOB_RECURSE HEADERS "../include/*.h")
file(GLOB SOURCES
    "ibl.cpp"
    "meshscenenode.cpp"
    "scene.cpp"
    "ssao.cpp"
    "textures.cpp"
    "vkapi.cpp")
#Z    "d3dapi.cpp")
add_library(YAGF ${HEADERS} ${SOURCES} ${COMP_SHADERS})
target_link_libraries(YAGF ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${FREETYPE_LIBRARY} ${OPENGL_LIBRARY} ${Vulkan_LIBRARY})

set(directory "${CMAKE_CURRENT_BINARY_DIR}/generatedShaders")
file(MAKE_DIRECTORY ${directory})

foreach(F ${COMP_SHADERS})
    get_filename_component(outF ${F} NAME_WE)
    set(outF ${outF}.h)
    add_custom_command(TARGET YAGF
        PRE_BUILD
        COMMAND glslc_exe ${F} -mfmt=c -o ${directory}/${outF}
        COMMENT "Compiling ${F}"
        DEPENDS glslc_exe)
endforeach()

# For shader content
include_directories(${CMAKE_CURRENT_BINARY_DIR})
