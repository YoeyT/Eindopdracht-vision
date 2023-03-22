#pragma once

#include <Common_Includes.h>

typedef struct IndexBuffer
{
    unsigned int ID;
    size_t StaticAllocSize;
    size_t DynamicAllocSize;

    unsigned int* DynamicData;
} IndexBuffer;


IndexBuffer* CreateIndexBuffer(const size_t StaticAllocSize, const size_t DynamicAllocSize);
void DeleteIndexBuffer(IndexBuffer* buffer);

void BindIndexBuffer(const IndexBuffer* buffer);
static inline void UnbindCurrentIndexBuffer() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }