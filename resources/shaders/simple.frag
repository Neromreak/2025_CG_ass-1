#version 150

in  vec3 pass_Normal;
in  vec3 pass_Pos;
out vec4 out_Color;

void main() {
  vec3 light_direction = normalize(vec3(0.0f, 0.0f, 0.0f) - pass_Pos);
  
  // normalize normal vector
  vec3 normal = normalize(pass_Normal);

  float theta = max(dot(normal, light_direction), 0.0f);
  // vec4 diffuse = vec4(theta * abs(normalize(pass_Normal)), 1.0f);
  vec4 diffuse = vec4(theta * 0.9f, theta * 0.9f, theta * 0.9f, 1.0f);
  vec4 ambient = vec4(0.05f, 0.05f, 0.05f, 1.0f);
  out_Color = ambient + diffuse;
}
