#version 150

in  vec3 pass_Normal;
out vec4 out_Color;

uniform vec3 frag_pos;
uniform int is_sun;

void main() {
  vec3 light_direction = normalize(vec3(0.0f, 0.0f, 0.0f) - frag_pos);
  float theta = max(dot(pass_Normal, light_direction), 0.0f);
  // vec4 diffuse = vec4(theta * abs(normalize(pass_Normal)), 1.0f);
  vec4 diffuse = vec4(theta * 0.8f, theta * 0.8f, theta * 0.8f, 1.0f);
  vec4 ambient = vec4(0.1f, 0.1f, 0.1f, 1.0f);
  out_Color = ambient + diffuse;

  if(is_sun == 1)
  {
    out_Color = vec4(0.9f, 0.6f, 0.2f, 1.0f);
  }
}
