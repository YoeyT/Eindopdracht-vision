#include "NeuralNetwork.h"

//TODO: debug global variable
unsigned int amountOfAnswers = 0;
unsigned int amountOfCorrectAnswers = 0;

static void BackPropagation(NeuralNetwork* network, ShaderProgram* computeShader, const unsigned int correctOutputIndex)
{
    const float nudgeFactor = 0.12; //TODO: deze kan in principe verwijderd worden, het bepaald hoe snel het algoritme naar dat lokale minimum toe streeft, i.e. Hoe snel gaat de gradient descent

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
    for(unsigned int i = 0; i < NEURONS_PER_HIDDEN_LAYER_COUNT; i++)
    {
        //Change the weights proportional to the output layer and last hidden layer
        float hiddenNeuronProportion = network->hiddenLayers[HIDDEN_LAYER_COUNT-1][i];
        float desiredInputValueProportion = 0.0;
        for(unsigned int j = 0; j < OUTPUT_COUNT; j++)
        {
            float desiredOutputValueProportion = network->outputLayer[j];
            desiredInputValueProportion += (network->lastHiddenToOutputLayerWeights[j][i] * desiredOutputValueProportion);
            network->lastHiddenToOutputLayerWeights[j][i] += (hiddenNeuronProportion * desiredOutputValueProportion * nudgeFactor);
        }

        network->hiddenLayers[HIDDEN_LAYER_COUNT-1][i] = desiredInputValueProportion;
    }

    //hidden layers to hidden layers
    for(int l = (HIDDEN_LAYER_COUNT-2); l >= 0; l--)
    {
        //bias
        for(unsigned int i = 0; i < NEURONS_PER_HIDDEN_LAYER_COUNT; i++)
            network->hiddenLayersBiases[l+1][i] += (network->hiddenLayers[l+1][i] * nudgeFactor); //TODO: als het nog niet werkt met meerdere layers, zet dit dan misschien uit

        //weights and desired inputs
        for(unsigned int i = 0; i < NEURONS_PER_HIDDEN_LAYER_COUNT; i++)
        {
            //Change the weights proportional to the left hidden layer (l) and the right hidden layer (l+1)
            float leftProportion = network->hiddenLayers[l][i];
            float desiredLeftProportion = 0.0;
            for(unsigned int j = 0; j < NEURONS_PER_HIDDEN_LAYER_COUNT; j++)
            {
                float rightProportion = network->hiddenLayers[l+1][j];
                desiredLeftProportion += (network->hiddenToHiddenLayerWeights[l][j][i] * rightProportion);
                network->hiddenToHiddenLayerWeights[l][j][i] += (leftProportion * rightProportion * nudgeFactor);
            }

            network->hiddenLayers[l][i] = desiredLeftProportion;
        }
    }

    //TODO: vind een manier om het neurale netwerk sneller te maken dmv compute shaders (zie onderstaande code)
    UseShaderProgramP(computeShader);

    //const unsigned int bufferBindingPoint = 0;
    //const unsigned int shaderID = GetID(computeShader);
    //unsigned int blockindex = glGetUniformBlockIndex(shaderID, "uWeights");
    //glUniformBlockBinding(shaderID, bufferBindingPoint, blockindex);

    //create buffer and send data to GPU
    // unsigned int buffer[10] = { 0 };

    // printf("error: %X\n", glGetError());

    // unsigned int ssboID;
    // glGenBuffers(1, &ssboID);
    //     printf("error: %X\n", glGetError());
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);
    //     printf("error: %X\n", glGetError());
    // glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(buffer), (void*)buffer, GL_DYNAMIC_COPY);
    //     printf("error: %X\n", glGetError());
    // glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboID);
    //     printf("error: %X\n", glGetError());
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // printf("error: %X\n", glGetError());

    // unsigned int blockIndex = glGetProgramResourceIndex(shaderID, GL_SHADER_STORAGE_BLOCK, "weights");
    // glShaderStorageBlockBinding(shaderID, blockIndex, 1);

    // glDispatchCompute(1, 1, 1);
    // glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // sleep(1);

    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboID);
    // void* p = glMapBufferRange(GL_SHADER_STORAGE_BLOCK, 0, sizeof(buffer), GL_MAP_READ_BIT);
    // memcpy(buffer, p, sizeof(buffer));
    // glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    // glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // for(unsigned int i = 0; i < 10; i++)
    //     printf("%d", buffer[i]);
    // printf("\n");

    //input layers to first hidden layer
    //first hidden layer biases
    for(unsigned int i = 0; i < NEURONS_PER_HIDDEN_LAYER_COUNT; i++)
        network->hiddenLayersBiases[0][i] += (network->hiddenLayers[0][i] * nudgeFactor);

    //weights
    for(unsigned int i = 0; i < NEURONS_PER_HIDDEN_LAYER_COUNT; i++)
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

// static void CalculateLayerValues(const unsigned int inputCount, const unsigned int outputCount, const float** weights, const float* inputLayer, const float* biases, float* outputLayer)
// {
//     for(unsigned int i = 0; i < outputCount; i++)
//     {
//         float weightedSum = 0.0;
//         for(unsigned int n = 0; n < inputCount; n++)
//             weightedSum += (weights[i][n] * inputLayer[n]);

//         outputLayer[i] = Sigmoid(weightedSum + biases[i]);
//     }
// }

NeuralNetwork* CreateNeuralNetwork()
{
    NeuralNetwork* ret = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));

    //initialize everything with random floats
    for(uint64_t i = 0; i < (sizeof(NeuralNetwork) >> 2); i++)
        *(((float*)ret) + i) = RandomFloat();

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


float nCost(const NeuralNetwork* network, const unsigned int correctOutputIndex)
{
    //calculate the cost of every output
    float costs[OUTPUT_COUNT];
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        costs[i] = powf(network->outputLayer[i], 2.0);
    costs[correctOutputIndex] = powf((network->outputLayer[correctOutputIndex] - 1.0), 2.0);

    float costTotal = 0.0;
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        costTotal += costs[i];

    return costTotal;
}

unsigned int PeekImage(NeuralNetwork* network, const uint32_t* imageData)
{
    //convert grayscaled image data to a float between 0 and 1
    for(unsigned int i = 0; i < INPUT_COUNT; i++)
        network->inputLayer[i] = ((float)((uint8_t)(imageData[i])) / 255.0);

    //TODO: veroorzaakt nog segfault
    // //input to first hidden layer
    // CalculateLayerValues( INPUT_COUNT, NEURONS_PER_HIDDEN_LAYER_COUNT, 
    //     (const float**)(network->inputToFirstHiddenLayerWeights), 
    //     (const float*)network->inputLayer,
    //     (const float*)&(network->hiddenLayersBiases[0]),
    //     (float*)&(network->hiddenLayers[0]) );

    // //hidden layers to hidden layers
    // for(unsigned int i = 1; i < HIDDEN_LAYER_COUNT; i++)
    // {
    //     CalculateLayerValues( NEURONS_PER_HIDDEN_LAYER_COUNT, NEURONS_PER_HIDDEN_LAYER_COUNT, 
    //         (const float**)&(network->hiddenToHiddenLayerWeights[i-1]),
    //         (const float*)&(network->hiddenLayers[i-1]),
    //         (const float*)&(network->hiddenLayersBiases[i]),
    //         (float*)&(network->hiddenLayers[i]) );
    // }

    // //last hidden layer to output layer
    // CalculateLayerValues( NEURONS_PER_HIDDEN_LAYER_COUNT, OUTPUT_COUNT, 
    //     (const float**)(network->lastHiddenToOutputLayerWeights), 
    //     (const float*)&(network->hiddenLayers[HIDDEN_LAYER_COUNT-1]),
    //     (const float*)(network->outputLayerBiases),
    //     (float*)(network->outputLayer) );


    //input to first hidden layer
    for(unsigned int i = 0; i < NEURONS_PER_HIDDEN_LAYER_COUNT; i++)
    {
        float weightedSum = 0.0;
        for(unsigned int n = 0; n < INPUT_COUNT; n++)
            weightedSum += (network->inputToFirstHiddenLayerWeights[i][n] * network->inputLayer[n]);

        network->hiddenLayers[0][i] = Sigmoid(weightedSum + (network->hiddenLayersBiases[0][i]));
    }

    //hidden layers to hidden layers
    for(unsigned int l = 1; l < HIDDEN_LAYER_COUNT; l++)
    {
        for(unsigned int i = 0; i < NEURONS_PER_HIDDEN_LAYER_COUNT; i++)
        {
            float weightedSum = 0.0;
            for(unsigned int j = 0; j < NEURONS_PER_HIDDEN_LAYER_COUNT; j++)
                weightedSum += (network->hiddenToHiddenLayerWeights[l-1][i][j] * network->hiddenLayers[l-1][j]);

            network->hiddenLayers[l][i] = Sigmoid(weightedSum + (network->hiddenLayersBiases[l][i]));
        }
    }

    //last hidden layer to output layer
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
    {
        float weightedSum = 0.0;
        for(unsigned int n = 0; n < NEURONS_PER_HIDDEN_LAYER_COUNT; n++)
            weightedSum += (network->lastHiddenToOutputLayerWeights[i][n] * network->hiddenLayers[HIDDEN_LAYER_COUNT-1][n]);

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

float Train(NeuralNetwork* network, ShaderProgram* computeShader, const uint32_t* imageData, const unsigned int correctOutputIndex)
{
    unsigned int highestValueIndex = PeekImage(network, imageData);
    float cost = nCost(network, correctOutputIndex);
    BackPropagation(network, computeShader, correctOutputIndex);

    return cost;

    //TODO: DEBUG
    amountOfAnswers++;
    if(highestValueIndex == correctOutputIndex)
        amountOfCorrectAnswers++;

    if((amountOfAnswers % 1000) == 0)
        printf("cost: %.2f\taccuracy: %.2f%%\timages Trained: %d\n", cost, (((float)amountOfCorrectAnswers / (float)amountOfAnswers) * 100.0), amountOfAnswers); // the lower the cost, the better the answer
    //DEBUG
}