#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cmath>
#include <limits>  

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

const char* fragmentShaderSource = R"glsl(
#version 330 core
in float lightIntensity;
out vec4 FragColor;
uniform vec4 objectColor;
uniform bool isGrid;
uniform bool GLOW;
void main() {
    if (isGrid) {
        FragColor = objectColor;
    } else if(GLOW){
        FragColor = vec4(objectColor.rgb * 100000, objectColor.a);
    } else {
        float fade = smoothstep(0.0, 10.0, lightIntensity*10);
        FragColor = vec4(objectColor.rgb * fade, objectColor.a);
    }
})glsl";

float screenHeight = 600.0f;
float screenWidth = 800.0f;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

const float G = 6.67430f * (float)pow(10, -11); // Gravitational constant
const float c = 299792458.0f;                   // FIX: speed of light, usado em UpdateGridVertices e não estava declarado

GLFWwindow* StartGLFW();

// FIX: sphericalToCartesian não existia em lugar nenhum do código - adicionada aqui,
// antes da classe Object que a utiliza.
glm::vec3 sphericalToCartesian(float raio, float theta, float phi) {
    float x = raio * sin(phi) * cos(theta);
    float y = raio * cos(phi);
    float z = raio * sin(phi) * sin(theta);
    return glm::vec3(x, y, z);
}

class Object {
public:
    std::vector<float> position;
    std::vector<float> velocity;
    float raios;
    float mass;
    bool Initalizing = false; // FIX: membro usado em UpdateGridVertices mas não existia

    // FIX: mass agora é parâmetro do construtor (antes fazia this->mass = mass,
    // que era a variável não inicializada atribuída a si mesma - undefined behavior)
    Object(std::vector<float> position, std::vector<float> velocity, float r, float m) {
        this->position = position;
        this->velocity = velocity;
        this->raios = r;
        this->mass = m;
    }

    void accelerate(float x, float y) {
        velocity[0] += x;
        velocity[1] += y;
    }

    void updatePos() {
        position[0] += velocity[0];
        position[1] += velocity[1];
    }

    // FIX: método usado em UpdateGridVertices (obj.GetPos()) mas não existia
    glm::vec3 GetPos() const {
        return glm::vec3(position[0], position[1], 0.0f);
    }

    // FIX: faltava fechar o for(j), retornar 'vertices' e fechar a função Draw.
    // Antes, DrawCircle estava sendo declarada DENTRO de Draw por erro de chaves.
    std::vector<float> Draw(int res = 10) {
        std::vector<float> vertices;
        for (int i = 0; i <= res; ++i) {
            float theta = (i / (float)res) * 2.0f * 3.141592653589f;
            float theta1 = ((i + 1) / (float)res) * 2.0f * 3.141592653589f;
            for (int j = 0; j < res; ++j) {
                float phi = (j / (float)res) * 3.141592653589f;
                float phi1 = ((j + 1) / (float)res) * 3.141592653589f;

                glm::vec3 v1 = sphericalToCartesian(raios, theta, phi);
                glm::vec3 v2 = sphericalToCartesian(raios, theta1, phi);
                glm::vec3 v3 = sphericalToCartesian(raios, theta1, phi1);
                glm::vec3 v4 = sphericalToCartesian(raios, theta, phi1);

                vertices.insert(vertices.end(), {v1.x, v1.y, v1.z});
                vertices.insert(vertices.end(), {v2.x, v2.y, v2.z});
                vertices.insert(vertices.end(), {v3.x, v3.y, v3.z});
                vertices.insert(vertices.end(), {v2.x, v2.y, v2.z});
                vertices.insert(vertices.end(), {v4.x, v4.y, v4.z});
                vertices.insert(vertices.end(), {v3.x, v3.y, v3.z});
            }
        }
        return vertices; // FIX: função declarava retorno mas não retornava nada
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
}; // FIX: só uma chave de fechamento de classe (antes havia "};" duplicado)

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
// FIX: removida a segunda definição duplicada de CreateShaderProgram que existia
// depois de main() - isso causava erro de "redefinition".

void UpdateCam(GLuint shaderProgram, glm::vec3 cameraPos) {
    glUseProgram(shaderProgram);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

std::vector<Object> objs;

int main() {
    GLFWwindow* window = StartGLFW();
    if (!window) return -1;

    glfwMakeContextCurrent(window);
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1);

    int res = 100;

    // FIX: construtor agora recebe (position, velocity, raio, mass) - antes só
    // recebia 3 argumentos e o 4º (mass) não existia no construtor.
    objs = {
        Object(std::vector<float>{200.0f, 500.0f}, std::vector<float>{5.0f, 0.0f}, 10.0f, 7.35f * (float)pow(10, 22)),
        Object(std::vector<float>{700.0f, 500.0f}, std::vector<float>{5.0f, 0.0f}, 10.0f, 7.35f * (float)pow(10, 22)),
    };

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        for (auto& obj : objs) {
            for (auto& obj2 : objs) {
                if (&obj2 == &obj) continue;
                float dx = obj2.position[0] - obj.position[0];
                float dy = obj2.position[1] - obj.position[1];
                float distance = sqrt(dx * dx + dy * dy);
                std::vector<float> direction = {dx / distance, dy / distance};
                distance *= 1000;

                float gforce = (G * obj.mass * obj2.mass) / (distance * distance); // FIX: era obj.mass*obj.mass, deveria envolver a outra massa
                float acc1 = gforce / obj.mass;

                std::vector<float> acc = {direction[0] * acc1, direction[1] * acc1};
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

std::vector<float> UpdateGridVertices(std::vector<float> vertices, const std::vector<Object>& objs) {
    float totalMass = 0.0f;
    float comY = 0.0f;
    for (const auto& obj : objs) {
        if (obj.Initalizing) continue;
        comY += obj.mass * obj.position[1]; // FIX: obj.position.y não existe (position é vector<float>) - usa position[1]
        totalMass += obj.mass;
    }
    if (totalMass > 0) comY /= totalMass;

    float originalMaxY = -std::numeric_limits<float>::infinity();
    for (size_t i = 0; i < vertices.size(); i += 3) { // FIX: int -> size_t (comparação com .size() sem sinal)
        originalMaxY = std::max(originalMaxY, vertices[i + 1]);
    }

    float verticalShift = comY - originalMaxY;
    std::cout << "vertical shift: " << verticalShift << " |         comY: " << comY
               << "|            originalmaxy: " << originalMaxY << std::endl;

    for (size_t i = 0; i < vertices.size(); i += 3) {
        glm::vec3 vertexPos(vertices[i], vertices[i + 1], vertices[i + 2]);
        glm::vec3 totalDisplacement(0.0f);
        for (const auto& obj : objs) {
            glm::vec3 toObject = obj.GetPos() - vertexPos;
            float distance = glm::length(toObject);
            float distance_m = distance * 1000.0f;
            float rs = (2 * G * obj.mass) / (c * c);

            float dz = 2 * sqrt(rs * (distance_m - rs));
            totalDisplacement.y += dz * 2.0f;
        }
        vertices[i + 1] = totalDisplacement.y + -abs(verticalShift);
    }

    return vertices;
}