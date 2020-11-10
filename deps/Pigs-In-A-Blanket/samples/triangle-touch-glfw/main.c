#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <psp2/kernel/clib.h> 
#include <pib.h>
#include <psp2/kernel/processmgr.h>
#include <taihen.h>
#include <GLES2/gl2.h>
#define GLFW_INCLUDE_ES2 1
#include <GLFW/glfw3.h>

GLuint programObject;

GLFWwindow *window;

GLsizei width;
GLsizei height;

GLfloat red, green, blue;


GLuint LoadShader(const GLchar *shaderSrc, GLenum type)
{
    GLuint shader;
    GLint compiled;

    printf("Creating Shader...\n");

    shader = glCreateShader(type);

    if(shader == 0)
    {
        printf("Failed to Create Shader\n");
        return 0;
    }

    glShaderSource(shader, 1, &shaderSrc, NULL);

    printf("Compiling Shader...\n");
    glCompileShader(shader);
    printf("Shader Compiled!\n");

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if(!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if(infoLen > 1)
        {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            printf("Error compiling shader:\n%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    printf("Shader Compiled!\n");
    return shader;
}

int Init()
{
    const GLchar vShaderStr[] =
        "void main(\n"
	    "float4 aPosition,\n"
	    "float4 out gl_Position: POSITION)\n"
	    "{\n"
	    "	gl_Position = aPosition;\n"
	    "}";

    const GLchar fShaderStr[] =
        "void main(\n"
        "uniform float4 aColor,\n"
        "float4 out gl_FragColor: COLOR)\n"
	    "{\n"
	    "	gl_FragColor = aColor;\n"
	    "}";

    GLuint vertexShader;
    GLuint fragmentShader;
    GLint linked;  
    
    vertexShader = LoadShader(vShaderStr, GL_VERTEX_SHADER);
    fragmentShader = LoadShader(fShaderStr, GL_FRAGMENT_SHADER);
    printf("Creating program\n");
    programObject = glCreateProgram();

    if(programObject == 0)
        return 0;
    printf("Created program\n");

    printf("Attaching Shaders...\n");
    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);
    printf("Attached Shaders\n");

    printf("Binding Attrib Location...\n");
    glBindAttribLocation(programObject, 0, "aPosition");
    printf("Binded Attrib Location\n");

    printf("Linking Program...\n");
    glLinkProgram(programObject);
    printf("Linked Program\n");

    printf("Getting Program...\n");
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

    if(!linked)
    {
        GLint infoLen = 0;

        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

        if(infoLen > 1)
        {
            char* infoLog = malloc(sizeof(char) * infoLen);

            glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
            printf("Error linking program:\n%s\n", infoLog);

            free(infoLog);
        }

        glDeleteProgram(programObject);
        return 0;
    }
    printf("Got Program\n");

    printf("Setting Clear Color...\n");
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    printf("Set Clear Color\n");
    return 1;
}

void Draw()
{
    GLfloat vVertices[] = {0.0f,  0.5f, 0.0f,
                            -0.5f, -0.5f, 0.0f,
                            0.5f, -0.5f,  0.0f};

    glViewport(0, 0, width, height);

    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(programObject);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(0);

    glUniform4f( glGetUniformLocation( programObject, "aColor" ), red, green, blue, 1.0);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents(); // Needed to update touch screen state
    red = green = blue = 1.0f;
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT))
    {
        green = blue = 0.0f;
    }
    int hatCount = 0;
    const unsigned char *hats = glfwGetJoystickHats(GLFW_JOYSTICK_1, &hatCount);
    for (int i = 0; i < hatCount; i++)
    {
        if(GLFW_HAT_UP & hats[i])
            printf("Direction Button Up Pressed\n");
        if(GLFW_HAT_RIGHT & hats[i])
            printf("Direction Button Right Pressed\n");
        if(GLFW_HAT_DOWN & hats[i])
            printf("Direction Button Down Pressed\n");
        if(GLFW_HAT_LEFT & hats[i])
            printf("Direction Button Left Pressed\n");
    }
    int count;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);
    for (int i = 0; i < count; i++)
    {
        char *axis = NULL;
        if (axes[i] > .05f || axes[i] < -.05f)
        {
            switch (i)
            {
            case 0:
                axis = "Left X";
                break;
            case 1:
                axis = "Left Y";
                break;
            case 2:
                axis = "Right X";
                break;
            case 3:
                axis = "Right Y";
                break;
            }
            printf("%s: %f\n", axis, axes[i]);
        }
    }
}

int main(void)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(960, 544, "test", NULL, NULL);  // In reality, only the Width matters, so make sure it's one of the supported values!
    printf("%s\n", glfwGetVersionString());
    glfwGetWindowSize(window, &width, &height);
    printf("%dx%d\n", width, height);
    glfwMakeContextCurrent(window);
    glClearColor(0.0f,0.0f,0.0f,1.0f);

    if(!Init())
        return 0;
    while (1)
    {
        Draw();
    }
    sceKernelDelayThread(10*1000*1000);
    sceKernelExitProcess(0);
    return 0;
}