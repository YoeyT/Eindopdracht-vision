#pragma once

#include <Common_Includes.h>

typedef struct Vertex2D //TODO: toch nog even kijken later of dit niet beter bij de vertexBuffer file kan
{
    float x;
    float y;
    float texX;
    float texY;
    float texSlot;
    float R;
    float G;
    float B;
    float A;
} Vertex2D;