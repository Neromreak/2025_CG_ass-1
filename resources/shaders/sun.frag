#version 150

in  vec3 pass_Normal;
in  vec3 pass_Pos;
out vec4 out_Color;

uniform vec3 CamPos;
uniform int IsCelShading;
uniform float Scale;

void main()
{
  // Normalize normal vector
  vec3 normal = normalize(pass_Normal);

  vec3 cam_direction = normalize(CamPos - pass_Pos);
  float cam_distance = distance(CamPos, pass_Pos);
  
  // Beta = angle between surface normal and camera direction
  float cos_beta = max(dot(normal, cam_direction), 0.0f);

  out_Color = vec4(0.8f + cos_beta * 0.5f,
                   0.3f + cos_beta * 0.6f,
                   0.1f + cos_beta * 0.4f,
                   1.0f);
  

  // ########### CEL SHADING: #######################################
  if(IsCelShading == 1)
  {
    // Add white outline around sphere
    // Outline threshold should be uniform for all spheres
    float cel_shading_outline_threshold = clamp(0.8f - (pow(Scale, 0.5f) * 0.25f), 0.2f, 0.8f);
    cel_shading_outline_threshold *= pow(cam_distance, 0.2f) * 0.5f;

    if(cos_beta < cel_shading_outline_threshold)
    {
      out_Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
      // Apply color discretization
      int cel_shade_count = 5;
      vec3 new_Color = vec3(0.0f, 0.0f, 0.0f);
      for(int i = 0; i < 3; ++i)
      {
        new_Color[i] = round(out_Color[i] * cel_shade_count) / cel_shade_count;
      }
      out_Color = vec4(new_Color, 1.0f);
    }
  }
}
