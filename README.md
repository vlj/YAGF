# YAGF
Yet Another GL Framework

MIT License except if file header states otherwise.

Dependencies:
* GLFW3
* Freetype 2
* Assimp
* Vulkan
* Shaderc
* GLM
* GLI
* gflags

All these dependencies are provided by vcpkg on Windows.
Visual Studio 2017 RC is required (YAGF requires variant and optional templates)

YOU NEED TO BUILD INSIDE A SUBFOLDER OF YAFG/ otherwise projects can't find
binaries/shader files at runtime.