#pragma once

#include <array>
#include <vector>
#include "VoxelTypes.hpp"
#include "World.hpp"
#include "WorldConfig.hpp"

class GreedyMesher {
    public:
    static constexpr int HEIGHT =   WorldSettings::CHUNK_HEIGHT;
    static constexpr int WIDTH =    WorldSettings::CHUNK_WIDTH;
    static constexpr int DEPTH =    WorldSettings::CHUNK_DEPTH;

    float atlasWidth    = CONFIG.atlasWidth;    // actual pixel width of atlas
    float atlasHeight   = CONFIG.atlasHeight; // actual pixel height of atlas
    float tileSizePx    = CONFIG.tileSize;
    glm::vec2 uvScale = glm::vec2(tileSizePx/atlasWidth, tileSizePx/atlasHeight);

    void GreedyMesh(const std::array<BlockType, (WIDTH + 2)*(HEIGHT + 2)*(DEPTH + 2)>& chunkBlockMap, std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int chunkX, int chunkZ, World& world);
    BlockType getChunkBlock(int x, int y, int z, const std::array<BlockType, (WIDTH + 2)*(HEIGHT + 2)*(DEPTH + 2)>& chunkBlockMap, int chunkX, int chunkZ, World& world, int dir);

    inline BlockType& maskAt(int u, int v, int rowWidth);
    inline const BlockType& maskAt(int u, int v, int rowWidth) const;

    private:
    inline int index(int x, int y, int z) const;
    void addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int x, int y, int z, int dir, const glm::vec2& atlasOffset);

    std::vector<BlockType> mask;
};