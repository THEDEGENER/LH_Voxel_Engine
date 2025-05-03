#include "PerlinNoise.hpp"

class Noise
{
    public:

    const siv::PerlinNoise::seed_type seed;
    double freq = 0.005;
    double pers = 0.6;
    const siv::PerlinNoise perlin{ seed };

    Noise(const siv::PerlinNoise::seed_type worldSeed) : seed(worldSeed)
    {
        
    }

    double getWorldNoise(int& worldX, int& worldZ)
    {
        double height = perlin.octave2D_01((worldX * freq), (worldZ * freq), 5, pers);
        return height;
    }
    private:

};