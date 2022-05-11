#version 330 core

uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

layout(location = 0) in vec4 position;

void main()
{
    gl_Position = projMatrix * viewMatrix * modelMatrix * position;
}
