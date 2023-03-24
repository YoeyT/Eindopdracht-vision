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

    uint8_t flags; //TODO: handig voor keypress callback functies, maar misschien nog beter om gewoon een functie te schrijven die checkt welke keys gepressed zijn

    void* data1;
    void* data2;
} State;

StateFn Start, Quit, Training, Testing;


GLFWwindow* Init();
void StartStateMachine();


//states
void Start(State* s);
void Quit(State* s);

void Training(State* s);
void Testing(State* s);