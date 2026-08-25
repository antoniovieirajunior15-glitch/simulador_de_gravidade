#include <iostream>
#include <vector>
#include <cmath>
#include <GLFW/glfw3.h>

float screenHeight = 600.0f;
float screenWidth = 800.0f;


const float G = 6.67430 * pow() // Gravitational constant

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