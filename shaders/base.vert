#version 330 core

in vec3 vertex_position;
in vec3 vertex_normal;
in vec2 vertex_texture;
in vec3 vertex_tangent;
in vec3 vertex_bitangent;

out vec3 PointNormal;
out vec3 PointPosition;

out vec2 TextureCoords;
out mat3 TBN;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

void main(){
  mat4 ModelViewMatrix = view * model;
  mat3 MV3 = mat3(ModelViewMatrix);

  // Normal in view space
  PointNormal = normalize(vec3(ModelViewMatrix * vec4(vertex_normal, 0.0)));

  vec3 normalView = vec3(PointNormal);
  vec3 tangentView = normalize(MV3 * vertex_tangent);
  vec3 bitangentView = normalize(MV3 * vertex_bitangent);
  bitangentView = cross(normalView, tangentView);

  TBN = transpose(mat3(tangentView, bitangentView, normalView));

  // Position in view space
  PointPosition = vec3(ModelViewMatrix * vec4(vertex_position ,1.0));

  // Pass texture coordinates to fragment shader
  TextureCoords = vertex_texture;

  // Convert position to clip coordinates and pass along
  gl_Position = proj * view * model * vec4(vertex_position, 1.0);
}