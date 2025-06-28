#version 150
#extension GL_ARB_explicit_attrib_location : require

// Vertex attributes of VAO
layout(location = 0) in vec3 in_Position;
layout(location = 1) in vec3 in_Normal;
layout(location = 2) in vec2 in_TexCoord;

// Matrix Uniforms as specified with glUniformMatrix4fv
uniform mat4 ModelMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ProjectionMatrix;
uniform mat4 NormalMatrix;

// Out variables
out vec3 pass_Normal;
out vec3 pass_Pos;
out vec2 pass_TexCoord;

void main(void)
{
  // Calculate projected position and normal
  pass_Pos = (ModelMatrix * vec4(in_Position, 1.0f)).xyz;
  gl_Position = ProjectionMatrix * ViewMatrix * vec4(pass_Pos, 1.0);
  pass_Normal = (NormalMatrix * vec4(in_Normal, 0.0f)).xyz;

  // Pass texture coordinates
  pass_TexCoord = in_TexCoord;
}
