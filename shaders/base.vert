#version 330 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexture;
in vec3 vertexTangent;
in vec3 vertexBitangent;

out vec3 PointNormal;
out vec3 PointPosition;

out vec2 TextureCoords;
out mat3 TBN;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelMatrix;

void main(){
  mat4 ModelViewMatrix = viewMatrix * modelMatrix;
  mat3 MV3 = mat3(ModelViewMatrix);

  // Normal in view space
  PointNormal = normalize(vec3(ModelViewMatrix * vec4(vertexNormal, 0.0)));

  vec3 normalView = vec3(PointNormal);
  vec3 tangentView = normalize(MV3 * vertexTangent);
  vec3 bitangentView = normalize(MV3 * vertexBitangent);
  bitangentView = cross(normalView, tangentView);

  TBN = transpose(mat3(tangentView, bitangentView, normalView));

  // Position in view space
  PointPosition = vec3(ModelViewMatrix * vec4(vertexPosition ,1.0));

  // Pass texture coordinates to fragment shader
  TextureCoords = vertexTexture;

  // Convert position to clip coordinates and pass along
  gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(vertexPosition, 1.0);
}