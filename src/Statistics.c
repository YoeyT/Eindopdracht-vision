#include "Statistics.h"

#include "GLMath.h"

typedef struct StatisticSet_o
{
    uint8_t xType;
    uint8_t yType;

    unsigned int lineCount;

    uint32_t colors[MAX_LINES];
    void* dataPointBuffers[MAX_LINES];
    size_t dataPointCounter[MAX_LINES];
} StatisticSet;


StatisticSet* CreateStatisticSet(const uint8_t xType, const uint8_t yType)
{
    StatisticSet* ret = (StatisticSet*)malloc(sizeof(StatisticSet));

    ret->xType = xType;
    ret->yType = yType;
    ret->lineCount = 0;

    return ret;
}

void DeleteStatisticsSet(StatisticSet* set)
{
    for(unsigned int i = 0; (i < set->lineCount); i++)
        free((void*)(set->dataPointBuffers[i]));
    free((void*)set);
}


void AddLineLayout(StatisticSet* set, const uint32_t color, const size_t maximumDataPointCount)
{
    unsigned int lineCount = set->lineCount;

    set->colors[lineCount] = color;
    set->dataPointBuffers[lineCount] = malloc(maximumDataPointCount * ((set->xType >> 4) + (set->yType >> 4)));
    set->dataPointCounter[lineCount] = 0;

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
    const unsigned int graphPixelWidth = DATASET_IMAGE_WIDTH;
    const unsigned int graphPixelHeight = DATASET_IMAGE_HEIGHT;

    const float xSpan = xMax - xMin;
    const float ySpan = yMax - yMin;

    const unsigned int xByteSize = (set->xType >> 4);
    const unsigned int yByteSize = (set->yType >> 4);
    const unsigned int byteStride = xByteSize + yByteSize;

    uint32_t* retImg = (uint32_t*)malloc(sizeof(uint32_t) * (graphPixelWidth * graphPixelHeight));
    memset((void*)retImg, 0xFF, sizeof(uint32_t) * (graphPixelWidth * graphPixelHeight));

    for(unsigned int i = 0; i < set->lineCount; i++)
    {
        const unsigned int dataPointSize = set->dataPointCounter[i];
        const uint32_t color = set->colors[i];
        const void* srcBuffer = set->dataPointBuffers[i];

        unsigned int xDataPoint[4] = { 0 }; //TODO: iets wat de type goed zet
        float yDataPoint[4] = { 0 };

        float xPixelCoords[4] = { 0 };
        float yPixelCoords[4] = { 0 };
        for(unsigned int j = 0; j < (dataPointSize - 3); j++)
        {
            //translate the dataPoints to coordinates in the graphs space
            for(unsigned int k = 0; k < 4; k++)
            {
                memcpy(&(xDataPoint[k]), (srcBuffer + (byteStride * (j + k))), xByteSize);
                memcpy(&(yDataPoint[k]), (srcBuffer + ((byteStride * (j + k)) + xByteSize)), yByteSize);

                xPixelCoords[k] = (((float)(xDataPoint[k]) - xMin) / xSpan) * (float)graphPixelWidth;
                yPixelCoords[k] = (((float)(yDataPoint[k]) - yMin) / ySpan) * (float)graphPixelHeight;
            }

            float xSpot = 0.0;
            float ySpot = 0.0;
            for(float t = 0.0; t < 1.0; t += 0.001)
            {
                xSpot = CubicBezierSpline(xPixelCoords[0], xPixelCoords[1], xPixelCoords[2], xPixelCoords[3], t); //x
                ySpot = CubicBezierSpline(yPixelCoords[0], yPixelCoords[1], yPixelCoords[2], yPixelCoords[3], t); //y

                //check if these coordinates are within the graph
                if((ySpot < (float)DATASET_IMAGE_HEIGHT) && (ySpot >= 0.0) && (xSpot < (float)DATASET_IMAGE_WIDTH) && (xSpot >= 0.0))
                    *(retImg + (((unsigned int)ySpot * graphPixelHeight) + (unsigned int)xSpot)) = color;
            }
        }
    }

    return retImg;
}