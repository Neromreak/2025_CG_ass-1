#version 150

// In variables
in vec3 pass_Normal;
in vec3 pass_Pos;

// Uniforms
uniform vec3 LightPositions[];
uniform vec3 LightColors[];
uniform float LightIntensities[];
uniform vec3 ObjColor;

// Out variables
out vec4 out_Color;

void main()
{
  // Temp DEBUG
  vec3 temp = (LightPositions[0] + LightColors[0] + ObjColor * LightIntensities[0]).xyz;

  vec3 light_direction = normalize(vec3(0.0f, 0.0f, 0.0f) - pass_Pos);
  
  // normalize normal vector
  vec3 normal = normalize(pass_Normal);

  float theta = max(dot(normal, light_direction), 0.0f);
  // vec4 diffuse = vec4(theta * abs(normalize(pass_Normal)), 1.0f);
  vec4 diffuse = vec4(theta * 0.9f, theta * 0.9f, theta * 0.9f, 1.0f);
  vec4 ambient = vec4(0.05f, 0.05f, 0.05f, 1.0f);
  out_Color = ambient + diffuse;
  
 // DEBUG
  out_Color += vec4(0.001f * temp, 1.0f);
}
