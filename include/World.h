#pragma once

#include <unordered_map>
#include <memory>
#include <utility>
#include "Chunk.h"
#include "include/shader_m.h"

struct PairHash {
    size_t operator()(const std::pair<int,int>& p) const noexcept {
      uint64_t key = (uint64_t(uint32_t(p.first)) << 32)
                   |  uint32_t(p.second);
      return std::hash<uint64_t>()(key);
    }
  };

class World {
    public:
    int cx;
    int cz;

    static constexpr int startingChunkSize = 6;

    glm::vec3 playerPos, oldPos;
    std::vector<glm::vec4> frustumPlanes;

    void createChunks()
    {
        for (int z = 0; z < startingChunkSize; z++)
        {
          for (int x = 0; x < startingChunkSize; x++)
          {
            auto key = std::make_pair(x, z);
            auto newChunk = std::make_unique<Chunk>(x, z);
            newChunk->generate();
            chunks.emplace(key, std::move(newChunk));
          }
        }
    }

    void updateVisibleChunks()
    {
      visibleChunks.clear();

      // figure out which chunk the player is in
      int playerChunkX = int(floor(playerPos.x / Chunk::WIDTH));
      int playerChunkZ = int(floor(playerPos.z / Chunk::DEPTH));
    
      // loop a 3×3 (or NxN) area around that chunk
      for (int dz = -3; dz <= 3; ++dz) {
        for (int dx = -3; dx <= 3; ++dx) {
          int cx = playerChunkX + dx;
          int cz = playerChunkZ + dz;
          auto key = std::make_pair(cx, cz);
    
          // if we haven’t generated that chunk yet, do so and store it
          auto it = chunks.find(key);
          if (it == chunks.end()) {
            auto newChunk = std::make_unique<Chunk>(cx, cz);
            newChunk->generate();
            it = chunks.emplace(key, std::move(newChunk)).first;
          }
          
          Chunk* chunkPtr = it->second.get();
          if (chunkPtr->IsAabbVisible(frustumPlanes))
          {
            // grab the raw pointer and add it to our render list
            visibleChunks.push_back(chunkPtr);
          }
        }
      }
    }

    void drawVisibleChunks(Shader& shader)
    {
        for (auto& chunk : visibleChunks) {
            chunk->draw(shader);
          }
    }

    void updatePlayerPos(const glm::vec3& newPos)
    {
      oldPos = playerPos;
      this->playerPos = newPos;

      int oldChunkX = int(floor(oldPos.x / Chunk::WIDTH));
      int oldChunkZ = int(floor(oldPos.z / Chunk::DEPTH));
      int newChunkX = int(floor(playerPos.x / Chunk::WIDTH));
      int newChunkZ = int(floor(playerPos.z / Chunk::DEPTH));

      // here if a boundry is crossed we want to push the job onto a queue
      if (oldChunkX != newChunkX || oldChunkZ != newChunkZ) {
        updateVisibleChunks();
      }
    }

    void updateFrustumPlanes(const std::vector<glm::vec4>& frustumPlanes)
    {
      this->frustumPlanes = frustumPlanes;
    }

    void manageChunks(const glm::vec3& newPos, Shader& shader, const std::vector<glm::vec4>& frustumPlanes)
    {
      updateFrustumPlanes(frustumPlanes);
      updatePlayerPos(newPos);
      updateVisibleChunks();
      drawVisibleChunks(shader);
    }

    private:

    bool running;
    std::unordered_map<std::pair<int,int>,
                     std::unique_ptr<Chunk>,
                     PairHash> chunks;

    // stores the chunk coordinate keys that are currently visible
    std::vector<Chunk*> visibleChunks;
};