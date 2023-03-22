#include "Renderer.h"
#include "Vertex2D.h"


void initVAO()
{
    //TODO: might belong somewhere else
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //vertex array buffer
    unsigned int uVertexArrayBufferID;
    glGenVertexArrays(1, &uVertexArrayBufferID);
    glBindVertexArray(uVertexArrayBufferID);

    //layout
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), NULL);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(sizeof(float) * 2));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(sizeof(float) * 4));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)(sizeof(float) * 5));
}

void StaticToGPU(const VertexBuffer* vBuffer, const IndexBuffer* iBuffer)
{
    if(GetStaticDataPtr(vBuffer) == NULL)
        return;

    //vertices to GPU
    BindVertexBuffer(vBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (sizeof(Vertex2D) * GetStaticAllocSize(vBuffer)), GetStaticDataPtr(vBuffer));
    UnbindCurrentVertexBuffer();

    //all quads assumed
    BindIndexBuffer(iBuffer);

    //standard anti clock-wise directions are {0, 1, 2}, {2, 3, 0}
    unsigned int v = 0;
    unsigned int i = 0;
    unsigned int tmp[6];
    while(i < iBuffer->StaticAllocSize)
    {
        tmp[0] = v + 0;
        tmp[1] = v + 1;
        tmp[2] = v + 2;
        tmp[3] = v + 2;
        tmp[4] = v + 3;
        tmp[5] = v + 0;

        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER, 
            (i * sizeof(unsigned int)), 
            (6 * sizeof(unsigned int)), 
            tmp
        );

        v += 4;
        i += 6;
    }
    UnbindCurrentIndexBuffer();
}

void DynamicToGPU(const VertexBuffer* vBuffer, const IndexBuffer* iBuffer)
{
    if(GetDynamicDataPtr(vBuffer) == NULL)
        return;

    //vertices to GPU
    BindVertexBuffer(vBuffer);
    glBufferSubData(
        GL_ARRAY_BUFFER, 
        (GetStaticAllocSize(vBuffer) * sizeof(Vertex2D)), 
        (GetBlockAllocSize(vBuffer) * (sizeof(Vertex2D) << LOG_BLOCK_SIZE)), 
        GetDynamicDataPtr(vBuffer)
    );
    UnbindCurrentVertexBuffer();

    //all quads assumed
    BindIndexBuffer(iBuffer);

    //standard anti clock-wise directions are {0, 1, 2}, {2, 3, 0}
    unsigned int v = ((iBuffer->StaticAllocSize / 6) * 4);
    unsigned int i = 0;
    unsigned int tmp[6];
    while(i < iBuffer->DynamicAllocSize)
    {
        tmp[0] = v + 0;
        tmp[1] = v + 1;
        tmp[2] = v + 2;
        tmp[3] = v + 2;
        tmp[4] = v + 3;
        tmp[5] = v + 0;

        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER, 
            ((iBuffer->StaticAllocSize + i) * sizeof(unsigned int)), 
            (6 * sizeof(unsigned int)), 
            tmp
        );

        v += 4;
        i += 6;
    }
    UnbindCurrentIndexBuffer();
}