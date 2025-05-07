#pragma once

#include <libs/glad/glad.h>
#include <vector>
#include "shader_m.h"
#include "VoxelTypes.hpp"

class Mesh
{
    public:
    Mesh();
    ~Mesh();

    void setData(std::vector<Vertex>& verts, std::vector<uint32_t>& idx);
    void draw(Shader& shader, GLuint& atlasText);

    private:
    void setupMesh();
    // GPU handles
    GLuint VAO = 0, VBO = 0, EBO = 0;
    GLsizei indexCount = 0;

    // CPU-side cache (optional)
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
};