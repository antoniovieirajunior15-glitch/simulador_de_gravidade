#include <iostream>
#include <GLFW/glfw3.h>

float screenHeight = 600.0f;
float screenWidth = 800.0f;


GLFWwindow*StartGLFW();

int main(){
    GLFWwindow* window= StartGLFW();
    
    float centrox= screenWidth/2.0f;
    float centroy = screenHeight/2.0f;
    float raio= 50.0f;
    int res =100;

    while(!glfwWindowShouldClose(window)){
        glClear(GL_COLOR_BUFFER_BIT);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2d(centrox,centroy);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

GLFWwindow*StartGLFW(){
    if (!glfwInit()){
        std::cerr<<"failed"<<std::endl;
        return nullptr;
    }
    GLFWwindow* window = glfwCreateWindow(800,600,"gravity",NULL,NULL);
    return window;
}

