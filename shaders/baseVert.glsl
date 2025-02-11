#version 330

in vec3 vertex_position;
in vec3 vertex_normal;
in vec2 vertex_texture;

out vec3 PointNormal;
out vec3 PointPosition;

out vec2 TextureCoords;
out mat4 ModelView;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

void main(){
  mat4 ModelViewMatrix = view * model;

  // Convert vertex normal, and light position to view coords
  // Normal in view space
  PointNormal = normalize(vec3(ModelViewMatrix * vec4(vertex_normal, 0.0)));

  // Position in view space
  PointPosition = vec3(ModelViewMatrix * vec4(vertex_position ,1.0));

  // Pass texture coordinates to fragment shader
  TextureCoords = vertex_texture;

  // Convert position to clip coordinates and pass along
  gl_Position = proj * view * model * vec4(vertex_position, 1.0);
}