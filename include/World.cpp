#include <unordered_map>
#include <memory>
#include <thread>
#include <utility>
#include "World.hpp"
#include "SafeQueue.hpp"
#include "shader_m.h"
#include "VoxelTypes.hpp"
#include "scripts/Loader.h"
#include "WorldConfig.hpp"
#include "libs/glad/glad.h"
#include "libs/glfw/glfw3.h"
#include <libs/glm/glm.hpp>
#include <libs/glm/gtc/matrix_transform.hpp>
#include <libs/glm/gtc/type_ptr.hpp>

World::World()
{
  atlasText = Loader::loadTexture("assets/textures/block_atlas.png");
  startWorldThreads();
  init_noise();
}

World::~World()
{
  generateQueue.close();
  uploadQueue.close();
  for (auto &t : threads)
  {
    t.join();
  }
}

BlockType World::getChunk(int nChunkX, int nChunkZ, int tx, int ty, int tz)
{
  int nx = (tx % WorldSettings::CHUNK_WIDTH + WorldSettings::CHUNK_WIDTH) % WorldSettings::CHUNK_WIDTH;
  int nz = (tz % WorldSettings::CHUNK_WIDTH + WorldSettings::CHUNK_WIDTH) % WorldSettings::CHUNK_WIDTH;

  auto key = std::make_pair(nChunkX, nChunkZ);

  auto it = chunks.find(key);
  if (it == chunks.end() || it->second->hasBeenGenerated == false)
  {
    return BlockType::NoBlock;
  }
  return it->second->getBlock(nx, ty, nz);
}

void World::updateVisibleChunks()
{
  visibleChunks.clear();
  // figure out which chunk the player is in
  int playerChunkX = int(floor(playerPos.x / WorldSettings::CHUNK_WIDTH));
  int playerChunkZ = int(floor(playerPos.z / WorldSettings::CHUNK_DEPTH));

  // loop a 3×3 (or NxN) area around that chunk
  static constexpr int R = 10;
  for (int dz = -R; dz <= R; ++dz)
  {
    for (int dx = -R; dx <= R; ++dx)
    {
      int cx = playerChunkX + dx;
      int cz = playerChunkZ + dz;
      auto key = std::make_pair(cx, cz);

      // if we haven’t generated that chunk yet, do so and store it
      auto it = chunks.find(key);
      if (it == chunks.end())
      {
        auto newChunk = std::make_unique<Chunk>(cx, cz, *this, noise);
        Chunk *rawChunkPtr = newChunk.get();
        rawChunkPtr->hasBeenGenerated = true;
        it = chunks.emplace(key, std::move(newChunk)).first;
        // upload new chunk to the queue for a thread to take
        generateQueue.push(ChunkJob{rawChunkPtr, JobType::GenerateAndBuild});
        rawChunkPtr->scheduled = true;
      }
      else
      {
        Chunk *exisitngChunk = it->second.get();
        if (exisitngChunk->dirty.load(std::memory_order_relaxed) && !exisitngChunk->scheduled.load(std::memory_order_relaxed))
        {
          generateQueue.push(ChunkJob{exisitngChunk, JobType::BuildOnly});
          exisitngChunk->scheduled = true;
        }
      }

      Chunk *chunkPtr = it->second.get();
      if (chunkPtr->IsAabbVisible(frustumPlanes))
      // if (true)
      {
        // grab the raw pointer and add it to our render list
        visibleChunks.push_back(chunkPtr);
      }
    }
  }
}

void World::uploadFinishedChunksToGPU()
{
  Chunk *finishedChunk = nullptr;
  while (uploadQueue.tryPop(finishedChunk))
  {
    finishedChunk->setData();
  }
}

void World::drawVisibleChunks(Shader &shader)
{
  std::cout << visibleChunks.size() << std::endl;
  for (auto &chunk : visibleChunks)
  {
    chunk->draw(shader, atlasText);
  }
}

void World::updatePlayerPos(const glm::vec3 &newPos, const std::vector<glm::vec4> &newFrustumPlanes)
{
  oldPos = playerPos;
  this->playerPos = newPos;
  if (newFrustumPlanes != frustumPlanes)
  {
    frustumPlanes = newFrustumPlanes;
    frustumDirty = true;
  }
  int oldChunkX = int(floor(oldPos.x / WorldSettings::CHUNK_WIDTH));
  int oldChunkZ = int(floor(oldPos.z / WorldSettings::CHUNK_DEPTH));
  int newChunkX = int(floor(playerPos.x / WorldSettings::CHUNK_WIDTH));
  int newChunkZ = int(floor(playerPos.z / WorldSettings::CHUNK_DEPTH));
  bool moved = (oldChunkX != newChunkX || oldChunkZ != newChunkZ);
  if (moved || frustumDirty)
  {
    updateVisibleChunks();
    frustumDirty = false;
  }
}

void World::workerThreadPool()
{
  while (true)
  {
    ChunkJob job = generateQueue.pop();
    if (job.chunk == nullptr)
    {
      break;
    }
    Chunk *c = job.chunk;
    c->dirty = false;
    if (job.type == JobType::GenerateAndBuild)
    {
      c->generate();
    }
    c->buildMesh();
    c->scheduled = false;
    uploadQueue.push(c);
  }
}

void World::startWorldThreads()
{
  auto hc = 1;
  unsigned int threadCount = hc > 0 ? hc : 1;
  for (unsigned int i = 0; i < threadCount; i++)
  {
    // this could also be a lambda function that calls this->workerThreadPool
    // which seems more readable but this is how i read about it
    threads.emplace_back(&World::workerThreadPool, this);
  }
}

void World::manageChunks(const glm::vec3 &newPos, Shader &shader, const std::vector<glm::vec4> &frustumPlanes)
{
  updatePlayerPos(newPos, frustumPlanes);
  uploadFinishedChunksToGPU();
  drawVisibleChunks(shader);
}

void World::init_noise()
{
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
  noise.SetSeed(WorldSettings::seed);
  noise.SetFractalType(FastNoiseLite::FractalType_FBm);
  noise.SetFrequency(0.01);
  noise.SetFractalOctaves(9);
  noise.SetFractalLacunarity(1.5);
  noise.SetFractalGain(0.02);
  noise.SetFrequency(0.007);
  noise.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
  noise.SetDomainWarpAmp(50.0);
}
