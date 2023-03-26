#pragma once

#include <Common_Includes.h>

#include "Shader.h"
#include "GLMath.h"
#include "Renderer.h"

struct State;
typedef void StateFn(struct State*);

typedef struct State
{
    StateFn* nextFn;
    GLFWwindow* window;
    ShaderLib* shaderLib;

    Mat4x4* projMat;
    Mat4x4* viewMat;

    VertexBuffer* vBuffer;
    IndexBuffer* iBuffer;

    uint8_t flags;

    void* data1;
    void* data2;
    void* data3;
} State;

StateFn Start, Quit, Training, Testing;


GLFWwindow* Init();
void StartStateMachine();


//states
void Start(State* s);
void Quit(State* s);

void Training(State* s);
void Testing(State* s);