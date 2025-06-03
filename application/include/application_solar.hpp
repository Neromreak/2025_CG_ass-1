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
  void keyCallback(int key, int action, int mods);
  // Handle delta mouse movement input
  void mouseCallback(double pos_x, double pos_y);
  // Handle resizing
  void resizeCallback(unsigned width, unsigned height);

  // Draw all objects
  void render() const;

 protected:
  void initializeShaderPrograms();
  void initializeGeometry();
  std::vector<float> generateGeometryStars();
  void initializeScene();
  // Update uniform values
  void uploadUniforms();
  // Upload projection matrix
  void uploadProjection();
  // Upload view matrix
  void uploadView();

// CPU representation of model
  model_object planet_object;
  model_object stars_object;
  
  // Camera transform matrix
  glm::fmat4 m_view_transform;
  // Camera projection matrix
  glm::fmat4 m_view_projection;

  SceneGraph* scene;
};

#endif