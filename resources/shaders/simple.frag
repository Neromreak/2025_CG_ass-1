#version 150

// In variables
in vec3 pass_Normal;
in vec3 pass_Pos;
in vec2 pass_TexCoord;

// Uniforms
// (vectors need a constant size, so I just set it to 128 even though probably
// ...less lights are needed)
uniform int LightCount;
uniform vec3 LightPositions[128];
uniform vec3 LightColors[128];
uniform float LightIntensities[128];

uniform vec3 ObjColor;
uniform vec3 CamPos;
uniform int IsCelShading;
uniform float Scale;

uniform sampler2D TextureColor;
uniform sampler2D TextureSpecular;
uniform bool TextureSpecularIsSet;
uniform sampler2D TextureNormal;
uniform bool TextureNormalIsSet;

// Out variables
out vec4 out_Color;


vec3 perturbNormal(vec3 vertex_pos, vec3 surf_norm)
{
  // Calculate some derivatives
  vec3 q0 = dFdx(vertex_pos.xyz);
  vec3 q1 = dFdy(vertex_pos.xyz);
  vec2 st0 = dFdx(pass_TexCoord.st);
  vec2 st1 = dFdy(pass_TexCoord.st);

  // Do some extremely fabulous intricate calculations
  vec3 S = normalize(q0 * st1.t - q1 * st0.t);
  vec3 T = normalize(-q0 * st1.s + q1 * st0.s);
  vec3 N = normalize(surf_norm);

  // Convert normal texture from color space [0,1]*3 into a usable vector [-1,1]*3
  vec3 mapN = texture(TextureNormal, pass_TexCoord).xyz * 2.0 - 1.0;
  float normalScale = 1.0f;
  mapN.xy = normalScale * mapN.xy ;
  mat3 tsn = mat3(S, T, N);
  // Add normal from texture to current normal
  return normalize(tsn * mapN);
}


void main()
{
  vec3 normal = pass_Normal;
  // Apply normal texture if given
  if (TextureNormalIsSet)
  {
    normal = perturbNormal(pass_Pos, pass_Normal);
  }

  // ########### AMBIENT: ###########################################
  float ambient_factor = 0.05f;
  vec3 ambient = vec3(ambient_factor * ObjColor[0],
		      ambient_factor * ObjColor[1],
		      ambient_factor * ObjColor[2]);
  

  // Normalize normal vector
  normal = normalize(normal);

  // Cam direction points towards the camera
  vec3 cam_direction = normalize(CamPos - pass_Pos);
  float cam_distance = distance(CamPos, pass_Pos);
  
  // For each light
  vec3 diffuse = vec3(0.0f, 0.0f, 0.0f);
  vec3 specular = vec3(0.0f, 0.0f, 0.0f);
  for(int i = 0; i < LightCount; ++i)
  {
    // ########### DIFFUSE: #########################################
    // Light direction points towards the light (so that cos_theta can easily be
    // ...calculated with the dot product)
    vec3 light_direction = normalize(LightPositions[i] - pass_Pos);
    
    // Theta = angle between surface normal and light direction
    float cos_theta = max(dot(normal, light_direction), 0.0f);

    vec3 diffuse_part = vec3(cos_theta * ObjColor[0] * LightColors[i][0],
  			     cos_theta * ObjColor[1] * LightColors[i][1],
			     cos_theta * ObjColor[2] * LightColors[i][2]);

    diffuse += diffuse_part * LightIntensities[i];
   

    // ########### SPECULAR: ########################################
    if(TextureSpecularIsSet)
    {
      vec3 light_reflect_direction = reflect(-light_direction, normal);

      // Alpha = angle between light reflection direction and camera direction
      float cos_alpha = max(dot(cam_direction, light_reflect_direction), 0.0f);
    
    
      float specular_exponent = 10.0f;
      vec3 specular_part = vec3(0.5f, 0.5f, 0.5f) * pow(cos_alpha, specular_exponent);

      specular += specular_part * LightIntensities[i];
    }
  }
  
  // ########### TEXTURE: ###########################################
  diffuse *= texture(TextureColor, pass_TexCoord).xyz;
  ambient *= texture(TextureColor, pass_TexCoord).xyz;
  specular *= texture(TextureSpecular, pass_TexCoord).xyz;


  // ########### FINAL COLOR: #######################################
  out_Color = vec4(ambient + diffuse + specular, 1.0f);


  // ########### CEL SHADING: #######################################
  if(IsCelShading == 1)
  {
    // Add white outline around sphere
    // Outline threshold should be uniform for all spheres
    float cel_shading_outline_threshold = clamp(0.8f - (pow(Scale, 0.5f) * 0.25f), 0.2f, 0.8f);
    cel_shading_outline_threshold *= pow(cam_distance, 0.2f) * 0.5f;
    
    // Beta = angle between surface normal and camera direction
    float cos_beta = max(dot(normal, cam_direction), 0.0f);

    if(cos_beta < cel_shading_outline_threshold)
    {
      out_Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
      // Apply color discretization
      int cel_shade_count = 3;
      vec3 new_Color = vec3(0.0f, 0.0f, 0.0f);
      for(int i = 0; i < 3; ++i)
      {
        new_Color[i] = round(out_Color[i] * cel_shade_count) / cel_shade_count;
      }
      out_Color = vec4(new_Color, 1.0f);
    }
  }
}
