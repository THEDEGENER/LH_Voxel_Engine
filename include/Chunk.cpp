#include "Chunk.hpp"
#include <array>
#include <vector>
#include <memory>
#include "Mesh.hpp"          
#include "VoxelTypes.hpp" 
#include "World.hpp"  
#include "GreedyMesher.hpp"
 

    Chunk::Chunk(int chunkX, int chunkZ, World& worldptr) : chunkX(chunkX), chunkZ(chunkZ), world(worldptr), dirty(true), scheduled(false),
        box{
            glm::vec3(chunkX * float(WIDTH), 0.0f, chunkZ * float(DEPTH)), 
            glm::vec3(chunkX * float(WIDTH) + float(WIDTH), 
            float(HEIGHT), chunkZ * float(DEPTH) + float(DEPTH))
        } 
    {
      // initialize everything to Air
      blocks.fill(BlockType::Air);
    }

    // set a block and mark dirty so we regenerate the mesh next frame
    void Chunk::setBlock(int x, int y, int z, BlockType type) {
      blocks[index(x,y,z)] = type;
    }

    // read without marking dirty
    BlockType Chunk::getBlock(int x, int y, int z) const {
      return blocks[index(x,y,z)];
    }

    void Chunk::generate()
    {
        for (int x = 0; x < WIDTH; x++)
        {
            for (int z = 0; z < DEPTH; z++)
            {
                int worldX = chunkX * WIDTH + x;
                int worldZ = chunkZ * DEPTH + z;

                double height = noise.getWorldNoise(worldX, worldZ);
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

    void Chunk::addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int x, int y, int z, int dir, glm::vec2& atlasOffset)
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

    void Chunk::buildMesh() 
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
                        bool isBorder = nx<0 || nx>=WIDTH || nz<0 || nz>=DEPTH;
                        bool neighborIsAir = !isBorder && getBlock(nx,ny,nz) == BlockType::Air;

                        if (neighborIsAir)
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
                        } else if (isBorder) {
                            
                            int worldX = chunkX * WIDTH + nx;
                            int worldZ = chunkZ * DEPTH + nz;
                            auto neighborHeight = noise.getWorldNoise(worldX, worldZ);
                            int neighborSurfaceY = static_cast<int>(neighborHeight * MAX_SURFACE);
                            bool neighborIsAirBorder = (y > neighborSurfaceY);

                            if (neighborIsAirBorder) {
                                if (d == 0 || d == 1 || d == 4 || d == 5) {
                                    glm::vec2 textcoord = atlasOffset.sides;
                                    addFaceQuad(verts, idx, x, y, z, d, textcoord);
                                } else if (d == 2) {
                                    glm::vec2 textcoord = atlasOffset.top;
                                    addFaceQuad(verts, idx, x, y, z, d, textcoord);
                                } else if (d == 3) {
                                    glm::vec2 textcoord = atlasOffset.bottom;
                                    addFaceQuad(verts, idx, x, y, z, d, textcoord);
                                }
                            }
                        }
                    }
                }
            }
        }
        // removed for multithreading
        // mesh.setData(verts, idx); 
    }

    void Chunk::setData()
    {
        mesh.setData(verts, idx);
    }

    bool Chunk::IsAabbVisible(const std::vector<glm::vec4>& frustumPlanes) {

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
    void Chunk::draw(Shader& shader, GLuint& atlasText) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(chunkX * WIDTH, 0.0f, chunkZ * DEPTH));
        shader.setMat4("model", model);
        mesh.draw(shader, atlasText);
    }

    std::vector<glm::vec3> Chunk::GetAABBVertices(const AABB& box) {
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


    // simple 1D indexing into a 3D array
    inline int Chunk::index(int x, int y, int z) const {
      return x + WIDTH * (y + HEIGHT * z);
    }

