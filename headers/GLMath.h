#pragma once

#include <Common_Includes.h>

#define E               2.71828182
#define PI              3.14159265
#define DEG_TO_RADS     (PI / 180.0)
#define RADS_TO_DEG     (180.0 / PI)


typedef struct Vec2f
{
    float x;
    float y;
} Vec2f;

//collumn-major order
typedef struct Mat4x4
{
    float m[4][4];
} Mat4x4;


Mat4x4 InitMat4x4(float mat[16]);
Mat4x4 OrthographicMat4x4(float left, float right, float bottom, float top, float near, float far);
Mat4x4 TranslationMat4x4(float x, float y, float z);
Mat4x4 MultiplyMat4x4(Mat4x4 left, Mat4x4 right);
void SetTransformMat4x4(Mat4x4* mat, float x, float y, float z);

void ConvertImageGrayScale(uint32_t* data, const int width, const int height);
void ConvolveImageKern3x3(const uint32_t* src, uint32_t* dest, const int width, const int height, const float kernel[3][3]);

float CubicBezierSpline(const float p0, const float p1, const float c0, const float c1, const float t);

static inline float DotProduct(Vec2f lhs, Vec2f rhs) { return (lhs.x * rhs.x) + (lhs.y * rhs.y); }
static inline float Sigmoid(const float x) { return (1.0 / (1.0 + powf(E, -x))); } //TODO: Sigmoid werkt erg goed met het neurale netwerk, maar misschien dat iets simpelers ook goed werkt: RELU  R(z) = min(1.0, max(0.0, z))
static inline float Lerp(const float p0, const float p1, const float t) { return p0 + (t * (p1 - p0)); }

static inline Vec2f RandomGradient()
{
    int randomInt = rand();
    return (Vec2f){ cosf((float)randomInt), sinf((float)randomInt) };
}

//between 0.0 and 1.0
static inline float RandomFloat() { return ((float)rand() / (float)RAND_MAX); }