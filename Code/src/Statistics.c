#include "Statistics.h"

#include "GLMath.h"

typedef struct StatisticSet_o
{
    uint8_t xType;
    uint8_t yType;
    float xLineInterval;
    float yLineInterval;

    unsigned int lineCount;

    uint32_t colors[MAX_LINES];
    void* dataPointBuffers[MAX_LINES];
    size_t dataPointCounter[MAX_LINES];
    unsigned int lineThickness[MAX_LINES];
} StatisticSet;

static void DrawDot(uint32_t* img, const unsigned int size, const unsigned int xMax, const unsigned int yMax, int xOff, int yOff, const uint32_t color)
{
    xOff -= (size >> 1);
    yOff -= (size >> 1);

    for(int y = 0; y < size; y++)
    {
        for(int x = 0; x < size; x++)
        {
            if(((xOff + x) < xMax) && ((xOff + x) >= 0) && ((yOff + y) < yMax) && ((yOff + y) >= 0))
                *(img + (((yOff+y) * yMax) + (xOff+x))) = color;
        }
    }
}

static void DrawLineV(uint32_t* img, const unsigned int size, const unsigned int xMax, const unsigned int yMax, const unsigned int xOff, const unsigned int yOff, const unsigned int len)
{
    for(unsigned int y = yOff; y < (yOff + len); y++)
        DrawDot(img, size, xMax, yMax, xOff, y, 0xFF000000);
}

static void DrawLineH(uint32_t* img, const unsigned int size, const unsigned int xMax, const unsigned int yMax, const unsigned int xOff, const unsigned int yOff, const unsigned int len)
{
    for(unsigned int x = xOff; x < (xOff + len); x++)
        DrawDot(img, size, xMax, yMax, x, yOff, 0xFF000000);
}

static void DrawBorderLines(uint32_t* img, const unsigned int xMax, const unsigned int yMax, const unsigned int xOff, const unsigned int yOff)
{
    DrawLineH(img, 5, xMax, yMax, xOff, yOff, xMax);
    DrawLineV(img, 5, xMax, yMax, xOff, yOff, yMax);
}


StatisticSet* CreateStatisticSet(const uint8_t xType, const uint8_t yType, const float xLineInterval, const float yLineInterval)
{
    StatisticSet* ret = (StatisticSet*)malloc(sizeof(StatisticSet));

    ret->xType = xType;
    ret->yType = yType;

    ret->xLineInterval = xLineInterval;
    ret->yLineInterval = yLineInterval;

    ret->lineCount = 0;

    return ret;
}

void DeleteStatisticsSet(StatisticSet* set)
{
    for(unsigned int i = 0; (i < set->lineCount); i++)
        free((void*)(set->dataPointBuffers[i]));
    free((void*)set);
}


void AddLineLayout(StatisticSet* set, const uint32_t color, const size_t maximumDataPointCount, const unsigned int lineThickness)
{
    unsigned int lineCount = set->lineCount;

    set->colors[lineCount] = color;
    set->dataPointBuffers[lineCount] = malloc(maximumDataPointCount * ((set->xType >> 4) + (set->yType >> 4)));
    set->dataPointCounter[lineCount] = 0;
    set->lineThickness[lineCount] = lineThickness;

    (set->lineCount)++;
}

void AddDataPoint(StatisticSet* set, unsigned int Lineindex, const void* xData, const void* yData)
{
    void* destBuffer = set->dataPointBuffers[Lineindex];
    const unsigned int xByteSize = (set->xType >> 4);
    const unsigned int yByteSize = (set->yType >> 4);
    const unsigned int byteStride = xByteSize + yByteSize;

    memcpy((destBuffer + (byteStride * set->dataPointCounter[Lineindex])), xData, xByteSize);
    memcpy((destBuffer + ((byteStride * set->dataPointCounter[Lineindex]) + xByteSize)), yData, yByteSize);

    (set->dataPointCounter[Lineindex])++;
}


uint32_t* GenerateGraph(StatisticSet* set, const float xMin, const float xMax, const float yMin, const float yMax)
{
    const unsigned int graphPixelWidth = GRAPH_IMAGE_WIDTH;
    const unsigned int graphPixelHeight = GRAPH_IMAGE_HEIGHT;
    const unsigned int xPadding = 20;
    const unsigned int yPadding = 20;

    const float xSpan = xMax - xMin;
    const float ySpan = yMax - yMin;

    const unsigned int xByteSize = (set->xType >> 4);
    const unsigned int yByteSize = (set->yType >> 4);
    const unsigned int byteStride = xByteSize + yByteSize;

    uint32_t* retImg = (uint32_t*)malloc(sizeof(uint32_t) * (graphPixelWidth * graphPixelHeight));
    memset((void*)retImg, 0xFF, sizeof(uint32_t) * (graphPixelWidth * graphPixelHeight));

    for(unsigned int i = 0; i < set->lineCount; i++)
    {
        const int dataPointSize = set->dataPointCounter[i];
        const uint32_t color = set->colors[i];
        const void* srcBuffer = set->dataPointBuffers[i];
        const unsigned int lineThickness = set->lineThickness[i];

        unsigned int xDataPoint[4] = { 0 }; //TODO: iets wat de type goed zet
        float yDataPoint[4] = { 0 };

        float xPixelCoords[4] = { 0 };
        float yPixelCoords[4] = { 0 };
        for(int j = 0; j < (dataPointSize - 3); j++)
        {
            //translate the dataPoints to coordinates in the graphs space
            for(unsigned int k = 0; k < 4; k++)
            {
                memcpy(&(xDataPoint[k]), (srcBuffer + (byteStride * (j + k))), xByteSize);
                memcpy(&(yDataPoint[k]), (srcBuffer + ((byteStride * (j + k)) + xByteSize)), yByteSize);

                xPixelCoords[k] = ((((float)(xDataPoint[k]) - xMin) / xSpan) * (float)(graphPixelWidth - xPadding)) + (float)xPadding;
                yPixelCoords[k] = ((((float)(yDataPoint[k]) - yMin) / ySpan) * (float)(graphPixelHeight - yPadding)) + (float)yPadding;
            }

            float xSpot = 0.0;
            float ySpot = 0.0;
            for(float t = 0.0; t < 1.0; t += 0.01)
            {
                xSpot = CubicBezierSpline(xPixelCoords[0], xPixelCoords[1], xPixelCoords[2], xPixelCoords[3], t); //x
                ySpot = CubicBezierSpline(yPixelCoords[0], yPixelCoords[1], yPixelCoords[2], yPixelCoords[3], t); //y

                //check if these coordinates are within the graph
                if((ySpot < (float)graphPixelHeight) && (ySpot >= 0.0) && (xSpot < (float)graphPixelWidth) && (xSpot >= 0.0))
                    DrawDot(retImg, lineThickness, graphPixelWidth, graphPixelHeight, (unsigned int)xSpot, (unsigned int)ySpot, color);
            }
        }
    }

    //borders and lines
    DrawBorderLines(retImg, graphPixelWidth, graphPixelHeight, xPadding, yPadding);
    
    float xLineCoord = 0.0;
    float yLineCoord = (((-yMin) / ySpan) * (float)(graphPixelHeight - yPadding)) + ((float)yPadding / 2.0);
    float xLineInterval = (float)(set->xLineInterval);
    for(float x = 0.0; x < xMax; x += xLineInterval)
    {
        xLineCoord = (((x - xMin) / xSpan) * (float)(graphPixelWidth - xPadding)) + (float)xPadding;
        DrawLineV(retImg, 5, graphPixelWidth, graphPixelHeight, (unsigned int)xLineCoord, (unsigned int)yLineCoord, yPadding);
    }

    xLineCoord = (((-xMin) / xSpan) * (float)(graphPixelWidth - xPadding)) + ((float)xPadding / 2.0);
    yLineCoord = 0.0;
    float yLineInterval = (float)(set->yLineInterval);
    for(float y = 0.0; y < yMax; y += yLineInterval)
    {
        yLineCoord = (((y - yMin) / ySpan) * (float)(graphPixelHeight - yPadding)) + (float)yPadding;
        DrawLineH(retImg, 5, graphPixelWidth, graphPixelHeight, (unsigned int)xLineCoord, (unsigned int)yLineCoord, xPadding);
    }

    return retImg;
}