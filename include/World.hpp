#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>
#include "VoxelTypes.hpp" // for BlockType, etc.
#include "SafeQueue.hpp"  // for SafeQueue<ChunkJob>
#include "shader_m.h"     // for Shader
#include "Chunk.hpp"
#include "FastNoiseLite.h"
#include "camera.h"

class Chunk;

struct PairHash;

class World
{
public:
    World();
    ~World();

    void manageChunks(const glm::vec3 &newPos, Shader &shader, const std::vector<glm::vec4> &frustumPlanes);
    BlockType getChunk(int nChunkX, int nChunkZ, int tx, int ty, int tz);

private:
    // Internal pipeline stages:
    void updatePlayerPos(const glm::vec3 &newPos, const std::vector<glm::vec4> &newFrustumPlanes);
    void updateVisibleChunks();
    void uploadFinishedChunksToGPU();
    void drawVisibleChunks(Shader &shader);

    // Worker threads:
    void startWorldThreads();
    void init_noise();
    void workerThreadPool();

    // Utility:

    // Data:
    glm::vec3 playerPos, oldPos;
    bool frustumDirty = false;
    std::vector<glm::vec4> frustumPlanes;

    std::unordered_map<std::pair<int, int>, std::unique_ptr<Chunk>, PairHash> chunks;
    std::vector<Chunk *> visibleChunks;

    SafeQueue<ChunkJob> generateQueue;
    SafeQueue<Chunk *> uploadQueue;
    std::vector<std::thread> threads;

    GLuint atlasText;

    FastNoiseLite noise;
};