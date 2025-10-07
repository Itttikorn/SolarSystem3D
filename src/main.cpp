#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>

#include <iostream>
#include <vector>
#include <cmath>

#define M_PI 3.14159265358979323846

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// Camera modes
enum CameraMode { FOLLOW_PLANET, FREE };
CameraMode cameraMode = FOLLOW_PLANET;
int followedPlanetIdx = 3; // Start with Earth

unsigned int loadTexture(const char* path);
void createSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, unsigned int sectorCount, unsigned int stackCount);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 5.0f, 20.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Sphere mesh data
std::vector<float> sphereVertices;
std::vector<unsigned int> sphereIndices;

struct Planet {
    float orbitRadius;
    float orbitSpeed; // radians/sec
    float selfRotateSpeed; // radians/sec
    float size;
    glm::vec3 color;
    unsigned int texture;
    float orbitAngle; // updated each frame
};

std::vector<Planet> planets;
glm::vec3 earthPos, moonPos;

// Input state for planet switching
bool qPressedLast = false;
bool ePressedLast = false;
bool wasdPressedLast = false;

// Orbit camera state for planet focus mode
float orbitYaw = 0.0f;   // horizontal angle around planet
float orbitPitch = 20.0f; // vertical angle (degrees)
float orbitDistance = 3.0f;

void updateCameraFollow();

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System Simulator | WASD - FreeCam | Q/E - Next Planet", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // build and compile our shader program
    Shader lightingShader("6.multiple_lights.vs", "6.multiple_lights.fs");
    Shader lightCubeShader("6.light_cube.vs", "6.light_cube.fs");

    // create sphere data
    const unsigned int sectorCount = 64;
    const unsigned int stackCount = 64;
    createSphere(sphereVertices, sphereIndices, 1.0f, sectorCount, stackCount);

    // setup sphere VAO/VBO/EBO
    unsigned int sphereVAO, sphereVBO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), &sphereVertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), &sphereIndices[0], GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // light cube VAO
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // load textures
    unsigned int sunTexture = loadTexture(FileSystem::getPath("resources/textures/sun.jpg").c_str());
    unsigned int mercuryTexture = loadTexture(FileSystem::getPath("resources/textures/mercury.jpg").c_str());
    unsigned int venusTexture = loadTexture(FileSystem::getPath("resources/textures/venus.jpg").c_str());
    unsigned int earthTexture = loadTexture(FileSystem::getPath("resources/textures/earth.jpg").c_str());
    unsigned int moonTexture = loadTexture(FileSystem::getPath("resources/textures/moon.jpg").c_str());
    unsigned int marsTexture = loadTexture(FileSystem::getPath("resources/textures/mars.jpg").c_str());
    unsigned int jupiterTexture = loadTexture(FileSystem::getPath("resources/textures/jupiter.jpg").c_str());
    unsigned int saturnTexture = loadTexture(FileSystem::getPath("resources/textures/saturn.jpg").c_str());
    unsigned int uranusTexture = loadTexture(FileSystem::getPath("resources/textures/uranus.jpg").c_str());
    unsigned int neptuneTexture = loadTexture(FileSystem::getPath("resources/textures/neptune.jpg").c_str());
    unsigned int asteroidTexture = loadTexture(FileSystem::getPath("resources/textures/asteroid.jpg").c_str());

    // Planets: orbitRadius (scaled AU), orbitSpeed, selfRotateSpeed, size, color, texture, orbitAngle
    // AU scale: 1.0f = 2.5 units (1 AU = 2.5f)
    planets = {
        // Sun (center, no orbit)
        { 0.0f, 0.0f, 0.5f, 0.625f, glm::vec3(1.0f, 0.9f, 0.3f), sunTexture, 0.0f },
        // Mercury (0.39 AU)
        { 0.975f, 4.15f, 1.0f, 0.045f, glm::vec3(0.7f, 0.7f, 0.7f), mercuryTexture, 0.0f },
        // Venus (0.72 AU)
        { 1.8f, 1.62f, 1.2f, 0.1125f, glm::vec3(1.0f, 0.8f, 0.5f), venusTexture, 0.0f },
        // Earth (1.00 AU)
        { 2.5f, 1.0f, 1.5f, 0.125f, glm::vec3(0.5f, 0.7f, 1.0f), earthTexture, 0.0f },
        // Moon (orbits Earth)
        { 0.2f, 12.0f, 2.0f, 0.0325f, glm::vec3(0.8f, 0.8f, 0.8f), moonTexture, 0.0f },
        // Mars (1.52 AU)
        { 3.8f, 0.53f, 1.0f, 0.0675f, glm::vec3(1.0f, 0.5f, 0.3f), marsTexture, 0.0f },
        // Jupiter (5.20 AU)
        { 13.0f, 0.08f, 0.8f, 0.25f, glm::vec3(1.0f, 0.8f, 0.5f), jupiterTexture, 0.0f },
        // Saturn (9.58 AU)
        { 23.95f, 0.03f, 0.7f, 0.2125f, glm::vec3(1.0f, 0.9f, 0.6f), saturnTexture, 0.0f },
        // Uranus (19.18 AU)
        { 47.95f, 0.011f, 0.6f, 0.15f, glm::vec3(0.7f, 0.9f, 1.0f), uranusTexture, 0.0f },
        // Neptune (30.07 AU)
        { 75.175f, 0.006f, 0.5f, 0.145f, glm::vec3(0.5f, 0.7f, 1.0f), neptuneTexture, 0.0f }
    };

    // Asteroid belt
    const int asteroidCount = 200;
    std::vector<glm::vec3> asteroidPositions;
    std::vector<float> asteroidScales;
    for (int i = 0; i < asteroidCount; ++i) {
        float angle = ((float)i / asteroidCount) * glm::two_pi<float>();
        float radius = 5.5f + static_cast<float>(rand()) / RAND_MAX * 2.5f;
        float height = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.25f;
        asteroidPositions.push_back(glm::vec3(
            cos(angle) * radius,
            height,
            sin(angle) * radius
        ));
        float scale = 0.02f + static_cast<float>(rand()) / RAND_MAX * 0.0175f;
        asteroidScales.push_back(scale);
    }

    // shader configuration
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);
    lightingShader.setInt("material.specular", 0); // use same texture for specular

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Camera follow/focus logic
        bool wasdPressed = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

        // Planet switching
        bool qPressed = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
        bool ePressed = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
        int planetCount = static_cast<int>(planets.size());
        if (qPressed && !qPressedLast) {
            followedPlanetIdx--;
            if (followedPlanetIdx < 1) followedPlanetIdx = 9;
            cameraMode = FOLLOW_PLANET;
        }
        if (ePressed && !ePressedLast) {
            followedPlanetIdx++;
            if (followedPlanetIdx > 9) followedPlanetIdx = 1;
            cameraMode = FOLLOW_PLANET;
        }
        qPressedLast = qPressed;
        ePressedLast = ePressed;

        if (cameraMode == FOLLOW_PLANET && wasdPressed && !wasdPressedLast) {
            cameraMode = FREE;
        }
        wasdPressedLast = wasdPressed;

        glClearColor(0.02f, 0.02f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Animate planet orbits
        planets[0].orbitAngle = 0.0f; // Sun doesn't orbit
        for (size_t i = 1; i < planets.size(); ++i) {
            planets[i].orbitAngle = currentFrame * planets[i].orbitSpeed;
        }
        // Animate moon around earth
        earthPos = glm::vec3(
            cos(planets[3].orbitAngle) * planets[3].orbitRadius,
            0.0f,
            sin(planets[3].orbitAngle) * planets[3].orbitRadius
        );
        moonPos = earthPos + glm::vec3(
            cos(planets[4].orbitAngle) * planets[4].orbitRadius,
            0.0f,
            sin(planets[4].orbitAngle) * planets[4].orbitRadius
        );

        // Camera follow logic
        if (cameraMode == FOLLOW_PLANET) {
            updateCameraFollow();
        }

        //Sun and extra point lights around the sun
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);
        lightingShader.setFloat("material.shininess", 1.0f);

        glm::vec3 sunPos(0.0f, 0.0f, 0.0f);

        // Sun as point light 0
        lightingShader.setVec3("pointLights[0].position", sunPos);
        lightingShader.setVec3("pointLights[0].ambient", glm::vec3(0.3f, 0.3f, 0.3f));
        lightingShader.setVec3("pointLights[0].diffuse", glm::vec3(0.7f, 0.7f, 0.7f));
        lightingShader.setVec3("pointLights[0].specular", glm::vec3(0.0f, 0.0f, 0.0f));
        lightingShader.setFloat("pointLights[0].constant", 1.0f);
        lightingShader.setFloat("pointLights[0].linear", 0.007f);
        lightingShader.setFloat("pointLights[0].quadratic", 0.0002f);

        // Add 6 extra point lights in a sphere around the sun for even illumination
        float sunRingRadius = 0.75f;
        for (int i = 0; i < 6; ++i) {
            float theta = glm::two_pi<float>() * i / 6.0f;
            float phi = glm::pi<float>() * (i % 2 == 0 ? 0.33f : 0.66f); // alternate latitude
            glm::vec3 ringPos = glm::vec3(
                sin(phi) * cos(theta) * sunRingRadius,
                cos(phi) * sunRingRadius,
                sin(phi) * sin(theta) * sunRingRadius
            );
            std::string idx = std::to_string(i + 1);
            lightingShader.setVec3("pointLights[" + idx + "].position", ringPos);
            lightingShader.setVec3("pointLights[" + idx + "].ambient", glm::vec3(0.15f, 0.15f, 0.15f));
            lightingShader.setVec3("pointLights[" + idx + "].diffuse", glm::vec3(0.35f, 0.35f, 0.35f));
            lightingShader.setVec3("pointLights[" + idx + "].specular", glm::vec3(0.0f, 0.0f, 0.0f)); // No reflection
            lightingShader.setFloat("pointLights[" + idx + "].constant", 1.0f);
            lightingShader.setFloat("pointLights[" + idx + "].linear", 0.07f);
            lightingShader.setFloat("pointLights[" + idx + "].quadratic", 0.017f);
        }

        lightingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        lightingShader.setVec3("dirLight.ambient", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("dirLight.diffuse", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("dirLight.specular", 0.0f, 0.0f, 0.0f);

        lightingShader.setVec3("spotLight.position", camera.Position);
        lightingShader.setVec3("spotLight.direction", camera.Front);
        lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
        lightingShader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);
        lightingShader.setFloat("spotLight.constant", 1.0f);
        lightingShader.setFloat("spotLight.linear", 0.09f);
        lightingShader.setFloat("spotLight.quadratic", 0.032f);
        lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 250.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // Draw planets
        for (size_t i = 0; i < planets.size(); ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            glm::vec3 pos;
            if (i == 0) {
                // Sun
                pos = glm::vec3(0.0f);
            }
            else if (i == 4) {
                // Moon
                pos = earthPos + glm::vec3(
                    cos(planets[4].orbitAngle) * planets[4].orbitRadius,
                    0.0f,
                    sin(planets[4].orbitAngle) * planets[4].orbitRadius
                );
            }
            else if (i == 3) {
                // Earth
                pos = earthPos;
            }
            else {
                pos = glm::vec3(
                    cos(planets[i].orbitAngle) * planets[i].orbitRadius,
                    0.0f,
                    sin(planets[i].orbitAngle) * planets[i].orbitRadius
                );
            }
            model = glm::translate(model, pos);
            model = glm::rotate(model, currentFrame * planets[i].selfRotateSpeed, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(planets[i].size));
            lightingShader.setMat4("model", model);
            glBindTexture(GL_TEXTURE_2D, planets[i].texture);
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(sphereIndices.size()), GL_UNSIGNED_INT, 0);
        }

        // Draw asteroid belt
        for (size_t i = 0; i < asteroidPositions.size(); ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, asteroidPositions[i]);
            model = glm::scale(model, glm::vec3(asteroidScales[i]));
            lightingShader.setMat4("model", model);
            glBindTexture(GL_TEXTURE_2D, asteroidTexture);
            glBindVertexArray(sphereVAO);
            glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(sphereIndices.size()), GL_UNSIGNED_INT, 0);
        }

        // Draw the sun as a light source
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        glBindVertexArray(lightCubeVAO);
        glm::mat4 sunLightModel = glm::mat4(1.0f);
        sunLightModel = glm::scale(sunLightModel, glm::vec3(0.075f));
        lightCubeShader.setMat4("model", sunLightModel);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(sphereIndices.size()), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);

    glfwTerminate();
    return 0;
}

// Camera follow logic
void updateCameraFollow()
{
    int idx = followedPlanetIdx;
    glm::vec3 pos;
    if (idx == 4) {
        pos = earthPos + glm::vec3(
            cos(planets[4].orbitAngle) * planets[4].orbitRadius,
            0.0f,
            sin(planets[4].orbitAngle) * planets[4].orbitRadius
        );
    }
    else if (idx == 3) {
        pos = earthPos;
    }
    else {
        pos = glm::vec3(
            cos(planets[idx].orbitAngle) * planets[idx].orbitRadius,
            0.0f,
            sin(planets[idx].orbitAngle) * planets[idx].orbitRadius
        );
    }
    // Orbit camera around the planet
    float yawRad = glm::radians(orbitYaw);
    float pitchRad = glm::radians(glm::clamp(orbitPitch, -89.0f, 89.0f));
    float r = orbitDistance + planets[idx].size * 4.0f;
    glm::vec3 offset;
    offset.x = r * cos(pitchRad) * sin(yawRad);
    offset.y = r * sin(pitchRad);
    offset.z = r * cos(pitchRad) * cos(yawRad);
    camera.Position = pos + offset;
    camera.Front = glm::normalize(pos - camera.Position);
    camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
}

// Sphere generation: position, normal, texcoord (8 floats per vertex)
void createSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, unsigned int sectorCount, unsigned int stackCount)
{
    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / radius;
    float s, t;

    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;

    for (unsigned int i = 0; i <= stackCount; ++i)
    {
        stackAngle = M_PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        y = radius * sinf(stackAngle);

        for (unsigned int j = 0; j <= sectorCount; ++j)
        {
            sectorAngle = j * sectorStep;
            x = xy * cosf(sectorAngle);
            z = xy * sinf(sectorAngle);

            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            s = 1.0f - (float)j / sectorCount;
            t = 1.0f - (float)i / stackCount;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
            vertices.push_back(s);
            vertices.push_back(t);
        }
    }

    unsigned int k1, k2;
    for (unsigned int i = 0; i < stackCount; ++i)
    {
        k1 = i * (sectorCount + 1);
        k2 = k1 + sectorCount + 1;

        for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stackCount - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else
            format = GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        stbi_image_free(data);
        std::cout << "Loaded " << path << " with " << nrComponents << " channels" << std::endl;
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (cameraMode == FREE) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    if (cameraMode == FREE) {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
    else if (cameraMode == FOLLOW_PLANET) {
        // Orbit camera around planet
        float sensitivity = 0.15f;
        orbitYaw += xoffset * sensitivity;
        orbitPitch += yoffset * sensitivity;
        if (orbitPitch > 89.0f) orbitPitch = 89.0f;
        if (orbitPitch < -89.0f) orbitPitch = -89.0f;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (cameraMode == FREE) {
        camera.ProcessMouseScroll(static_cast<float>(yoffset));
    }
    else if (cameraMode == FOLLOW_PLANET) {
        orbitDistance -= static_cast<float>(yoffset) * 0.2f;
        if (orbitDistance < 0.5f) orbitDistance = 0.5f;
        if (orbitDistance > 30.0f) orbitDistance = 30.0f;
    }
}