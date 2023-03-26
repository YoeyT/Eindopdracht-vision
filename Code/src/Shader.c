#include <Common_Includes.h>
#include "Shader.h"

typedef struct ShaderProgram_o
{
    unsigned int ID;
    int variableCache[16];
    unsigned char cacheSize;
} ShaderProgram;

typedef struct ShaderLib_o
{
    ShaderProgram shaders[MAX_SHADERS];
    unsigned int shaderProgramAmount;
} ShaderLib;

static const char* ReadShaderFile(const char* filePath)
{
    FILE* file = fopen(filePath, "rb");
    if(file == NULL)
    {
        printf("failed to load shader file: %s\n", filePath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    unsigned int fileSize = ftell(file);
    rewind(file);

    char* shaderCode = (char*)malloc((sizeof(char) * fileSize) + 1);
    fread((void*)shaderCode, 1, fileSize, file);
    shaderCode[fileSize] = '\0';

    fclose(file);
    return (const char*)shaderCode;
}

static inline GLenum ShaderType(const char* filePath)
{
    char* dotPtr = memchr(filePath, '.', strlen(filePath));
    if(dotPtr == NULL)
        return 0;

    dotPtr++;
    if(strcmp("fragment", dotPtr) == 0)         { return GL_FRAGMENT_SHADER; }
    else if(strcmp("vertex", dotPtr) == 0)      { return GL_VERTEX_SHADER; }
    else if(strcmp("compute", dotPtr) == 0)     { return GL_COMPUTE_SHADER; }
    else if(strcmp("geometry", dotPtr) == 0)    { return GL_GEOMETRY_SHADER; }

    return 0;
}

static unsigned int CompileShader(GLenum type, const char* source)
{
    unsigned int uShaderID = glCreateShader(type);
    glShaderSource(uShaderID, 1, &source, NULL);
    glCompileShader(uShaderID);

    int err;
    glGetShaderiv(uShaderID, GL_COMPILE_STATUS, &err);
    if(err == GL_FALSE)
    {
        int length;
        glGetShaderiv(uShaderID, GL_INFO_LOG_LENGTH, &length);
        char* errorMessage = (char*)alloca(sizeof(char) * length);
        glGetShaderInfoLog(uShaderID, length, &length, errorMessage);
        printf("error when compiling shader with ID: %d\n", uShaderID);
        printf(errorMessage);
        glDeleteShader(uShaderID);
        return 0;
    }

    return uShaderID;
}

static unsigned int CreateShaderProgramGPU(const char** schaderFilePaths, unsigned int shaderAmount)
{
    unsigned int uProg = glCreateProgram();

    GLenum shaderType = 0;
    const char* shaderCode = NULL;
    unsigned int shaderIDs[8] = { 0 };
    for(unsigned int i = 0; i < shaderAmount; i++)
    {
        shaderCode = ReadShaderFile(schaderFilePaths[i]);
        shaderType = ShaderType(schaderFilePaths[i]);
        if((shaderCode == NULL) || (shaderType == 0))
        {
            printf("could not find file or shader type, file: %s\n", schaderFilePaths[i]);
            return 0;
        }

        shaderIDs[i] = CompileShader(shaderType, shaderCode);
        glAttachShader(uProg, shaderIDs[i]);

        free((void*)shaderCode);
    }

    glLinkProgram(uProg);
    glValidateProgram(uProg);

    for(unsigned int i = 0; i < shaderAmount; i++)
        glDeleteShader(shaderIDs[i]);

    return uProg;
}

/*=================================================================================================*/

ShaderLib* CreateShaderLibrary(const char** schaderFilePaths, unsigned int shaderAmount)
{
    ShaderLib* ret = (ShaderLib*)malloc(sizeof(ShaderLib));
    
    ret->shaders[0].ID = CreateShaderProgramGPU(schaderFilePaths, shaderAmount);
    ret->shaders[0].cacheSize = 0;
    ret->shaderProgramAmount = 1;

    glUseProgram(ret->shaders[0].ID);

    return ret;
}

void DeleteShaderLib(ShaderLib* library)
{
    for(unsigned int i = 0; i < library->shaderProgramAmount; i++)
        glDeleteProgram(library->shaders[i].ID);
    free((void*)library);
}

void UseShaderProgramL(ShaderLib* library, const unsigned int index)
{
    if(index >= library->shaderProgramAmount)
        return;
    glUseProgram(library->shaders[index].ID);
}

ShaderProgram* GetShader(ShaderLib* library, const unsigned int index)
{
    if(index >= library->shaderProgramAmount)
        return NULL;
    return &library->shaders[index];
}

unsigned int AppendShader(ShaderLib* library, const char** schaderFilePaths, unsigned int shaderAmount)
{
    unsigned int indexHandle = library->shaderProgramAmount;
    if(indexHandle >= MAX_SHADERS)
        return MAX_SHADERS;

    library->shaders[indexHandle].ID = CreateShaderProgramGPU(schaderFilePaths, shaderAmount);
    library->shaders[indexHandle].cacheSize = 0;
    library->shaderProgramAmount++;

    return indexHandle;
}

void PopBackShader(ShaderLib* library)
{
    library->shaderProgramAmount--;
}

/*=================================================================================================*/

void AddVarToCache(ShaderProgram* program, const char* variable)
{
    program->variableCache[program->cacheSize] = glGetUniformLocation(program->ID, variable);
    program->cacheSize++;
}

void AddVarsToCache(ShaderProgram* program, const char** variables, const unsigned int variableSize)
{
    for(size_t i = 0; i < variableSize; i++)
        AddVarToCache(program, variables[i]);
}

const int GetVarFromCache(ShaderProgram* program, const unsigned int index)
{
    if(index >= program->cacheSize)
        return -1;
    return program->variableCache[index];
}

unsigned int GetID(const ShaderProgram* program)
{
    return program->ID;
}

void LoadUniformVarInts(ShaderProgram* program, const unsigned int index, const unsigned int size, const int* values)
{
    glUniform1iv(program->variableCache[index], size, (GLint*)values);
}

void LoadUniformMat4x4(ShaderProgram* program, const unsigned int index, const Mat4x4* mat)
{
    glUniformMatrix4fv(program->variableCache[index], 1, GL_FALSE, (GLfloat*)mat);
}

void UseShaderProgramP(const ShaderProgram* program)
{
    glUseProgram(program->ID);
}