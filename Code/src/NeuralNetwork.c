#include "NeuralNetwork.h"

static void BackPropagation(NeuralNetwork* network, const unsigned int correctOutputIndex)
{
    const float nudgeFactor = network->nudgeValue;

    //desired nudges in output
    const float correctOutput = network->outputLayer[correctOutputIndex];
    //wrong answers
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        network->outputLayer[i] = -(network->outputLayer[i]);
    
    //right answer
    network->outputLayer[correctOutputIndex] = (1.0 - correctOutput);

    //last hidden layer to output layer weights and biases
    //bias
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        network->outputLayerBiases[i] += (network->outputLayer[i] * nudgeFactor);

    //weights and desired inputs
    unsigned int hiddenLayerCount = network->hiddenLayerCount;
    unsigned int lastHiddenLayerNeuronCount = network->neuronsPerHiddenLayers[hiddenLayerCount - 1];
    for(unsigned int i = 0; i < lastHiddenLayerNeuronCount; i++)
    {
        //Change the weights proportional to the output layer and last hidden layer
        float hiddenNeuronProportion = network->hiddenLayers[hiddenLayerCount-1][i];
        float desiredInputValueProportion = 0.0;
        for(unsigned int j = 0; j < OUTPUT_COUNT; j++)
        {
            float desiredOutputValueProportion = network->outputLayer[j];
            desiredInputValueProportion += (network->lastHiddenToOutputLayerWeights[j][i] * desiredOutputValueProportion);
            network->lastHiddenToOutputLayerWeights[j][i] += (hiddenNeuronProportion * desiredOutputValueProportion * nudgeFactor);
        }

        network->hiddenLayers[hiddenLayerCount-1][i] = desiredInputValueProportion;
    }

    //hidden layers to hidden layers
    for(int l = (hiddenLayerCount-2); l >= 0; l--)
    {
        unsigned int leftHiddenLayerNeuronCount = network->neuronsPerHiddenLayers[l];
        unsigned int rightHiddenLayerNeuronCount = network->neuronsPerHiddenLayers[l+1];

        //bias
        for(unsigned int i = 0; i < rightHiddenLayerNeuronCount; i++)
            network->hiddenLayersBiases[l+1][i] += (network->hiddenLayers[l+1][i] * nudgeFactor);

        //weights and desired inputs
        for(unsigned int i = 0; i < leftHiddenLayerNeuronCount; i++)
        {
            //Change the weights proportional to the left hidden layer (l) and the right hidden layer (l+1)
            float leftProportion = network->hiddenLayers[l][i];
            float desiredLeftProportion = 0.0;
            for(unsigned int j = 0; j < rightHiddenLayerNeuronCount; j++)
            {
                float rightProportion = network->hiddenLayers[l+1][j];
                desiredLeftProportion += (network->hiddenToHiddenLayerWeights[l][j][i] * rightProportion);
                network->hiddenToHiddenLayerWeights[l][j][i] += (leftProportion * rightProportion * nudgeFactor);
            }

            network->hiddenLayers[l][i] = desiredLeftProportion;
        }
    }

    //input layers to first hidden layer
    unsigned int firstHiddenLayerNeuronCount = network->neuronsPerHiddenLayers[0];

    //first hidden layer biases
    for(unsigned int i = 0; i < firstHiddenLayerNeuronCount; i++)
        network->hiddenLayersBiases[0][i] += (network->hiddenLayers[0][i] * nudgeFactor);

    //weights
    for(unsigned int i = 0; i < firstHiddenLayerNeuronCount; i++)
    {
        //Change the weights proportional to the output layer and last hidden neuron layer
        float desiredOutputProportion = network->hiddenLayers[0][i];
        for(unsigned int j = 0; j < INPUT_COUNT; j++)
        {
            float inputNeuronProportion = network->inputLayer[j];
            network->inputToFirstHiddenLayerWeights[i][j] += (inputNeuronProportion * desiredOutputProportion * nudgeFactor);
        }
    }
}


NeuralNetwork* CreateNeuralNetwork(unsigned int* neuronsPerHiddenLayersList, const unsigned int hiddenLayerCount, const float nudgeValue)
{
    if((hiddenLayerCount == 0) || (hiddenLayerCount > MAX_HIDDEN_LAYER_COUNT))
        return NULL;

    NeuralNetwork* ret = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));

    //initialize everything with random floats
    for(size_t i = 0; i < (sizeof(NeuralNetwork) >> 2); i++)
        *(((float*)ret) + i) = RandomFloat();

    for(unsigned int i = 0; i < hiddenLayerCount; i++)
        ret->neuronsPerHiddenLayers[i] = neuronsPerHiddenLayersList[i];
    ret->hiddenLayerCount = hiddenLayerCount;
    ret->nudgeValue = nudgeValue;

    return ret;
}

void DeleteNeuralNetwork(NeuralNetwork* network)
{
    free((void*)network);
}


void SaveNeuralNetworkToFile(const NeuralNetwork* network, const char* fileName)
{
    FILE* file = fopen(fileName, "wb");
    if(file != NULL)
    {
        fwrite((void*)network, sizeof(NeuralNetwork), 1, file);
        fclose(file);
    }
}

void LoadNeuralNetworkFromFile(NeuralNetwork* network, const char* fileName)
{
    FILE* file = fopen(fileName, "rb");
    if(file != NULL)
    {
        fread((void*)network, sizeof(NeuralNetwork), 1, file);
        fclose(file);
    }
}


float nLoss(const NeuralNetwork* network, const unsigned int correctOutputIndex)
{
    //calculate the loss of every output
    float squaredError[OUTPUT_COUNT];
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        squaredError[i] = powf(network->outputLayer[i], 2.0);
    squaredError[correctOutputIndex] = powf((network->outputLayer[correctOutputIndex] - 1.0), 2.0);

    float lossTotal = 0.0;
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        lossTotal += squaredError[i];

    return (lossTotal / (float)OUTPUT_COUNT);
}

unsigned int PeekImage(NeuralNetwork* network, const uint32_t* imageData)
{
    //convert grayscaled image data to a float between 0 and 1
    for(unsigned int i = 0; i < INPUT_COUNT; i++)
        network->inputLayer[i] = ((float)((uint8_t)(imageData[i])) / 255.0);

    //input to first hidden layer
    unsigned int firstHiddenLayerNeuronCount = network->neuronsPerHiddenLayers[0];
    for(unsigned int i = 0; i < firstHiddenLayerNeuronCount; i++)
    {
        float weightedSum = 0.0;
        for(unsigned int n = 0; n < INPUT_COUNT; n++)
            weightedSum += (network->inputToFirstHiddenLayerWeights[i][n] * network->inputLayer[n]);

        network->hiddenLayers[0][i] = Sigmoid(weightedSum + (network->hiddenLayersBiases[0][i]));
    }

    //hidden layers to hidden layers
    unsigned int hiddenLayerCount = network->hiddenLayerCount;
    for(unsigned int l = 1; l < hiddenLayerCount; l++)
    {
        unsigned int leftHiddenLayerNeuronCount = network->neuronsPerHiddenLayers[l-1];
        unsigned int rightHiddenLayerNeuronCount = network->neuronsPerHiddenLayers[l];
        for(unsigned int i = 0; i < rightHiddenLayerNeuronCount; i++)
        {
            float weightedSum = 0.0;
            for(unsigned int j = 0; j < leftHiddenLayerNeuronCount; j++)
                weightedSum += (network->hiddenToHiddenLayerWeights[l-1][i][j] * network->hiddenLayers[l-1][j]);

            network->hiddenLayers[l][i] = Sigmoid(weightedSum + (network->hiddenLayersBiases[l][i]));
        }
    }

    //last hidden layer to output layer
    unsigned int lastHiddenLayerNeuronCount = network->neuronsPerHiddenLayers[hiddenLayerCount - 1];
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
    {
        float weightedSum = 0.0;
        for(unsigned int n = 0; n < lastHiddenLayerNeuronCount; n++)
            weightedSum += (network->lastHiddenToOutputLayerWeights[i][n] * network->hiddenLayers[hiddenLayerCount-1][n]);

        network->outputLayer[i] = Sigmoid(weightedSum + (network->outputLayerBiases[i]));
    }

    //highest value in output layer
    float highestValue = 0.0;
    unsigned int highestValueIndex = 0;
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
    {
        if(network->outputLayer[i] > highestValue)
        {
            highestValue = network->outputLayer[i];
            highestValueIndex = i;
        }
    }
    return highestValueIndex;
}

float Train(NeuralNetwork* network, const uint32_t* imageData, const unsigned int correctOutputIndex)
{
    PeekImage(network, imageData);
    float loss = nLoss(network, correctOutputIndex);
    BackPropagation(network, correctOutputIndex);

    return loss;
}