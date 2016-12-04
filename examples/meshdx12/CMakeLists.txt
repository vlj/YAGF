project(mesh)

find_package(gflags REQUIRED)
include_directories(${gflags_INCLUDE_DIR})

file(GLOB_RECURSE HEADERS "*.h")
file(GLOB SOURCES
	"interface.cpp"
	"mesh.cpp"
	"shaders.cpp"
	"vulkan_interface.cpp")
add_executable(mesh ${HEADERS} ${SOURCES})
target_link_libraries(mesh YAGF ${GLEW_LIBRARY} glfw3dll ${FREETYPE_LIBRARY} ${OPENGL_LIBRARY} gflags ${ASSIMP_LIBRARIES})
