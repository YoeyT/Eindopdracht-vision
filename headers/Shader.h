#pragma once

#include "GLMath.h"

#define MAX_SHADERS 4

typedef struct ShaderProgram_o ShaderProgram;
typedef struct ShaderLib_o ShaderLib;

ShaderLib* CreateShaderLibrary(const char** schaderFilePaths, unsigned int shaderAmount);
void DeleteShaderLib(ShaderLib* library);
void UseShaderProgramL(ShaderLib* library, const unsigned int index);
ShaderProgram* GetShader(ShaderLib* library, const unsigned int index);
unsigned int AppendShader(ShaderLib* library, const char** schaderFilePaths, unsigned int shaderAmount);
void PopBackShader(ShaderLib* library); //probably useless

//caching
void AddVarToCache(ShaderProgram* program, const char* variable);
void AddVarsToCache(ShaderProgram* program, const char** variables, const unsigned int variableSize);
const int GetVarFromCache(ShaderProgram* program, const unsigned int index);
unsigned int GetID(const ShaderProgram* program);

//uniforms
void LoadUniformVarInts(ShaderProgram* program, const unsigned int index, const unsigned int size, const int* values);
void LoadUniformMat4x4(ShaderProgram* program, const unsigned int index, const Mat4x4* mat);

void UseShaderProgramP(const ShaderProgram* program);