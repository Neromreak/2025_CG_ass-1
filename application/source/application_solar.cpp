#include "application_solar.hpp"
#include "window_handler.hpp"

#include "utils.hpp"
#include "shader_loader.hpp"
#include "model_loader.hpp"
#include "texture_loader.hpp"
#include "scene_graph.hpp"
#include "geometry_node.hpp"
#include "camera_node.hpp"
#include "point_light_node.hpp"
#include "node.hpp"

#include <glbinding/gl/gl.h>
// Use gl definitions from glbinding 
using namespace gl;

// Dont load gl bindings from glfw
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <numbers>
#include <chrono>



ApplicationSolar::ApplicationSolar(std::string const& resource_path):
  Application{resource_path},
  planet_object{},
  stars_object{},
  circle_object{},
  m_view_transform{glm::translate(glm::fmat4{}, glm::fvec3{0.0f, 0.0f, 30.0f})},
  m_view_projection{utils::calculate_projection_matrix(initial_aspect_ratio)}
{
  // Modify far and near clipping planes of the projection matrix to extend render distance (https://www.terathon.com/gdc07_lengyel.pdf)
  m_view_projection[2][2] = -0.9999f;
  m_view_projection[3][2] = -0.1999f;

  // Initialization queue
  initializeTextures();
  initializeScene();
  initializeGeometry();
  initializeShaderPrograms();

  // Enable the option to adjust point sizes in the shaders
  glEnable(GL_PROGRAM_POINT_SIZE);
}

ApplicationSolar::~ApplicationSolar()
{
  // Clean up buffers and VAOs
  glDeleteBuffers(1, &planet_object.vertex_BO);
  glDeleteBuffers(1, &planet_object.element_BO);
  glDeleteVertexArrays(1, &planet_object.vertex_AO);

  glDeleteBuffers(1, &stars_object.vertex_BO);
  glDeleteVertexArrays(1, &stars_object.vertex_AO);
  
  glDeleteBuffers(1, &circle_object.vertex_BO);
  glDeleteVertexArrays(1, &circle_object.vertex_AO);
}



// ########### LIFE CYCLE FUNCTIONS #################################
void ApplicationSolar::physics()
{
  // Get delta time for frame rate independent physics
  static auto time_last = std::chrono::steady_clock::now();
  auto time_now = std::chrono::steady_clock::now();
  long long delta_time_ms = (std::chrono::duration_cast<std::chrono::milliseconds>(time_now - time_last)).count();
  delta_time_ms = std::min(delta_time_ms, (long long)50);
  time_last = time_now;


  glm::fmat4 view_t = m_view_transform;

  glm::fmat4 cam2origin;
  glm::fmat4 origin2cam;
  if (abs(rot_h) > 0.00001f || abs(rot_v) > 0.00001f)
  {
    glm::vec3 view_pos{view_t[3][0] / view_t[3][3], view_t[3][1] / view_t[3][3], view_t[3][2] / view_t[3][3] };
    cam2origin = glm::translate(glm::fmat4{}, -view_pos);
    origin2cam = glm::translate(glm::fmat4{}, view_pos);
  }


  // ROTATION | Apply previously captured rotation onto the camera:
  // Rotate around vertical axis
  if (abs(rot_h) > 0.00001f)
  {
     glm::fmat4 yaw = glm::rotate(glm::fmat4{}, rot_h * -rotation_speed, glm::fvec3{0.0f, 1.0f, 0.0f});
     m_view_transform = ( origin2cam * yaw * cam2origin ) * m_view_transform;
    // Rotation values have to be set back to zero (the mouse movement method is callback and can't reset them)
    rot_h = 0.0f;
  }

  // Extract horizontal view direction (movement should be influenced by horizontal rotation but not by vertical rotation
  //                                    ... and vertical rotation needs the view direction)
  glm::vec3 view_dir_h{ view_t[2][0] / view_t[3][3], 0, view_t[2][2] / view_t[3][3] };
  view_dir_h = glm::normalize(view_dir_h);
  glm::vec3 left_dir_h{ view_t[0][0] / view_t[3][3], view_t[0][1] / view_t[3][3], view_t[0][2] / view_t[3][3]};

  // Rotate around local left axis
  if (abs(rot_v) > 0.00001f)
  {
     glm::fmat4 pitch = glm::rotate(glm::fmat4{}, rot_v * -rotation_speed, left_dir_h);
     m_view_transform = ( origin2cam * pitch * cam2origin ) * m_view_transform;
    // Rotation values have to be set back to zero (the mouse movement method is callback and can't reset them)
    rot_v = 0.0f;
  }

  // MOVEMENT | Apply previously captured movement onto the camera:
  static glm::vec3 translation_lerp_aim{ m_view_transform[3][0] / m_view_transform[3][3],
                                         m_view_transform[3][1] / m_view_transform[3][3],
                                         m_view_transform[3][2] / m_view_transform[3][3] };

  // Calculate movement
  float speed = is_speed_slower ? movement_speed * 0.1f : movement_speed;
  if (move_x != 0)
  {
    translation_lerp_aim[0] += left_dir_h[0] * speed * move_x * delta_time_ms;
    translation_lerp_aim[2] += left_dir_h[2] * speed * move_x * delta_time_ms;
  }
  if (move_y != 0)
  {
    translation_lerp_aim[1] += speed * move_y * delta_time_ms;
  }
  if (move_z != 0)
  {
    translation_lerp_aim[0] += view_dir_h[0] * speed * move_z * delta_time_ms;
    translation_lerp_aim[2] += view_dir_h[2] * speed * move_z * delta_time_ms;
  }

  // Apply transformation in world space (no local oriented movement)
  // Lerp movement for a more smooth feeling
  m_view_transform[3][0] = lerp(m_view_transform[3][0], translation_lerp_aim[0], 0.015f * delta_time_ms);
  m_view_transform[3][1] = lerp(m_view_transform[3][1], translation_lerp_aim[1], 0.015f * delta_time_ms);
  m_view_transform[3][2] = lerp(m_view_transform[3][2], translation_lerp_aim[2], 0.015f * delta_time_ms);

  // Update the shaders
  uploadView();
}


bool is_first_frame = true;
void ApplicationSolar::render() const
{
  if (is_first_frame)
  {
    is_first_frame = false;
    render_first_frame();
  }

  // Render skybox (Ass4):
  // ...(is done before the scene but without depth info); (Tutorial I used as assistance: https://learnopengl.com/Advanced-OpenGL/Cubemaps)
  glDepthMask(GL_FALSE);
  // Bind cube shader
  glUseProgram(m_shaders.at("skybox").handle);

  // Texture 'color'
  // Select texture unit
  glActiveTexture(GL_TEXTURE0);
  // Bind texture object
  glBindTexture(skybox_texture.target, skybox_texture.handle);
  glUniform1i(m_shaders.at("skybox").u_locs.at("TextureColor"), 0);

  // Bind the VAO to draw
  glBindVertexArray(cube_object.vertex_AO);
  // Draw bound vertex array using bound shader
  glDrawElements(cube_object.draw_mode, cube_object.num_elements, model::INDEX.type, NULL);

  // Unbind VA
  glBindVertexArray(0);
  // Reactivate depth mas so that everything else is drawn on top of the skybox
  glDepthMask(GL_TRUE);

  
  // Traverse the scene graph tree
  scene->get_root()->render(&m_shaders, &m_view_transform, scene->get_root()->get_world_transform());


  // Render Stars (Ass2):
  // Bind shader
  glUseProgram(m_shaders.at("vao").handle);
  
  // Upload Identity matrix as ModelMatrix for stars (no transformation as all stars are one object)
  glUniformMatrix4fv(m_shaders.at("vao").u_locs.at("ModelMatrix"),
                      1, GL_FALSE, glm::value_ptr(glm::fmat4{}));

  // Bind the VAO to draw
  glBindVertexArray(stars_object.vertex_AO);

  // Draw bound vertex array using bound shader
  glDrawArrays(stars_object.draw_mode, 0, stars_object.num_elements);
}


void ApplicationSolar::render_first_frame() const
{
  // Method only called in the first frame. Is needed because some shaders need uniforms
  // ...that don't change throughout the program but can't be set in the initialization
  // ...because some other initialization is done after. (???)

  // Extract parameters of all lights in the scene to give to the shaders as uniforms:
  std::vector<glm::vec3> light_positions{};
  std::vector<glm::vec3> light_colors{};
  std::vector<float> light_intensities{};

  // Iterate through scene to find all lights
  std::list<Node*> remaining_nodes{ scene->get_root() };
  while (remaining_nodes.size() > 0)
  {
    Node* front_node = remaining_nodes.front();
    for (Node* new_node : front_node->get_children())
    {
      remaining_nodes.push_back(new_node);
    }
    if (typeid(*front_node) == typeid(PointLightNode))
    {
      PointLightNode* light_node = static_cast<PointLightNode*>(front_node);
      // For each found light add position, color and intensity to vectors
      glm::fmat4 pos_mat4 = light_node->get_local_transform();
      light_positions.push_back({ pos_mat4[3][0] / pos_mat4[3][3], pos_mat4[3][1] / pos_mat4[3][3], pos_mat4[3][2] / pos_mat4[3][3] });
      light_colors.push_back(light_node->get_color());
      light_intensities.push_back(light_node->get_intensity());
    }
    remaining_nodes.pop_front();
  }

  if (light_positions.size() > 128)
  {
    throw "Too many lights, the frag shader limits the light amount (can be adjusted in the frag shader)";
  }

  // Give light parameters (They won't change throughout the life cycle)
  glUseProgram(m_shaders.at("planet").handle);

  glUniform1i(m_shaders.at("planet").u_locs.at("LightCount"),
    light_positions.size());
  glUniform3fv(m_shaders.at("planet").u_locs.at("LightPositions"),
    light_positions.size(), glm::value_ptr(light_positions[0]));
  glUniform3fv(m_shaders.at("planet").u_locs.at("LightColors"),
    light_colors.size(), glm::value_ptr(light_colors[0]));
  glUniform1fv(m_shaders.at("planet").u_locs.at("LightIntensities"),
    light_intensities.size(), light_intensities.data());
}


void ApplicationSolar::uploadView() {
  // Vertices are transformed in camera space, so camera transform must be inverted
  glm::fmat4 view_matrix = glm::inverse(m_view_transform);
  // Upload matrix to gpu
  // Bind and upload to planet shader
  glUseProgram(m_shaders.at("planet").handle);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));

  // Bind and upload to sun shader
  glUseProgram(m_shaders.at("sun").handle);
  glUniformMatrix4fv(m_shaders.at("sun").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));

  // Bind and upload to vao shader
  glUseProgram(m_shaders.at("vao").handle);
  glUniformMatrix4fv(m_shaders.at("vao").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));

  // Bind and upload to skybox shader
  glUseProgram(m_shaders.at("skybox").handle);
  glUniformMatrix4fv(m_shaders.at("skybox").u_locs.at("ViewMatrix"),
                     1, GL_FALSE, glm::value_ptr(view_matrix));
}


void ApplicationSolar::uploadProjection() {
  // Upload matrix to gpu
  // Bind and upload to planet shader
  glUseProgram(m_shaders.at("planet").handle);
  glUniformMatrix4fv(m_shaders.at("planet").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));

  // Bind and upload to sun shader
  glUseProgram(m_shaders.at("sun").handle);
  glUniformMatrix4fv(m_shaders.at("sun").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));

  // Bind and upload to vao shader
  glUseProgram(m_shaders.at("vao").handle);
  glUniformMatrix4fv(m_shaders.at("vao").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));

  // Bind and upload to skybox shader
  glUseProgram(m_shaders.at("skybox").handle);
  glUniformMatrix4fv(m_shaders.at("skybox").u_locs.at("ProjectionMatrix"),
                     1, GL_FALSE, glm::value_ptr(m_view_projection));
}


// Update uniform locations
void ApplicationSolar::uploadUniforms() { 
  // upload uniform values to new locations
  uploadView();
  uploadProjection();
}



// ########### INITIALIZATION FUNCTIONS #############################
// Load shader sources
void ApplicationSolar::initializeShaderPrograms()
{
  // Planet shader:
  // Store shader program objects in container
  m_shaders.emplace("planet", shader_program{{{GL_VERTEX_SHADER,m_resource_path + "shaders/simple.vert"},
                                           {GL_FRAGMENT_SHADER, m_resource_path + "shaders/simple.frag"}}});
  // Request uniform locations for shader program
  m_shaders.at("planet").u_locs["NormalMatrix"] = -1;
  m_shaders.at("planet").u_locs["ModelMatrix"] = -1;
  m_shaders.at("planet").u_locs["ViewMatrix"] = -1;
  m_shaders.at("planet").u_locs["ProjectionMatrix"] = -1;

  m_shaders.at("planet").u_locs["LightCount"] = -1;
  m_shaders.at("planet").u_locs["LightPositions"] = -1;
  m_shaders.at("planet").u_locs["LightColors"] = -1;
  m_shaders.at("planet").u_locs["LightIntensities"] = -1;

  m_shaders.at("planet").u_locs["ObjColor"] = -1;
  m_shaders.at("planet").u_locs["CamPos"] = -1;
  m_shaders.at("planet").u_locs["IsCelShading"] = -1;
  m_shaders.at("planet").u_locs["Scale"] = -1;
  
  m_shaders.at("planet").u_locs["TextureColor"] = -1;
  m_shaders.at("planet").u_locs["TextureSpecular"] = -1;
  m_shaders.at("planet").u_locs["TextureSpecularIsSet"] = -1;
  m_shaders.at("planet").u_locs["TextureNormal"] = -1;
  m_shaders.at("planet").u_locs["TextureNormalIsSet"] = -1;


  // Sun shader:
  // Store shader program objects in container
  m_shaders.emplace("sun", shader_program{ {{GL_VERTEX_SHADER,m_resource_path + "shaders/sun.vert"},
                                           {GL_FRAGMENT_SHADER, m_resource_path + "shaders/sun.frag"}} });
  // Request uniform locations for shader program
  m_shaders.at("sun").u_locs["NormalMatrix"] = -1;
  m_shaders.at("sun").u_locs["ModelMatrix"] = -1;
  m_shaders.at("sun").u_locs["ViewMatrix"] = -1;
  m_shaders.at("sun").u_locs["ProjectionMatrix"] = -1;

  m_shaders.at("sun").u_locs["CamPos"] = -1;
  m_shaders.at("sun").u_locs["IsCelShading"] = -1;
  m_shaders.at("sun").u_locs["Scale"] = -1;

  m_shaders.at("sun").u_locs["TextureColor"] = -1;


  // VAO shader:
  // Store shader program objects in container
  m_shaders.emplace("vao", shader_program{ {{GL_VERTEX_SHADER, m_resource_path + "shaders/vao.vert"},
                                          {GL_FRAGMENT_SHADER, m_resource_path + "shaders/vao.frag"}} });
  // Request uniform locations for shader program
  m_shaders.at("vao").u_locs["ModelMatrix"] = -1;
  m_shaders.at("vao").u_locs["ViewMatrix"] = -1;
  m_shaders.at("vao").u_locs["ProjectionMatrix"] = -1;


  // Skybox shader:
  // Store shader program objects in container
  m_shaders.emplace("skybox", shader_program{ {{GL_VERTEX_SHADER,m_resource_path + "shaders/skybox.vert"},
                                           {GL_FRAGMENT_SHADER, m_resource_path + "shaders/skybox.frag"}} });
  // Request uniform locations for shader program
  m_shaders.at("skybox").u_locs["ViewMatrix"] = -1;
  m_shaders.at("skybox").u_locs["ProjectionMatrix"] = -1;

  m_shaders.at("skybox").u_locs["TextureColor"] = -1;
}


// Load textures
void ApplicationSolar::initializeTextures()
{
  // Load mercury texture:
  m_textures.emplace("mercury", texture_object{});
  texture_object* texture = &m_textures.at("mercury");
  texture->target = GL_TEXTURE_2D;
  // 24bit PNG worked. JPG has some issues where the buffer size doesn't account for the alpha values but still puts alpha in the buffer
  pixel_data texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/mercurymap1k.png"));

  // DEBUG print pixel buffer contents to file
  /*
  std::ofstream debug_file(m_resource_path + "../DEBUG.txt");
  debug_file << "DEBUG:\n";
  int len = texture_data->pixels.size();
  debug_file << "Lenght: " << len << "\n";
  for (int i = 0; i < len / 4; ++i)
  {
    std::string temp{ "Pixel " + std::to_string(i) + " (" + std::to_string(i % 1000) + "x" + std::to_string(i / 1000) + "): " };
    debug_file << temp;
    for (int j = temp.size(); j < 22; ++j) { debug_file << " "; }
    debug_file
      << (int)texture_data->pixels[i * 4 + 0] << " "
      << (int)texture_data->pixels[i * 4 + 1] << " "
      << (int)texture_data->pixels[i * 4 + 2] << "\n";
  }
  debug_file << "Lenght: " << len << "\n";
  */

  // Initialize texture
  glActiveTexture(GL_TEXTURE0);                     // Activate and select 0 as the Texture Unit which subsequent texture state calls will affect
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)
    
  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
                                                                      // -> minification filter will perform a linear blend between samples to get the end color
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR
                                                                      // -> magnification filter will perform a linear blend between samples to get the end color

  // Define texture data and format
  glTexImage2D(texture->target,       // Texturing target (binding point of the texture)
               0,                     // Level of detail number (0 for only base image)
               GL_RGBA8,              // Color format for the texture (number of color components)
               1000,                  // Width
               500,                   // Height
               0,                     // Border thickness (must be 0 (cool parameter bro))
               GL_RGBA,               // Format of source file data
               GL_UNSIGNED_BYTE,      // Data type of source file (mostly precision)
               texture_data.ptr());  // Pointer to texture data in memory

  // Load mercury texture normal:
  m_textures.emplace("mercury_normal", texture_object{});
  texture = &m_textures.at("mercury_normal");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/mercurynormal1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load venus texture:
  m_textures.emplace("venus", texture_object{});
  texture = &m_textures.at("venus");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/venusmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());

  // Load venus texture normal:
  m_textures.emplace("venus_normal", texture_object{});
  texture = &m_textures.at("venus_normal");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/venusnormal1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load earth texture color:
  m_textures.emplace("earth", texture_object{});
  texture = &m_textures.at("earth");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/earthmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());

  // Load earth texture specular:
  m_textures.emplace("earth_spec", texture_object{});
  texture = &m_textures.at("earth_spec");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/earthspec1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());
  
  // Load earth texture normal:
  m_textures.emplace("earth_normal", texture_object{});
  texture = &m_textures.at("earth_normal");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/earthnormal1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());

  
  // Load moon texture:
  m_textures.emplace("moon", texture_object{});
  texture = &m_textures.at("moon");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/moonmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());

  // Load moon texture normal:
  m_textures.emplace("moon_normal", texture_object{});
  texture = &m_textures.at("moon_normal");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/moonnormal1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load mars texture:
  m_textures.emplace("mars", texture_object{});
  texture = &m_textures.at("mars");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/marsmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());

  // Load mars normal specular:
  m_textures.emplace("mars_normal", texture_object{});
  texture = &m_textures.at("mars_normal");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/marsnormal1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load jupiter texture:
  m_textures.emplace("jupiter", texture_object{});
  texture = &m_textures.at("jupiter");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/jupitermap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load saturn texture:
  m_textures.emplace("saturn", texture_object{});
  texture = &m_textures.at("saturn");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/saturnmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load uranus texture:
  m_textures.emplace("uranus", texture_object{});
  texture = &m_textures.at("uranus");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/uranusmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load neptune texture:
  m_textures.emplace("neptune", texture_object{});
  texture = &m_textures.at("neptune");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/neptunemap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load pluto texture:
  m_textures.emplace("pluto", texture_object{});
  texture = &m_textures.at("pluto");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/plutomap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load sun texture:
  m_textures.emplace("sun", texture_object{});
  texture = &m_textures.at("sun");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/sunmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load geralt texture:
  m_textures.emplace("geralt", texture_object{});
  texture = &m_textures.at("geralt");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/geraltmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load deathstar texture:
  m_textures.emplace("deathstar", texture_object{});
  texture = &m_textures.at("deathstar");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/deathstarmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());

  // Load deathstar texture normal:
  m_textures.emplace("deathstar_normal", texture_object{});
  texture = &m_textures.at("deathstar_normal");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/deathstarnormal1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load spacestation texture:
  m_textures.emplace("spacestation", texture_object{});
  texture = &m_textures.at("spacestation");
  texture->target = GL_TEXTURE_2D;
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/spacestationmap1k.png"));

  // Initialize texture
  glGenTextures(1, &texture->handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(texture->target, texture->handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  glTexImage2D(texture->target, 0, GL_RGBA8, 1000, 500, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());


  // Load skybox texture:
  skybox_texture = texture_object{};
  skybox_texture.target = GL_TEXTURE_CUBE_MAP;

  // Initialize texture
  glGenTextures(1, &skybox_texture.handle);               // Generate the texture object as GLuint (acts as a pointer / referenceID)
  glBindTexture(skybox_texture.target, skybox_texture.handle);  // Bind texture to texturing target (basically determines texture dimension)

  // Define texture sampling parameters
  glTexParameteri(skybox_texture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MIN_FILTER to GL_LINEAR
  glTexParameteri(skybox_texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Set the parameter TEXTURE_MAG_FILTER to GL_LINEAR

  // Define texture data and format
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/skybox1map2k.png"));
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGBA8, 2000, 2000, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGBA8, 2000, 2000, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGBA8, 2000, 2000, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());
  
  texture_data = pixel_data(texture_loader::file(m_resource_path + "textures/skybox2map2k.png"));
  glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGBA8, 2000, 2000, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGBA8, 2000, 2000, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());
  glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGBA8, 2000, 2000, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture_data.ptr());
}


// Load models (the raw model files containing the vertices information)
void ApplicationSolar::initializeGeometry()
{
  // Sphere:
  model planet_model = model_loader::obj(m_resource_path + "models/sphere.obj", model::NORMAL | model::TEXCOORD);

  // Generate vertex array object
  glGenVertexArrays(1, &planet_object.vertex_AO);
  // Bind the array for attaching buffers
  glBindVertexArray(planet_object.vertex_AO);

  // Generate generic buffer
  glGenBuffers(1, &planet_object.vertex_BO);
  // Bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, planet_object.vertex_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * planet_model.data.size(), planet_model.data.data(), GL_STATIC_DRAW);

  // Activate first attribute on GPU
  glEnableVertexAttribArray(0);
  // First attribute (in_Position) is 3 floats
  glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::POSITION]);
  // Activate second attribute on GPU
  glEnableVertexAttribArray(1);
  // Second attribute (in_Normal) is 3 floats
  glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::NORMAL]);
  // Activate third attribute on GPU
  glEnableVertexAttribArray(2);
  // Third attribute (in_TexCoord) is 2 floats
  glVertexAttribPointer(2, model::TEXCOORD.components, model::TEXCOORD.type, GL_FALSE, planet_model.vertex_bytes, planet_model.offsets[model::TEXCOORD]);

  // Generate generic buffer
  glGenBuffers(1, &planet_object.element_BO);
  // Bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planet_object.element_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, model::INDEX.size * planet_model.indices.size(), planet_model.indices.data(), GL_STATIC_DRAW);

  // Store type of primitive to draw
  planet_object.draw_mode = GL_TRIANGLES;
  // Transfer number of indices to model object 
  planet_object.num_elements = GLsizei(planet_model.indices.size());


  // Points:
  std::vector<float> stars_model = generateGeometryStars();

  // Generate vertex array object
  glGenVertexArrays(1, &stars_object.vertex_AO);
  // Bind the array for attaching buffers
  glBindVertexArray(stars_object.vertex_AO);

  // Generate generic buffer
  glGenBuffers(1, &stars_object.vertex_BO);
  // Bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, stars_object.vertex_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, stars_model.size() * sizeof(float), stars_model.data(), GL_STATIC_DRAW);

  // Activate first attribute on GPU (in_Position)
  glEnableVertexAttribArray(0);
  // First attribute are the positions (3 floats for each vertex attribute with 6 floats offset (stride) between the vertex attributes and no initial offset)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0));
  // Activate second attribute on GPU (in_Color)
  glEnableVertexAttribArray(1);
  // Second attribute are the colors (3 floats for each vertex attribute with 6 floats offset (stride) between the vertex attributes and 3 floats initial offset)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));  

  // Element Array Buffer (for draw order of vertecies) not needed as the stars_model are drawn as single points (point cloud)

  // Store type of primitive to draw (single points without edges or faces inbetween)
  stars_object.draw_mode = GL_POINTS;
  // Transfer number of indices to model object 
  stars_object.num_elements = GLsizei(stars_model.size() / 6);


  // Circle:
  std::vector<float> circle_model = generateGeometryCircle();

  // Generate vertex array object
  glGenVertexArrays(1, &circle_object.vertex_AO);
  // Bind the array for attaching buffers
  glBindVertexArray(circle_object.vertex_AO);

  // Generate generic buffer
  glGenBuffers(1, &circle_object.vertex_BO);
  // Bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, circle_object.vertex_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, circle_model.size() * sizeof(float), circle_model.data(), GL_STATIC_DRAW);

  // Activate first attribute on GPU (in_Position)
  glEnableVertexAttribArray(0);
  // First attribute are the positions (3 floats for each vertex attribute with 6 floats offset (stride) between the vertex attributes and no initial offset)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0));
  // Activate second attribute on GPU (in_Color)
  glEnableVertexAttribArray(1);
  // Second attribute are the colors (3 floats for each vertex attribute with 6 floats offset (stride) between the vertex attributes and 3 floats initial offset)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

  // Element Array Buffer (for draw order of vertecies) not needed as the a LINE_LOOP just connects the vertecies in order (and last with first)
  
  // Store type of primitive to draw
  circle_object.draw_mode = GL_LINE_LOOP;
  // Transfer number of indices to model object 
  circle_object.num_elements = GLsizei(circle_model.size() / 6);


  // Cube:
  model cube_model = model_loader::obj(m_resource_path + "models/cube.obj");

  // Generate vertex array object
  glGenVertexArrays(1, &cube_object.vertex_AO);
  // Bind the array for attaching buffers
  glBindVertexArray(cube_object.vertex_AO);

  // Generate generic buffer
  glGenBuffers(1, &cube_object.vertex_BO);
  // Bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, cube_object.vertex_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)* cube_model.data.size(), cube_model.data.data(), GL_STATIC_DRAW);

  // Activate first attribute on GPU
  glEnableVertexAttribArray(0);
  // First attribute (in_Position) is 3 floats
  glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, cube_model.vertex_bytes, cube_model.offsets[model::POSITION]);

  // Generate generic buffer
  glGenBuffers(1, &cube_object.element_BO);
  // Bind this as a vertex array buffer containing all attributes
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_object.element_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, model::INDEX.size* cube_model.indices.size(), cube_model.indices.data(), GL_STATIC_DRAW);

  // Store type of primitive to draw
  cube_object.draw_mode = GL_TRIANGLES;
  // Transfer number of indices to model object 
  cube_object.num_elements = GLsizei(cube_model.indices.size());


  // Space station:
  model spacestation_model = model_loader::obj(m_resource_path + "models/spacestation.obj");

  // Generate vertex array object
  glGenVertexArrays(1, &spacestation_object.vertex_AO);
  // Bind the array for attaching buffers
  glBindVertexArray(spacestation_object.vertex_AO);

  // Generate generic buffer
  glGenBuffers(1, &spacestation_object.vertex_BO);
  // Bind this as an vertex array buffer containing all attributes
  glBindBuffer(GL_ARRAY_BUFFER, spacestation_object.vertex_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)* spacestation_model.data.size(), spacestation_model.data.data(), GL_STATIC_DRAW);

  // Activate first attribute on GPU
  glEnableVertexAttribArray(0);
  // First attribute (in_Position) is 3 floats
  glVertexAttribPointer(0, model::POSITION.components, model::POSITION.type, GL_FALSE, spacestation_model.vertex_bytes, spacestation_model.offsets[model::POSITION]);
  // Activate second attribute on GPU
  glEnableVertexAttribArray(1);
  // Second attribute (in_Normal) is 3 floats
  glVertexAttribPointer(1, model::NORMAL.components, model::NORMAL.type, GL_FALSE, spacestation_model.vertex_bytes, spacestation_model.offsets[model::NORMAL]);
  // Activate third attribute on GPU
  glEnableVertexAttribArray(2);
  // Third attribute (in_TexCoord) is 2 floats
  glVertexAttribPointer(2, model::TEXCOORD.components, model::TEXCOORD.type, GL_FALSE, spacestation_model.vertex_bytes, spacestation_model.offsets[model::TEXCOORD]);


  // Generate generic buffer
  glGenBuffers(1, &spacestation_object.element_BO);
  // Bind this as a vertex array buffer containing all attributes
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spacestation_object.element_BO);
  // Configure currently bound array buffer
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, model::INDEX.size* spacestation_model.indices.size(), spacestation_model.indices.data(), GL_STATIC_DRAW);

  // Store type of primitive to draw
  spacestation_object.draw_mode = GL_TRIANGLES;
  // Transfer number of indices to model object 
  spacestation_object.num_elements = GLsizei(spacestation_model.indices.size());


  // Unbind VA
  glBindVertexArray(0);
}


// (Randomly) create geometry for the stars (Ass2)
std::vector<float> ApplicationSolar::generateGeometryStars()
{
  // Add stars (Ass2)
  std::srand(std::time(nullptr));

  std::vector<float> star_data{};

  // Create data for the stars
  int star_count = 50'000;
  float distance = 1500.0f;
  float pi = acos(0.0f) * 2.0f;
  for (int i = 0; i < star_count; ++i)
  {
    // Position:
    // Create random 3D vector on a sphere (evenly distributed)
    float alpha = ((float)std::rand() / RAND_MAX) * 2.0f * pi;
    float beta = acos((((float)std::rand() / RAND_MAX) * 2.0f ) - 1.0f);

    float x = sin(beta) * cos(alpha);
    float y = cos(beta);
    float z = sin(beta) * sin(alpha);

    // Extend vectors
    float deviation = 1 + (((float)std::rand() / RAND_MAX) * 0.3);
    x *= distance * deviation;
    y *= distance * deviation;
    z *= distance * deviation;

    // Color:
    float f1 = (float)std::rand() / RAND_MAX * 0.7f;
    float f2 = (float)std::rand() / RAND_MAX * 0.8f;

    float r = 1.0f;
    float g = 1.0f - f1;
    float b = std::fmax(1.0f - f1 - f2, 0.0f);

    // Add Pos and Color to container
    star_data.push_back(x);
    star_data.push_back(y);
    star_data.push_back(z);
    star_data.push_back(r);
    star_data.push_back(g);
    star_data.push_back(b);
  }

  return star_data;
}


// Generate a circle that can be used as an orbit (Ass2)
std::vector<float> ApplicationSolar::generateGeometryCircle()
{
  std::vector<float> vertex_container{};

  // Increment and rotate vertecies to form a circle
  int vertex_count = 360;
  float pi = acos(0.0f) * 2.0f;
  for (int i = 0; i < vertex_count; ++i)
  {
    // Rotation
    glm::fmat4 rotation_matrix = glm::rotate(glm::fmat4{}, ((2.0f * pi) / vertex_count) * i, glm::fvec3{ 0.0f, 1.0f, 0.0f });
    glm::vec4 vertex = rotation_matrix * glm::vec4{ 1.0, 0.0, 0.0, 1.0 };

    // Add geometry
    vertex_container.push_back(vertex[0] / vertex[3]);
    vertex_container.push_back(vertex[1] / vertex[3]);
    vertex_container.push_back(vertex[2] / vertex[3]);

    // Color
    float r = std::min((cos(((2.0f * pi) / vertex_count) * i) * 0.5f ) + 0.8f, 1.0f);
    float g = 0.5f;
    float b = std::min(1.2f - ( cos(((2.0f * pi) / vertex_count) * i) * 0.5f), 1.0f);
    vertex_container.push_back(r);
    vertex_container.push_back(g);
    vertex_container.push_back(b);
  }

  return vertex_container;
}


// Create and fill scene with nodes (camera, objects, lights)
void ApplicationSolar::initializeScene()
{
  // Create scene graph and root
  scene = SceneGraph::get_instance();
  Node* root = new Node{ "root", nullptr, glm::fmat4{}, glm::fmat4{}, 0.0f, nullptr };
  scene->set_root(root);
  
  // Add planet holders to scene root
  // Mercury holder
  glm::fmat4 local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 6.0f });
  Node* holder_mer = new Node{ "Mercury Holder", root, local_transform, glm::fmat4{}, 1.0f * SIMULATION_SPEED, &circle_object };
  
  // Venusholder
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 10.0f });
  Node* holder_ven = new Node{ "Venus Holder", root, local_transform, glm::fmat4{}, 0.8f * SIMULATION_SPEED, &circle_object };

  // Earth and moon holders
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 14.0f });
  Node* holder_ear = new Node{ "Earth Holder", root, local_transform, glm::fmat4{}, 0.6f * SIMULATION_SPEED, &circle_object };
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{0.0f, 0.0f, 1.5f});
  Node* holder_moo = new Node{ "Moon Holder", holder_ear, local_transform, glm::fmat4{}, 1.2f * SIMULATION_SPEED, &circle_object };

  // Mars holder
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 18.0f });
  Node* holder_mar = new Node{ "Mars Holder", root, local_transform, glm::fmat4{}, 0.4f * SIMULATION_SPEED, &circle_object };

  // Jupiter and moons holders
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 26.0f });
  Node* holder_jup = new Node{ "Jupiter Holder", root, local_transform, glm::fmat4{}, 0.2f * SIMULATION_SPEED, &circle_object };
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 2.1f });
  Node* holder_jup1 = new Node{ "Jupiter moon 1 Holder", holder_jup, local_transform, glm::fmat4{}, 0.7f * SIMULATION_SPEED, &circle_object };
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 3.2f });
  Node* holder_jup2 = new Node{ "Jupiter moon 2 Holder", holder_jup, local_transform, glm::fmat4{}, 1.2f * SIMULATION_SPEED, &circle_object };
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 4.3f });
  Node* holder_jup3 = new Node{ "Jupiter moon 3 Holder", holder_jup, local_transform, glm::fmat4{}, 1.3f * SIMULATION_SPEED, &circle_object };
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 5.6f });
  Node* holder_jup4 = new Node{ "Jupiter moon 4 Holder", holder_jup, local_transform, glm::fmat4{}, 0.9f * SIMULATION_SPEED, &circle_object };

  // Saturn and moons holder
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 32.0f });
  Node* holder_sat = new Node{ "Saturn Holder", root, local_transform, glm::fmat4{}, 0.1f * SIMULATION_SPEED, &circle_object };
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 1.8f });
  Node* holder_sat1 = new Node{ "Saturn moon 1 Holder", holder_sat, local_transform, glm::fmat4{}, 2.0f * SIMULATION_SPEED, &circle_object };
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 3.8f });
  Node* holder_sat2 = new Node{ "Saturn moon 2 Holder", holder_sat, local_transform, glm::fmat4{}, 1.0f * SIMULATION_SPEED, &circle_object };
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 1.0f });
  Node* holder_sat21 = new Node{ "Saturn moon 2 moon 1 Holder", holder_sat2, local_transform, glm::fmat4{}, 0.9f * SIMULATION_SPEED, &circle_object };
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 5.2f });
  Node* holder_sat3 = new Node{ "Saturn moon 3 Holder", holder_sat, local_transform, glm::fmat4{}, 0.5f * SIMULATION_SPEED, &circle_object };

  // Uranus holder
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 38.0f });
  Node* holder_ura = new Node{ "Uranus Holder", root, local_transform, glm::fmat4{}, 0.05f * SIMULATION_SPEED, &circle_object };

  // Neptune holder
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 44.0f });
  Node* holder_nep = new Node{ "Neptune Holder", root, local_transform, glm::fmat4{}, 0.03f * SIMULATION_SPEED, &circle_object };
  
  // Add planets (with texture) to planet holders and apply scale
  // Mercury
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.35f });
  GeometryNode* mer = new GeometryNode{ "Mercury", holder_mer, {}, local_transform, glm::fmat4{}, 2.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("mercury"), nullptr, &m_textures.at("mercury_normal")};

  // Venus
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.8f });
  GeometryNode* ven = new GeometryNode{ "Venus", holder_ven, {}, local_transform, glm::fmat4{}, -3.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("venus"), nullptr, &m_textures.at("venus_normal") };

  // Earth and moon
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.8f });
  GeometryNode* ear = new GeometryNode{ "Earth", holder_ear, {}, local_transform, glm::fmat4{}, 4.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("earth"), &m_textures.at("earth_spec"), &m_textures.at("earth_normal")};
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.25f });
  GeometryNode* moo = new GeometryNode{ "Moon", holder_moo, {}, local_transform, glm::fmat4{}, 0.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("moon"), nullptr, &m_textures.at("moon_normal") };

  // Mars
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.45f });
  GeometryNode* mar = new GeometryNode{ "Mars", holder_mar, {}, local_transform, glm::fmat4{}, 3.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("mars"), nullptr, &m_textures.at("mars_normal")};

  // Jupiter and moons
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 1.6f });
  GeometryNode* jup = new GeometryNode{ "Jupiter", holder_jup, {}, local_transform, glm::fmat4{}, 1.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("jupiter") };
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.25f });
  GeometryNode* jup1 = new GeometryNode{ "Jupiter moon 1", holder_jup1, {}, local_transform, glm::fmat4{}, 6.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("geralt") };
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.19f });
  GeometryNode* jup2 = new GeometryNode{ "Jupiter moon 2", holder_jup2, {}, local_transform, glm::fmat4{}, 1.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("deathstar"), nullptr, &m_textures.at("deathstar_normal")};
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.60f });
  GeometryNode* jup3 = new GeometryNode{ "Jupiter moon 3", holder_jup3, {}, local_transform, glm::fmat4{}, 0.0f * SIMULATION_SPEED,
                                        &spacestation_object, glm::vec3{ 1.0f }, &m_textures.at("spacestation") };
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.09f });
  GeometryNode* jup4 = new GeometryNode{ "Jupiter moon 4", holder_jup4, {}, local_transform, glm::fmat4{}, 2.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("pluto") };

  // Saturn and moons
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 1.4f });
  GeometryNode* sat = new GeometryNode{ "Saturn", holder_sat, {}, local_transform, glm::fmat4{}, 0.4f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("saturn") };
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.05f });
  GeometryNode* sat1 = new GeometryNode{ "Saturn moon 1", holder_sat1, {}, local_transform, glm::fmat4{}, 2.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("pluto") };
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.2f });
  GeometryNode* sat2 = new GeometryNode{ "Saturn moon 2", holder_sat2, {}, local_transform, glm::fmat4{}, 20.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("pluto") };
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.1f });
  GeometryNode* sat21 = new GeometryNode{ "Saturn moon 2 moon 1", holder_sat21, {}, local_transform, glm::fmat4{}, 1.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("pluto") };
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.09f });
  GeometryNode* sat3 = new GeometryNode{ "Saturn moon 3", holder_sat3, {}, local_transform, glm::fmat4{}, 3.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("pluto") };

  // Uranus
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 1.2f });
  GeometryNode* ura = new GeometryNode{ "Uranus", holder_ura, {}, local_transform, glm::fmat4{}, -2.0f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("uranus") };

  // Neptune
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 1.1f });
  GeometryNode* nep = new GeometryNode{ "Neptune", holder_nep, {}, local_transform, glm::fmat4{}, 2.5f * SIMULATION_SPEED,
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("neptune") };

  // Add lighting and sun
  local_transform = glm::translate(glm::fmat4{}, glm::vec3{ 0.0f, 0.0f, 0.0f });
  PointLightNode* light_sun = new PointLightNode{ "Sun light", root, local_transform, glm::fmat4{}, 0.0f,
                                                  glm::vec3{ 1.0f, 1.0f, 1.0f }, 1.0f };
  local_transform = glm::scale(glm::fmat4{}, glm::vec3{3.0f});
  GeometryNode* sun = new GeometryNode{ "Sun", light_sun, local_transform, glm::fmat4{},
                                        &planet_object, glm::vec3{ 1.0f }, &m_textures.at("sun") };

  // Add camera
  CameraNode* cam_main = new CameraNode{ "Main Camera", root };
}



// ########### INPUT HANDLING #######################################
// Handle key Input
void ApplicationSolar::keyCallback(GLFWwindow* window, int key, int action, int mods)
{
  // Read keys and store user controls
  move_x = 0;
  move_y = 0;
  move_z = 0;

  
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_REPEAT)
  {
    move_z = -1;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_REPEAT)
  {
    move_x = -1;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_REPEAT)
  {
    move_z = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_REPEAT)
  {
    move_x = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_REPEAT ||
      glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
  {
    move_y = -1;
  }
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_REPEAT ||
      glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
  {
    move_y = 1;
  }
  if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_REPEAT)
  {
    is_speed_slower = true;
  }
  else
  {
    is_speed_slower = false;
  }
  if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
  {
    // Switch cel shading off
    if (cel_shading)
    {
      cel_shading = false;

      glUseProgram(m_shaders.at("planet").handle);
      glUniform1i(m_shaders.at("planet").u_locs.at("IsCelShading"), 0);
      
      glUseProgram(m_shaders.at("sun").handle);
      glUniform1i(m_shaders.at("sun").u_locs.at("IsCelShading"), 0);
    }
  }
  if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
  {
    // Switch cell shading on
    if (!cel_shading)
    {
      cel_shading = true;

      glUseProgram(m_shaders.at("planet").handle);
      glUniform1i(m_shaders.at("planet").u_locs.at("IsCelShading"), 1);

      glUseProgram(m_shaders.at("sun").handle);
      glUniform1i(m_shaders.at("sun").u_locs.at("IsCelShading"), 1);
    }
  }

  // Log for debugging on key H
  if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
  {
    // DEBUG
    std::cout << "DEBUG INFO:\n";
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());
    std::cout << "Current time: " << ms.count() << "ms\n";
    std::cout << "m_view_transform:\n";
    print_glm4matf(m_view_transform);
    std::cout << "local_transform:\n";
    glm::fmat4 local_transform{};
    print_glm4matf(local_transform);
    std::cout << "after scaling:\n";
    local_transform = glm::scale(glm::fmat4{}, glm::vec3{ 0.5f });
    print_glm4matf(local_transform);
    std::cout << "after rotating:\n";
    local_transform = glm::rotate(local_transform, 0.4f, glm::vec3{0.0f, 1.0f, 0.0f});
    print_glm4matf(local_transform);
  }
}


// Handle delta mouse movement input
void ApplicationSolar::mouseCallback(double pos_x, double pos_y) {
  // Read and store mouse movement inputs
  rot_h = float(pos_x);
  rot_v = float(pos_y);
}


// Handle resizing
void ApplicationSolar::resizeCallback(unsigned width, unsigned height) {
  // Recalculate projection matrix for new aspect ration
  m_view_projection = utils::calculate_projection_matrix(float(width) / float(height));
  // Upload new projection matrix
  uploadProjection();
}


// Main
int main(int argc, char* argv[])
{
  std::cout << "Hints:\n"
            << "  Use WASD to move around.\n"
            << "  Use QE / SHIFT CTRL to go up and down.\n"
            << "  Use the mouse to move the camera.\n"
            << "  Hold ALT to move slower.\n"
            << "  Use 1/2 to switch between cel shading and smooth shading.\n";

  // Start the render applicaton
  Application::run<ApplicationSolar>(argc, argv, 3, 2);
}