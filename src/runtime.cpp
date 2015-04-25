#include <vector>



const char *screenquadshader = "#version 330\n"
"layout(location = 0) in vec2 Position;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(Position, 0., 1.);\n"
"}\n";


std::vector<void(*)()> CleanTable;