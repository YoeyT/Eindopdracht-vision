#include "StateMachine.h"

#include "dirent.h"
#include <unistd.h> //TODO: tijdelijk

#include "Texture.h"
#include "UserInput.h"
#include "NeuralNetwork.h"
#include "Statistics.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" //het fijne hieraan is dat niks extern is (ik gebruik het bovendien alleen om plaatjes in te lezen)
//credits gaan naar alle bijdragers: https://github.com/nothings/stb

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

    //shader library prep
    const char* shaderFilePaths[2] = { "res/shaders/basicShader.fragment", "res/shaders/basicShader.vertex" };
    const char* args[2] = { "utexSlots", "uOrtProjMat" };
    const char* computeShaderFilePath[1] = { "res/shaders/nnBackPropagation.compute" };
    ShaderLib* shaders =  CreateShaderLibrary(shaderFilePaths, 2);
    AppendShader(shaders, computeShaderFilePath, 1);
    AddVarsToCache(GetShader(shaders, 0), args, 2);
    s->shaderLib = shaders;


    //texture prep
    int usedTexSlots[3] = { 0, 1, 2 };
    LoadUniformVarInts(GetShader(s->shaderLib, 0), 0, 3, usedTexSlots);

    Texture wit = { 1, 1, 0, "res/textures/white.data" };
    LoadTexGPU(&wit, NULL); //basically reserve space in GPU
    LoadTexRawData(&wit);


    //vertex and index buffer
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

    s->nextFn = Training;
}

void Quit(State* s)
{
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
    ShaderProgram* computeShader = GetShader(s->shaderLib, 1);
    IndexBuffer* iBuffer = s->iBuffer;

    Mat4x4 projMat = OrthographicMat4x4(0.0, ((float)DATASET_IMAGE_WIDTH * 2.0), 0.0, (float)DATASET_IMAGE_HEIGHT, -1.0, 1.0);
    Mat4x4 viewMat = TranslationMat4x4(0.0, 0.0, 0.0);
    Mat4x4 final;

    SetKeyPressCallback(window);

    Texture image = { DATASET_IMAGE_WIDTH, DATASET_IMAGE_HEIGHT, 1 };
    Texture graph = { DATASET_IMAGE_WIDTH, DATASET_IMAGE_HEIGHT, 2 };
    LoadTexGPU(&image, NULL); //basically reserve space on GPU
    LoadTexGPU(&graph, NULL);

    srand(0); //zorgt ervoor dat alles binnen redelijk grenzen deterministisch blijft
    Vec2f cameraPos = { 0.0f, 0.0f };

    //directory strings //TODO: debug code, remove later
    // const char* trainingPath = "res/dataSets/mnist/trainingSet";
    // const char* dirLUT[OUTPUT_COUNT] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    // const char* trainingPath = "res/dataSets/shapes";
    // const char* dirLUT[OUTPUT_COUNT] = { "circle", "square", "star", "triangle" };
    const char* trainingPath = "res/dataSets/animals/train";
    const char* dirLUT[OUTPUT_COUNT] = { "cat", "dog", "wild" };

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

    stbi_set_flip_vertically_on_load(true);

    //convolution testing
    //uint32_t* convolvedImage = (uint32_t*)malloc(sizeof(uint32_t) * (DATASET_IMAGE_WIDTH * DATASET_IMAGE_HEIGHT));
    //const float edgeKernelV[3][3] = { { -0.2, 0.0, 0.2 }, { -1.0, 0.0, 1.0 }, { -0.2, 0.0, 0.2 } };
    //const float edgeKernelH[3][3] = { { -0.2, -1.0, -0.2 }, { 0.0, 0.0, 0.0 }, { 0.2, 1.0, 0.2 } };
    //const float edgeKernelHV[3][3] = { { -0.1, -1.0, 0.0 }, { -1.0, 0.0, 1.0 }, { 0.0, 1.0, 0.1 } };
    //const float sharpenKernel[3][3] = { { 0.0, -1.0, 0.0 }, { -1.0, 5.0, -1.0 }, { 0.0, -1.0, 0.0 } };

    NeuralNetwork* network = CreateNeuralNetwork();
    StatisticSet* stat = CreateStatisticSet(STAT_UINT, STAT_FLOAT);

    unsigned int imagesTrained = 0;
    const unsigned int amountOfImagesToTrain = 7000;
    float totalCost = 0.0;
    float averageCost = 0.0;
    AddLineLayout(stat, 0xFF000000, amountOfImagesToTrain);

    double beginTimer = glfwGetTime();
    while(!glfwWindowShouldClose(window) && (s->flags != 0x01) && (imagesTrained < amountOfImagesToTrain))
    {
        Clear();

        SetTransformMat4x4(&viewMat, cameraPos.x, cameraPos.y, 0.0);
        final = MultiplyMat4x4(projMat, viewMat);
        LoadUniformMat4x4(basicShader, 1, &final);

        int randomDirIndex = (rand() % OUTPUT_COUNT);
        if((fileDir = readdir(dirs[randomDirIndex])) != NULL)
        {
            snprintf(buffer, 128, "%s/%s/%s", trainingPath, dirLUT[randomDirIndex], fileDir->d_name);
            stbi_uc* data = stbi_load(image.filePath, &image.width, &image.height, NULL, 4);
            if(data != NULL)
            {
                ConvertImageGrayScale((uint32_t*)data, image.width, image.height); //niet nodig met de mnist/shapes dataset natuurlijk

                //ConvolveImageKern3x3((uint32_t*)data, convolvedImage, image.width, image.height, edgeKernelHV);

                LoadSubTexGPU(&image, 0, 0, (uint32_t*)data);
                totalCost += Train(network, computeShader, (uint32_t*)data, randomDirIndex);
                imagesTrained++;
                averageCost = totalCost / (float)imagesTrained;

                AddDataPoint(stat, 0, (void*)(&imagesTrained), (void*)(&averageCost));

                printf("\r%d%% done", (unsigned int)(((float)imagesTrained / (float)amountOfImagesToTrain) * 100.0));

                stbi_image_free((void*)data);
            }
        }

        Draw(iBuffer, basicShader);

        //DynamicToGPU(vBuffer, &iBuffer);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    uint32_t* graphImg = GenerateGraph(stat, 0.0, (float)amountOfImagesToTrain, 0.5, (float)(OUTPUT_COUNT-1));
    LoadSubTexGPU(&graph, 0, 0, graphImg);
    free((void*)graphImg);
    DeleteStatisticsSet(stat);

    printf("\ntrained %d images in %.2f minutes\n", imagesTrained, ((glfwGetTime() - beginTimer) / 60.0));
    //SaveNeuralNetworkToFile(network, "res/savedNeuralNetworks/shapes.nn"); //TODO: optioneel, heb betere interface nodig

    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        closedir(dirs[i]);

    if((s->flags != 0x01) && (imagesTrained != amountOfImagesToTrain))
    {
        DeleteNeuralNetwork(network);
        s->nextFn = Quit;
    }
    else
    {
        s->data1 = (void*)network;
        s->nextFn = Testing;
    }
    s->flags = 0;
}


void Testing(State* s)
{
    GLFWwindow* window = s->window;
    ShaderProgram* basicShader = GetShader(s->shaderLib, 0);
    IndexBuffer* iBuffer = s->iBuffer;
    NeuralNetwork* network = (NeuralNetwork*)s->data1;

    Mat4x4 projMat = OrthographicMat4x4(0.0, ((float)DATASET_IMAGE_WIDTH * 2.0), 0.0, (float)DATASET_IMAGE_HEIGHT, -1.0, 1.0);
    Mat4x4 viewMat = TranslationMat4x4(0.0, 0.0, 0.0);
    Mat4x4 final; //TODO: voeg toe aan state machine, zodat training en testing scherm hetzelfde scherm hebben

    Texture image = { DATASET_IMAGE_WIDTH, DATASET_IMAGE_HEIGHT, 1 };
    //LoadTexGPU(&image, NULL); //basically reserve space in GPU //TODO: beweeg naar Start state misschien?

    //SetKeyPressCallback(window);

    Vec2f cameraPos = { 0.0f, 0.0f };
    unsigned int peekedImages = 0;
    unsigned int correctAnswers = 0;
    float totalCost = 0.0;
    //double lastTime = glfwGetTime(); //TODO: later handig voor time profiling

    //directory strings
    // const char* testingPath = "res/dataSets/mnist/trainingSet";
    // const char* dirLUT[OUTPUT_COUNT] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    // const char* testingPath = "res/dataSets/shapes";
    // const char* dirLUT[OUTPUT_COUNT] = { "circle", "square", "star", "triangle" };
    const char* testingPath = "res/dataSets/animals/val";
    const char* dirLUT[OUTPUT_COUNT] = { "cat", "dog", "wild" };

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

    stbi_set_flip_vertically_on_load(true);

    //uint32_t* convolvedImage = (uint32_t*)malloc(sizeof(uint32_t) * (DATASET_IMAGE_WIDTH * DATASET_IMAGE_HEIGHT));
    //const float edgeKernelV[3][3] = { { -0.2, 0.0, 0.2 }, { -1.0, 0.0, 1.0 }, { -0.2, 0.0, 0.2 } };
    //const float edgeKernelH[3][3] = { { -0.2, -1.0, -0.2 }, { 0.0, 0.0, 0.0 }, { 0.2, 1.0, 0.2 } };
    //const float edgeKernelHV[3][3] = { { -0.1, -1.0, 0.0 }, { -1.0, 0.0, 1.0 }, { 0.0, 1.0, 0.1 } };
    //const float sharpenKernel[3][3] = { { 0.0, -1.0, 0.0 }, { -1.0, 5.0, -1.0 }, { 0.0, -1.0, 0.0 } };

    while(!glfwWindowShouldClose(window))
    {
        Clear();

        SetTransformMat4x4(&viewMat, cameraPos.x, cameraPos.y, 0.0);
        final = MultiplyMat4x4(projMat, viewMat);
        LoadUniformMat4x4(basicShader, 1, &final);

        int randomDirIndex = (rand() % OUTPUT_COUNT);
        if((fileDir = readdir(dirs[randomDirIndex])) != NULL)
        {
            snprintf(buffer, 128, "%s/%s/%s", testingPath, dirLUT[randomDirIndex], fileDir->d_name);
            stbi_uc* data = stbi_load(image.filePath, &image.width, &image.height, NULL, 4);
            if(data != NULL)
            {
                ConvertImageGrayScale((uint32_t*)data, image.width, image.height); //not needed for the shapes/mnist datasets
                
                //ConvolveImageKern3x3((uint32_t*)data, convolvedImage, image.width, image.height, edgeKernelHV);

                LoadSubTexGPU(&image, 0, 0, (uint32_t*)data);
                unsigned int highestValueIndex = PeekImage(network, (uint32_t*)data);
                totalCost += nCost(network, randomDirIndex);

                peekedImages++;
                if(highestValueIndex == randomDirIndex)
                    correctAnswers++;

                if((peekedImages % 1000) == 0)
                    printf("average cost: %.2f\taccuracy: %.2f%%\timages peeked: %d\n", (totalCost / (float)peekedImages), (((float)correctAnswers / (float)peekedImages) * 100.0), peekedImages);

                stbi_image_free((void*)data);
            }
        }

        Draw(iBuffer, basicShader);

        //DynamicToGPU(vBuffer, &iBuffer);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for(unsigned int i = 0; i < OUTPUT_COUNT; i++)
        closedir(dirs[i]);
    DeleteNeuralNetwork(network);

    s->nextFn = Quit;
}