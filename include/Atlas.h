#include <iostream>
#include "scripts/Loader.h"

class Textures 
{
    public:
    static auto bind()
    {
        auto atlas = Loader::loadTexture("assets/textures/block_atlas.png");
        return atlas;
    }
};