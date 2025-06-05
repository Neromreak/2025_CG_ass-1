#version 150

in  vec3 pass_Normal;
in  vec3 pass_Pos;
out vec4 out_Color;

uniform vec3 CamPos;

void main() {
  vec3 cam_direction = normalize(CamPos - pass_Pos);
  
  // Normalize normal vector
  vec3 normal = normalize(pass_Normal);

  float theta = max(dot(normal, cam_direction), 0.0f);
  //vec4 diffuse = vec4(abs(normalize(pass_Normal)), 1.0f);
  out_Color = vec4(0.8f + theta * 0.5f,
                   0.3f + theta * 0.6f,
                   0.1f + theta * 0.4f,
                   1.0f);
}
