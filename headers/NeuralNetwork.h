#pragma once

#include <Common_Includes.h>

#include "GLMath.h"
#include "Texture.h"
#include "Shader.h"

#define INPUT_COUNT                         (DATASET_IMAGE_WIDTH * DATASET_IMAGE_HEIGHT)
#define OUTPUT_COUNT                        3
#define NEURONS_PER_HIDDEN_LAYER_COUNT      64
#define HIDDEN_LAYER_COUNT                  1 //minimum of 1 hidden layer

typedef struct NeuralNetwork
{
    float inputLayer[INPUT_COUNT];
    float hiddenLayers[HIDDEN_LAYER_COUNT][NEURONS_PER_HIDDEN_LAYER_COUNT];
    float outputLayer[OUTPUT_COUNT];

    float inputToFirstHiddenLayerWeights[NEURONS_PER_HIDDEN_LAYER_COUNT][INPUT_COUNT];
    float hiddenToHiddenLayerWeights[HIDDEN_LAYER_COUNT-1][NEURONS_PER_HIDDEN_LAYER_COUNT][NEURONS_PER_HIDDEN_LAYER_COUNT]; // structured like this: [hidden layer weights from left to right][the neurons in the layer to the RIGHT of these weights][all the weights connected to each of the before mentioned neurons]
    float lastHiddenToOutputLayerWeights[OUTPUT_COUNT][NEURONS_PER_HIDDEN_LAYER_COUNT];

    float hiddenLayersBiases[HIDDEN_LAYER_COUNT][NEURONS_PER_HIDDEN_LAYER_COUNT];
    float outputLayerBiases[OUTPUT_COUNT];
} NeuralNetwork;

//creates neural network on heap and initializes it with random floats between 0.0 and 1.0
NeuralNetwork* CreateNeuralNetwork();
void DeleteNeuralNetwork(NeuralNetwork* network);

//file IO
void SaveNeuralNetworkToFile(const NeuralNetwork* network, const char* fileName);
void LoadNeuralNetworkFromFile(NeuralNetwork* network, const char* fileName);

//calculate the cost of its output
float nCost(const NeuralNetwork* network, const unsigned int correctOutputIndex);

//try to predict the image without training the neural network, returns index to the predicted answer
unsigned int PeekImage(NeuralNetwork* network, const uint32_t* imageData);

//trains the neural network with one image specified by imageData
void Train(NeuralNetwork* network, ShaderProgram* computeShader, const uint32_t* imageData, const unsigned int correctOutputIndex);