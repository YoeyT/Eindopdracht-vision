#include "Texture.h"

void LoadTexRawData(Texture* tex)
{
    FILE* file = fopen(tex->filePath, "rb");
    if(file == NULL)
        return;

    uint32_t* data = malloc(sizeof(uint32_t) * (tex->width * tex->height));
    fread(data, sizeof(uint32_t), (tex->width * tex->height), file);

    LoadSubTexGPU(tex, 0, 0, data);

    free(data);
    fclose(file);
}

void LoadTexGPU(Texture* tex, const uint32_t* data)
{
    glActiveTexture(GL_TEXTURE0 + tex->slot);
    glGenTextures(1, &(tex->ID));
    glBindTexture(GL_TEXTURE_2D, tex->ID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTextureUnit(tex->slot, tex->ID);
}

void LoadSubTexGPU(Texture* tex, const unsigned int xOff, const unsigned int yOff, const uint32_t* data)
{
    glActiveTexture(GL_TEXTURE0 + tex->slot);
    glTexSubImage2D(GL_TEXTURE_2D, 0, xOff, yOff, tex->width, tex->height, GL_RGBA, GL_UNSIGNED_BYTE, (void*)data);
}