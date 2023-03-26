#include <Common_Includes.h>

#include "IndexBuffer.h"

IndexBuffer* CreateIndexBuffer(const size_t StaticAllocSize, const size_t DynamicAllocSize)
{
    IndexBuffer* iBuf = (IndexBuffer*)malloc(sizeof(IndexBuffer));
    glGenBuffers(1, &(iBuf->ID));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iBuf->ID);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (sizeof(unsigned int) * (StaticAllocSize + DynamicAllocSize)), NULL, GL_DYNAMIC_DRAW);

    iBuf->StaticAllocSize = StaticAllocSize;
    iBuf->DynamicAllocSize = DynamicAllocSize;
    
    return iBuf;
}

void DeleteIndexBuffer(IndexBuffer* buffer)
{
    free((void*)buffer);
}

void BindIndexBuffer(const IndexBuffer* buffer)
{
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->ID);
}