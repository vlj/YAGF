project(mesh)

file(GLOB_RECURSE HEADERS "*.h")
file(GLOB SOURCES
	"interface.cpp"
	"mesh.cpp"
	"shaders.cpp"
	"vulkan_interface.cpp")
add_executable(mesh ${HEADERS} ${SOURCES})
target_link_libraries(mesh YAGF ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${FREETYPE_LIBRARY} ${OPENGL_LIBRARY})
