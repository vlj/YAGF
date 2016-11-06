project(mesh)

file(GLOB_RECURSE HEADERS "*.h")
file(GLOB SOURCES "*.cpp")
add_executable(mesh ${HEADERS} ${SOURCES})
target_link_libraries(mesh ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${FREETYPE_LIBRARY} ${OPENGL_LIBRARY})
