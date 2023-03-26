#pragma once

#include <Common_Includes.h>

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Shader.h"

void initVAO();

void StaticToGPU(const VertexBuffer* vBuffer, const IndexBuffer* iBuffer);
void DynamicToGPU(const VertexBuffer* vBuffer, const IndexBuffer* iBuffer);

static inline void Clear()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

static inline void Draw(const IndexBuffer* buffer, const ShaderProgram* shaderProg)
{
    UseShaderProgramP(shaderProg);
    BindIndexBuffer(buffer);
    glDrawElements(GL_TRIANGLES, (buffer->StaticAllocSize + buffer->DynamicAllocSize), GL_UNSIGNED_INT, NULL);
}