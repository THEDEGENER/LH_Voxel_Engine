
#include <libs/glad/glad.h>
#include <libs/glm/glm.hpp>
#include <libs/glm/gtc/matrix_transform.hpp>
#include "scripts/Loader.h"
#include "VoxelTypes.hpp"
#include <vector>
#include <iostream>
#include "shader_m.h"
#include "Mesh.hpp"

Mesh::Mesh()
{
    setupMesh();
}
Mesh::~Mesh()
{
    if (VAO)
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
}
// Upload new vertex/index data to the GPU
void Mesh::setData(std::vector<Vertex> &verts,
                   std::vector<uint32_t> &idx)
{
    vertices.swap(verts);
    indices.swap(idx);
    indexCount = (GLsizei)indices.size();
    // Bind and update buffers
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 vertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(uint32_t),
                 indices.data(),
                 GL_STATIC_DRAW);
}
void Mesh::draw(Shader &shader, GLuint &atlasText)
{
    glBindTexture(GL_TEXTURE_2D, atlasText);
    glActiveTexture(GL_TEXTURE0);
    shader.use();
    // shader.setInt("atlasTex", 0);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
// GPU handles
GLuint VAO = 0, VBO = 0, EBO = 0;
GLsizei indexCount = 0;
// CPU-side cache (optional)
std::vector<Vertex> vertices;
std::vector<uint32_t> indices;
void Mesh::setupMesh()
{
    /* removed the binding for buffer data because i was just binding 0 data
     * and then setting a nullptr to the data which was unessecary when potentially
     * the chunk may not be drawn for sometime.
     */
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    // Reserve empty buffers
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Position));
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));
    // TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords));
    glBindVertexArray(0);
}
