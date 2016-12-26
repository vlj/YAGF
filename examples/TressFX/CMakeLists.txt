project(tressFX)

find_package(gflags REQUIRED)
include_directories(${gflags_INCLUDE_DIR})

file(GLOB_RECURSE HEADERS "TressFX.Vulkan/*.h")
file(GLOB SOURCES
	"TressFX.Vulkan/main.cpp"
	"TressFX.Vulkan/sample.cpp"
	"TressFX.Vulkan/StringTokenizer.cpp"
	"TressFX.Vulkan/TFXFileIO.cpp"
	)
add_executable(tressFX ${HEADERS} ${SOURCES})
target_link_libraries(tressFX YAGF ${GLEW_LIBRARY} glfw3dll ${FREETYPE_LIBRARY} ${OPENGL_LIBRARY} gflags)
