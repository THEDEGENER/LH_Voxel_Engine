#pragma once

#include <array>
#include <vector>
#include "VoxelTypes.hpp"
#include "World.hpp"
#include "WorldConfig.hpp"

class World;

class GreedyMesher {
    public:
    static constexpr int HEIGHT =   256;
    static constexpr int WIDTH =    16;
    static constexpr int DEPTH =    16;

    float atlasWidth    = CONFIG.atlasWidth;    // actual pixel width of atlas
    float atlasHeight   = CONFIG.atlasHeight; // actual pixel height of atlas
    float tileSizePx    = CONFIG.tileSizePx;
    glm::vec2 uvScale = glm::vec2(tileSizePx/atlasWidth, tileSizePx/atlasHeight);

    void GreedyMesh(const std::array<BlockType, WIDTH * HEIGHT * DEPTH>& chunkBlockMap, std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int chunkX, int chunkZ, World& world);
    BlockType getChunkBlock(int x, int y, int z, const std::array<BlockType, WIDTH * HEIGHT * DEPTH>& chunkBlockMap, int chunkX, int chunkZ, World& world, int dir);

    inline BlockType& maskAt(int u, int v, int rowWidth);
    inline const BlockType& maskAt(int u, int v, int rowWidth) const;

    private:
    inline int index(int x, int y, int z) const;
    void addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int x, int y, int z, int dir, const glm::vec2& atlasOffset);
    
    std::vector<BlockType> mask;
};