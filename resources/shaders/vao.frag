# version 150

// In variables
in vec3 pass_Color;

// Uniforms


// Out variables
out vec4 out_Color;


void main ()
{
  // Simply pass the color
  out_Color = vec4 (pass_Color, 1.0);
}