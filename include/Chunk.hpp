#pragma once

#include <array>
#include <vector>
#include "VoxelTypes.hpp"   
#include "Noise.h"



class Chunk
{
    public:
    Chunk(int chunkX, int chunkZ) : chunkX(chunkX), chunkZ(chunkZ), dirty(true), scheduled(false),
        box{
            glm::vec3(chunkX * float(WIDTH), 0.0f, chunkZ * float(DEPTH)), 
            glm::vec3(chunkX * float(WIDTH) + float(WIDTH), 
            float(HEIGHT), chunkZ * float(DEPTH) + float(DEPTH))
        };

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

    private:
    void setData();
    void draw(Shader& shader, GLuint& atlasText);
    std::vector<glm::vec3> GetAABBVertices(const AABB& box);
    inline int index(int x, int y, int z) const;
    bool IsAabbVisible(const std::vector<glm::vec4>& frustumPlanes);
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
}