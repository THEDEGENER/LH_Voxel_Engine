#include "Chunk.hpp"
#include <array>
#include <vector>
#include <memory>
#include "Mesh.hpp"          
#include "VoxelTypes.hpp" 
#include "World.hpp"  
#include "WorldConfig.hpp"
 

    Chunk::Chunk(int chunkX, int chunkZ, World& worldptr) : chunkX(chunkX), chunkZ(chunkZ), dirty(true), scheduled(false), hasBeenGenerated(false),
        box{
            glm::vec3(chunkX * float(WorldSettings::CHUNK_WIDTH), 0.0f, chunkZ * float(WorldSettings::CHUNK_DEPTH)), 
            glm::vec3(chunkX * float(WorldSettings::CHUNK_WIDTH) + float(WorldSettings::CHUNK_WIDTH), 
            float(WorldSettings::CHUNK_HEIGHT), chunkZ * float(WorldSettings::CHUNK_DEPTH) + float(WorldSettings::CHUNK_DEPTH))
        }, world(worldptr)
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
        for (int x = 0; x < WorldSettings::CHUNK_WIDTH; x++)
        {
            for (int z = 0; z < WorldSettings::CHUNK_DEPTH; z++)
            {
                int worldX = chunkX * WorldSettings::CHUNK_WIDTH + x;
                int worldZ = chunkZ * WorldSettings::CHUNK_DEPTH + z;

                // double height = noise.getWorldNoise(worldX, worldZ);
                // int surfaceY = static_cast<int>( height * WorldSettings::MAX_SURFACE);
                int surfaceY = 200;
                for (int y = 0; y < WorldSettings::CHUNK_HEIGHT; y++)
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
        hasBeenGenerated = true;
    }

    void Chunk::addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, int x, int y, int z, int dir, BlockType type)
    {
        // populate vectors then they are passed to mesh in the build function
        float atlasWidth = 1024.0f;   // actual pixel width of atlas
        float atlasHeight = 512.0f;  // actual pixel height of atlas
        float tileSizePx = 16.0f;

        auto atlasOffset = blockTextureOffsets.at(type);

        glm::vec2 textCoord;
        if (dir == 0 || dir == 1 || dir == 4 || dir == 5) {
            textCoord = atlasOffset.sides;
        } else if (dir == 2) {
            textCoord = atlasOffset.top;
        } else if (dir == 3) {
            textCoord = atlasOffset.bottom;
        }

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
            glm::vec2 uv = faceUVs[i] * uvScale + textCoord * uvScale;

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

    void Chunk::addFaceQuad(std::vector<Vertex>& verts, std::vector<uint32_t>& idx, std::array<glm::vec3, 4>& corners, int faceDir, BlockType type) {

        float atlasWidth = 1024.0f;   // actual pixel width of atlas
        float atlasHeight = 512.0f;  // actual pixel height of atlas
        float tileSizePx = 16.0f;

        auto atlasOffset = blockTextureOffsets.at(type);

        glm::vec2 textCoord;
        if (faceDir == 0 || faceDir == 1 || faceDir == 4 || faceDir == 5) {
            textCoord = atlasOffset.sides;
        } else if (faceDir == 2) {
            textCoord = atlasOffset.top;
        } else {
            textCoord = atlasOffset.bottom;
        }

        glm::vec2 uvScale = glm::vec2(tileSizePx / atlasWidth, tileSizePx / atlasHeight);

        auto baseIndicies = verts.size();
        for (int i = 0; i < 4; i++) {
            glm::vec2 uv = faceUVs[i] * uvScale + textCoord * uvScale;
            verts.push_back(Vertex{
            .Position = corners[i],
            .Normal = dirOffsets[faceDir],
            .TexCoords = uv
            });   
        }

        idx.push_back(baseIndicies + 0);
        idx.push_back(baseIndicies + 1);
        idx.push_back(baseIndicies + 2);

        idx.push_back(baseIndicies + 2);
        idx.push_back(baseIndicies + 3);
        idx.push_back(baseIndicies + 0);
    }

    void Chunk::greedy()
    {
        int dims[3] = {WorldSettings::CHUNK_WIDTH, WorldSettings::CHUNK_HEIGHT, WorldSettings::CHUNK_DEPTH};
        
        for (int dir = 0; dir < 3; dir++) {
            int i, j, k, l, w, h
                , u = (dir + 1) % 3 
                , v = (dir + 2) % 3;
                glm::vec3 x = {0, 0, 0}, q = {0, 0, 0};

            std::vector<BlockType> mask(dims[u] * dims[v], BlockType::Air);
            
            q[dir] = 1;
            for (x[dir] = -1; x[dir] < dims[dir]; ) {
                int n = 0;
                for (x[v] = 0; x[v] < dims[v]; ++x[v])
                for (x[u] = 0; x[u] < dims[u]; ++x[u]) {
                    bool a = (x[dir] >= 0) ? (getBlock(x[0],x[1],x[2]) != BlockType::Air) : false;
                    bool b = (x[dir] < dims[dir]-1) ? (getBlock(x[0]+q[0],x[1]+q[1],x[2]+q[2]) != BlockType::Air) : false;
                    BlockType m;
                    if (a && !b) {
                        // face pointing toward +dir
                        m = getBlock(
                          int(x[0]), 
                          int(x[1]), 
                          int(x[2])
                        );
                    }
                    else if (!a && b) {
                        // face pointing toward –dir, so sample the neighbor
                        m = getBlock(
                          int(x[0] + q[0]), 
                          int(x[1] + q[1]), 
                          int(x[2] + q[2])
                        );
                    }
                    else {
                        m = BlockType::Air;
                    }
                    mask[n++] = m;
                }

                ++x[dir];
                n = 0;
                if (x[dir] >= 0 && x[dir] < dims[dir]) {
                    for (j = 0; j < dims[v]; j++)
                    for (i = 0; i < dims[u]; ) {

                        if (mask[n] != BlockType::Air) {

                            for (w = 1; i + w < dims[u] && mask[n + w] == mask[n]; ++w) {

                            }
                            bool done = false;
                            for (h = 1; j + h < dims[v]; h++) {
                                for (k = 0; k < w; k++) {
                                    if (mask[n + k + h * dims[u]] != mask[n]) {
                                        done = true;
                                        break;
                                    }
                                }
                                if (done) {
                                    break;
                                }
                            }
                            BlockType block_t = mask[n];
                            x[u] = i; x[v] = j;
                            glm::vec3 du{0, 0, 0}; 
                            glm::vec3 dv{0, 0, 0}; 
                            du[u] = w;
                            dv[v] = h;
                            glm::vec3 origin{x[0], x[1], x[2]};

                            std::array<glm::vec3,4> corners = {
                                origin,
                                origin + du,
                                origin + du + dv,
                                origin + dv
                            };

                            std::cout << static_cast<int>(block_t) << std::endl;
                            addFaceQuad(verts, idx, corners, dir, block_t);

                            for (l = 0; l < h; l++)
                            for (k = 0; k < w; k++) {
                                mask[n + k + l * dims[u]] = BlockType::Air;
                            }
                            i += w; n += w;
                        } else {
                            i++, n++;
                        }
                    }
                }  
            }
        }
    }

    void Chunk::buildMesh() 
    {
        verts.clear();
        idx.clear();
        verts.reserve(WorldSettings::CHUNK_SIZE * 24);
        idx.reserve(WorldSettings::CHUNK_SIZE * 36);

        for (int x = 0; x < WorldSettings::CHUNK_WIDTH; x++)
        {
            for (int z = 0; z < WorldSettings::CHUNK_DEPTH; z++)
            {
                for (int y = 0; y < WorldSettings::CHUNK_HEIGHT; y++)
                {
                    if (getBlock(x, y, z) == BlockType::Air) continue;

                    BlockType type = getBlock(x, y, z);
                    // this is now a struct of top , sides , bottom
                    for (int d = 0; d < 6; d++)
                    {
                        // loop through the 6 faces and use a direction offset array to store to values
                        glm::ivec3 offsets = dirOffsets[d];
                        int tx = x + offsets.x;
                        int ty = y + offsets.y;
                        int tz = z + offsets.z;

                        int chunkOffsetX = 0;
                        if (tx <  0) chunkOffsetX = -1;
                        if (tx >= WorldSettings::CHUNK_WIDTH) chunkOffsetX = +1;

                        int chunkOffsetZ = 0;
                        if (tz <  0) chunkOffsetZ = -1;
                        if (tz >= WorldSettings::CHUNK_WIDTH) chunkOffsetZ = +1;

                        if (chunkOffsetX == 0 && chunkOffsetZ == 0) {
                            if (getBlock(tx, ty, tz) == BlockType::Air) {
                                addFaceQuad(verts, idx, x, y, z, d, type);
                            }
                        } else {
                            int nChunkX = chunkX + chunkOffsetX;
                            int nChunkZ = chunkZ + chunkOffsetZ;

                            if (world.getChunk(nChunkX, nChunkZ, tx, ty, tz) == BlockType::Air) {
                                addFaceQuad(verts, idx, x, y, z, d, type);
                            }
                        }
                    }
                }
            }
        }
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
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(chunkX * WorldSettings::CHUNK_WIDTH, 0.0f, chunkZ * WorldSettings::CHUNK_DEPTH));
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
      return x + WorldSettings::CHUNK_WIDTH * (y + WorldSettings::CHUNK_HEIGHT * z);
    }
