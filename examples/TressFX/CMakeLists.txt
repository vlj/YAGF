project(tressFX)

find_package(gflags REQUIRED)
include_directories(${gflags_INCLUDE_DIR})
include_directories("../../deps/tressfx/amd_tressfx/inc")
include_directories("../../deps/tressfx/amd_lib/shared/common/inc")

add_definitions("-DAMD_TRESSFX_VULKAN=1")

file(GLOB SHADERS "shaders/*.comp" "shaders/*.vert" "shaders/*.frag")
file(GLOB_RECURSE HEADERS "TressFX.Vulkan/*.h")
file(GLOB SOURCES
	"TressFX.Vulkan/main.cpp"
	"TressFX.Vulkan/sample.cpp"
	"TressFX.Vulkan/StringTokenizer.cpp"
	"TressFX.Vulkan/TFXFileIO.cpp"
	)
add_executable(tressFX ${HEADERS} ${SOURCES} ${SHADERS})
target_link_libraries(tressFX YAGF ${GLEW_LIBRARY} glfw3dll ${FREETYPE_LIBRARY} ${OPENGL_LIBRARY} gflags)

add_definitions("-DAMD_TRESSFX_VULKAN=1")

set(directory "${CMAKE_CURRENT_BINARY_DIR}/generatedShaders")
file(MAKE_DIRECTORY ${directory})


foreach(F ${SHADERS})
    get_filename_component(outF ${F} NAME_WE)
    set(outF ${outF}.h)
    add_custom_command(TARGET tressFX
        PRE_BUILD
        COMMAND ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/glslc.exe ${F} -mfmt=c -o ${directory}/${outF}
        COMMENT "Compiling ${F}")
endforeach()

# For shader content
include_directories(${CMAKE_CURRENT_BINARY_DIR})
