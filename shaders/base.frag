#version 330 core

uniform sampler2D diffuseMap;
uniform sampler2D alphaMap;
uniform sampler2D normalMap;

uniform vec4 baseColor;
uniform vec4 specularColor;
uniform float alpha;
uniform float phongExp;
uniform float specularCoeff;
uniform vec3 viewSpaceLightPosition;
uniform vec4 ambientColor;
uniform bool usesNormalMap;

in vec3 PointNormal;
in vec3 PointPosition;
in vec2 TextureCoords;
in mat3 TBN;

out vec4 finalColor;

// light source properties
vec3 Ld = vec3(1.0, 1.0, 1.0);
vec3 Ls = vec3(1.0 ,1.0, 1.0);
vec3 La = vec3(0.0, 0.0, 0.5);

vec4 diffuseColor = texture(diffuseMap, TextureCoords) + baseColor;
float opacity = texture(alphaMap, TextureCoords).r + alpha;

float Ks = 0.7;
float Kd = 0.6;
float Ka = 0.1;

void main() {
	vec3 normalMapValue = normalize(vec3(texture(normalMap, TextureCoords)) * 2.0 - 1.0);
	vec3 normal = usesNormalMap ? normalMapValue : PointNormal;

	// Phong illumination
	vec3 L = (usesNormalMap ? TBN : mat3(1.0)) * normalize(viewSpaceLightPosition - PointPosition);
	vec3 V = (usesNormalMap ? TBN : mat3(1.0)) * normalize(-PointPosition);
	vec3 R = normalize(reflect(-L, normal));

	float dotProdDiff = clamp(dot(L, normal), 0.0, 1.0);
	float dotProdSpec = clamp(dot(R, V), 0.0, 1.0);
	float specularFactor = max(pow(dotProdSpec, phongExp), 0.0);

	vec3 Id = Ld * Kd * dotProdDiff * vec3(diffuseColor);
	vec3 Is = Ls * Ks * specularFactor;
	vec3 Ia = La * Ka;

	vec4 phongColor = vec4(Id + Is + Ia, opacity);
	finalColor = phongColor;
	// finalColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
}