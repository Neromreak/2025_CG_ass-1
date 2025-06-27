#ifndef APPLICATION_SOLAR_HPP
#define APPLICATION_SOLAR_HPP

#include <vector>

#include "application.hpp"
#include "model.hpp"
#include "structs.hpp"
#include "scene_graph.hpp"

// GPU representation of model
class ApplicationSolar : public Application {
 public:
  // Allocate and initialize objects
  ApplicationSolar(std::string const& resource_path);
  // Free allocated objects
  ~ApplicationSolar();

  // React to key input
  void keyCallback(GLFWwindow* window, int key, int action, int mods) override;
  // Handle delta mouse movement input
  void mouseCallback(double pos_x, double pos_y) override;
  // Handle resizing
  void resizeCallback(unsigned width, unsigned height) override;

  // Draw all objects
  void render() const override;
  void render_first_frame() const;

  // Execute logic and physics
  void physics() override;

 protected:
  void initializeShaderPrograms();
  void initializeTextures();
  void initializeGeometry();
  std::vector<float> generateGeometryStars();
  std::vector<float> generateGeometryCircle();
  void initializeScene();
  // Update uniform values
  void uploadUniforms();
  // Upload projection matrix
  void uploadProjection();
  // Upload view matrix
  void uploadView();

// Model objects (CPU representation of model)
  model_object planet_object;
  model_object stars_object;
  model_object circle_object;

// Texture objects (CPU representation of textures)
  texture_object earth_texture;
  
  // Camera transform matrix
  glm::fmat4 m_view_transform;
  // Camera projection matrix
  glm::fmat4 m_view_projection;

  SceneGraph* scene;

  const float SIMULATION_SPEED = 0.2f;

  // Variables for input
  float movement_speed = 0.019f;
  int move_x = 0;
  int move_y = 0;
  int move_z = 0;
  float rotation_speed = 0.001f;
  float rot_h = 0.0f;
  float rot_v= 0.0f;
  bool cel_shading = false;
};

#endif