#pragma once

#include <Common_Includes.h>

typedef struct Texture
{
    int width;
    int height;
    const unsigned int slot;
    const char* filePath;

    unsigned int ID;
} Texture;

void LoadTexRawData(Texture* tex);

void LoadTexGPU(Texture* tex, const uint32_t* data);
void LoadSubTexGPU(Texture* tex, const unsigned int xOff, const unsigned int yOff, const uint32_t* data);
