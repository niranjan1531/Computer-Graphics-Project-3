// Include standard headers
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <array>
#include <stack>   
#include <string>
#include <fstream>
#include <sstream>
// Include GLEW
#include <GL/glew.h>
// Include GLFW
#include <GLFW/glfw3.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace glm;
using namespace std;
// Include AntTweakBar
#include <AntTweakBar.h>

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <common/texture.hpp>

const int window_width = 600, window_height = 600;

typedef struct Vertex {
	float Position[4];
	float Normal[3];
	float UV[2];
	void SetPosition(float *coords) {
		Position[0] = coords[0];
		Position[1] = coords[1];
		Position[2] = coords[2];
		Position[3] = 1.0;
	}
	void SetNormal(float *coords) {
		Normal[0] = coords[0];
		Normal[1] = coords[1];
		Normal[2] = coords[2];
	}
	void SetUV(float* coords) {
		UV[0] = coords[0];
		UV[1] = coords[1];
	}
};

// function prototypes
int initWindow(void);
void initOpenGL(void);
void createVAOs(Vertex[], GLushort[], int);
void loadObject(char*, glm::vec4, Vertex* &, GLushort* &, int);
void createObjects(void);
void pickObject(void);
void renderScene(void);
void cleanup(void);
static void keyCallback(GLFWwindow*, int, int, int, int);
static void mouseCallback(GLFWwindow*, int, int, int);
void resetScene(void);
GLuint loadTessShaders(const char*, const char*, const char*, const char*);

// GLOBAL VARIABLES
GLFWwindow* window;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;

GLuint gPickedIndex = -1;
std::string gMessage;
const size_t IndexCount = 318;
float origColor[4]; 
int i = 0;


GLuint programID;
GLuint pickingProgramID;

const GLuint NumObjects = 3;	// ATTN: THIS NEEDS TO CHANGE AS YOU ADD NEW OBJECTS
GLuint VertexArrayId[NumObjects];
GLuint VertexBufferId[NumObjects];
GLuint IndexBufferId[NumObjects];

// TL
size_t VertexBufferSize[NumObjects];
size_t IndexBufferSize[NumObjects];
size_t NumIdcs[NumObjects];
size_t NumVerts[NumObjects];

bool selectCamera = false;
bool moveCameraUp = false;
bool moveCameraDown = false;
bool moveCameraRight = false;
bool moveCameraLeft = false;
bool shouldResetScene = false;
float tessellationLevel = 3.0f;
bool shouldTessellateModel = false;
bool shouldDisplayWireframeMode = false;
bool shouldTexureModel = false;
bool isShiftPressed = false;

GLfloat cameraAngleTheta = 3.14 / 4;
GLfloat cameraAnglePhi = asin(1 / (sqrt(3)));
GLfloat cameraSphereRadius = sqrt(432);

float pickingColor[10] = { 0 / 255.0f, 1 / 255.0f, 2 / 255.0f, 3 / 255.0f,  4 / 255.0f, 5 / 255.0f, 6 / 255.0f, 7 / 255.0f, 8 / 255.0f, 9 / 255.0f };


GLuint MatrixID;
GLuint ModelMatrixID;
GLuint ViewMatrixID;
GLuint ProjMatrixID;
GLuint PickingMatrixID;
GLuint pickingColorID;
GLuint Texture;
GLuint TextureID;
GLuint lightID;
GLuint tessProgramID;
GLuint tessMatrixID;
GLuint tessModelMatrixID;
GLuint tessViewMatrixID;
GLuint tessProjectionMatrixID;
GLuint tessLightID;
GLfloat tessellationLevelInnerID;
GLfloat tessellationLevelOuterID;

Vertex* Verts;
GLushort* Idcs;

// Declare global objects
// TL
const size_t CoordVertsCount = 6;
Vertex CoordVerts[CoordVertsCount];

const size_t GridVertsCount = 46;
Vertex GridVerts[GridVertsCount];

int initWindow(void) {
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	// FOR MAC

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Khare ,Vibhor(8651 1659)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Initialize the GUI
	// TwInit(TW_OPENGL_CORE, NULL);
	// TwWindowSize(window_width, window_height);
	// TwBar * GUI = TwNewBar("Picking");
	// TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	// TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &gMessage, NULL);

	// Set up inputs
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);

	return 0;
}

void initOpenGL(void) {
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	gProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	//gProjectionMatrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	gViewMatrix = glm::lookAt(glm::vec3(12.0, 12.0, 12.0f),	// eye
		glm::vec3(0.0, 0.0, 0.0),	// center
		glm::vec3(0.0, 1.0, 0.0));	// up

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");
	pickingProgramID = LoadShaders("Picking.vertexshader", "Picking.fragmentshader");
    tessProgramID = loadTessShaders("Tessellation.vs.glsl", "Tessellation.tc.glsl", "Tessellation.te.glsl",
                                "Tessellation.fs.glsl");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ProjMatrixID = glGetUniformLocation(programID, "P");
    lightID = glGetUniformLocation(programID, "lightPosition_worldspace");

	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");

    tessModelMatrixID = glGetUniformLocation(tessProgramID, "M");
    tessViewMatrixID = glGetUniformLocation(tessProgramID, "V");
    tessProjectionMatrixID = glGetUniformLocation(tessProgramID, "P");
    tessLightID = glGetUniformLocation(tessProgramID, "lightPosition_worldspace");
    tessellationLevelInnerID = glGetUniformLocation(tessProgramID, "tessellationLevelInner");
    tessellationLevelOuterID = glGetUniformLocation(tessProgramID, "tessellationLevelOuter");


	// TL
	// Define objects
	createObjects();

	// ATTN: create VAOs for each of the newly created objects here:
	VertexBufferSize[0] = sizeof(CoordVerts);
	NumVerts[0] = CoordVertsCount;

	createVAOs(CoordVerts, NULL, 0);

	VertexBufferSize[1] = sizeof(GridVerts);
	NumVerts[1] = GridVertsCount;

	createVAOs(GridVerts, NULL, 1);
}

GLuint loadTessShaders(const char *tess_vert_file_path, const char *tess_ctrl_file_path, const char *tess_eval_file_path,
    const char *tess_frag_file_path) {
    GLuint tessVertShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint tessCtrlShaderID = glCreateShader(GL_TESS_CONTROL_SHADER);
    GLuint tessEvalShaderID = glCreateShader(GL_TESS_EVALUATION_SHADER);
    GLuint tessFragShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    string tessVertexShaderCode;
    ifstream tessVertexShaderStream(tess_vert_file_path, std::ios::in);
    if(tessVertexShaderStream.is_open()) {
        string line = "";
        while(std::getline(tessVertexShaderStream, line)) {
            tessVertexShaderCode += "\n" + line;
        }
        tessVertexShaderStream.close();
    } else {
        printf("Impossible to open %s.\n", tess_vert_file_path);
        getchar();
        return 0;
    }

    string tessCtrlShaderCode;
    ifstream tessCtrlShaderStream(tess_ctrl_file_path, std::ios::in);
    if(tessCtrlShaderStream.is_open()) {
        string line = "";
        while(std::getline(tessCtrlShaderStream, line)) {
            tessCtrlShaderCode += "\n" + line;
        }
        tessCtrlShaderStream.close();
    } else {
        printf("Impossible to open %s\n", tess_ctrl_file_path);
        getchar();
        return 0;
    }

    string tessEvalShaderCode;
    ifstream tessEvalShaderStream(tess_eval_file_path, std::ios::in);
    if(tessEvalShaderStream.is_open()) {
        string line = "";
        while(std::getline(tessEvalShaderStream, line)) {
            tessEvalShaderCode += "\n" + line;
        }
        tessEvalShaderStream.close();
    } else {
        printf("Impossible to open %s.\n", tess_eval_file_path);
        getchar();
        return 0;
    }

    string tessFragShaderCode;
    ifstream tessFragShaderStream(tess_frag_file_path, std::ios::in);
    if(tessFragShaderStream.is_open()) {
        string line = "";
        while(std::getline(tessFragShaderStream, line)) {
            tessFragShaderCode += "\n" + line;
        }
        tessFragShaderStream.close();
    } else {
        printf("Impossible to open %s.\n", tess_frag_file_path);
        getchar();
        return 0;
    }

    GLint result = false;
    int infoLogLength;

    printf("Compiling shader: %s\n", tess_vert_file_path);
    char const* tessVertSourcePointer = tessVertexShaderCode.c_str();
    glShaderSource(tessVertShaderID, 1, &tessVertSourcePointer, NULL);
    glCompileShader(tessVertShaderID);
    glGetShaderiv(tessVertShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(tessVertShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0) {
        std::vector<char> tessVertShaderErrMsg(infoLogLength + 1);
        glGetShaderInfoLog(tessVertShaderID, infoLogLength, NULL, &tessVertShaderErrMsg[0]);
        printf("%s\n", &tessVertShaderErrMsg[0]);
    }

    printf("Compiling shader: %s\n", tess_ctrl_file_path);
    char const* tessCtrlSourcePointer = tessCtrlShaderCode.c_str();
    glShaderSource(tessCtrlShaderID, 1, &tessCtrlSourcePointer, NULL);
    glCompileShader(tessCtrlShaderID);
    glGetShaderiv(tessCtrlShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(tessCtrlShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0) {
        std::vector<char> tessCtrlShaderErrMsg(infoLogLength + 1);
        glGetShaderInfoLog(tessCtrlShaderID, infoLogLength, NULL, &tessCtrlShaderErrMsg[0]);
        printf("%s\n", &tessCtrlShaderErrMsg[0]);
    }

    printf("Compiling shader: %s\n", tess_eval_file_path);
    char const* tessEvalSourcePointer = tessEvalShaderCode.c_str();
    glShaderSource(tessEvalShaderID, 1, &tessEvalSourcePointer, NULL);
    glCompileShader(tessEvalShaderID);
    glGetShaderiv(tessEvalShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(tessEvalShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0) {
        std::vector<char> tessEvalShaderErrMsg(infoLogLength + 1);
        glGetShaderInfoLog(tessEvalShaderID, infoLogLength, NULL, &tessEvalShaderErrMsg[0]);
        printf("%s\n", &tessEvalShaderErrMsg[0]);
    }

    printf("Compiling shader: %s\n", tess_frag_file_path);
    char const* tessFragSourcePointer = tessFragShaderCode.c_str();
    glShaderSource(tessFragShaderID, 1, &tessFragSourcePointer, NULL);
    glCompileShader(tessFragShaderID);
    glGetShaderiv(tessFragShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(tessFragShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0) {
        std::vector<char> tessFragShaderErrMsg(infoLogLength + 1);
        glGetShaderInfoLog(tessFragShaderID, infoLogLength, NULL, &tessFragShaderErrMsg[0]);
        printf("%s\n", &tessFragShaderErrMsg[0]);
    }

    printf("Linking Shader\n");
    GLuint tessProgramID = glCreateProgram();
    glAttachShader(tessProgramID, tessVertShaderID);
    glAttachShader(tessProgramID, tessCtrlShaderID);
    glAttachShader(tessProgramID, tessEvalShaderID);
    glAttachShader(tessProgramID, tessFragShaderID);
    glLinkProgram(tessProgramID);

    glGetProgramiv(tessProgramID, GL_LINK_STATUS, &result);
    glGetProgramiv(tessProgramID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0) {
        std::vector<char> tessProgramErrMsg(infoLogLength + 1);
        glGetProgramInfoLog(tessProgramID, infoLogLength, NULL, &tessProgramErrMsg[0]);
        printf("%s\n", &tessProgramErrMsg[0]);
    }

    glDetachShader(tessProgramID, tessVertShaderID);
    glDetachShader(tessProgramID, tessCtrlShaderID);
    glDetachShader(tessProgramID, tessEvalShaderID);
    glDetachShader(tessProgramID, tessFragShaderID);

    glDeleteShader(tessVertShaderID);
    glDeleteShader(tessCtrlShaderID);
    glDeleteShader(tessEvalShaderID);
    glDeleteShader(tessFragShaderID);

    return tessProgramID;
}

void createVAOs(Vertex Vertices[], unsigned short Indices[], int ObjectId) {
	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t NormalOffset = sizeof(Vertices[0].Position);
	const size_t UVoffset = sizeof(Vertices[0].Normal) + NormalOffset;

	// Create Vertex Array Object
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);
	glBindVertexArray(VertexArrayId[ObjectId]);

	// Create Buffer for vertex data
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Vertices, GL_STATIC_DRAW);

	// Create Buffer for indices
	if (Indices != NULL) {
		glGenBuffers(1, &IndexBufferId[ObjectId]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId[ObjectId]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferSize[ObjectId], Indices, GL_STATIC_DRAW);
	}

	// Assign vertex attributes
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)NormalOffset);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)UVoffset);	// TL

	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// normal
	glEnableVertexAttribArray(2);	// uv

	// Disable our Vertex Buffer Object 
	glBindVertexArray(0);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create a VBO: %s \n",
			gluErrorString(ErrorCheckValue)
		);
	}
}

// Ensure your .obj files are in the correct format and properly loaded by looking at the following function
void loadObject(char* file, glm::vec4 color, Vertex* &out_Vertices, GLushort* &out_Indices, int ObjectId) {
	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ(file, vertices, uvs, normals);

	std::vector<GLushort> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	const size_t vertCount = indexed_vertices.size();
	const size_t idxCount = indices.size();

	// populate output arrays
	out_Vertices = new Vertex[vertCount];
	for (int i = 0; i < vertCount; i++) {
		out_Vertices[i].SetPosition(&indexed_vertices[i].x);
		out_Vertices[i].SetNormal(&indexed_normals[i].x);
		out_Vertices[i].SetUV(&indexed_uvs[i].r);
	}
	out_Indices = new GLushort[idxCount];
	for (int i = 0; i < idxCount; i++) {
		out_Indices[i] = indices[i];
	}

	// set global variables!!
	NumIdcs[ObjectId] = idxCount;
	VertexBufferSize[ObjectId] = sizeof(out_Vertices[0]) * vertCount;
	IndexBufferSize[ObjectId] = sizeof(GLushort) * idxCount;
}

void createObjects(void) {

	//Textures
	TextureID = loadBMP_custom("vibhor.bmp");

	//-- COORDINATE AXES --//
	CoordVerts[0] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 1.0 } };
	CoordVerts[1] = { { 5.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 1.0 } };
	CoordVerts[2] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 1.0, 1.0 } };
	CoordVerts[3] = { { 0.0, 5.0, 0.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 1.0, 1.0 } };
	CoordVerts[4] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 1.0 } };
	CoordVerts[5] = { { 0.0, 0.0, 5.0, 1.0 }, { 0.0, 0.0, 1.0 }, { 1.0, 1.0 } };

	//-- GRID --//
	
	// ATTN: Create your grid vertices here!
	for (int i = 0; i < 22; i = i + 2) {
		GridVerts[i] = { { 5.0, 0.0, (float)(5 - i / 2), 1.0 }, { 0.0, 0.0, 1.0 }, {0.5,0.5} };
		GridVerts[i + 1] = { { -5.0, 0.0, (float)(5 - i / 2), 1.0 }, { 0.0, 0.0, 1.0 }, {0.5,0.5} };
	}

	for (int i = 0; i < 22; i = i + 2) {
		GridVerts[i + 24] = { { (float)(5 - i / 2), 0.0, 5.0, 1.0 }, { 0.0, 0.0, 1.0 }, {0.5,0.5} };
		GridVerts[i + 25] = { {(float)(5 - i / 2), 0.0, -5.0, 1.0}, { 0.0, 0.0, 1.0 }, {0.5,0.5} };
	}

	//-- .OBJs --//

	// ATTN: Load your models here through .obj files -- example of how to do so is as shown
	// Vertex* Verts;
	// GLushort* Idcs;
	loadObject("trial2.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), Verts, Idcs, 2);
	createVAOs(Verts, Idcs, 2);

}

void pickObject(void) {
	// Clear the screen in white
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pickingProgramID);
	{
		glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, in the "MVP" uniform
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);

		// ATTN: DRAW YOUR PICKING SCENE HERE. REMEMBER TO SEND IN A DIFFERENT PICKING COLOR FOR EACH OBJECT BEFOREHAND
		glBindVertexArray(0);
	}
	glUseProgram(0);
	// Wait until all the pending drawing commands are really done.
	// Ultra-mega-over slow ! 
	// There are usually a long time between glDrawElements() and
	// all the fragments completely rasterized.
	glFlush();
	glFinish();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Read the pixel at the center of the screen.
	// You can also use glfwGetMousePos().
	// Ultra-mega-over slow too, even for 1 pixel, 
	// because the framebuffer is on the GPU.
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned char data[4];
	glReadPixels(xpos, window_height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data); // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top

	// Convert the color back to an integer ID
	gPickedIndex = int(data[0]);

	if (gPickedIndex >= IndexCount) { 
        // Any number > vertices-indices is background!
        gMessage = "background";
    }
    else {
        // if(i==0)
        // {
        //     origColor[0] = Verts[gPickedIndex].Color[0];
        //     origColor[1] = Verts[gPickedIndex].Color[1];
        //     origColor[2] = Verts[gPickedIndex].Color[2];
        //     origColor[3] = Verts[gPickedIndex].Color[3];
        //     Vertices[gPickedIndex].SetColor(white);
        //     i = 1;
        // }
        // else
        // {
        //     Vertices[gPickedIndex].SetColor(origColor);
        //     i = 0;
        // }
    }

	// Uncomment these lines to see the picking shader in effect
	glfwSwapBuffers(window);
	// continue; // skips the normal rendering
}

void moveVertex(void) {
    glm::mat4 ModelMatrix = glm::mat4(1.0);
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glm::vec4 vp = glm::vec4(viewport[0], viewport[1], viewport[2], viewport[3]);

    int bufferWidth; int bufferHeight; int currWidth; int currHeight;
    glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);
    glfwGetWindowSize(window, &currWidth, &currHeight);

    if (gPickedIndex >= IndexCount) { 
        // Any number > vertices-indices is background!
        gMessage = "background";
    }
    else {
        std::ostringstream oss;
        oss << "point " << gPickedIndex;
        gMessage = oss.str();

        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        glm::vec3 wcoords = glm::unProject(glm::vec3(xpos*(bufferWidth/currWidth),(currHeight - ypos)*(bufferHeight/currHeight), 0.0), ModelMatrix, gProjectionMatrix, vp);
        //std::cout<<"("<<wcoords.x<<", "<<wcoords.y<<", "<<wcoords.z<<") \n";
        float newCoords[4];

        newCoords[0] = -wcoords.x;
        newCoords[1] = wcoords.y;
        newCoords[2] = Verts[gPickedIndex].Position[2];
        newCoords[3] = 1.0f;

        // printf("%s", shouldZTranslate ? "true" : "false");
        Verts[gPickedIndex].SetPosition(newCoords);
        // printf("\n%f %f %f\n", Vertices[gPickedIndex].Position[0],Vertices[gPickedIndex].Position[1],Vertices[gPickedIndex].Position[2]);
    }
}

void renderScene(void) {
	//ATTN: DRAW YOUR SCENE HERE. MODIFY/ADAPT WHERE NECESSARY!

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.2f, 0.0f);
	// Re-clear the screen for real rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (moveCameraLeft) {
		cameraAngleTheta -= 0.01f;
	}
	if (moveCameraRight) {
		cameraAngleTheta += 0.01f;
	}
	if (moveCameraUp) {
		cameraAnglePhi -= 0.01f;
	}
	if (moveCameraDown) {
		cameraAnglePhi += 0.01f;
	}
	if (shouldResetScene || moveCameraLeft || moveCameraRight || moveCameraDown || moveCameraUp) {
		float camX = cameraSphereRadius * cos(cameraAnglePhi) * sin(cameraAngleTheta);
		float camY = cameraSphereRadius * sin(cameraAnglePhi);
		float camZ = cameraSphereRadius * cos(cameraAnglePhi) * cos(cameraAngleTheta);
		gViewMatrix = glm::lookAt(glm::vec3(camX, camY, camZ),	// eye
			glm::vec3(0.0, 0.0, 0.0),	// center
			glm::vec3(0.0, 1.0, 0.0));	// up
	}

    if(shouldDisplayWireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    } else if(shouldTexureModel){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glm::vec3 lightPos = glm::vec3(20.0f, 20.0f, 0.0f);
    glm::mat4x4 ModelMatrix = glm::mat4(1.0);
	glUseProgram(programID);
	{
        glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		//Texture
		
		Texture = glGetUniformLocation(programID, "myTextureSampler");
		glActiveTexture(GL_TEXTURE0);

		
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		glBindVertexArray(VertexArrayId[0]);	// Draw CoordAxes
		glDrawArrays(GL_LINES, 0, NumVerts[0]);

		glBindVertexArray(VertexArrayId[1]);	// Draw Grid
		glDrawArrays(GL_LINES, 0, NumVerts[1]);

		glBindTexture(GL_TEXTURE_2D, TextureID);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

        if(!shouldTessellateModel && (shouldDisplayWireframeMode || shouldTexureModel )) {
    		glBindVertexArray(VertexArrayId[2]);	// Draw CoordAxes
    		glDrawElements(GL_TRIANGLES, NumIdcs[2], GL_UNSIGNED_SHORT, (void*)0);
        }
			
		glBindVertexArray(0);
	}

    if(shouldTessellateModel) {
        glUseProgram(tessProgramID);
        {
            glUniform3f(tessLightID, lightPos.x, lightPos.y, lightPos.z);
            glUniformMatrix4fv(tessViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
            glUniformMatrix4fv(tessProjectionMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
            glUniformMatrix4fv(tessModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

            glUniform1f(tessellationLevelInnerID, tessellationLevel);
            glUniform1f(tessellationLevelOuterID, tessellationLevel);

            glPatchParameteri(GL_PATCH_VERTICES, 3);
            glBindVertexArray(VertexArrayId[2]);
            glDrawElements(GL_PATCHES, NumIdcs[2], GL_UNSIGNED_SHORT, (void*)0);


            glBindVertexArray(0);
        }
    }
	glUseProgram(0);
	// Draw GUI
	// TwDraw();

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void cleanup(void) {
	// Cleanup VBO and shader
	for (int i = 0; i < NumObjects; i++) {
		glDeleteBuffers(1, &VertexBufferId[i]);
		glDeleteBuffers(1, &IndexBufferId[i]);
		glDeleteVertexArrays(1, &VertexArrayId[i]);
	}
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

// Alternative way of triggering functions on keyboard events
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_LEFT:
            moveCameraLeft = true;
            break;
        case GLFW_KEY_RIGHT:
            moveCameraRight = true;
            break;
        case GLFW_KEY_UP:
            moveCameraDown = true;
            break;
        case GLFW_KEY_DOWN:
            moveCameraUp = true;
            break;
        case GLFW_KEY_R:
            shouldResetScene = true;
            resetScene();
            break;
        case GLFW_KEY_LEFT_SHIFT:
            isShiftPressed = true;
            break;
        case GLFW_KEY_RIGHT_SHIFT:
            isShiftPressed = true;
            break;
        default:
            break;
        }
    } else if(action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_LEFT:
            moveCameraLeft = false;
            break;
        case GLFW_KEY_RIGHT:
            moveCameraRight = false;
            break;
        case GLFW_KEY_UP:
            moveCameraDown = false;
            break;
        case GLFW_KEY_DOWN:
            moveCameraUp = false;
            break;
        case GLFW_KEY_R:
            shouldResetScene = false;
            break;
        case GLFW_KEY_P:
            shouldTessellateModel = !shouldTessellateModel;
            break;
        case GLFW_KEY_F:
            if(isShiftPressed) {
                shouldTexureModel = !shouldTexureModel;
            } else {
                shouldDisplayWireframeMode = ! shouldDisplayWireframeMode;
            }
            break;
        case GLFW_KEY_LEFT_SHIFT:
            isShiftPressed = false;
            break;
        case GLFW_KEY_RIGHT_SHIFT:
            isShiftPressed = false;
            break;
        default:
            break;
        }
    }
}

void resetScene(void) {
    cameraAngleTheta = 3.142 / 4;
    cameraAnglePhi = asin(1 / sqrt(3));
    // loadControlPoints(2);
}

// Alternative way of triggering functions on mouse click events
static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        pickObject();
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        pickObject();
    }
}

int main(void) {
	// TL
	// ATTN: Refer to https://learnopengl.com/Getting-started/Transformations, https://learnopengl.com/Getting-started/Coordinate-Systems,
	// and https://learnopengl.com/Getting-started/Camera to familiarize yourself with implementing the camera movement

	// ATTN (Project 3 only): Refer to https://learnopengl.com/Getting-started/Textures to familiarize yourself with mapping a texture
	// to a given mesh

	// Initialize window
	int errorCode = initWindow();
	if (errorCode != 0)
		return errorCode;

	// Initialize OpenGL pipeline
	initOpenGL();

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;
	do {
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0){ // If last prinf() was more than 1sec ago
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
            moveVertex();
        }

		// DRAWING POINTS
		renderScene();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}