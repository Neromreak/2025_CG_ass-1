#version 150

// In variables
in vec3 pass_Normal;
in vec3 pass_Pos;

// Uniforms
// (vectors need a constant size, so I just set it to 128 even though probably
// ...less lights are needed)
uniform int LightCount;
uniform vec3 LightPositions[128];
uniform vec3 LightColors[128];
uniform float LightIntensities[128];
uniform vec3 ObjColor;
uniform vec3 CamPos;

// Out variables
out vec4 out_Color;


void main()
{
  // Temp DEBUG
  vec3 temp = (LightPositions[0] + LightColors[0] + ObjColor * LightIntensities[0]).xyz;
  

  // ########### AMBIENT: #############################################
  float ambient_factor = 0.05f;
  vec3 ambient = vec3(ambient_factor * ObjColor[0],
		      ambient_factor * ObjColor[1],
		      ambient_factor * ObjColor[2]);
  

  // ########### DIFFUSE: #############################################
  // Normalize normal vector
  vec3 normal = normalize(pass_Normal);
  
  // For each light
  vec3 diffuse = vec3(0.0f, 0.0f, 0.0f);
  vec3 specular = vec3(0.0f, 0.0f, 0.0f);
  for(int i = 0; i < LightCount; ++i)
  {    
    // Light direction points towards the light (so that cos_theta can easily be
    // ...calculated with the dot product)
    vec3 light_direction = normalize(LightPositions[i] - pass_Pos);
    
    // Theta = angle between surface normal and light direction
    float cos_theta = max(dot(normal, light_direction), 0.0f);

    vec3 diffuse_part = vec3(cos_theta * ObjColor[0] * LightColors[i][0],
  			     cos_theta * ObjColor[1] * LightColors[i][1],
			     cos_theta * ObjColor[2] * LightColors[i][2]);

    diffuse += diffuse_part * LightIntensities[i];
   

    // ########### SPECULAR: ##########################################
    // Cam direction points towards the camera
    vec3 cam_direction = normalize(CamPos - pass_Pos);

    vec3 light_reflect_direction = reflect(-light_direction, normal);

    // Alpha = angle between light reflection direction and camera direction
    float cos_alpha = max(dot(cam_direction, light_reflect_direction), 0.0f);
    
    
    float specular_exponent = 10.0f;
    vec3 specular_part = vec3(0.35f, 0.35f, 0.35f) * pow(cos_alpha, specular_exponent);

    specular += specular_part * LightIntensities[i];
  }
  

  // ########### FINAL COLOR: #########################################
  out_Color = vec4(ambient + diffuse + specular, 1.0f);

  // DEBUG
  out_Color += vec4(temp * 0.0001f, 1.0f);
  out_Color += vec4(CamPos * 0.0001f, 1.0f);
}
