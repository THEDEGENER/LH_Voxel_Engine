// Chunk.h

#pragma once
#include <array>
#include <vector>
#include <memory>
#include "Mesh.h"          
#include "VoxelTypes.h"
#include <libs/glm/gtc/noise.hpp> 
#include "include/PerlinNoise.hpp"    

class Chunk {
public:
    // classic Minecraft chunk size
    static constexpr int WIDTH  = 16;
    static constexpr int HEIGHT = 256;
    static constexpr int DEPTH  = 16;

    static constexpr int MAX_SURFACE = 64;

    const siv::PerlinNoise::seed_type seed = 123456u;

    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    int chunkX;
    int chunkZ;

    Chunk(int chunkX, int chunkZ)
      : chunkX(chunkX), chunkZ(chunkZ), dirty(true),
        box{ glm::vec3(chunkX * float(WIDTH), 0.0f, chunkZ * float(DEPTH)),
             glm::vec3(chunkX * float(WIDTH) + float(WIDTH),
                float(HEIGHT), chunkZ * float(DEPTH) + float(DEPTH)) }
    {
      // initialize everything to Air
      blocks.fill(BlockType::Air);
    }

    // set a block and mark dirty so we regenerate the mesh next frame
    void setBlock(int x, int y, int z, BlockType type) {
      blocks[index(x,y,z)] = type;
      dirty = true;
    }

    // read without marking dirty
    BlockType getBlock(int x, int y, int z) const {
      return blocks[index(x,y,z)];
    }

    void generate()
    {
        const siv::PerlinNoise perlin{ seed };
        double freq = 0.005;
        double pers = 0.6;
        
        for (int x = 0; x < WIDTH; x++)
        {
            for (int z = 0; z < DEPTH; z++)
            {
                int worldX = chunkX * WIDTH + x;
                int worldZ = chunkZ * DEPTH + z;

                double height = perlin.octave2D_01((worldX * freq), (worldZ * freq), 5, pers);
                int surfaceY = static_cast<int>( height * MAX_SURFACE);
                for (int y = 0; y < HEIGHT; y++)
                {
                    if (y == surfaceY)
                    {
                        setBlock(x, y, z, BlockType::Grass);
                    } else if (y < surfaceY && y > surfaceY - 5)
                    {
                        setBlock(x, y, z, BlockType::Dirt);
                    } else if (y < surfaceY - 5)
                    {
                        setBlock(x, y, z, BlockType::Stone);
                    }
                }
            }
        }
    }

    void addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int x, int y, int z, int dir, glm::vec2 atlasOffset)
    {
        // populate vectors then they are passed to mesh in the build function
        float atlasWidth = 1024.0f;   // actual pixel width of atlas
        float atlasHeight = 512.0f;  // actual pixel height of atlas
        float tileSizePx = 16.0f;

        auto baseIndicies = verts.size();
        auto normal = dirOffsets[dir];

        glm::vec2 uvScale = glm::vec2(tileSizePx / atlasWidth, tileSizePx / atlasHeight);

        // iterate for the 4 points ands add the offsets to the x, y, z to get the coordinates in local
        // space and then add that to the vertex array so when drawn each struct is each triangle
        for (int i = 0; i < 4; i++)
        {
            // this grabs the unique value for the given direction to increment x, y or z
            glm::ivec3 pos = glm::vec3(x, y, z) + vertexOffsets[dir][i];

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

    void buildMesh() 
    {
        verts.clear();
        idx.clear();
        for (int x = 0; x < WIDTH; x++)
        {
            for (int z = 0; z < DEPTH; z++)
            {
                for (int y = 0; y < HEIGHT; y++)
                {
                    if (getBlock(x, y, z) == BlockType::Air) continue;

                    BlockType type = getBlock(x, y, z);
                    // this is now a struct of top , sides , bottom
                    auto atlasOffset = blockTextureOffsets.at(type);
                    for (int d = 0; d < 6; d++)
                    {
                        // loop through the 6 faces and use a direction offset array to store to values
                        glm::ivec3 offsets = dirOffsets[d];
                        int nx = x + offsets.x;
                        int ny = y + offsets.y;
                        int nz = z + offsets.z;

                        // assume out of bounds is air for now
                        bool isBorder = nx<0 || nx>=WIDTH || ny<0 || ny>=HEIGHT || nz<0 || nz>=DEPTH;
                        bool neighborIsAir = !isBorder && getBlock(nx,ny,nz) == BlockType::Air;

                        if (isBorder || neighborIsAir)
                        {
                            // try to regen perlin noise for border faces to see if it improves performance
                            if (d == 0 || d == 1 || d == 4 || d == 5)
                            {
                                glm::vec2 textcoord = atlasOffset.sides;
                                addFaceQuad(verts, idx, x, y, z, d, textcoord);

                            } else if (d == 2)
                            {
                                glm::vec2 textcoord = atlasOffset.top;
                                addFaceQuad(verts, idx, x, y, z, d, textcoord);

                            } else if (d == 3)
                            {
                                glm::vec2 textcoord = atlasOffset.bottom;
                                addFaceQuad(verts, idx, x, y, z, d, textcoord);

                            }
                        }
                    }
                }
            }
        }
        mesh.setData(verts, idx); 
    }

    bool IsAabbVisible(const std::vector<glm::vec4>& frustumPlanes) {

        const glm::vec3& vmin = this->box.vmin;
        const glm::vec3& vmax = this->box.vmax;

        for (size_t i = 0; i < frustumPlanes.size(); ++i) {
            const glm::vec4& g = frustumPlanes[i];
            if ((glm::dot(g, glm::vec4(vmin.x, vmin.y, vmin.z, 1.0f)) < 0.0) &&
             (glm::dot(g, glm::vec4(vmax.x, vmin.y, vmin.z, 1.0f)) < 0.0) &&
             (glm::dot(g, glm::vec4(vmin.x, vmax.y, vmin.z, 1.0f)) < 0.0) &&
             (glm::dot(g, glm::vec4(vmax.x, vmax.y, vmin.z, 1.0f)) < 0.0) &&
             (glm::dot(g, glm::vec4(vmin.x, vmin.y, vmax.z, 1.0f)) < 0.0) &&
             (glm::dot(g, glm::vec4(vmax.x, vmin.y, vmax.z, 1.0f)) < 0.0) &&
             (glm::dot(g, glm::vec4(vmin.x, vmax.y, vmax.z, 1.0f)) < 0.0) &&
             (glm::dot(g, glm::vec4(vmax.x, vmax.y, vmax.z, 1.0f)) < 0.0))
            {
             // Not visible - all returned negative
             return false;
            }
        }

        // Potentially visible
        return true;
    }
 
    // just draws the mesh (one glDrawElements call under the hood)
    void draw(Shader& shader) {
        if (dirty) {
            buildMesh();
            dirty = false;
        }

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(chunkX * WIDTH, 0.0f, chunkZ * DEPTH));
        shader.setMat4("model", model);
        mesh.draw(shader);
    }

    std::vector<glm::vec3> GetAABBVertices(const AABB& box) {
        const glm::vec3& vmin = box.vmin;
        const glm::vec3& vmax = box.vmax;
    
        // First create the 8 corners
        const glm::vec3 corners[8] = {
            glm::vec3(vmin.x, vmin.y, vmin.z),
            glm::vec3(vmin.x, vmax.y, vmin.z),
            glm::vec3(vmin.x, vmin.y, vmax.z),
            glm::vec3(vmin.x, vmax.y, vmax.z),
            glm::vec3(vmax.x, vmin.y, vmin.z),
            glm::vec3(vmax.x, vmax.y, vmin.z),
            glm::vec3(vmax.x, vmin.y, vmax.z),
            glm::vec3(vmax.x, vmax.y, vmax.z)
        };
    
        // Now connect the corners to form 12 lines
        std::vector<glm::vec3> vertices = {
            corners[0], corners[1], // Line 1
            corners[2], corners[3], // Line 2
            corners[4], corners[5],
            corners[6], corners[7],
    
            corners[0], corners[2],
            corners[1], corners[3],
            corners[4], corners[6],
            corners[5], corners[7],
    
            corners[0], corners[4],
            corners[1], corners[5],
            corners[2], corners[6],
            corners[3], corners[7]  // Line 12
        };
    
        return vertices;
    }

private:
    // simple 1D indexing into a 3D array
    inline int index(int x, int y, int z) const {
      return x + WIDTH * (y + HEIGHT * z);
    }

    AABB box;
    bool dirty;          
    std::array<BlockType, WIDTH*HEIGHT*DEPTH> blocks;
    Mesh mesh;
};
