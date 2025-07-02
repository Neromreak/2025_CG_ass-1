#version 150

// In variables
in vec3 pass_TexCoord;

// Uniforms
uniform samplerCube TextureColor;

// Out variables
out vec4 out_Color;


void main()
{
  vec4 tex_color = texture(TextureColor, pass_TexCoord);
  out_Color = tex_color - vec4(0.3f, 0.3f, 0.3f, 0.0f);
}
