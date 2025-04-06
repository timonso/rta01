#version 330 core

uniform vec3 lightColor;

out vec4 finalColor;

void main() {
  finalColor = vec4(lightColor, 1.0);
}