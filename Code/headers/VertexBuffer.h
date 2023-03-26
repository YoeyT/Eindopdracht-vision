#pragma once

#include <Common_Includes.h>

#include "Vertex2D.h"

#define LOG_BLOCK_SIZE 2

typedef struct VertexBuffer_o VertexBuffer;


VertexBuffer* CreateVertexBuffer(const size_t StaticAllocSize, size_t DynamicAllocSize);
void DeleteVertexBuffer(VertexBuffer* buffer);

void BindVertexBuffer(const VertexBuffer* buffer);
static inline void UnbindCurrentVertexBuffer() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

void SetStaticVertexData(VertexBuffer* buffer, Vertex2D* data);


Vertex2D* GetStaticDataPtr(const VertexBuffer* buffer);
Vertex2D* GetDynamicDataPtr(const VertexBuffer* buffer);
size_t GetStaticAllocSize(const VertexBuffer* buffer);
uint16_t GetBlockAllocSize(const VertexBuffer* buffer);


//dynamic draw

//copies the vertex data to the dynamic vertex buffer, then returns a pointer of its new location
int AppendBlock(VertexBuffer* vBuffer, Vertex2D* block);

//be sure to delete a vertex block that exists
void DeleteBlock(VertexBuffer* vBuffer, const unsigned int index);

void ModifyBlock(VertexBuffer* vBuffer, Vertex2D* block, const unsigned int index);