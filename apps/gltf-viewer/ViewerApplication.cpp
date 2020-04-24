#include "ViewerApplication.hpp"

#include <iostream>
#include <numeric>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#include "utils/cameras.hpp"

#include <stb_image_write.h>
#include "utils/images.hpp"

// Each vertex attribute is identified by an index
// What vertex attribute we use, and what are their index is defined by the vertex shader
// we will use (more information later).
// position, normal and texcoord is pretty standard for 3D applications
const std::vector<std::string> ATTRIBUTES = {"POSITION", "NORMAL", "TEXCOORD_0"};

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


  float epsilon = 0.001f;
  // check if the scene is flat
  glm::vec3 up = glm::vec3(0, 1, 0);
  glm::vec3 eye = glm::vec3(0, 0, 100);

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


  // Lambda function to draw the scene
  /*const auto drawScene = [&](const Camera &camera) {
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

    // The recursive function that should draw a node
    // We use a std::function because a simple lambda cannot be recursive
    const std::function<void(int, const glm::mat4 &)> drawNode =
        [&](int nodeIdx, const glm::mat4 &parentMatrix) {
          // TODO The drawNode function
          const auto &node = model.nodes[nodeIdx];

          glm::mat4 modelMatrix = getLocalToWorldMatrix(node, parentMatrix);

          if(node.mesh >= 0) {
            const glm::mat4 modelViewMatrix = viewMatrix * modelMatrix;
            const glm::mat4 modelViewProjMatrix = projMatrix * modelViewMatrix;
            const glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelViewMatrix));

            glUniformMatrix4fv(modelViewMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelViewMatrix));
            glUniformMatrix4fv(modelViewProjMatrixLocation, 1, GL_FALSE, glm::value_ptr(modelViewProjMatrix));
            glUniformMatrix4fv(normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(normalMatrix));

            const auto &mesh = model.meshes[node.mesh];
            const auto &vaoRange = meshIndexToVaoRange[node.mesh];

            for(size_t primitiveIdx = 0; primitiveIdx < mesh.primitives.size(); ++primitiveIdx) {
              const auto &primitive = mesh.primitives[primitiveIdx];
              bindMaterial(primitive.material);
              
              const GLuint vao = vertexArrayObjects[vaoRange.begin + primitiveIdx];

              glBindVertexArray(vao);

              if(primitive.indices >= 0) {
                const auto &accessor = model.accessors[primitive.indices];
                const auto &bufferView = model.bufferViews[accessor.bufferView];
                const auto byteOffset = bufferView.byteOffset + accessor.byteOffset;

                glDrawElements(primitive.mode, GLsizei(accessor.count), accessor.componentType,(const GLvoid*)byteOffset);
              } else {
                const auto accessorIdx = (*begin(primitive.attributes)).second;
                const auto &accessor = model.accessors[accessorIdx];
                glDrawArrays(primitive.mode, 0, GLsizei(accessor.count));
              }
            }
            glBindVertexArray(0);
          }

          // Draw children
          for (const auto childNodeIdx : node.children) {
            drawNode(childNodeIdx, modelMatrix);
          }
        };

    // Draw the scene referenced by gltf file
    if (model.defaultScene >= 0) {
      // TODO Draw all nodes
      for(const auto nodeIdx : model.scenes[model.defaultScene].nodes) {
        drawNode(nodeIdx, glm::mat4(1));
      }
    }
  };

  // Render to image output
  if(!m_OutputPath.empty()) {
    const int numComponents = 3;
    std::vector<unsigned char> pixels(m_nWindowWidth * m_nWindowHeight * numComponents);
    renderToImage(m_nWindowWidth, m_nWindowHeight, numComponents, pixels.data(), [&]() {
      drawScene(cameraController->getCamera());
    });
    flipImageYAxis(m_nWindowWidth, m_nWindowHeight, numComponents, pixels.data());
    const auto strPath = m_OutputPath.string();
    stbi_write_png(
      strPath.c_str(),
      m_nWindowWidth,
      m_nWindowHeight,
      numComponents,
      pixels.data(),
      0
    );

    return 0;
  }

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
      }
      ImGui::End();
    }

    imguiRenderFrame();

    glfwPollEvents(); // Poll for and process events

    auto ellapsedTime = glfwGetTime() - seconds;
    auto guiHasFocus =
        ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    if (!guiHasFocus) {
      cameraController->update(float(ellapsedTime));
    }

    m_GLFWHandle.swapBuffers(); // Swap front and back buffers
  }
*/
  // TODO clean up allocated GL data

  return 0;
}

ViewerApplication::ViewerApplication(const fs::path &appPath, uint32_t width,
    uint32_t height,
    const std::vector<float> &lookatArgs, const std::string &vertexShader,
    const std::string &fragmentShader, const fs::path &output) :
    m_nWindowWidth(width),
    m_nWindowHeight(height),
    m_AppPath{appPath},
    m_AppName{m_AppPath.stem().string()},
    m_ImGuiIniFilename{m_AppName + ".imgui.ini"},
    m_ShadersRootPath{m_AppPath.parent_path() / "shaders"},
    m_OutputPath{output}
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