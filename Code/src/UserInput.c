#include "UserInput.h"
#include "StateMachine.h"

static void ResizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    //State* programState = glfwGetWindowUserPointer(window);
    //*(programState->projMat) = OrthographicMat4x4(0.0, (float)DATASET_IMAGE_WIDTH, 0.0, (float)DATASET_IMAGE_HEIGHT, -1.0, 1.0);
}

static void KeyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        State* programState = glfwGetWindowUserPointer(window);
        if(key == GLFW_KEY_X)
        {
            programState->flags |= 0x01;
        }
    }
}


void SetResizeCallback(GLFWwindow* window)
{
    glfwSetWindowSizeCallback(window, ResizeCallback);
}

void SetKeyPressCallback(GLFWwindow* window)
{
    glfwSetKeyCallback(window, KeyPressCallback);
}