#version 330

uniform vec4 baseColor;
uniform vec4 specularColor;
uniform float specularCoeff;
uniform float phongExp;
uniform vec3 lightPosition;

in vec3 PointNormal;
in vec3 PointPosition;

out vec4 finalColor;

// light source properties
vec3 Ld = vec3(1.0, 1.0, 1.0);
vec3 Ls = vec3(1.0 ,1.0, 1.0);
vec3 La = vec3(0.0, 0.0, 0.5);

float Ks = specularCoeff;
float Kd = 0.5;
float Ka = 0.2;

void main() {
	vec3 L = normalize(lightPosition - PointPosition);
	vec3 V = normalize(-PointPosition);
	vec3 R = reflect(-L, PointNormal);

	float dotProdDiff = max(dot(L, PointNormal), 0.0);
	float dotProdSpec = max(dot(R, V), 0.0);
	float specularFactor = max(pow(dotProdSpec, phongExp), 0.0);
	
	vec3 Id = Ld * Kd * dotProdDiff * vec3(baseColor);
	vec3 Is = Ls * Ks * specularFactor;
	vec3 Ia = La * Ka;

	vec4 phongColor = vec4(Id + Is + Ia, 1.0f);

	finalColor = vec4(1.0, 0.0, 0.0, 1.0);
}