#include "StateMachine.h"

#include "dirent.h"

#include "Texture.h"
#include "UserInput.h"
#include "NeuralNetwork.h"
#include "Statistics.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" //het fijne hieraan is dat niks extern is (ik gebruik het bovendien alleen om plaatjes in te lezen)
//credits gaan naar alle bijdragers: https://github.com/nothings/stb

/*============================ arguments ============================*/

typedef struct TrainingArgs
{
    //neural net args
    unsigned int neuronsPerLayer[MAX_HIDDEN_LAYER_COUNT];
    unsigned int LayerCount;
    float nudgeFactor;
} TrainingArgs;



GLFWwindow* Init()
{
    GLFWwindow* window;

    if (!glfwInit())
        return NULL;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //windowed screen
    window = glfwCreateWindow((DATASET_IMAGE_WIDTH * 2.0), DATASET_IMAGE_HEIGHT, "vision project", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        return NULL;
    }

    glfwMakeContextCurrent(window);

    glfwSwapInterval(0); //comment this to lock FPS to moniter refresh rate

    GLenum err = glewInit();
    if (err != GLEW_OK)
        return NULL;
    
    return window;
}

void StartStateMachine()
{
    State s = { Start };
    while(s.nextFn) { s.nextFn(&s); }
}


/*============================ states ============================*/

void Start(State* s)
{
    //initialize a window
    GLFWwindow* window = Init();
    if(window == NULL)
    {
        s->nextFn = NULL;
        return;
    }
    s->window = window;
    glfwSetWindowUserPointer(window, (void*)s);
    SetResizeCallback(window);
    stbi_set_flip_vertically_on_load(true); //OpenGL expects 0, 0 to be bottom left

    //shader library prep
    const char* shaderFilePaths[2] = { "res/shaders/basicShader.fragment", "res/shaders/basicShader.vertex" };
    const char* args[2] = { "utexSlots", "uOrtProjMat" };
    ShaderLib* shaders =  CreateShaderLibrary(shaderFilePaths, 2);
    AddVarsToCache(GetShader(shaders, 0), args, 2);
    s->shaderLib = shaders;

    //texture prep
    int usedTexSlots[3] = { 0, 1, 2 };
    LoadUniformVarInts(GetShader(s->shaderLib, 0), 0, 3, usedTexSlots);

    Texture wit = { 1, 1, 0, "res/textures/white.data" };
    LoadTexGPU(&wit, NULL); //basically reserve space in GPU
    LoadTexRawData(&wit);

    //view matrices
    Mat4x4 projMat = OrthographicMat4x4(0.0, ((float)DATASET_IMAGE_WIDTH * 2.0), 0.0, (float)DATASET_IMAGE_HEIGHT, -1.0, 1.0);
    Mat4x4 viewMat = TranslationMat4x4(0.0, 0.0, 0.0);
    Mat4x4 final = MultiplyMat4x4(projMat, viewMat);
    LoadUniformMat4x4(GetShader(shaders, 0), 1, &final);

    //vertex and index buffers
    Vertex2D fPositions[2][4] = 
    {
        { //image vertices
            { 0.0, 0.0,                                                 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0 },
            { (float)DATASET_IMAGE_WIDTH, 0.0,                          1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0 },
            { (float)DATASET_IMAGE_WIDTH, (float)DATASET_IMAGE_HEIGHT,  1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 },
            { 0.0, (float)DATASET_IMAGE_HEIGHT,                         0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 }
        },
        { //graph vertices
            { (float)DATASET_IMAGE_WIDTH, 0.0,                                  0.0, 0.0, 2.0, 1.0, 1.0, 1.0, 1.0 },
            { ((float)DATASET_IMAGE_WIDTH * 2.0), 0.0,                          1.0, 0.0, 2.0, 1.0, 1.0, 1.0, 1.0 },
            { ((float)DATASET_IMAGE_WIDTH * 2.0), (float)DATASET_IMAGE_HEIGHT,  1.0, 1.0, 2.0, 1.0, 1.0, 1.0, 1.0 },
            { (float)DATASET_IMAGE_WIDTH, (float)DATASET_IMAGE_HEIGHT,          0.0, 1.0, 2.0, 1.0, 1.0, 1.0, 1.0 }
        }
    };

    const size_t staticAllocSize = 2;
    const uint16_t dynamicAllocSize = 0;
    VertexBuffer* vBuffer = CreateVertexBuffer((staticAllocSize * 4), (dynamicAllocSize * 4));
    IndexBuffer* iBuffer = CreateIndexBuffer((staticAllocSize * 6), (dynamicAllocSize * 6));

    initVAO();
    SetStaticVertexData(vBuffer, (Vertex2D*)fPositions);
    StaticToGPU(vBuffer, iBuffer);

    s->vBuffer = vBuffer;
    s->iBuffer = iBuffer;

    //training arguments init
    TrainingArgs* tArgs = (TrainingArgs*)malloc(sizeof(TrainingArgs));
    tArgs->neuronsPerLayer[0] = 50;
    tArgs->neuronsPerLayer[1] = 1;
    tArgs->LayerCount = 2;
    tArgs->nudgeFactor = 0.1;
    s->data1 = (void*)tArgs;

    //statistics setup
    StatisticSet* stat = CreateStatisticSet(STAT_UINT, STAT_FLOAT, 10.0, 0.01);
    AddLineLayout(stat, 0xFF008000, MAX_NEURON_PER_HIDDEN_LAYER_COUNT, 11);
    s->data3 = (void*)stat;

    s->nextFn = Training;
}

void Quit(State* s)
{
    DeleteStatisticsSet((void*)(s->data3));
    free((void*)(s->data1));
    DeleteIndexBuffer(s->iBuffer);
    DeleteVertexBuffer(s->vBuffer);
    DeleteShaderLib(s->shaderLib);
    glfwTerminate();

    s->nextFn = NULL;
}


/*============================ neural network related states ============================*/

void Training(State* s)
{
    GLFWwindow* window = s->window;
    ShaderProgram* basicShader = GetShader(s->shaderLib, 0);
    IndexBuffer* iBuffer = s->iBuffer;
    TrainingArgs* tArgs = (TrainingArgs*)(s->data1);

    SetKeyPressCallback(window);

    Texture image = { DATASET_IMAGE_WIDTH, DATASET_IMAGE_HEIGHT, 1 };
    LoadTexGPU(&image, NULL); //basically reserve space on GPU

    srand(0); //zorgt ervoor dat alles binnen redelijk grenzen deterministisch blijft

    //directory strings
    const char* trainingPath = "res/dataSets/mnist/trainingSet";
    const char* dirLUT[OUTPUT_COUNT] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    // const char* trainingPath = "res/dataSets/shapes";
    // const char* dirLUT[OUTPUT_COUNT] = { "circle", "square", "star", "triangle" };
    // const char* trainingPath = "res/dataSets/animals/train";
    // const char* dirLUT[OUTPUT_COUNT] = { "cat", "dog", "wild" };

    //directory parse code
    char buffer[128];
    image.filePath = buffer;
    DIR* dirs[OUTPUT_COUNT];
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
    {
        snprintf(buffer, 128, "%s/%s", trainingPath, dirLUT[i]);
        dirs[i] = opendir(buffer);
    }
    struct dirent *fileDir;

    //convolution testing
    //uint32_t* convolvedImage = (uint32_t*)malloc(sizeof(uint32_t) * (DATASET_IMAGE_WIDTH * DATASET_IMAGE_HEIGHT));
    //const float sobelKernelV[3][3] = { { -1.0, 0.0, 1.0 }, { -2.0, 0.0, 2.0 }, { -1.0, 0.0, 1.0 } };
    //const float sobelKernelH[3][3] = { { -1.0, -2.0, -1.0 }, { 0.0, 0.0, 0.0 }, { 1.0, 2.0, 1.0 } };
    //const float sobelKernelD[3][3] = { { -2.0, -1.0, 0.0 }, { -1.0, 0.0, 1.0 }, { 0.0, 1.0, 2.0 } };
    //const float sobelKernelHV[3][3] = { { -0.1, -1.0, 0.0 }, { -1.0, 0.0, 1.0 }, { 0.0, 1.0, 0.1 } };
    //const float sharpenKernel[3][3] = { { 0.0, -1.0, 0.0 }, { -1.0, 5.0, -1.0 }, { 0.0, -1.0, 0.0 } };

    NeuralNetwork* network = CreateNeuralNetwork(tArgs->neuronsPerLayer, tArgs->LayerCount, tArgs->nudgeFactor);

    unsigned int imagesTrained = 0;
    const unsigned int amountOfImagesToTrain = 35000;
    double beginTimer = glfwGetTime();
    while(!glfwWindowShouldClose(window) && (s->flags != 0x01) && (imagesTrained < amountOfImagesToTrain))
    {
        Clear();

        int randomDirIndex = (rand() % OUTPUT_COUNT);
        if((fileDir = readdir(dirs[randomDirIndex])) != NULL)
        {
            snprintf(buffer, 128, "%s/%s/%s", trainingPath, dirLUT[randomDirIndex], fileDir->d_name);
            stbi_uc* data = stbi_load(image.filePath, &image.width, &image.height, NULL, 4);
            if(data != NULL)
            {
                ConvertImageGrayScale((uint32_t*)data, image.width, image.height); //niet nodig met de mnist/shapes dataset natuurlijk

                //ConvolveImageKern3x3((uint32_t*)data, convolvedImage, image.width, image.height, sobelKernelH);

                LoadSubTexGPU(&image, 0, 0, (uint32_t*)data);
                Train(network, (uint32_t*)data, randomDirIndex);
                imagesTrained++;

                //printf("\r%d%% done", (unsigned int)(((float)imagesTrained / (float)amountOfImagesToTrain) * 100.0));

                stbi_image_free((void*)data);
            }
        }

        Draw(iBuffer, basicShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    printf("\ntrained %d images in %.2f minutes\n", imagesTrained, ((glfwGetTime() - beginTimer) / 60.0));

    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        closedir(dirs[i]);

    if((s->flags != 0x01) && (imagesTrained != amountOfImagesToTrain))
    {
        DeleteNeuralNetwork(network);
        s->nextFn = Quit;
    }
    else
    {
        s->data2 = (void*)network;
        s->nextFn = Testing;
    }
    s->flags = 0;
}


void Testing(State* s)
{
    GLFWwindow* window = s->window;
    ShaderProgram* basicShader = GetShader(s->shaderLib, 0);
    IndexBuffer* iBuffer = s->iBuffer;
    TrainingArgs* tArgs = (TrainingArgs*)(s->data1);
    NeuralNetwork* network = (NeuralNetwork*)(s->data2);
    StatisticSet* stat = (StatisticSet*)(s->data3);

    Texture image = { DATASET_IMAGE_WIDTH, DATASET_IMAGE_HEIGHT, 1 };
    Texture graph = { GRAPH_IMAGE_WIDTH, GRAPH_IMAGE_HEIGHT, 2 };
    LoadTexGPU(&graph, NULL);

    //directory strings
    const char* testingPath = "res/dataSets/mnist/trainingSet";
    const char* dirLUT[OUTPUT_COUNT] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    // const char* testingPath = "res/dataSets/shapes";
    // const char* dirLUT[OUTPUT_COUNT] = { "circle", "square", "star", "triangle" };
    // const char* testingPath = "res/dataSets/animals/val";
    // const char* dirLUT[OUTPUT_COUNT] = { "cat", "dog", "wild" };

    //directory parse code
    char buffer[128];
    image.filePath = buffer;
    DIR* dirs[OUTPUT_COUNT];
    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
    {
        snprintf(buffer, 128, "%s/%s", testingPath, dirLUT[i]);
        dirs[i] = opendir(buffer);
    }
    struct dirent *fileDir;

    //uint32_t* convolvedImage = (uint32_t*)malloc(sizeof(uint32_t) * (DATASET_IMAGE_WIDTH * DATASET_IMAGE_HEIGHT));
    //const float sobelKernelV[3][3] = { { -1.0, 0.0, 1.0 }, { -2.0, 0.0, 2.0 }, { -1.0, 0.0, 1.0 } };
    //const float sobelKernelH[3][3] = { { -1.0, -2.0, -1.0 }, { 0.0, 0.0, 0.0 }, { 1.0, 2.0, 1.0 } };
    //const float sobelKernelD[3][3] = { { -2.0, -1.0, 0.0 }, { -1.0, 0.0, 1.0 }, { 0.0, 1.0, 2.0 } };
    //const float sobelKernelHV[3][3] = { { -0.1, -1.0, 0.0 }, { -1.0, 0.0, 1.0 }, { 0.0, 1.0, 0.1 } };
    //const float sharpenKernel[3][3] = { { 0.0, -1.0, 0.0 }, { -1.0, 5.0, -1.0 }, { 0.0, -1.0, 0.0 } };

    unsigned int amountOfImagesToTest = 42000;
    unsigned int testedImages = 0;
    unsigned int correctAnswers = 0;
    float totalLoss = 0.0;
    double beginTimer = glfwGetTime();
    while(!glfwWindowShouldClose(window) && (testedImages < amountOfImagesToTest))
    {
        Clear();

        int randomDirIndex = (rand() % OUTPUT_COUNT);
        if((fileDir = readdir(dirs[randomDirIndex])) != NULL)
        {
            snprintf(buffer, 128, "%s/%s/%s", testingPath, dirLUT[randomDirIndex], fileDir->d_name);
            stbi_uc* data = stbi_load(image.filePath, &image.width, &image.height, NULL, 4);
            if(data != NULL)
            {
                ConvertImageGrayScale((uint32_t*)data, image.width, image.height); //not needed for the shapes/mnist datasets
                
                //ConvolveImageKern3x3((uint32_t*)data, convolvedImage, image.width, image.height, sobelKernelH);

                LoadSubTexGPU(&image, 0, 0, (uint32_t*)data);
                unsigned int highestValueIndex = PeekImage(network, (uint32_t*)data);
                totalLoss += nLoss(network, randomDirIndex);

                testedImages++;
                if(highestValueIndex == randomDirIndex)
                    correctAnswers++;

                stbi_image_free((void*)data);
            }
        }
        Draw(iBuffer, basicShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    float averageLoss = (totalLoss / (float)testedImages);
    unsigned int hiddenLayerNeuronCount = tArgs->neuronsPerLayer[1];
    unsigned int nudgeFactor = (unsigned int)(tArgs->nudgeFactor * 100.0);
    AddDataPoint(stat, 0, (void*)(&hiddenLayerNeuronCount), (void*)(&averageLoss));

    printf("neurons in second hidden layer: %d, total layers: %d, nudge factor: %.2f\n", hiddenLayerNeuronCount, (tArgs->LayerCount + 2), tArgs->nudgeFactor);
    printf("average Loss: %.3f, accuracy: %.2f%%, images peeked: %d, in %.2f minutes\n", averageLoss, (((float)correctAnswers / (float)testedImages) * 100.0), testedImages, ((glfwGetTime() - beginTimer) / 60.0));

    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        closedir(dirs[i]);
    DeleteNeuralNetwork(network);

    if(tArgs->neuronsPerLayer[1] < MAX_NEURON_PER_HIDDEN_LAYER_COUNT)
    {
        tArgs->neuronsPerLayer[1] += 1;
        //tArgs->nudgeFactor += 0.01;
        s->nextFn = Training;
    }
    else
    {
        uint32_t* graphImg = GenerateGraph(stat, 0.0, (float)256, 0.0, 0.1);
        LoadSubTexGPU(&graph, 0, 0, graphImg);
        free((void*)graphImg);

        while(!glfwWindowShouldClose(window))
        {
            Clear();
            Draw(iBuffer, basicShader);
            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        s->nextFn = Quit;
    }
}