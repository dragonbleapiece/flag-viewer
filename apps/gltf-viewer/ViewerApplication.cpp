#include "ViewerApplication.hpp"

#include <iostream>
#include <numeric>
#include <chrono>
#include <thread>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include "utils/cameras.hpp"
#include "utils/images.hpp"

#include "PLink.hpp"

struct ShapeVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

const float FRAMERATE_MILLISECONDS = 1000. / 60.;

void keyCallback(
    GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
    glfwSetWindowShouldClose(window, 1);
  }
}

int ViewerApplication::run()
{
  // Loader shaders
  const auto glslProgram =
      compileProgram({m_ShadersRootPath / m_AppName / m_vertexShader,
          m_ShadersRootPath / m_AppName / m_fragmentShader});

  // For the size of the flag | cloth
  const float STEP = 0.5;

  const auto modelViewProjMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uModelViewProjMatrix");
  const auto modelViewMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uModelViewMatrix");
  const auto normalMatrixLocation =
      glGetUniformLocation(glslProgram.glId(), "uNormalMatrix");


  // LIGHTS

  const auto lightDirectionLocation =
      glGetUniformLocation(glslProgram.glId(), "uLightDirection");
  const auto lightIntensityLocation =
      glGetUniformLocation(glslProgram.glId(), "uLightIntensity");

  // GLOBAL
  float mass = 1.f;
  float viscosity = 0.0024f;
  float rigidity = 0.00965f;
  float gravity = 0.5f;
  const float PHYSICS_SCALE = 1e-5;

  glm::vec3 windAmplitude(0.05f, 0.f, 2.25f);
  glm::vec3 windFrequency(glm::pi<float>(), 0.f, glm::pi<float>());

  glm::vec3 up = glm::vec3(0, 1, 0);
  glm::vec3 eye = glm::vec3(0, 0, 35);

   // Build projection matrix
  auto maxDistance = 100.f;
  const auto projMatrix =
      glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight,
          0.001f * maxDistance, 1.5f * maxDistance);


  const float cameraSpeed = 1.f;

  // TODO Implement a new CameraController model and use it instead. Propose the
  // choice from the GUI
  std::unique_ptr<CameraController> cameraController = std::make_unique<TrackballCameraController>(m_GLFWHandle.window(), 0.01f);
  if (m_hasUserCamera) {
    cameraController->setCamera(m_userCamera);
  } else {
    // TODO Use scene bounds to compute a better default camera
    cameraController->setCamera(
        Camera{eye, glm::vec3(0, 0, 0), up});
  }


  // Setup OpenGL state for rendering
  glEnable(GL_DEPTH_TEST);
  glslProgram.use();

  // Light Variables
  glm::vec3 lightDirection(1., 1., 1.);
  glm::vec3 lightIntensity(1., 1., 1.);

  // Calculate vertices

  std::vector<ShapeVertex> data;

  for(size_t i = 0; i < m_nClothWidth; ++i) {
    for(size_t j = 0; j < m_nClothHeight; ++j) {
      ShapeVertex vertex;
            
      vertex.texCoords.x = float(i) / float(m_nClothWidth);
      vertex.texCoords.y = float(j) / float(m_nClothHeight);

      vertex.normal = glm::vec3(0, 0, 1); // perpendicular with cloth
      
      vertex.position = glm::vec3(i - float(m_nClothWidth) / 2., j - float(m_nClothHeight) / 2., 0.) * STEP;

      data.push_back(vertex);
    }
  }


  // Store the indexes

  std::vector<GLuint> indexes;

  for(size_t i = 0; i < m_nClothWidth - 1; ++i) {
    size_t offset = i * m_nClothHeight;
    for(size_t j = 0; j < m_nClothHeight - 1; ++j) {
        indexes.push_back(offset + j);
        indexes.push_back(offset + j + 1);
        indexes.push_back(offset + m_nClothHeight + j + 1);
        indexes.push_back(offset + j);
        indexes.push_back(offset + m_nClothHeight + j);
        indexes.push_back(offset + m_nClothHeight + j + 1);
    }
  }

  // Generate VAO
  GLuint vao;
  glGenVertexArrays(1, &vao);

  // Bind VAO
  glBindVertexArray(vao);

  const GLuint VERTEX_ATTR_POSITION = 0;
  const GLuint VERTEX_ATTR_NORMAL = 1;
  const GLuint VERTEX_ATTR_TEXTURE = 2;
  glEnableVertexAttribArray(VERTEX_ATTR_POSITION);
  glEnableVertexAttribArray(VERTEX_ATTR_NORMAL);
  glEnableVertexAttribArray(VERTEX_ATTR_TEXTURE);

   // Generate VBO
  GLuint vbo;
  glGenBuffers(1, &vbo);

  // Bind VBO to VAO
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // Insert Data
  glBufferData(
      GL_ARRAY_BUFFER,
      data.size() * sizeof(ShapeVertex),
      &data[0],
      GL_DYNAMIC_DRAW
  );

  // Generate IBO
  GLuint ibo;
  glGenBuffers(1, &ibo);

  // Bind IBO to VAO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(GLuint), &indexes[0], GL_STATIC_DRAW);

  //Specify vertex attributes
  glVertexAttribPointer(
      VERTEX_ATTR_POSITION,
      3,
      GL_FLOAT,
      GL_FALSE,
      sizeof(ShapeVertex),
      (const GLvoid*)(offsetof(ShapeVertex, position))
  );

  glVertexAttribPointer(
      VERTEX_ATTR_NORMAL,
      3,
      GL_FLOAT,
      GL_FALSE,
      sizeof(ShapeVertex),
      (const GLvoid*)(offsetof(ShapeVertex, normal))
  );

  glVertexAttribPointer(
      VERTEX_ATTR_TEXTURE,
      2,
      GL_FLOAT,
      GL_FALSE,
      sizeof(ShapeVertex),
      (const GLvoid*)(offsetof(ShapeVertex, texCoords))
  );


  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // PHYSICS

  std::vector<std::shared_ptr<PPoint>> ppoints;

  // Immovible extremity
  for(size_t i = 0; i < m_nClothHeight; ++i) {
    PFixedPoint pfpoint(data[i].position, 0.);
    ppoints.push_back(std::make_shared<PFixedPoint>(pfpoint));
  }

  // Inside
  for(size_t i = m_nClothHeight; i < data.size() - m_nClothHeight; ++i) {
    PPoint ppoint(data[i].position, mass);
    ppoints.push_back(std::make_shared<PPoint>(ppoint));
  }

  // Extremity
  for(size_t i = data.size() - m_nClothHeight; i < data.size(); ++i) {
    PPoint ppoint(data[i].position, mass * 0.9);
    ppoints.push_back(std::make_shared<PPoint>(ppoint));
  }

  std::vector<PLink> plinks;

  /// Structural Mesh + Diagonal Mesh
  // For fixed Point
  for(size_t j = 0; j < m_nClothHeight - 1; ++j) {
    // Horizontal
    plinks.push_back(PLink(ppoints[j], ppoints[m_nClothHeight + j]));
    // Diagonal left-top corner to right-bottom corner
    plinks.push_back(PLink(ppoints[j], ppoints[m_nClothHeight + j + 1]));
  }

  // For internal
  for (size_t i = 1; i < m_nClothWidth - 1; ++i) {
    for (size_t j = 0; j < m_nClothHeight - 1; ++j) {
      // Horizontal
      plinks.push_back(PLink(ppoints[i * m_nClothHeight + j], ppoints[(i + 1) * m_nClothHeight + j]));
      // Verical
      plinks.push_back(PLink(ppoints[i * m_nClothHeight + j], ppoints[i * m_nClothHeight + j + 1]));
      // Diagonal left-bottom corner to right-top corner
      plinks.push_back(PLink(ppoints[(i - 1) * m_nClothHeight + j + 1], ppoints[i * m_nClothHeight + j]));
      // Diagonal left-top corner to right-bottom corner
      plinks.push_back(PLink(ppoints[i * m_nClothHeight + j], ppoints[(i + 1) * m_nClothHeight + j + 1]));
    }
  }

  // For extrema
  for (size_t i = 0; i < m_nClothWidth - 1; ++i) {
    // Horizontal
    plinks.push_back(PLink(ppoints[(i + 1) * m_nClothHeight - 1], ppoints[(i + 2) * m_nClothHeight - 1]));
  }
  
  for (size_t j = 0; j < m_nClothHeight - 1; ++j) {
    // Vertical
    plinks.push_back(PLink(ppoints[(m_nClothWidth - 1) * m_nClothHeight + j], ppoints[(m_nClothWidth - 1) * m_nClothHeight + j + 1]));
    // Diagonal left-bottom corner to right-top corner
    plinks.push_back(PLink(ppoints[(m_nClothWidth - 2) * m_nClothHeight + j + 1], ppoints[(m_nClothWidth - 1) * m_nClothHeight + j]));
  }

  /// Bridge Mesh
  for(size_t i = 0; i < m_nClothWidth - 2; ++i) {
    for(size_t j = 0; j < m_nClothHeight - 2; ++j) {

      if(i > 0) { // no need for fixed points
        // Vertical Bridge
        plinks.push_back(PLink(ppoints[i * m_nClothHeight + j], ppoints[i * m_nClothHeight + j + 2]));
      }

      // Horizontal Bridge
      plinks.push_back(PLink(ppoints[i * m_nClothHeight + j], ppoints[(i + 2) * m_nClothHeight + j]));
      
    }
  }
  

  // Lambda function to simulate physics
  const auto simulateScene = [&](const float h) {
    
    const float fe = 1. / h;

    const glm::vec3 wind = windAmplitude * glm::cos(windFrequency * float(glfwGetTime())) * fe;
    const glm::vec3 g = glm::vec3(0, -gravity * fe, 0);

    PLink::s_k = rigidity * fe * fe;
    PLink::s_z = viscosity * fe;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    for(size_t i = 0; i < plinks.size(); ++i) {
      plinks[i].execute();
    }

    // For positions
    for(size_t i = 0; i < data.size(); ++i) {
      ppoints[i]->applyForce(g + wind); // apply gravity and wind
      ppoints[i]->execute(h);
      ppoints[i]->clearForce();
      data[i].position = ppoints[i]->position();
    }

    // For Normals
    for(size_t i = 0; i < m_nClothWidth; ++i) {
      for(size_t j = 0; j < m_nClothHeight; ++j) {
        glm::vec3 sum(0.);
        float count = 0.f;
        if(j > 0 && i > 0) { // Top - Left (2 triangles)
          sum += glm::cross(
            ppoints[i * m_nClothHeight + j - 1]->position() - ppoints[i * m_nClothHeight + j]->position(),
            ppoints[(i - 1) * m_nClothHeight + j - 1]->position() - ppoints[i * m_nClothHeight + j]->position()
          );
          ++count;

          sum += glm::cross(
            ppoints[(i - 1) * m_nClothHeight + j - 1]->position() - ppoints[i * m_nClothHeight + j]->position(),
            ppoints[(i - 1) * m_nClothHeight + j]->position() - ppoints[i * m_nClothHeight + j]->position()
          );
          ++count;
        }

        if(j < m_nClothWidth - 1 && i < m_nClothWidth - 1) { // Bottom - Right (2 triangles)
          sum += glm::cross(
            ppoints[(i + 1) * m_nClothHeight + j + 1]->position() - ppoints[i * m_nClothHeight + j]->position(),
            ppoints[(i + 1) * m_nClothHeight + j]->position() - ppoints[i * m_nClothHeight + j]->position()
          );
          ++count;

          sum += glm::cross(
            ppoints[i * m_nClothHeight + j + 1]->position() - ppoints[i * m_nClothHeight + j]->position(),
            ppoints[(i + 1) * m_nClothHeight + j + 1]->position() - ppoints[i * m_nClothHeight + j]->position()
          );
          ++count;
        }

        if (j > 0 && i < m_nClothWidth - 1) { // Top - Right
          sum += glm::cross(
            ppoints[i * m_nClothHeight + j - 1]->position() - ppoints[i * m_nClothHeight + j]->position(),
            ppoints[(i + 1) * m_nClothHeight + j]->position() - ppoints[i * m_nClothHeight + j]->position()
          );
          ++count;
        }

        if(i > 0 && j < m_nClothWidth - 1) { // Left - Bottom
          sum += glm::cross(
            ppoints[(i - 1) * m_nClothHeight + j]->position() - ppoints[i * m_nClothHeight + j]->position(),
            ppoints[i * m_nClothHeight + j + 1]->position() - ppoints[i * m_nClothHeight + j]->position()
          );
          ++count;
        }

        data[i * m_nClothHeight + j].normal = glm::normalize(sum / count);
      }
    }

    void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    std::memcpy(ptr, &data[0], data.size() * sizeof(ShapeVertex));
    bool done = glUnmapBuffer(GL_ARRAY_BUFFER);
    assert(done);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
  };

  // Lambda function to draw the scene
  const auto drawScene = [&](const Camera &camera) {
    glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto viewMatrix = camera.getViewMatrix();

    glm::vec3 ligthDirInViewSpace(glm::normalize(viewMatrix * glm::vec4(lightDirection, 0.))); 

    if(lightDirectionLocation >= 0 ) {
      glUniform3fv(lightDirectionLocation, 1, glm::value_ptr(ligthDirInViewSpace));
    }

    if(lightIntensityLocation >= 0) {
      glUniform3fv(lightIntensityLocation, 1, glm::value_ptr(lightIntensity));
    }

    const glm::mat4 modelMatrix(1);

    const glm::mat4 modelViewMatrix = viewMatrix * modelMatrix;
    const glm::mat4 modelViewProjMatrix = projMatrix * modelViewMatrix;
    const glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelViewMatrix));

    glUniformMatrix4fv(modelViewMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelViewMatrix));
    glUniformMatrix4fv(modelViewProjMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelViewProjMatrix));
    glUniformMatrix4fv(normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    glBindVertexArray(vao);

    glDrawElements(GL_TRIANGLES, indexes.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

  };

  // Loop until the user closes the window
  for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose();
       ++iterationCount) {
    const auto seconds = glfwGetTime();

    const auto camera = cameraController->getCamera();
    drawScene(camera);

    // GUI code:
    imguiNewFrame();

    {
      ImGui::Begin("GUI");
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
          1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("eye: %.3f %.3f %.3f", camera.eye().x, camera.eye().y,
            camera.eye().z);
        ImGui::Text("center: %.3f %.3f %.3f", camera.center().x,
            camera.center().y, camera.center().z);
        ImGui::Text(
            "up: %.3f %.3f %.3f", camera.up().x, camera.up().y, camera.up().z);

        ImGui::Text("front: %.3f %.3f %.3f", camera.front().x, camera.front().y,
            camera.front().z);
        ImGui::Text("left: %.3f %.3f %.3f", camera.left().x, camera.left().y,
            camera.left().z);

        if (ImGui::Button("CLI camera args to clipboard")) {
          std::stringstream ss;
          ss << "--lookat " << camera.eye().x << "," << camera.eye().y << ","
             << camera.eye().z << "," << camera.center().x << ","
             << camera.center().y << "," << camera.center().z << ","
             << camera.up().x << "," << camera.up().y << "," << camera.up().z;
          const auto str = ss.str();
          glfwSetClipboardString(m_GLFWHandle.window(), str.c_str());
        }

        // Radio buttons to switch camera type
        static int cameraControllerType = 0;
        if (ImGui::RadioButton("Trackball", &cameraControllerType, 0)) {
          cameraController = std::make_unique<TrackballCameraController>(m_GLFWHandle.window(), cameraSpeed);
          cameraController->setCamera(camera);
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("FirstPerson", &cameraControllerType, 1)) {
          cameraController = std::make_unique<FirstPersonCameraController>(m_GLFWHandle.window(), cameraSpeed * maxDistance);
          cameraController->setCamera(camera);
        }
      }

      if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float theta = 0.0f;
        static float phi = 0.0f;
        static bool lightFromCamera = true;
        ImGui::Checkbox("Light from camera", &lightFromCamera);
        if (lightFromCamera) {
          lightDirection = -camera.front();
        } else {
          if (ImGui::SliderFloat("Theta", &theta, 0.f, glm::pi<float>()) ||
              ImGui::SliderFloat("Phi", &phi, 0, 2.f * glm::pi<float>())) {
            lightDirection = glm::vec3(
              glm::sin(theta) * glm::cos(phi),
              glm::cos(theta),
              glm::sin(theta) * glm::sin(phi)
            );
          }
        }

        static glm::vec3 lightColor(1.f, 1.f, 1.f);
        static float lightIntensityFactor = 1.f;
        if (ImGui::ColorEdit3("Light Color", (float *)&lightColor) ||
            ImGui::SliderFloat("Ligth Intensity", &lightIntensityFactor, 0.f, 10.f)) {
          lightIntensity = lightColor * lightIntensityFactor;
        }
      }

      if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
        static float g = gravity * 10.f;
        static float k = rigidity / PHYSICS_SCALE;
        static float z = viscosity / PHYSICS_SCALE;

        if (ImGui::SliderFloat("Gravity", &g, 0.f, 10.f)) {
          gravity = g / 10.f;
        }

        if (ImGui::SliderFloat("Rigidity", &k, 0.f, 1000.f)) {
          rigidity = k * PHYSICS_SCALE;
        }

        if (ImGui::SliderFloat("Viscosity", &z, 0.f, 1000.f)) {
          viscosity = z * PHYSICS_SCALE;
        }

        if(ImGui::SliderFloat3("Wind Amplitude", &windAmplitude.x, 0.f, 5.f)) {
          // VOID
        }

        if(ImGui::SliderFloat3("Wind Frequency", &windFrequency.x, 0.f, 2.f * glm::pi<float>())) {
          // VOID
        }
      }
      ImGui::End();
    }

    imguiRenderFrame();

    glfwPollEvents(); // Poll for and process events

    // For Physics rendering
    simulateScene(glfwGetTime() - seconds);

    auto elapsedTime = glfwGetTime() - seconds;
    auto guiHasFocus =
        ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    if (!guiHasFocus) {
      cameraController->update(float(elapsedTime));
    }

    m_GLFWHandle.swapBuffers(); // Swap front and back buffers
  
    // Regulate FPS
    if (elapsedTime < FRAMERATE_MILLISECONDS) {
      std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(FRAMERATE_MILLISECONDS - elapsedTime * 1000.f));
    }
  
  }

  // TODO clean up allocated GL data
  glDeleteBuffers(1, &ibo);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);

  return 0;
}

ViewerApplication::ViewerApplication(const fs::path &appPath, uint32_t width,
    uint32_t height, uint32_t fWidth, uint32_t fHeight,
    const std::vector<float> &lookatArgs, const std::string &vertexShader,
    const std::string &fragmentShader) :
    m_nWindowWidth(width),
    m_nWindowHeight(height),
    m_nClothWidth(fWidth),
    m_nClothHeight(fHeight),
    m_AppPath{appPath},
    m_AppName{m_AppPath.stem().string()},
    m_ImGuiIniFilename{m_AppName + ".imgui.ini"},
    m_ShadersRootPath{m_AppPath.parent_path() / "shaders"}
{
  if (!lookatArgs.empty()) {
    m_hasUserCamera = true;
    m_userCamera =
        Camera{glm::vec3(lookatArgs[0], lookatArgs[1], lookatArgs[2]),
            glm::vec3(lookatArgs[3], lookatArgs[4], lookatArgs[5]),
            glm::vec3(lookatArgs[6], lookatArgs[7], lookatArgs[8])};
  }

  if (!vertexShader.empty()) {
    m_vertexShader = vertexShader;
  }

  if (!fragmentShader.empty()) {
    m_fragmentShader = fragmentShader;
  }

  ImGui::GetIO().IniFilename =
      m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows
                                  // positions in this file

  glfwSetKeyCallback(m_GLFWHandle.window(), keyCallback);

  printGLVersion();
}