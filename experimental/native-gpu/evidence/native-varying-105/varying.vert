#version 450
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;
layout(location = 0) out vec4 interpolated_color;
void main() {
    gl_Position = position;
    interpolated_color = color;
}
