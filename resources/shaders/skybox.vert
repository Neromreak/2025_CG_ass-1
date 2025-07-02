#version 150
#extension GL_ARB_explicit_attrib_location : require

// Vertex attributes of VAO
layout(location = 0) in vec3 in_Position;

// Matrix Uniforms as specified with glUniformMatrix4fv
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;

// Out variables
out vec3 pass_TexCoord;

void main(void)
{
  // Ignore view positoin and only take rotation as if the camera was in
  // ...the coordinate origin (and therefore in the center of the cube)
  mat4 view_rotation = ViewMatrix;
  view_rotation[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);

  // Calculate projected position and normal
  gl_Position = ProjectionMatrix * view_rotation * vec4(in_Position, 1.0);

  // Pass texture coordinates
  pass_TexCoord = in_Position;
}
