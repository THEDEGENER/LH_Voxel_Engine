#include <unordered_map>
#include <memory>
#include <thread>
#include <utility>
#include "World.hpp"
#include "SafeQueue.hpp"
#include "shader_m.h"
#include "VoxelTypes.hpp"

class World {
    public:

    glm::vec3 playerPos, oldPos;
    std::vector<glm::vec4> frustumPlanes, oldFrustum;

    World() 
    {
      atlasText = Loader::loadTexture("assets/textures/block_atlas.png");
      startWorldThreads();
    }

    ~World()
    {
      generateQueue.close();
      uploadQueue.close();
      for(auto& t : threads)
      {
        t.join();
      }
    }

    void updateVisibleChunks()
    {
      visibleChunks.clear();

      // figure out which chunk the player is in
      int playerChunkX = int(floor(playerPos.x / Chunk::WIDTH));
      int playerChunkZ = int(floor(playerPos.z / Chunk::DEPTH));
    
      // loop a 3×3 (or NxN) area around that chunk
      static constexpr int R = 3;
      for (int dz = -R; dz <= R; ++dz) {
        for (int dx = -R; dx <= R; ++dx) {
          int cx = playerChunkX + dx;
          int cz = playerChunkZ + dz;
          auto key = std::make_pair(cx, cz);
    
          // if we haven’t generated that chunk yet, do so and store it
          auto it = chunks.find(key);
          if (it == chunks.end()) {
            auto newChunk = std::make_unique<Chunk>(cx, cz);
            Chunk* rawChunkPtr = newChunk.get();
            it = chunks.emplace(key, std::move(newChunk)).first;
            // upload new chunk to the queue for a thread to take
            generateQueue.push(ChunkJob{ rawChunkPtr, JobType::GenerateAndBuild });
            rawChunkPtr->scheduled = true;
          } else {
            Chunk* exisitngChunk = it->second.get();
            if (exisitngChunk->dirty.load(std::memory_order_relaxed) && !exisitngChunk->scheduled.load(std::memory_order_relaxed))
            {
              generateQueue.push(ChunkJob{ exisitngChunk, JobType::BuildOnly });
              exisitngChunk->scheduled = true;
            }
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

    void uploadFinishedChunksToGPU()
    {
      Chunk* finishedChunk = nullptr;
      while(uploadQueue.tryPop(finishedChunk))
      {
        finishedChunk->setData();
      }
    }

    void drawVisibleChunks(Shader& shader)
    {
      std::cout << "numChunks " << visibleChunks.size() << std::endl;
      for (auto& chunk : visibleChunks) 
      {
        chunk->draw(shader, atlasText);
      }
    }

    void updatePlayerPos(const glm::vec3& newPos, const std::vector<glm::vec4>& newFrustumPlanes)
    {
      oldPos = playerPos;
      this->playerPos = newPos;

      if (newFrustumPlanes != frustumPlanes)
      {
        frustumPlanes = newFrustumPlanes;
        frustumDirty = true;
      }

      int oldChunkX = int(floor(oldPos.x / Chunk::WIDTH));
      int oldChunkZ = int(floor(oldPos.z / Chunk::DEPTH));
      int newChunkX = int(floor(playerPos.x / Chunk::WIDTH));
      int newChunkZ = int(floor(playerPos.z / Chunk::DEPTH));

      bool moved = (oldChunkX != newChunkX || oldChunkZ != newChunkZ);
      if(moved || frustumDirty)
      {
        updateVisibleChunks();
        frustumDirty = false;
      }

    }

    void workerThreadPool()
    {
      while(true)
      {
        ChunkJob job = generateQueue.pop();
        if (job.chunk == nullptr)
        {
          break;
        }

        Chunk* c = job.chunk;

        c->dirty = false;

        if (job.type == JobType::GenerateAndBuild)
        {
          std::cout << "thread: " << std::this_thread::get_id() << " is starting a gen task" << std::endl;
          c->generate();
        }
        std::cout << "thread: " << std::this_thread::get_id() << " is starting a build task" << std::endl;
        c->buildMesh();
        c->scheduled = false;
        uploadQueue.push(c);
      }
    }

    void startWorldThreads()
    {
      auto hc = 1;
      unsigned int threadCount = hc > 0 ? hc : 1;
      std::cout << "threadCount: " << threadCount << std::endl;
      for (unsigned int i = 0; i < threadCount; i++)
      {
        // this could also be a lambda function that calls this->workerThreadPool
        // which seems more readable but this is how i read about it
        threads.emplace_back(&World::workerThreadPool, this);
      }
    }

    void manageChunks(const glm::vec3& newPos, Shader& shader, const std::vector<glm::vec4>& frustumPlanes)
    {
      updatePlayerPos(newPos, frustumPlanes);
      uploadFinishedChunksToGPU();
      drawVisibleChunks(shader);
    }

    private:

    // atomic is a type that multi threads can operate on but only one at 
    // a time, was introduced to better synchronise threads with its addition in 
    // the standard library
    bool frustumDirty = false;
    std::unordered_map<std::pair<int,int>,
                     std::unique_ptr<Chunk>,
                     PairHash> chunks;

    // stores the chunk coordinate keys that are currently visible
    std::vector<Chunk*> visibleChunks;
    SafeQueue<ChunkJob> generateQueue;
    SafeQueue<Chunk*> uploadQueue;
    std::vector<std::thread> threads;

    GLuint atlasText;
};