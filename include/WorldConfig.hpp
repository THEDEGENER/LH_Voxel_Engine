// WorldConfig.hpp

#pragma once

#include <iostream>
#include "PerlinNoise.hpp"

struct WorldSettings {
    siv::PerlinNoise::seed_type seed = 238947;
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
    static constexpr int MAX_SURFACE = 64;
};

#define CONFIG WorldSettings::instance()