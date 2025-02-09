#version 330 core

in vec3 color;

// TODO: Enable this to receive UV input to this fragment shader 
in vec2 uv;

// TODO: Enable this to access the texture sampler
uniform sampler2D textureSampler;

out vec3 finalColor;

void main()
{
	finalColor = color;

	// TODO: Enable this for texture lookup. 
	// The texture() function takes as input the texture sampler 
	// and UV coordinate and returns a texture color at the UV location. 
	 finalColor = color * texture(textureSampler, uv).rgb;
}
