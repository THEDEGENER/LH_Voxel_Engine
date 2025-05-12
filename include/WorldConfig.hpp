// WorldConfig.hpp

#pragma once

#include <iostream>

struct WorldSettings {
    int seed = 238947;
    float atlasWidth = 1024.0f;
    float atlasHeight = 512.0f;
    float tileSizePx = 16.0f;

    static WorldSettings& instance() {
        static WorldSettings instance;
        return instance;
    }

    static constexpr int CHUNK_WIDTH = 16;
    static constexpr int CHUNK_HEIGHT = 256;
    static constexpr int CHUNK_DEPTH = 16;
    static constexpr int CHUNK_SIZE = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH;
    static constexpr int MAX_SURFACE = 34;
};

#define CONFIG WorldSettings::instance()