#version 410 core

layout (location = 0) in vec3 inPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 project;

void main() {
    gl_Position = project * view * model * vec4(inPosition, 1.0);
}