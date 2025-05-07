#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <filesystem>
#include "include/stb_image.h"
#include <iostream>

class Loader {
    public:
        static std::string getPath(const std::string &relativePath) 
        {
            return std::filesystem::current_path().string() + "/" + relativePath;
        }

        static unsigned int loadTexture(std::string path)
        {
            stbi_set_flip_vertically_on_load(true);
            std::string full_path = Loader::getPath(path);
            std::cout << full_path << std::endl;
            unsigned int textureID;
            glGenTextures(1, &textureID);
        
            int width, height, nrComponents;
            unsigned char *data = stbi_load(full_path.c_str(), &width, &height, &nrComponents, 0);
            if (data)
            {
                GLenum format = 0;
                if (nrComponents == 1)
                    format = GL_RED;
                else if (nrComponents == 3)
                    format = GL_RGB;
                else if (nrComponents == 4)
                    format = GL_RGBA;
            
                glBindTexture(GL_TEXTURE_2D, textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);

                
            
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                
            
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Texture failed to load at path: " << path << std::endl;
                stbi_image_free(data);
            }
        
            return textureID;
        }

        static unsigned int loadCubemap(std::vector<std::string> faces)
        {
            unsigned int textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        
            int width, height, nrChannels;
            for (unsigned int i = 0; i < faces.size(); i++)
            {
                unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
                if (data)
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                    stbi_image_free(data);
                }
                else
                {
                    std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
                    stbi_image_free(data);
                }
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
            return textureID;
        }
};





#endif