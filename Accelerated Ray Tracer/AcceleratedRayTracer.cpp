﻿#include "AcceleratedRayTracer.h"

const int width = 800, height = 600; 
int frameCnt; bool f; float lastTime, currentTime, lastx, lasty;
Camera camera(vec3(0.0f, 0.0f, 3.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!f) { f = 1; lastx = xpos, lasty = ypos; return; }
    float daltax = xpos - lastx, daltay = ypos - lasty;
    lastx = xpos, lasty = ypos;
    camera.ProcessMouseMovement(daltax, daltay);
}

int main() 
{
    // 初始化 GLFW
    if (!glfwInit()) 
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Ray Tracing", nullptr, nullptr);
    if (!window) 
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); 
    glfwSetCursorPosCallback(window, mouse_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) 
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    // 设置视口
    glViewport(0, 0, width, height);

    // 加载 Shader
    Shader shader("VertexShader.glsl", "FragmentShader.glsl");

    // 加载模型
    Model model;
    if (!model.LoadModel("Bunny_Low.obj")) 
    {
        cerr << "Failed to load model" << endl;
        return -1;
    }
    //for (int i = 0; i < model.triangles.size(); i++)
    //    cout << model.triangles[i].v0.x << " " << model.triangles[i].v0.y << " " << model.triangles[i].v0.z << " "<<
    //    model.triangles[i].v1.x << " " << model.triangles[i].v1.y << " " << model.triangles[i].v1.z << " " <<
    //    model.triangles[i].v2.x << " " << model.triangles[i].v2.y << " " << model.triangles[i].v2.z << " " <<
    //    model.triangles[i].n.x << " " << model.triangles[i].n.y << " " << model.triangles[i].n.z << endl;
    
    // 设置 VAO/VBO/EBO
    uint VAO, VBO, EBO, UBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(screenIndices), screenIndices, GL_STATIC_DRAW);
    glGenBuffers(1, &UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Triangle) * 1000, NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Triangle) * model.triangles.size(), &model.triangles[0]);
    uint blockIndex = glGetUniformBlockIndex(shader.ID, "TriangleBlock");
    glUniformBlockBinding(shader.ID, blockIndex, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, UBO);
    // 设置顶点属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window)) 
    {
        frameCnt++; currentTime = glfwGetTime();
        if (currentTime - lastTime >= 1.0) 
        { 
            double fps = frameCnt / (currentTime - lastTime);
            std::stringstream ss;
            ss << "Ray Tracing - FPS: " << fps;
            glfwSetWindowTitle(window, ss.str().c_str());
            frameCnt = 0; lastTime = currentTime;
        }
        // 输入处理
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.position += vec3(0.05) * normalize(camera.forward);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.position -= vec3(0.05) * normalize(camera.right);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.position -= vec3(0.05) * normalize(camera.forward);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.position += vec3(0.05) * normalize(camera.right);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.position += vec3(0.05) * normalize(camera.up);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.position -= vec3(0.05) * normalize(camera.up);
        // 清屏
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 使用 Shader
        shader.use();

        // 传递 Uniform 数据
        shader.SetUniformVec3("camera.position", camera.position);
        shader.SetUniformVec3("camera.forward", camera.forward);
        shader.SetUniformVec3("camera.right", camera.right);
        shader.SetUniformVec3("camera.up", camera.up);
        shader.SetUniform1i("triangleCount", model.triangles.size());

        // 绘制屏幕平面
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        /*
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(camera.position.x, camera.position.y, camera.position.z, 0.0, 0.0, 0.0,
            camera.worldUp.x, camera.worldUp.y, camera.worldUp.z);
        model.DrawWireframe();
        */

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 释放资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
}