#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 TexCoords;
void main() {
    TexCoords = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform float iTime;
uniform vec2 iResolution;
uniform vec3 camPos;
uniform vec3 camDir;
uniform vec3 camUp;
uniform float fov;
vec3 blackHolePos = vec3(0,0,0);
float horizonRadius = 1.0;
float diskInner = 1.5;
float diskOuter = 4.0;
float G = 1.0;
float c = 1.0;
float accretionDisk(vec3 pos) {
    float angle = iTime * 0.5;
    vec2 rotated = vec2(pos.x * cos(angle) - pos.z * sin(angle), pos.x * sin(angle) + pos.z * cos(angle));
    float r = length(rotated);
    float h = abs(pos.y);
    if (r > diskInner && r < diskOuter && h < 0.1) {
        return 1.0 - (r - diskInner) / (diskOuter - diskInner);
    }
    return 0.0;
}
vec3 rayDirection(vec2 uv, vec3 eye, vec3 center, vec3 up, float fov) {
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);
    return normalize(f * fov + s * uv.x + u * uv.y);
}

void main() {
    vec2 uv = (TexCoords - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    vec3 rayDir = rayDirection(uv, camPos, camPos + camDir, camUp, fov);
    vec3 pos = camPos;
    float t = 0.0;
    float dt = 0.05;
    vec3 color = vec3(0);
    bool hitHorizon = false;
    bool hitDisk = false;
    for(int i = 0; i < 1000; i++) {
        pos += rayDir * dt;
        float r = length(pos - blackHolePos);
        if (r < horizonRadius) {
            hitHorizon = true;
            break;
        }
        float disk = accretionDisk(pos);
        if (disk > 0.0) {
            color = vec3(1, 0.5, 0) * disk;
            hitDisk = true;
            break;
        }
        // gravitational lensing
        vec3 toBH = blackHolePos - pos;
        float dist = length(toBH);
        if (dist > 0.1) {
            vec3 force = toBH / (dist * dist * dist) * G / (c * c);
            rayDir += force * dt;
            rayDir = normalize(rayDir);
        }
        t += dt;
        if (t > 100.0) break;
    }
    if (hitDisk) {
        FragColor = vec4(color, 1);
    } else if (hitHorizon) {
        FragColor = vec4(0,0,0,1);
    } else {
        // black sky with stars, lensed
        vec3 bg = vec3(0.0);
        // use final rayDir for lensing effect on stars
            vec2 st = vec2(atan(rayDir.x, rayDir.z), asin(rayDir.y)) * 50.0;
        vec2 ist = floor(st);
        vec2 fst = fract(st);
        float minDist = 1.0;
        for(int y = -1; y <= 1; y++) {
            for(int x = -1; x <= 1; x++) {
                vec2 neighbor = vec2(float(x), float(y));
                vec2 point = ist + neighbor + vec2(0.5);
                vec2 diff = neighbor + vec2(0.5) - fst;
                minDist = min(minDist, length(diff));
            }
        }
            float star = 1.0 - smoothstep(0.0, 0.02, minDist);
        // vary color slightly
        vec3 starColor = vec3(0.9, 0.95, 1.0) + 0.1 * sin(ist.x * 10.0 + ist.y * 5.0);
        bg += star * starColor;
        FragColor = vec4(bg, 1);
    }
}
)";

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Black Hole", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(shaderProgram);
    int iTimeLoc = glGetUniformLocation(shaderProgram, "iTime");
    int iResolutionLoc = glGetUniformLocation(shaderProgram, "iResolution");
    int camPosLoc = glGetUniformLocation(shaderProgram, "camPos");
    int camDirLoc = glGetUniformLocation(shaderProgram, "camDir");
    int camUpLoc = glGetUniformLocation(shaderProgram, "camUp");
    int fovLoc = glGetUniformLocation(shaderProgram, "fov");
    float camPos[3] = {0,1,5};
    float camDir[3] = {0,0,-1};
    float blackHolePos[3] = {0,0,0};
    float yaw = 3.14159f; // pi, to look at -z
    float pitch = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        // camera controls
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            camPos[0] += camDir[0] * 0.1f;
            camPos[1] += camDir[1] * 0.1f;
            camPos[2] += camDir[2] * 0.1f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            camPos[0] -= camDir[0] * 0.1f;
            camPos[1] -= camDir[1] * 0.1f;
            camPos[2] -= camDir[2] * 0.1f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            float right[3] = {camDir[1]*0 - camDir[2]*1, camDir[2]*0 - camDir[0]*0, camDir[0]*1 - camDir[1]*0}; // cross with (0,1,0)
            float len = sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
            right[0] /= len; right[1] /= len; right[2] /= len;
            camPos[0] -= right[0] * 0.1f;
            camPos[1] -= right[1] * 0.1f;
            camPos[2] -= right[2] * 0.1f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            float right[3] = {camDir[1]*0 - camDir[2]*1, camDir[2]*0 - camDir[0]*0, camDir[0]*1 - camDir[1]*0};
            float len = sqrt(right[0]*right[0] + right[1]*right[1] + right[2]*right[2]);
            right[0] /= len; right[1] /= len; right[2] /= len;
            camPos[0] += right[0] * 0.1f;
            camPos[1] += right[1] * 0.1f;
            camPos[2] += right[2] * 0.1f;
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) yaw -= 0.02f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) yaw += 0.02f;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) pitch += 0.02f;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) pitch -= 0.02f;
        if (pitch > 1.5f) pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;
        camDir[0] = sin(yaw) * cos(pitch);
        camDir[1] = sin(pitch);
        camDir[2] = cos(yaw) * cos(pitch);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
        glClear(GL_COLOR_BUFFER_BIT);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glUniform1f(iTimeLoc, glfwGetTime());
        glUniform2f(iResolutionLoc, width, height);
        glUniform3fv(camPosLoc, 1, camPos);
        glUniform3fv(camDirLoc, 1, camDir);
        glUniform3f(camUpLoc, 0, 1, 0);
        glUniform1f(fovLoc, 1.0);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}