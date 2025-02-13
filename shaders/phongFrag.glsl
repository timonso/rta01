#version 330 core

uniform sampler2D diffuseMap;
uniform sampler2D alphaMap;

uniform vec4 baseColor;
uniform vec4 specularColor;
uniform float alpha;
uniform float phongExp;
uniform float specularCoeff;
uniform vec3 lightPosition;
uniform vec4 ambientColor;

in vec3 PointNormal;
in vec3 PointPosition;
in vec2 TextureCoords;

out vec4 finalColor;

// light source properties
vec3 Ld = vec3(1.0, 1.0, 1.0);
vec3 Ls = vec3(1.0 ,1.0, 1.0);
vec3 La = vec3(0.0, 0.0, 0.5);

vec4 diffuseColor = texture(diffuseMap, TextureCoords) + baseColor;
float opacity = texture(alphaMap, TextureCoords).r + alpha;

float Ks = specularCoeff;
float Kd = 0.5;
float Ka = 0.2;

void main() {
	// Phong illumination
	vec3 L = normalize(lightPosition - PointPosition);
	vec3 V = normalize(-PointPosition);
	vec3 R = normalize(reflect(-L, PointNormal));

	float dotProdDiff = max(dot(L, PointNormal), 0.0);
	float dotProdSpec = max(dot(R, V), 0.0);
	float specularFactor = max(pow(dotProdSpec, phongExp), 0.0);

	vec3 Id = Ld * Kd * dotProdDiff * vec3(diffuseColor);
	vec3 Is = Ls * Ks * specularFactor;
	vec3 Ia = La * Ka;

	vec4 phongColor = vec4(Id + Is + Ia, 1.0);

	finalColor = phongColor;
}