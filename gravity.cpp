#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cmath>

const char* vertexShaderSource = R"glsl(#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main(){
    gl_Position = projection * view * model * vec4(aPos, 1.0);
})glsl";

const char* fragmentShaderSource = R"glsl(#version 330 core
out vec4 FragColor;
uniform vec4 objectColor;
void main() {
    FragColor = objectColor;
})glsl";

bool running = true;
bool pause = false;
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f,  1.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
float lastX = 400.0f, lastY = 300.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

const double G = 6.6743e-11;
const float c = 299792458.0f;
float initMass = 5.0f * std::pow(10.0f, 20.0f) / 5.0f;

GLFWwindow* StartGLU();
GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource);
void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount);
void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
glm::vec3 sphericalToCartesian(float r, float theta, float phi);
void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount);

class Object {
public:
    GLuint VAO = 0, VBO = 0;
    glm::vec3 position = glm::vec3(400, 300, 0);
    glm::vec3 velocity = glm::vec3(0, 0, 0);
    size_t vertexCount = 0;
    glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

    bool Initalizing = false;
    bool Launched = false;
    bool target = false;

    float mass;
    float density;
    float radius;

    glm::vec3 LastPos = position;

    Object(glm::vec3 initPosition, glm::vec3 initVelocity, float mass, float density = 3344) {   
        this->position = initPosition;
        this->velocity = initVelocity;
        this->mass = mass;
        this->density = density;
        this->radius = std::pow(((3.0f * this->mass / this->density) / (4.0f * 3.14159265359f)), (1.0f / 3.0f)) / 100000.0f;

        std::vector<float> vertices = Draw();
        vertexCount = vertices.size();
        CreateVBOVAO(VAO, VBO, vertices.data(), vertexCount);
    }

    std::vector<float> Draw() {
        std::vector<float> vertices;
        int stacks = 10;
        int sectors = 10;

        for (float i = 0.0f; i < stacks; ++i) {
            float theta1 = (i / stacks) * glm::pi<float>();
            float theta2 = (i + 1.0f) / stacks * glm::pi<float>();
            for (float j = 0.0f; j < sectors; ++j) {
                float phi1 = j / sectors * 2.0f * glm::pi<float>();
                float phi2 = (j + 1.0f) / sectors * 2.0f * glm::pi<float>();
                glm::vec3 v1 = sphericalToCartesian(radius, theta1, phi1);
                glm::vec3 v2 = sphericalToCartesian(radius, theta1, phi2);
                glm::vec3 v3 = sphericalToCartesian(radius, theta2, phi1);
                glm::vec3 v4 = sphericalToCartesian(radius, theta2, phi2);

                vertices.insert(vertices.end(), {v1.x, v1.y, v1.z});
                vertices.insert(vertices.end(), {v2.x, v2.y, v2.z});
                vertices.insert(vertices.end(), {v3.x, v3.y, v3.z});

                vertices.insert(vertices.end(), {v2.x, v2.y, v2.z});
                vertices.insert(vertices.end(), {v4.x, v4.y, v4.z});
                vertices.insert(vertices.end(), {v3.x, v3.y, v3.z});
            }   
        }
        return vertices;
    }
    
    void UpdatePos() {
        this->position += this->velocity / 94.0f;
        this->radius = std::pow(((3.0f * this->mass / this->density) / (4.0f * 3.14159265359f)), (1.0f / 3.0f)) / 100000.0f;
    }

    void UpdateVertices() {
        std::vector<float> vertices = Draw();
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    }

    glm::vec3 GetPos() const { return this->position; }

    void accelerate(float x, float y, float z) {
        this->velocity += glm::vec3(x, y, z) / 96.0f;
    }

    float CheckCollision(const Object& other) {
        float distance = glm::distance(other.position, this->position);
        if (other.radius + this->radius > distance) {
            return -0.2f;
        }
        return 1.0f;
    }
};

std::vector<Object> objs = {};
std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Object>& objs);
GLuint gridVAO, gridVBO;

int main() {
    GLFWwindow* window = StartGLU();
    if (!window) return -1;

    GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    glUseProgram(shaderProgram);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 750000.0f);
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    cameraPos = glm::vec3(0.0f, 1000.0f, 5000.0f);

    objs = {
        Object(glm::vec3(3844, 0, 0), glm::vec3(0, 0, 228), 7.34767309e22f, 3344),
        Object(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), 5.97219e24f, 5515),
    };

    std::vector<float> gridVertices = CreateGridVertices(100000.0f, 50, objs);
    CreateVBOVAO(gridVAO, gridVBO, gridVertices.data(), gridVertices.size());

    while (!glfwWindowShouldClose(window) && running) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        UpdateCam(shaderProgram, cameraPos);

        if (!objs.empty() && objs.back().Initalizing) {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                objs.back().mass *= 1.0f + 1.0f * deltaTime;
                objs.back().radius = std::pow((3.0f * objs.back().mass / objs.back().density) / (4.0f * 3.14159265359f), 1.0f / 3.0f) / 100000.0f;
                objs.back().UpdateVertices();
            }
        }

        glUseProgram(shaderProgram);
        glUniform4f(objectColorLoc, 1.0f, 1.0f, 1.0f, 0.25f);
        gridVertices = CreateGridVertices(10000.0f, 50, objs);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_DYNAMIC_DRAW);
        DrawGrid(shaderProgram, gridVAO, gridVertices.size());

        for (auto& obj : objs) {
            glUniform4f(objectColorLoc, obj.color.r, obj.color.g, obj.color.b, obj.color.a);

            for (auto& obj2 : objs) {
                if (&obj2 != &obj && !obj.Initalizing && !obj2.Initalizing) {
                    glm::vec3 diff = obj2.GetPos() - obj.GetPos();
                    float distance = glm::length(diff);

                    if (distance > 0.0f) {
                        glm::vec3 direction = diff / distance;
                        float distance_m = distance * 1000.0f;
                        double Gforce = (G * obj.mass * obj2.mass) / (distance_m * distance_m);

                        float acc1 = static_cast<float>(Gforce / obj.mass);
                        glm::vec3 acc = direction * acc1;
                        
                        if (!pause) {
                            obj.accelerate(acc.x, acc.y, acc.z);
                        }

                        obj.velocity *= obj.CheckCollision(obj2);
                    }
                }
            }

            if (obj.Initalizing) {
                obj.radius = std::pow(((3.0f * obj.mass / obj.density) / (4.0f * 3.14159265359f)), (1.0f / 3.0f)) / 100000.0f;
                obj.UpdateVertices();
            }

            if (!pause) {
                obj.UpdatePos();
            }

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(obj.vertexCount / 3));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (auto& obj : objs) {
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteBuffers(1, &obj.VBO);
    }
    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteProgram(shaderProgram);
    
    glfwTerminate();
    return 0;
}

GLFWwindow* StartGLU() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    // Configurações cruciais para compatibilidade macOS + Windows
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "3D Gravity Simulation", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW." << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos) {
    glUseProgram(shaderProgram);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    float cameraSpeed = 1000.0f * deltaTime;
    bool shiftPressed = (mods & GLFW_MOD_SHIFT) != 0;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraUp;

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) pause = true;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE) pause = false;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        running = false;
    }

    if (!objs.empty() && objs.back().Initalizing) {
        if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
            if (!shiftPressed) objs.back().position.y += 0.5f;
            else objs.back().position.z += 0.5f;
        }
        if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
            if (!shiftPressed) objs.back().position.y -= 0.5f;
            else objs.back().position.z -= 0.5f;
        }
        if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT)) objs.back().position.x += 0.5f;
        if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT)) objs.back().position.x -= 0.5f;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos); 
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            objs.emplace_back(glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0f, 0.0f, 0.0f), initMass);
            objs.back().Initalizing = true;
        }
        if (action == GLFW_RELEASE && !objs.empty()) {
            objs.back().Initalizing = false;
            objs.back().Launched = true;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    float cameraSpeed = 50000.0f * deltaTime;
    if (yoffset > 0) cameraPos += cameraSpeed * cameraFront;
    else if (yoffset < 0) cameraPos -= cameraSpeed * cameraFront;
}

glm::vec3 sphericalToCartesian(float r, float theta, float phi) {
    float x = r * sin(theta) * cos(phi);
    float y = r * cos(theta);
    float z = r * sin(theta) * sin(phi);
    return glm::vec3(x, y, z);
}

void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount) {
    glUseProgram(shaderProgram);
    glm::mat4 model = glm::mat4(1.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(gridVAO);
    glPointSize(5.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount / 3));
    glBindVertexArray(0);
}

std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Object>& objs) {
    std::vector<float> vertices;
    float step = size / divisions;
    float halfSize = size / 2.0f;

    for (int yStep = 3; yStep <= 3; ++yStep) {
        float y = -halfSize * 0.3f + yStep * step;
        for (int zStep = 0; zStep <= divisions; ++zStep) {
            float z = -halfSize + zStep * step;
            for (int xStep = 0; xStep < divisions; ++xStep) {
                float xStart = -halfSize + xStep * step;
                float xEnd = xStart + step;
                vertices.push_back(xStart); vertices.push_back(y); vertices.push_back(z);
                vertices.push_back(xEnd);   vertices.push_back(y); vertices.push_back(z);
            }
        }
    }

    for (int xStep = 0; xStep <= divisions; ++xStep) {
        float x = -halfSize + xStep * step;
        for (int yStep = 3; yStep <= 3; ++yStep) {
            float y = -halfSize * 0.3f + yStep * step;
            for (int zStep = 0; zStep < divisions; ++zStep) {
                float zStart = -halfSize + zStep * step;
                float zEnd = zStart + step;
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zStart);
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zEnd);
            }
        }
    }

    for (size_t i = 0; i < vertices.size(); i += 3) {
        glm::vec3 vertexPos(vertices[i], vertices[i+1], vertices[i+2]);
        glm::vec3 totalDisplacement(0.0f);

        for (const auto& obj : objs) {
            glm::vec3 toObject = obj.GetPos() - vertexPos;
            float distance = glm::length(toObject);

            float distance_m = distance * 1000.0f;
            float rs = static_cast<float>((2.0 * G * obj.mass) / (c * c));

            if (distance_m > rs) {
                float z = 2.0f * std::sqrt(rs * (distance_m - rs)) * 100.0f;
                totalDisplacement.y += z;
            }
        }
        
        vertexPos += totalDisplacement;
        vertices[i+1] = vertexPos.y / 15.0f - 3000.0f;
    }

    return vertices;
}