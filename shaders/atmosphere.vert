#version 330 core

in vec3 vertexPosition;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

out vec3 worldSpacePosition;

void main() {
    vec4 worldSpacePosition4 = modelMatrix * vec4(vertexPosition, 1.0);
    gl_Position = projectionMatrix * viewMatrix * worldSpacePosition4;
    worldSpacePosition = vec3(worldSpacePosition4);
}