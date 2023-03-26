#include "GLMath.h"

static float ConvolveAtOffsetKern3x3(const uint32_t* src,  const int width, const int xOff, const int yOff, const float kernel[3][3])
{
    float sum = 0.0;
    for(unsigned int x = 0; x < 3; x++)
    {
        for(unsigned int y = 0; y < 3; y++)
            sum += (((uint8_t)(*(src + ((yOff + y) * width) + (xOff + x))) / 255.0) * kernel[x][y]);
    }
    return sum;
}

Mat4x4 InitMat4x4(float mat[16])
{
    Mat4x4 retMat;
    memcpy(retMat.m, mat, sizeof(Mat4x4));
    return retMat;
}

Mat4x4 OrthographicMat4x4(float left, float right, float bottom, float top, float near, float far)
{
    float xSpan = right - left;
    float ySpan = top - bottom;
    float zSpan = far - near;

    float T1 = -((right + left) / xSpan);
    float T2 = -((top + bottom) / ySpan);
    float T3 = -((far + near) / zSpan);

    float fMat[16] = 
    {
        (2.0 / xSpan), 0.0, 0.0, 0.0, 
        0.0, (2.0 / ySpan), 0.0, 0.0, 
        0.0, 0.0, (-2.0 / zSpan), 0.0, 
        T1, T2, T3, 1.0
    };

    return InitMat4x4(fMat);
}

Mat4x4 TranslationMat4x4(float x, float y, float z)
{
    float fMat[16] = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, x, y, z, 1.0};
    return InitMat4x4(fMat);
}

Mat4x4 MultiplyMat4x4(Mat4x4 left, Mat4x4 right)
{
    Mat4x4 retMat;

    for(unsigned int col = 0; col < 4; col++)
    {
        for(unsigned int row = 0; row < 4; row++)
        {
            retMat.m[col][row] = 
                (left.m[0][row] * right.m[col][0]) + 
                (left.m[1][row] * right.m[col][1]) + 
                (left.m[2][row] * right.m[col][2]) + 
                (left.m[3][row] * right.m[col][3]);
        }
    }

    return retMat;
}

void SetTransformMat4x4(Mat4x4* mat, float x, float y, float z)
{
    mat->m[3][0] = x;
    mat->m[3][1] = y;
    mat->m[3][2] = z;
}

void ConvertImageGrayScale(uint32_t* data, const int width, const int height)
{
    uint32_t tmp = 0;
    uint8_t gray = 0;
    for(unsigned int i = 0; i < (width * height); i++)
    {
        tmp = data[i];
        gray = (uint8_t)((((tmp & 0x00FF0000) >> 16) + ((tmp & 0x0000FF00) >> 8) + (tmp & 0x000000FF)) / 3);
        data[i] = (uint32_t)(0xFF000000 + (gray << 16) + (gray << 8) + gray);
    }
}

void ConvolveImageKern3x3(const uint32_t* src, uint32_t* dest, const int width, const int height, const float kernel[3][3])
{
    float sum = 0.0;
    uint8_t gray = 0;
    for(unsigned int y = 1; y < (height - 1); y++)
    {
        for(unsigned int x = 1; x < (width - 1); x++)
        {
            sum = ConvolveAtOffsetKern3x3(src, width, (x-1), (y-1), kernel);
            gray = (uint8_t)(maxf(minf((sum * 255.0), 255.0), 0.0));
            *(dest + (y * width) + x) = (uint32_t)(0xFF000000 + (gray << 16) + (gray << 8) + gray);
        }
    }
}

//not the fastest way, but very elegant and easy to understand
float CubicBezierSpline(const float p0, const float p1, const float c0, const float c1, const float t)
{
    float a = Lerp(p0, c0, t);
    float b = Lerp(c0, c1, t);
    float c = Lerp(c1, p1, t);

    float d = Lerp(a, b, t);
    float e = Lerp(b, c, t);

    return Lerp(d, e, t);
}