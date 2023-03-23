#pragma once

#include <Common_Includes.h>

#define MAX_LINES   4

//highest 4 bits are size in bytes of data type, last 4 bits are just an ID for the type
#define STAT_FLOAT  0x40
#define STAT_UINT   0x41

typedef struct StatisticSet_o StatisticSet;

//TODO: 70x80 font implementeren op stats (na het inleveren van de opdracht eventueel)
//TODO: iets om ook nog uit te zoeken, het is waarschijnlijk mogelijk om met fourier transform een polynoom te genereren die elk punt op de grafiek exact kan berekenen met 1 formule, misschien iets om naar te kijken als de runtime het aan kan
//Dit kan ^, maar is erg onpraktisch hiervoor https://en.wikipedia.org/wiki/Polynomial_interpolation

StatisticSet* CreateStatisticSet(const uint8_t xType, const uint8_t yType);
void DeleteStatisticsSet(StatisticSet* set);

void AddLineLayout(StatisticSet* set, const uint32_t color, const size_t maximumDataPointCount); //color should be ARGB format
void AddDataPoint(StatisticSet* set, unsigned int Lineindex, const void* xData, const void* yData); //watch out: will seg fault if you add too many dataPoints

uint32_t* GenerateGraph(StatisticSet* set, const float xMin, const float xMax, const float yMin, const float yMax); //retruns graph image in ARGB format, dont forget to free the image