#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cmath>

const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main(){
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)glsl";

const char* fragmentShaderSource = R"glsl(#version 330 core
uniform vec3 objectColor;
void main(){gl_FragColor = vec4(objectColor, objectColor.a);}
)glsl";




float screenHeight = 600.0f;
float screenWidth = 800.0f;


glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm:: vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

const float G = 6.67430 * pow(10,-11); // Gravitational constant

GLFWwindow* StartGLFW();

class Object {
public:
    std::vector<float> position;
    std::vector<float> velocity;
    float raios;
    float mass;

    Object(std::vector<float> position, std::vector<float> velocity, float r){
        this->position = position;
        this->velocity = velocity;
        this->raios = r;
        this->mass = mass;
    }
    void accelerate(float x, float y) {
        velocity[0] += x;
        velocity[1] += y;
    }

    void updatePos() {
        position[0] += velocity[0];
        position[1] += velocity[1];
    }

    void DrawCircle(float centrox, float centroy, float raio, int res) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2d(centrox, centroy);
        for (int i = 0; i <= res; ++i) {
            float angle = 2.0f * 3.141592653589f * (static_cast<float>(i) / res);
            float x = centrox + cos(angle) * raio;
            float y = centroy + sin(angle) * raio;
            glVertex2d(x, y);
        }
        glEnd();
    }
};


GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Vertex shader
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

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    // Shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos) {
    glUseProgram(shaderProgram);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

int main() {
    GLFWwindow* window = StartGLFW();
    if (!window) return -1;

    glfwMakeContextCurrent(window);
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1); 

    int res = 100;

    std::vector<Object> objs = {
        Object(std::vector<float>{200.0f, 500.0f}, std::vector<float>{5.0f, 0.0f}, 7.35 * pow(10, 22)),
        Object(std::vector<float>{700.0f, 500.0f}, std::vector<float>{5.0f, 0.0f}, 7.35 * pow(10, 22)),
    };

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        for (auto& obj : objs) {
            for (auto&obj2 : objs){
                if(&obj2 == &obj) continue;
                float dx = obj2.position[0] - obj.position[0];
                float dy = obj2.position[1] - obj.position[1];
                float distance = sqrt(dx * dx + dy * dy);
                std::vector<float> direction = { dx / distance, dy / distance };
                distance *= 1000;

                float gforce = (G * obj.mass * obj.mass) / (distance * distance);
                float acc1 = gforce / obj.mass;

                std::vector<float> acc = { direction[0] * acc1, direction[1] * acc1 };
                obj.accelerate(acc[0], acc[1]);
            }
            obj.updatePos();
            obj.DrawCircle(obj.position[0], obj.position[1], obj.raios, res);

            if (obj.position[1] < 0 || obj.position[1] > screenHeight)
                obj.velocity[1] *= -0.95f;

            if (obj.position[0] < 0 || obj.position[0] > screenWidth)
                obj.velocity[0] *= -0.95f;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

GLFWwindow* StartGLFW() {

    if (!glfwInit()) {
        std::cerr << "failed" << std::endl;
        return nullptr;
    }
    return glfwCreateWindow(800, 600, "gravity", NULL, NULL);
}
GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource) {
    // Vertex shader
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

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    // Shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}
void CreateVBOVAO(GLuint& VBO, GLuint& VAO, const std::vector<float>& vertices) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
