project(lib)

file(GLOB_RECURSE HEADERS "../include/*.h")
file(GLOB SOURCES
    "ibl.cpp"
    "meshscenenode.cpp"
    "scene.cpp"
    "ssao.cpp"
    "textures.cpp")
add_library(YAGF ${HEADERS} ${SOURCES})
target_link_libraries(YAGF ${GLEW_LIBRARY} ${GLFW_LIBRARIES} ${FREETYPE_LIBRARY} ${OPENGL_LIBRARY})
