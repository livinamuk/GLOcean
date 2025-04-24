#pragma once
#include <string>
#include <vector>

#include <stb_image.h>

struct CubemapTexutre {

    GLuint ID;

    void Create(std::vector<std::string>& textures) {
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
        int width, height, nrChannels;
        for (unsigned int i = 0; i < textures.size(); i++) {
            unsigned char* data = stbi_load(textures[i].c_str(), &width, &height, &nrChannels, 0);
            if (data) {
                GLint format = GL_RGB;
                if (nrChannels == 4)
                    format = GL_RGBA;
                if (nrChannels == 1)
                    format = GL_RED;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else {
                std::cout << "Failed to load cubemap\n";
                stbi_image_free(data);
            }
        }
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
};

struct Skybox {

    GLuint vao = 0;
    CubemapTexutre cubemap;

    void Init() {

        // Load cubemap textures
        std::vector<std::string> skyboxTextureFilePaths;
        skyboxTextureFilePaths.push_back("res/textures/right.png");
        skyboxTextureFilePaths.push_back("res/textures/left.png");
        skyboxTextureFilePaths.push_back("res/textures/top.png");
        skyboxTextureFilePaths.push_back("res/textures/bottom.png");
        skyboxTextureFilePaths.push_back("res/textures/front.png");
        skyboxTextureFilePaths.push_back("res/textures/back.png");
        cubemap.Create(skyboxTextureFilePaths);

        // Init cube vertices (for skybox)	
        std::vector<glm::vec3> cubeVertices;
        std::vector<unsigned int> cubeIndices;
        float d = 0.5f;
        cubeVertices.push_back(glm::vec3(-d, d, d));	// Top
        cubeVertices.push_back(glm::vec3(-d, d, -d));
        cubeVertices.push_back(glm::vec3(d, d, -d));
        cubeVertices.push_back(glm::vec3(d, d, d));
        cubeVertices.push_back(glm::vec3(-d, -d, d));	// Bottom
        cubeVertices.push_back(glm::vec3(-d, -d, -d));
        cubeVertices.push_back(glm::vec3(d, -d, -d));
        cubeVertices.push_back(glm::vec3(d, -d, d));
        cubeVertices.push_back(glm::vec3(-d, d, d));	// Z front
        cubeVertices.push_back(glm::vec3(-d, -d, d));
        cubeVertices.push_back(glm::vec3(d, -d, d));
        cubeVertices.push_back(glm::vec3(d, d, d));
        cubeVertices.push_back(glm::vec3(-d, d, -d));	// Z back
        cubeVertices.push_back(glm::vec3(-d, -d, -d));
        cubeVertices.push_back(glm::vec3(d, -d, -d));
        cubeVertices.push_back(glm::vec3(d, d, -d));
        cubeVertices.push_back(glm::vec3(d, d, -d));	// X front
        cubeVertices.push_back(glm::vec3(d, -d, -d));
        cubeVertices.push_back(glm::vec3(d, -d, d));
        cubeVertices.push_back(glm::vec3(d, d, d));
        cubeVertices.push_back(glm::vec3(-d, d, -d));	// X back
        cubeVertices.push_back(glm::vec3(-d, -d, -d));
        cubeVertices.push_back(glm::vec3(-d, -d, d));
        cubeVertices.push_back(glm::vec3(-d, d, d));
        cubeIndices = { 0, 1, 3, 1, 2, 3, 7, 5, 4, 7, 6, 5, 11, 9, 8, 11, 10, 9, 12, 13, 15, 13, 14, 15, 16, 17, 19, 17, 18, 19, 23, 21, 20, 23, 22, 21 };
        unsigned int cubeVbo;
        unsigned int cubeEbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &cubeVbo);
        glGenBuffers(1, &cubeEbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
        glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(glm::vec3), &cubeVertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(unsigned int), &cubeIndices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void Draw() {

    }

};