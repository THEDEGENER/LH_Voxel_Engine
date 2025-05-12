#pragma once

#include <array>
#include <vector>
#include "shader_m.h"
#include "VoxelTypes.hpp"
#include "Mesh.hpp"   
#include "WorldConfig.hpp"
#include "FastNoiseLite.h"


class World;

class Chunk
{
    public:
    Chunk(int chunkX, int chunkZ, World& world, FastNoiseLite& noiseptr);

    int chunkX;
    int chunkZ;

    std::atomic<bool> dirty;
    std::atomic<bool> scheduled;
    std::atomic<bool> hasBeenGenerated;

    std::array<BlockType, WorldSettings::CHUNK_WIDTH*WorldSettings::CHUNK_HEIGHT*WorldSettings::CHUNK_DEPTH> blocks;

    void generate();
    void buildMesh();
    void draw(Shader& shader, GLuint& atlasText);
    void setData();
    BlockType getBlock(int x, int y, int z) const;

    bool IsAabbVisible(const std::vector<glm::vec4>& frustumPlanes);
    std::vector<glm::vec3> GetAABBVertices(const AABB& box);

    private:
    inline int index(int x, int y, int z) const;
    void addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int x, int y, int z, int dir, BlockType type);
    void addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, std::array<glm::vec3, 4>& corners, int faceDir, BlockType type);
    void greedy();
    void setBlock(int x, int y, int z, BlockType type);

    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    AABB box;          
    Mesh mesh;
    World& world;
    FastNoiseLite& noise;
};