// GreedyMesher.hpp
#pragma once

#include "World.hpp"
#include "VoxelTypes.hpp"
#include "Chunk.hpp"
#include "GreedyMesher.hpp"
#include <array>
#include <vector>
#include "WorldConfig.hpp"
 

void GreedyMesh(const std::array<BlockType, (WIDTH + 2)*(HEIGHT + 2)*(DEPTH + 2)>& chunkBlockMap, std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int chunkX, int chunkZ, World& world)
{
    for (int dir = 0; dir < 6; dir++)
    {
        // loop 6x for each face direction creating a mask from the
        auto& axis = faceAxes[dir];
        auto& offset = dirOffsets[dir];
        int maxSize[3];
        maxSize[0] = WIDTH;
        maxSize[1] = HEIGHT;
        maxSize[2] = DEPTH;
        int maxU = maxSize[axis.u];      // e.g. if A.u==0, size[X], if A.u==1, size[Y]
        int maxV = maxSize[axis.v];
        int maxW = maxSize[axis.w] + 1;
        for (int w = 0; w < maxW; ++w) {
            // prepare 2D mask for this slice
            mask.assign(maxU * maxV, BlockType::Air);
            for (int v = 0; v < maxV; ++v) {
              for (int u = 0; u < maxU; ++u) {
                // 1) map (u,v,w) into real (x,y,z)
                int x = (axis.u == 0 ? u : axis.v == 0 ? v : w);
                int y = (axis.u == 1 ? u : axis.v == 1 ? v : w);
                int z = (axis.u == 2 ? u : axis.v == 2 ? v : w);
          
                // 2) fetch current and neighbor
                BlockType current   = getChunkBlock(x, y, z, chunkBlockMap, chunkX, chunkZ, world, dir);
                BlockType neighbour = getChunkBlock(
                                       x + offset.x,
                                       y + offset.y,
                                       z + offset.z, chunkBlockMap, chunkX, chunkZ, world, dir);
          
                // 3) build mask in [u][v]
                if (current != BlockType::Air && neighbour == BlockType::Air)
                    maskAt(u, v, maxU) = current;
                // else is already Air from assign
              }
            }
            for (int v = 0; v < maxV; ++v) {
                for (int u = 0; u < maxU; ++u) {
                    BlockType type = maskAt(u, v, maxU);
                    // skip empty cells
                    if (type == BlockType::Air) continue;
                    int width = 1;
                    int height = 1;
                    while(u + width < maxU && mask[(u + width) + v * maxU] == type)
                        width++;
                    
                    bool done = true;
                    while(v + height < maxV && done)
                    {
                        for (int k = 0; k < width; k++)
                        {
                            if (mask[(u + k) + (v + height) * maxU] != type)
                            {
                                done = false;
                                break;
                            }
                        }
                        if (done) height++;
                    }
                    int coords[3];
                    coords[axis.u] = u;
                    coords[axis.v] = v;
                    coords[axis.w] = w;
                    // Convert padded coordinates back to chunk-local coordinates
                    int realX = coords[0] - 1;
                    int realY = coords[1] - 1;
                    int realZ = coords[2] - 1;
                    auto atlasOffset = blockTextureOffsets.at(type);
                    
                    if (dir == 0 || dir == 1 || dir == 4 || dir == 5)
                    {
                        glm::vec2 textCoords = atlasOffset.sides;
                        addFaceQuad(verts, idx, realX, realY, realZ, dir, textCoords);
                    } 
                    else if (dir == 2) 
                    {
                        glm::vec2 textCoords = atlasOffset.top;
                        addFaceQuad(verts, idx, realX, realY, realZ, dir, textCoords);
                    }
                    else if (dir == 3)
                    {
                        glm::vec2 textCoords = atlasOffset.bottom;
                        addFaceQuad(verts, idx, realX, realY, realZ, dir, textCoords);
                    } 
                    for (int yy = 0; yy < height; ++yy)
                    for (int xx = 0; xx < width;  ++xx)
                      mask[(u + xx) + (v + yy) * maxU] = BlockType::Air;
                }
            }
        }
    }
}

  // read without marking dirty
BlockType getChunkBlock(int x, int y, int z, const std::array<BlockType, (WIDTH + 2)*(HEIGHT + 2)*(DEPTH + 2)>& chunkBlockMap, int chunkX, int chunkZ, World& world, int dir) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT || z < 0 || z >= DEPTH) {
        return world.globalGetNeighbourChunkBlock(chunkX, chunkZ, x, y, z, dir);
    }
    return chunkBlockMap[index(x, y, z)];
}
// Helper to index into the flat 2D mask buffer
inline BlockType& maskAt(int u, int v, int rowWidth) {
    return mask[u + v * rowWidth];
}
inline const BlockType& maskAt(int u, int v, int rowWidth) const {
    return mask[u + v * rowWidth];
}
inline int index(int x, int y, int z) const {
    return x + WIDTH * (y + HEIGHT * z);
}
void addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int x, int y, int z, int dir, const glm::vec2& atlasOffset)
{
    // populate vectors then they are passed to mesh in the build function
    auto baseIndicies = verts.size();
    auto normal = dirOffsets[dir];
    // iterate for the 4 points ands add the offsets to the x, y, z to get the coordinates in local
    // space and then add that to the vertex array so when drawn each struct is each triangle
    for (int i = 0; i < 4; i++)
    {
        // this grabs the unique value for the given direction to increment x, y or z
        glm::vec3 pos = glm::vec3(x, y, z) + vertexOffsets[dir][i];
        // this takes the scale from the dimensions of the atlas photo and then sets the u0, v0
        glm::vec2 uv = faceUVs[i] * uvScale + atlasOffset * uvScale;
        verts.push_back(Vertex{
            .Position = pos,
            .Normal = normal,
            .TexCoords = uv
        });
    }
    // the base idx is based on the size of the verts array so that way it servers as the offset at
    // that given position when you add an array 
    idx.push_back(baseIndicies + 0);
    idx.push_back(baseIndicies + 1);
    idx.push_back(baseIndicies + 2);
    idx.push_back(baseIndicies + 2);
    idx.push_back(baseIndicies + 3);
    idx.push_back(baseIndicies + 0);
    
}