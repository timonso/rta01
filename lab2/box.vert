#version 330 core

// Input
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexColor;

// TODO: Enable this for vertex's UV input to this vertex shader 
layout(location = 2) in vec2 vertexUV;

// Output data, to be interpolated for each fragment
out vec3 color;

// TODO: Enable this for UV output to fragment shader 
out vec2 uv;

// Matrix for vertex transformation
uniform mat4 MVP;

void main() {
    // Transform vertex
    gl_Position =  MVP * vec4(vertexPosition, 1);
    
    // Pass vertex color to the fragment shader
    color = vertexColor;

    // TODO: Enable this to pass UV to the fragment shader    
    uv = vertexUV;
}
