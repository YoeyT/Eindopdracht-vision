#pragma once

#include <Common_Includes.h>

#include "GLMath.h"
#include "Texture.h"
#include "Shader.h"

#define INPUT_COUNT                         (DATASET_IMAGE_WIDTH * DATASET_IMAGE_HEIGHT)
#define OUTPUT_COUNT                        10

#define MAX_HIDDEN_LAYER_COUNT              8
#define MAX_NEURON_PER_HIDDEN_LAYER_COUNT   256


typedef struct NeuralNetwork
{
    float inputLayer[INPUT_COUNT];
    float hiddenLayers[MAX_HIDDEN_LAYER_COUNT][MAX_NEURON_PER_HIDDEN_LAYER_COUNT];
    float outputLayer[OUTPUT_COUNT];

    float inputToFirstHiddenLayerWeights[MAX_NEURON_PER_HIDDEN_LAYER_COUNT][INPUT_COUNT];
    float hiddenToHiddenLayerWeights[MAX_HIDDEN_LAYER_COUNT-1][MAX_NEURON_PER_HIDDEN_LAYER_COUNT][MAX_NEURON_PER_HIDDEN_LAYER_COUNT]; // structured like this: [hidden layer weights from left to right][the neurons in the layer to the RIGHT of these weights][all the weights connected to each of the before mentioned neurons]
    float lastHiddenToOutputLayerWeights[OUTPUT_COUNT][MAX_NEURON_PER_HIDDEN_LAYER_COUNT];

    float hiddenLayersBiases[MAX_HIDDEN_LAYER_COUNT][MAX_NEURON_PER_HIDDEN_LAYER_COUNT];
    float outputLayerBiases[OUTPUT_COUNT];

    float nudgeValue; //this determines how fast the backpropagation strides towards the local minimum, in a gradient descent sense
    unsigned int hiddenLayerCount;
    unsigned int neuronsPerHiddenLayers[MAX_HIDDEN_LAYER_COUNT]; //index 0 is always the first hidden layer from input to output
} NeuralNetwork;

//creates neural network on heap and initializes it with random floats between 0.0 and 1.0
NeuralNetwork* CreateNeuralNetwork(unsigned int* neuronsPerHiddenLayersList, const unsigned int hiddenLayerCount, const float nudgeValue);
void DeleteNeuralNetwork(NeuralNetwork* network);

//file IO
void SaveNeuralNetworkToFile(const NeuralNetwork* network, const char* fileName);
void LoadNeuralNetworkFromFile(NeuralNetwork* network, const char* fileName);

//calculate the loss of its output
float nLoss(const NeuralNetwork* network, const unsigned int correctOutputIndex);

//try to predict the image without training the neural network, returns index to the predicted answer
unsigned int PeekImage(NeuralNetwork* network, const uint32_t* imageData);

//trains the neural network with one image specified by imageData, returns the loss
float Train(NeuralNetwork* network, const uint32_t* imageData, const unsigned int correctOutputIndex);