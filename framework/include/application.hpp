#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "structs.hpp"

#include <glm/gtc/type_precision.hpp>

#include <map>

struct GLFWwindow;
// GPU representation of model
class Application {
 public:
  template<typename T>
  static void run(int argc, char* argv[], unsigned ver_major, unsigned ver_minor);

  // Allocate and initialize objects
  Application(std::string const& resource_path);
  // Free shader resources
  virtual ~Application();

  // Update viewport and field of view
  void resize_callback(unsigned width, unsigned height);
  // Handle key input
  void key_callback(GLFWwindow* window, int key, int action, int mods);
  // Handle mouse movement input
  void mouse_callback(GLFWwindow* window, double pos_x, double pos_y);
  // Recompile shaders form source files
  void reloadShaders(bool throwing);

// Functions which are implemented in derived classes
  // Update uniform locations and values
  inline virtual void uploadUniforms() {};
  // React to key input
  inline virtual void keyCallback(int key, int action, int mods) {};
  // Handle delta mouse movement input
  inline virtual void mouseCallback(double pos_x, double pos_y) {};
  // Update framebuffer textures
  inline virtual void resizeCallback(unsigned width, unsigned height) {};
  // Draw all objects
  virtual void render() const = 0;
  // Logic / Physics
  virtual void physics() = 0;

 protected:
  void updateUniformLocations();
  
  std::string m_resource_path; 

  // Container for the shader programs
  std::map<std::string, shader_program> m_shaders{};

  // Resolution when 
  static const glm::uvec2 initial_resolution; 
  static const float initial_aspect_ratio; 
};


#include "utils.hpp"
#include "window_handler.hpp"

template<typename T>
void Application::run(int argc, char* argv[], unsigned ver_major, unsigned ver_minor) {  

    GLFWwindow* window = window_handler::initialize(initial_resolution, ver_major, ver_minor);
    
    std::string resource_path = utils::read_resource_path(argc, argv);
    T* application = new T{resource_path};

    window_handler::set_callback_object(window, application);

    // Do intial shader load an uniform upload
    application->reloadShaders(true);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Rendering loop
    while (!glfwWindowShouldClose(window)) {
      // Query input
      glfwPollEvents();
      // Clear buffer
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      // Execute logic and physics
      application->physics();
      // Draw geometry
      application->render();
      // Swap draw buffer to front
      glfwSwapBuffers(window);
      // Display fps
      window_handler::show_fps(window);
    }

    delete application;
    window_handler::close_and_quit(window, EXIT_SUCCESS);
}

#endif