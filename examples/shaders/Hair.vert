
#version 430
uniform mat4 ModelMatrix;
uniform mat4 ViewProjectionMatrix;
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Tangent;
out float depth;
out vec3 tangent;
void main(void) {
  gl_Position = ViewProjectionMatrix * ModelMatrix * vec4(Position, 1.);
  depth = gl_Position.z;
  tangent = Tangent;
}