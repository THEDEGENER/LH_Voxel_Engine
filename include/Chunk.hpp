#pragma once

#include <array>
#include <vector>
#include "shader_m.h"
#include "VoxelTypes.hpp"
#include "Mesh.h"   
#include "Noise.h"



class Chunk
{
    public:
    Chunk(int chunkX, int chunkZ);

    int chunkX;
    int chunkZ;

    static constexpr int WIDTH  = 16;
    static constexpr int HEIGHT = 256;
    static constexpr int DEPTH  = 16;

    static constexpr int MAX_SURFACE = 64;

    std::atomic<bool> dirty;
    std::atomic<bool> scheduled;

    void generate();
    void buildMesh();
    void draw(Shader& shader, GLuint& atlasText);
    void setData();

    bool IsAabbVisible(const std::vector<glm::vec4>& frustumPlanes);
    std::vector<glm::vec3> GetAABBVertices(const AABB& box);

    private:
    inline int index(int x, int y, int z) const;
    void addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int x, int y, int z, int dir, glm::vec2& atlasOffset);
    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);

    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    const siv::PerlinNoise::seed_type seed = 123456u;

    AABB box;          
    std::array<BlockType, WIDTH*HEIGHT*DEPTH> blocks;
    Mesh mesh;
    Noise noise{ seed };
};