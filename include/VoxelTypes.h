// VoxelTypes.h

#pragma once

#include <libs/glad/glad.h>
#include <libs/glm/glm.hpp>
#include <libs/glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <string>
#include <vector>
using namespace std;

// type is explicitly set the 8bit int taking less space for 1000s of blocks
enum class BlockType : uint8_t { Air, Dirt, Grass, Stone };

struct Block {
    BlockType b_type;

};

struct BlockFaceOffsets {
    glm::ivec2 top;
    glm::ivec2 sides;
    glm::ivec2 bottom;
};

inline const std::unordered_map<BlockType, BlockFaceOffsets> blockTextureOffsets = {
    { BlockType::Dirt,  { {24, 29}, {24, 29}, {24, 29} } }, // all faces use tile at (1, 2)
    { BlockType::Grass, { {29, 22}, {28, 24}, {24, 29} } }, // top (0,0), sides (2,0), bottom (1,2)
    { BlockType::Stone, { {30, 2}, {30, 2}, {30, 2} } }, // all faces use tile at (3,1)
};

struct Plane {
    glm::vec3 normal = { 0.f, 1.f, 0.f };

    float distance = 0.f;
};

struct Frustum {
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

// six directions in the same order your "Direction dir" enum uses
inline const glm::vec3 dirOffsets[6] = {
    { 0,  0,  1},   // +Z (front)
    { 0,  0, -1},   // -Z (back)
    { 0,  1,  0},   // +Y (top)
    { 0, -1,  0},   // -Y (bottom)
    { 1,  0,  0},   // +X (right)
    {-1,  0,  0}    // -X (left)
  };

  // for each face, the 4 verts (in CCW order to give the right winding)
inline const glm::vec3 vertexOffsets[6][4] = {
    // FRONT
    { {0,0,1}, {1,0,1}, {1,1,1}, {0,1,1} },
    // BACK
    { {1,0,0}, {0,0,0}, {0,1,0}, {1,1,0} },
    // TOP
    { {0,1,1}, {1,1,1}, {1,1,0}, {0,1,0} },
    // BOTTOM
    { {0,0,0}, {1,0,0}, {1,0,1}, {0,0,1} },
    // RIGHT
    { {1,0,1}, {1,0,0}, {1,1,0}, {1,1,1} },
    // LEFT
    { {0,0,0}, {0,0,1}, {0,1,1}, {0,1,0} }
};

inline const glm::vec2 faceUVs[4] = {
    {0,0}, {1,0}, {1,1}, {0,1}
};

struct AABB {
    /*
        If the size or position of a chunk ever does change it is possible
        to recompute the corners of the bounding box but for now this is not
        needed.
    */
    glm::vec3 vmin;
    glm::vec3 vmax;
};

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct ChunkJob {
    int cx, cz;
};

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // Push a new item into the queue.
    void push(T value) {
        {
            std::lock_guard<std::mutex> lk(m_);
            q_.push(std::move(value));
        }
        cv_.notify_one();
    }

    // Try to pop; returns false immediately if empty.
    bool tryPop(T& out) {
        std::lock_guard<std::mutex> lk(m_);
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    // Block until there's an item, then pop it.
    void waitPop(T& out) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [&]{ return !q_.empty(); });
        out = std::move(q_.front());
        q_.pop();
    }

    // Check if empty (not super useful in multi‐threaded code but handy sometimes)
    bool empty() const {
        std::lock_guard<std::mutex> lk(m_);
        return q_.empty();
    }

private:
    mutable std::mutex            m_;
    std::condition_variable       cv_;
    std::queue<T>                 q_;
};

extern ThreadSafeQueue<ChunkJob> chunkJobs;
extern ThreadSafeQueue<std::unique_ptr<Chunk>> readyChunks;

