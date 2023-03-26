#include <Common_Includes.h>

#include "VertexBuffer.h"
#include "Vertex2D.h"

typedef struct VertexBuffer_o
{
    unsigned int ID;
    size_t StaticAllocSize;
    size_t DynamicAllocSize;

    Vertex2D* StaticData;
    Vertex2D* DynamicData;

    //dynamic data buffers
    unsigned int* openBlockPositions;

    uint16_t blockAmount;
    uint16_t blockGapAmount;
    uint16_t blockAllocSize; /*blockGapAmount + blockAmount*/
} VertexBuffer;


VertexBuffer* CreateVertexBuffer(const size_t StaticAllocSize, size_t DynamicAllocSize)
{
    VertexBuffer* buffer = (VertexBuffer*)malloc(sizeof(VertexBuffer));

    glGenBuffers(1, &buffer->ID);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->ID);

    glBufferData(GL_ARRAY_BUFFER, (sizeof(Vertex2D) * (StaticAllocSize + DynamicAllocSize)), NULL, GL_DYNAMIC_DRAW);

    buffer->StaticAllocSize = StaticAllocSize;
    buffer->DynamicAllocSize = DynamicAllocSize;
    buffer->blockAmount = 0;
    buffer->blockGapAmount = 0;
    buffer->blockAllocSize = 0;

    buffer->DynamicData = (Vertex2D*)malloc(sizeof(Vertex2D) * DynamicAllocSize);
    buffer->openBlockPositions = (unsigned int*)malloc(sizeof(unsigned int) * (DynamicAllocSize >> LOG_BLOCK_SIZE));

    for(unsigned int i = 0; i < (DynamicAllocSize >> LOG_BLOCK_SIZE); i++)
        buffer->openBlockPositions[i] = i;

    return buffer;
}

void DeleteVertexBuffer(VertexBuffer* buffer)
{
    free((void*)buffer->DynamicData);
    free((void*)buffer->openBlockPositions);
    free((void*)buffer);
}


void BindVertexBuffer(const VertexBuffer* buffer)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer->ID);
}

void SetStaticVertexData(VertexBuffer* buffer, Vertex2D* data)
{
    buffer->StaticData = data;
}

Vertex2D* GetStaticDataPtr(const VertexBuffer* buffer)    { return buffer->StaticData; }
Vertex2D* GetDynamicDataPtr(const VertexBuffer* buffer)   { return buffer->DynamicData; }
size_t GetStaticAllocSize(const VertexBuffer* buffer)     { return buffer->StaticAllocSize; }
uint16_t GetBlockAllocSize(const VertexBuffer* buffer)    { return buffer->blockAllocSize; }


int AppendBlock(VertexBuffer* vBuffer, Vertex2D* block)
{
    //prevent too much dynamic allocation
    if(vBuffer->DynamicAllocSize <= ((size_t)vBuffer->blockAmount << LOG_BLOCK_SIZE))
        return -1;

    uint16_t newBlockIndex = vBuffer->openBlockPositions[vBuffer->blockAmount];
    memcpy((void*)(&vBuffer->DynamicData[(newBlockIndex << LOG_BLOCK_SIZE)]), (void*)block, (sizeof(Vertex2D) * (1 << LOG_BLOCK_SIZE)));

    if(vBuffer->blockGapAmount > 0)
        vBuffer->blockGapAmount--;
    else
        vBuffer->blockAllocSize++;

    vBuffer->blockAmount++;
    //return &vBuffer->DynamicData[(newBlockIndex << LOG_BLOCK_SIZE)];
    return newBlockIndex;
}

void DeleteBlock(VertexBuffer* vBuffer, const unsigned int index)
{
    memset((void*)(&vBuffer->DynamicData[(index << LOG_BLOCK_SIZE)]), 0, (sizeof(Vertex2D) * (1 << LOG_BLOCK_SIZE)));

    vBuffer->openBlockPositions[vBuffer->blockAmount] = index;
    vBuffer->blockAmount--;

    if((vBuffer->blockAllocSize - 1) == index)
        vBuffer->blockAllocSize--;
    else
        vBuffer->blockGapAmount++;
}

void ModifyBlock(VertexBuffer* vBuffer, Vertex2D* block, const unsigned int index)
{
    memcpy((void*)(&vBuffer->DynamicData[(index << LOG_BLOCK_SIZE)]), (void*)block, (sizeof(Vertex2D) * (1 << LOG_BLOCK_SIZE)));
}