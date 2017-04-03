#include <bits/stdc++.h>
#include <mpg123.h>
#include <ao/ao.h>
#include <assert.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

typedef pair<int,int> II;
typedef vector<int> VI;
typedef vector<II> VII;
typedef long long int LL;
typedef unsigned long long int ULL;

#define rep(i, a, b) for(i = a; i < b; i++)
#define rev(i, a, b) for(i = a; i > b; i--)
#define INF INT_MAX
#define PB push_back
#define MP make_pair
#define F first
#define S second
#define SET(a,b) memset(a, b, sizeof(a))

#define BITS 8

map<string, int> rot;

float z[10][20];
string state[10][20], type[10][20];
bool exists[10][20];
int id[10][20];

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;


int blockX, blockY;
string blockState;

struct GLMatrices {
    glm::mat4 projectionO, projectionP;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;

int proj_type;
int score, moveCount, level = 1, rotateDirection[4];
float angle, updateTime = 1.5;
double mouseX, mouseY, last_update_time, current_time;
string baseLoc;
VAO *scoreBackground, *tile[10][20], *tileBorder[10][20];
VAO *blockStanding, *blockSleepingX, *blockSleepingY;
bool selected, gameOver;
bool levelUp, takingInput, * keyStates = new bool[500];
const float screenLeftX = -11.0;
const float screenRightX = 11.0;
const float screenTopY = 11.0;
const float screenBottomY = -11.0;
const float scoreLeftX = 5.0;
const float scoreRightX = 10.0;
float displayLeft = -11.0, displayRight = 5.0;
float displayTop = 11.0, displayBottom = -11.0;
float horizontalZoom = 0, verticalZoom = 0;
float camera_rotation_angle = 90;
double goalx, goaly;
GLFWwindow* windowCopy;
GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	// printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	// fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	// printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	// fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	// fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	// fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    // fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Game specific code *
 **************************/
// Convention followed everywhere is up, down, left, right (correspond to 0, 1, 2 and 3 respectively)

void resetMouseCoordinates();
void resetTileState();

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if(!takingInput)
    return;
  // Function is called first on GLFW_PRESS.
  if (action == GLFW_RELEASE) {
    keyStates[key] = false;
  }
  else if (action == GLFW_PRESS) {
    keyStates[key] = true;
    if(key == GLFW_KEY_UP) {
        rotateDirection[rot["up"]]++;
    }
    else if(key == GLFW_KEY_RIGHT) {
        rotateDirection[rot["right"]]++;
    }

    else if(key == GLFW_KEY_DOWN) {
        rotateDirection[rot["down"]]++;
    }
    else if(key == GLFW_KEY_LEFT) {
        rotateDirection[rot["left"]]++;
    }
    else if(key == GLFW_KEY_SPACE) {
      proj_type += 1;
      proj_type %= 11;   
    }
    else if(key == GLFW_KEY_Q) {
      quit(window);
    }
    else if(key == GLFW_KEY_T)
    {
      angle -= 1;
    }
    else if(key == GLFW_KEY_R)
    {
      angle += 1;
    }
    moveCount++;
  }
  return;
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	if(key == 'q') quit(window);
  return;
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_RELEASE) {
      keyStates[button] = false;
      resetMouseCoordinates();
    }
    else if(action == GLFW_PRESS) {
      keyStates[button] = true;
    }
    return;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  if(yoffset == 1)
  {

  }
  else if(yoffset == -1)
  {

  }
}

void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = M_PI/2;

    // sets the viewport of openGL renderer
    glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    // Store the projection matrix in a variable for future use

    // Perspective projection for 3D views
    Matrices.projectionP = glm::perspective(fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projectionO = glm::ortho(screenLeftX, screenRightX, screenBottomY, screenTopY, 0.1f, 500.0f);

}
VAO *rectangle, *cam, *floor_vao;

// Creates the rectangle object used in this sample code
void createRectangle ()
{
    // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
  -0.5, 0.5, 0.5, 
  -0.5, -0.5, 0.5, 
  0.5, -0.5, 0.5,
  -0.5, 0.5, 0.5, 
  0.5, -0.5, 0.5,
  0.5, 0.5, 0.5,
  0.5, 0.5, 0.5,
  0.5, -0.5, 0.5,
  0.5, -0.5, -0.5,
  0.5, 0.5, 0.5,
  0.5, -0.5, -0.5,
  0.5, 0.5, -0.5,
  0.5, 0.5, -0.5,
  0.5, -0.5, -0.5,
  -0.5, -0.5, -0.5,
  0.5, 0.5, -0.5,
  -0.5, -0.5, -0.5,
  -0.5, 0.5, -0.5,
  -0.5, 0.5, -0.5,
  -0.5, -0.5, -0.5,
  -0.5, -0.5, 0.5, 
  -0.5, 0.5, -0.5,
  -0.5, -0.5, 0.5, 
  -0.5, 0.5, 0.5, 
  -0.5, 0.5, -0.5,
  -0.5, 0.5, 0.5, 
  0.5, 0.5, 0.5,
  -0.5, 0.5, -0.5,
  0.5, 0.5, 0.5,
  0.5, 0.5, -0.5,
  -0.5, -0.5, 0.5, 
  -0.5, -0.5, -0.5,
  0.5, -0.5, -0.5,
  -0.5, -0.5, 0.5, 
  0.5, -0.5, -0.5,
  0.5, -0.5, 0.5,
  -0.5, 0.5, 0.5,
  0.5, 0.5, -0.5,
  0.5, 0.75, -0.5,
    };

    static const GLfloat color_buffer_data [] = {
  1.0f, 1.0f, 0.0f,
  1.0f, 1.0f, 0.0f,
  1.0f, 1.0f, 0.0f,
  1.0f, 1.0f, 0.0f,
  1.0f, 1.0f, 0.0f,
  1.0f, 1.0f, 0.0f,
  1.0f, 0.0f, 1.0f,
  1.0f, 0.0f, 1.0f,
  1.0f, 0.0f, 1.0f,
  1.0f, 0.0f, 1.0f,
  1.0f, 0.0f, 1.0f,
  1.0f, 0.0f, 1.0f,
  0.0f, 1.0f, 1.0f,
  0.0f, 1.0f, 1.0f,
  0.0f, 1.0f, 1.0f,
  0.0f, 1.0f, 1.0f,
  0.0f, 1.0f, 1.0f,
  0.0f, 1.0f, 1.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  0.0f, 1.0f, 0.0f, 
  0.0f, 1.0f, 0.0f, 
  0.0f, 1.0f, 0.0f, 
  0.0f, 1.0f, 0.0f, 
  0.0f, 1.0f, 0.0f, 
  0.0f, 1.0f, 0.0f, 
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0.0f, 0.0f, 1.0f,
  0, 0, 0,
  0, 0, 0,
  1, 1, 1,
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    rectangle = create3DObject(GL_TRIANGLES, 13*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createCam ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
  -0.1, 0, 0,
  0.1, 0, 0, 
  0, 0.1, 0,
    };

    static const GLfloat color_buffer_data [] = {
  1, 1, 1,
  1, 1, 1,
  1, 1, 1,
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    cam = create3DObject(GL_TRIANGLES, 1*3, vertex_buffer_data, color_buffer_data, GL_LINE);
}
void createFloor ()
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data [] = {
  -2, -1, 2,
  2, -1, 2, 
  -2, -1, -2,
  -2, -1, -2,
  2, -1, 2, 
  2, -1, -2,
    };

    static const GLfloat color_buffer_data [] = {
  0.65, 0.165, 0.165,
  0.65, 0.165, 0.165,
  0.65, 0.165, 0.165,
  0.65, 0.165, 0.165,
  0.65, 0.165, 0.165,
  0.65, 0.165, 0.165,
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    floor_vao = create3DObject(GL_TRIANGLES, 2*3, vertex_buffer_data, color_buffer_data, GL_FILL);
}


void initialize()
{
  int i, j;

  score = 0;
  level = 1;
  angle = 0.0f;
  moveCount = 0;
  gameOver = false;
  levelUp = true;
  takingInput = true;
  srand((unsigned)time(0));

  baseLoc = "back";
  // Initializing the pressed state of all keys to false
  
  memset(keyStates, false, sizeof(keyStates));
  
  blockState = "standing";

  resetMouseCoordinates();
  resetTileState();
}

void resetTileState ()
{
  int i = 0;
  while(i != 10)
  {
    int j = 0;
    while(j != 20)
    {
      exists[i][j++] = false;
    }
    i++;
  }
}

void resetMouseCoordinates()
{
  mouseX = 100.0;
  mouseY = 100.0;
}


void buildtile(int i, int j, int X, int Y, int z, double r1, double r2, double b1, double b2, double g1, double g2) {
        float zdisp = 0.2;
        float xydisp = 1;
        GLfloat vertex_buffer_data [] = {
          X, Y, z, // vertex 1
          X, Y-xydisp, z, // vertex 2
          X+xydisp, Y, z, // vertex 3

          X+xydisp, Y, z, // vertex 3
          X, Y-xydisp, z, // vertex 2
          X+xydisp, Y-xydisp, z,  // vertex 4

          X, Y, z, // vertex 1
          X, Y, z-zdisp, // vertex 5
          X, Y-xydisp, z, // vertex 2

          X, Y, z-zdisp, // vertex 5
          X, Y-xydisp, z-zdisp, // vertex 6
          X, Y-xydisp, z, // vertex 2

          X+xydisp, Y, z, // vertex 3
          X+xydisp, Y, z-zdisp, // vertex 7
          X+xydisp, Y-xydisp, z, // vertex 4

          X+xydisp, Y, z-zdisp, // vertex 7
          X+xydisp, Y-xydisp, z-zdisp, // vertex 8
          X+xydisp, Y-xydisp, z, // vertex 4

          X, Y, z, // vertex 1
          X+xydisp, Y, z, // vertex 3
          X, Y, z-zdisp, // vertex 5

          X+xydisp, Y, z-zdisp, // vertex 7
          X+xydisp, Y, z, // vertex 3
          X, Y, z-zdisp, // vertex 5

          X, Y-xydisp, z, // vertex 2
          X+xydisp, Y-xydisp, z, // vertex 4
          X, Y-xydisp, z-zdisp, // vertex 6

          X+xydisp, Y-xydisp, z-zdisp, // vertex 8
          X+xydisp, Y-xydisp, z, // vertex 4
          X, Y-xydisp, z-zdisp, // vertex 6

          X, Y, z-zdisp, // vertex 5
          X, Y-xydisp, z-zdisp, // vertex 6
          X+xydisp, Y, z-zdisp, // vertex 7

          X+xydisp, Y, z-zdisp, // vertex 7
          X, Y-xydisp, z-zdisp, // vertex 6
          X+xydisp, Y-xydisp, z-zdisp  // vertex 8
        };

        GLfloat color_buffer_data [] = {
          r1, g1, b1, // color 1
          r1, g1, b1, // color 2
          r1, g1, b1, // color 3

          r1, g1, b1, // color 1
          r1, g1, b1, // color 3
          r1, g1, b1,  // color 4

          r2, g2, b2, // color 1
          r2, g2, b2, // color 5
          r2, g2, b2, // color 2

          r2, g2, b2, // color 5
          r2, g2, b2, // color 6
          r2, g2, b2, // color 2

          r2, g2, b2, // color 3
          r2, g2, b2, // color 7
          r2, g2, b2, // color 4

          r2, g2, b2, // color 7
          r2, g2, b2, // color 8
          r2, g2, b2, // color 4

          r2, g2, b2, // color 1
          r2, g2, b2, // color 3
          r2, g2, b2, // color 5

          r2, g2, b2, // color 7
          r2, g2, b2, // color 3
          r2, g2, b2, // color 5

          r2, g2, b2, // color 2
          r2, g2, b2, // color 4
          r2, g2, b2, // color 6

          r2, g2, b2, // color 8
          r2, g2, b2, // color 4
          r2, g2, b2, // color 6

          r1, g1, b1, // color 5
          r1, g1, b1, // color 6
          r1, g1, b1, // color 7

          r1, g1, b1, // color 7
          r1, g1, b1, // color 6
          r1, g1, b1, // color 8
        };

        // create3DObject creates and returns a handle to a VAO that can be used later
        tile[i][j] = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

        float boarder_diff = 0.01f;
        GLfloat vertex_buffer_data2 [] = {
          X, Y, z+boarder_diff, // vertex 1
          X, Y-1, z+boarder_diff, // vertex 2

          X+1, Y, z+boarder_diff, // vertex 3
          X, Y, z+boarder_diff, // vertex 1

          X, Y-1, z+boarder_diff, // vertex 2
          X+1, Y-1, z+boarder_diff,  // vertex 4

          X+1, Y, z+boarder_diff, // vertex 3
          X+1, Y-1, z+boarder_diff  // vertex 4
        };

        GLfloat color_buffer_data2 [] = {
          r2, g2, b2, // color 1
          r2, g2, b2, // color 2

          r2, g2, b2, // color 3
          r2, g2, b2, // color 1

          r2, g2, b2, // color 2
          r2, g2, b2, // color 4

          r2, g2, b2, // color 3
          r2, g2, b2, // color 4
        };

        tileBorder[i][j] = create3DObject(GL_LINES, 8, vertex_buffer_data2, color_buffer_data2, GL_FILL);
}
// Create the grid that the block will move on
void createTiles ()
{
  int i, j, c, initX = -7, initY = 5, id1 = 0;
  FILE * file;
  float r1, g1, b1, r2, g2, b2, z1;

  switch (level) {
      case 1:
          file = fopen("level01.txt", "r");
          break;
      case 2:
          file = fopen("level02.txt", "r");
          break;
      case 3: 
          file = fopen("level09.txt", "r");
          break;
      default : 
          break;
  }

  for(i = 0; i < 10; i++)
  {
    for(j = 0; j < 20; j++)
    {
      z1 = 0.0f;
      c = getc(file);
      if(c == 'o' || c == 'S' || c == '.' || c == 'B' || c == 'X' || c == 'b' || c == 'x')
      {
        exists[i][j] = true;
        id[i][j] = c;
        z[i][j] = 0.0f;
        switch(c) {
          case 'S': {
            blockX = (int)initX;
            blockY = (int)initY; 
          }
          case 'o' : {
            r1 = 51/255;
            g1 = 51/255;
            b1 = 255/255;
            r2 = 0/255;
            g2 = 0/255;
            b2 = 102/255;
            type[i][j] = "normal";
            state[i][j] = "present"; 
          }
          break;
          case '.': {
            r1 = 51/255;
            g1 = 255/255;
            b1 = 255/255;
            r2 = 0/255;
            g2 = 0/255;
            b2 = 102/255;
            type[i][j] = "fragile";
            state[i][j] = "present";
          }
          break;
          case 'X': {
                  type[i][j] = "bridgeHeavySwitch";
          }
          case 'B': {
                  r1 = 0.8f;
                  g1 = 0.8f;
                  b1 = 0.8f;

                  r2 = 0.3f;
                  g2 = 0.3f;
                  b2 = 0.3f;

                  type[i][j] = "bridgeLightSwitch";
                  state[i][j] = "present";
          }
          break;

          case 'x': {
                  type[i][j] = "bridgeHeavy";
          }
          case 'b': {
          r1 = 0.8f;
          g1 = 0.8f;
          b1 = 0.8f;

          r2 = 0.3f;
          g2 = 0.3f;
          b2 = 0.3f;

          type[i][j] = "bridgeLight";
          state[i][j] = "absent";
          id[i][j] -= 32;

          }
          break;
        }

      buildtile(i,j,initX,initY,z1, r1,r2,b1, b2,g1,g2);
      }
      else if(c == 'T')
      {
        goalx= (int) initX;
        goaly = (int) initY;
      }
      else if(c == '\n')
        break;
      else if(c == EOF) 
        break;
      initX++;
    }
    if(c == EOF)
    {
      fclose(file);
      break;
    }
    initX = -7;
    initY--;
  }
}

void createBlock1 ()
{
    float standingx1 = 0, standingx2 = 1, standingy0 = 0, standingy1 = -1, standingz0 = 0, standingz2= 2;
      // GL3 accepts only Triangles. Quads are not supported
    GLfloat vertex_buffer_data1 [] = {
      standingx1, standingy0, standingz0, // vertex 1
      standingx1, standingy1, standingz0, // vertex 2
      standingx2, standingy0, standingz0, // vertex 3
      standingx2, standingy0, standingz0, // vertex 3
      standingx2, standingy1, standingz0, // vertex 4
      standingx1, standingy1, standingz0, // vertex 2
      standingx1, standingy0, standingz0, // vertex 1
      standingx1, standingy0, standingz2, // vertex 5
      standingx1, standingy1, standingz0, // vertex 2
      standingx1, standingy0, standingz2, // vertex 5
      standingx1, standingy1, standingz2, // vertex 6
      standingx1, standingy1, standingz0, // vertex 2
      standingx2, standingy0, standingz0, // vertex 3
      standingx2, standingy0, standingz2, // vertex 7
      standingx2, standingy1, standingz0, // vertex 4
      standingx2, standingy0, standingz2, // vertex 7
      standingx2, standingy1, standingz2, // vertex 8
      standingx2, standingy1, standingz0, // vertex 4
      standingx1, standingy0, standingz0, // vertex 1
      standingx2, standingy0, standingz0, // vertex 3
      standingx1, standingy0, standingz2, // vertex 5
      standingx2, standingy0, standingz2, // vertex 7
      standingx2, standingy0, standingz0, // vertex 3
      standingx1, standingy0, standingz2, // vertex 5
      standingx1, standingy1, standingz0, // vertex 2
      standingx2, standingy1, standingz0, // vertex 4
      standingx1, standingy1, standingz2, // vertex 6
      standingx2, standingy1, standingz2, // vertex 8
      standingx2, standingy1, standingz0, // vertex 4
      standingx1, standingy1, standingz2, // vertex 6
      standingx1, standingy0, standingz2, // vertex 5
      standingx1, standingy1, standingz2, // vertex 6
      standingx2, standingy0, standingz2, // vertex 7
      standingx2, standingy0, standingz2, // vertex 7
      standingx1, standingy1, standingz2, // vertex 6
      standingx2, standingy1, standingz2  // vertex 8
    };

    GLfloat color_buffer_data1 [] = {
      0, 204/255, 102/205, // color 1
      0, 204/255, 102/205, // color 2
      0, 204/255, 102/205, // color 3

      0, 204/255, 102/205, // color 3
      0, 204/255, 102/205, // color 2
      0, 204/255, 102/205, // color 4

      1.0f, 204/255, 229/255, // color 1
      1.0f, 204/255, 229/255, // color 5
      1.0f, 204/255, 229/255, // color 2

      1.0f, 204/255, 229/255, // color 5
      1.0f, 204/255, 229/255, // color 6
      1.0f, 204/255, 229/255, // color 2

      1.0f, 204/255, 229/255, // color 3
      1.0f, 204/255, 229/255, // color 7
      1.0f, 204/255, 229/255, // color 4

      1.0f, 204/255, 229/255, // color 7
      1.0f, 204/255, 229/255, // color 8
      1.0f, 204/255, 229/255, // color 4

      1.0f, 204/255, 229/255, // color 1
      1.0f, 204/255, 229/255, // color 3
      1.0f, 204/255, 229/255, // color 5

      1.0f, 204/255, 229/255, // color 7
      1.0f, 204/255, 229/255, // color 3
      1.0f, 204/255, 229/255, // color 5

      1.0f, 204/255, 229/255, // color 2
      1.0f, 204/255, 229/255, // color 4
      1.0f, 204/255, 229/255, // color 6

      1.0f, 204/255, 229/255, // color 8
      1.0f, 204/255, 229/255, // color 4
      1.0f, 204/255, 229/255, // color 6

      0, 204/255, 102/205, // color 5
      0, 204/255, 102/205, // color 6
      0, 204/255, 102/205, // color 7

      0, 204/255, 102/205, // color 7
      0, 204/255, 102/205, // color 6
      0, 204/255, 102/205, // color 8
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    blockStanding = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data1, color_buffer_data1, GL_FILL);
}

void createBlock2() {

    float sleepingz0 = 0, sleepingx0 = 0, sleepingy0 = 0, sleepingz1= 1 , sleepingy1 = -1, sleepingx2 = 2;
    GLfloat vertex_buffer_data2 [] = {
      sleepingx0, sleepingy0, sleepingz0, // vertex 1
      sleepingx0, sleepingy1,sleepingz0, // vertex 2
      sleepingx2, sleepingy0, sleepingz0, // vertex 3

      sleepingx2, sleepingy0, sleepingz0, // vertex 3
      sleepingx0, sleepingy1,sleepingz0, // vertex 2
      sleepingx2, sleepingy1,sleepingz0,  // vertex 4

      sleepingx0, sleepingy0, sleepingz0, // vertex 1
      sleepingx0, sleepingy0, sleepingz1, // vertex 5
      sleepingx0, sleepingy1,sleepingz0, // vertex 2

      sleepingx0, sleepingy0, sleepingz1, // vertex 5
      sleepingx0, sleepingy1,sleepingz1, // vertex 6
      sleepingx0, sleepingy1,sleepingz0, // vertex 2

      sleepingx2, sleepingy0, sleepingz0, // vertex 3
      sleepingx2, sleepingy0, sleepingz1, // vertex 7
      sleepingx2, sleepingy1,sleepingz0, // vertex 4

      sleepingx2, sleepingy0, sleepingz1, // vertex 7
      sleepingx2, sleepingy1,sleepingz1, // vertex 8
      sleepingx2, sleepingy1,sleepingz0, // vertex 4

      sleepingx0, sleepingy0, sleepingz0, // vertex 1
      sleepingx2, sleepingy0, sleepingz0, // vertex 3
      sleepingx0, sleepingy0, sleepingz1, // vertex 5

      sleepingx2, sleepingy0, sleepingz1, // vertex 7
      sleepingx2, sleepingy0, sleepingz0, // vertex 3
      sleepingx0, sleepingy0, sleepingz1, // vertex 5

      sleepingx0, sleepingy1,sleepingz0, // vertex 2
      sleepingx2, sleepingy1,sleepingz0, // vertex 4
      sleepingx0, sleepingy1,sleepingz1, // vertex 6

      sleepingx2, sleepingy1,sleepingz1, // vertex 8
      sleepingx2, sleepingy1,sleepingz0, // vertex 4
      sleepingx0, sleepingy1,sleepingz1, // vertex 6

      sleepingx0, sleepingy0, sleepingz1, // vertex 5
      sleepingx0, sleepingy1,sleepingz1, // vertex 6
      sleepingx2, sleepingy0, sleepingz1, // vertex 7

      sleepingx2, sleepingy0, sleepingz1, // vertex 7
      sleepingx0, sleepingy1, sleepingz1, // vertex 6
      sleepingx2, sleepingy1, sleepingz1  // vertex 8
    };

    GLfloat color_buffer_data2 [] = {
      1.0f, 204/255, 229/255, // color 1
      1.0f, 204/255, 229/255, // color 2
      1.0f, 204/255, 229/255, // color 3

      1.0f, 204/255, 229/255, // color 3
      1.0f, 204/255, 229/255, // color 2
      1.0f, 204/255, 229/255, // color 4

      0, 204/255, 102/205, // color 1
      0, 204/255, 102/205, // color 5
      0, 204/255, 102/205, // color 2

      0, 204/255, 102/205, // color 5
      0, 204/255, 102/205, // color 6
      0, 204/255, 102/205, // color 2

      0, 204/255, 102/205, // color 3
      0, 204/255, 102/205, // color 7
      0, 204/255, 102/205, // color 4

      0, 204/255, 102/205, // color 7
      0, 204/255, 102/205, // color 8
      0, 204/255, 102/205, // color 4

      1.0f, 204/255, 229/255, // color 1
      1.0f, 204/255, 229/255, // color 3
      1.0f, 204/255, 229/255, // color 5

      1.0f, 204/255, 229/255, // color 7
      1.0f, 204/255, 229/255, // color 3
      1.0f, 204/255, 229/255, // color 5

      1.0f, 204/255, 229/255, // color 2
      1.0f, 204/255, 229/255, // color 4
      1.0f, 204/255, 229/255, // color 6

      1.0f, 204/255, 229/255, // color 8
      1.0f, 204/255, 229/255, // color 4
      1.0f, 204/255, 229/255, // color 6

      1.0f, 204/255, 229/255, // color 5
      1.0f, 204/255, 229/255, // color 6
      1.0f, 204/255, 229/255, // color 7

      1.0f, 204/255, 229/255, // color 7
      1.0f, 204/255, 229/255, // color 6
      1.0f, 204/255, 229/255, // color 8
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    blockSleepingX = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data2, color_buffer_data2, GL_FILL);
}

void createBlock3() {

    float sleepingx0 = 0, sleepingx1 = 1, sleepingz0 = 0, sleepingz1 = 1, sleepingy0 = 0, sleepingy2 = -2;
    GLfloat vertex_buffer_data3 [] = {
      sleepingx0, sleepingy0, sleepingz0, // vertex 1
      sleepingx0, sleepingy2, sleepingz0, // vertex 2
      sleepingx1, sleepingy0, sleepingz0, // vertex 3

      sleepingx1, sleepingy0, sleepingz0, // vertex 3
      sleepingx0, sleepingy2, sleepingz0, // vertex 2
      sleepingx1, sleepingy2, sleepingz0,  // vertex 4

      sleepingx0, sleepingy0, sleepingz0, // vertex 1
      sleepingx0, sleepingy0, sleepingz1, // vertex 5
      sleepingx0, sleepingy2, sleepingz0, // vertex 2

      sleepingx0, sleepingy0, sleepingz1, // vertex 5
      sleepingx0, sleepingy2, sleepingz1, // vertex 6
      sleepingx0, sleepingy2, sleepingz0, // vertex 2

      sleepingx1, sleepingy0, sleepingz0, // vertex 3
      sleepingx1, sleepingy0, sleepingz1, // vertex 7
      sleepingx1, sleepingy2, sleepingz0, // vertex 4

      sleepingx1, sleepingy0, sleepingz1, // vertex 7
      sleepingx1, sleepingy2, sleepingz1, // vertex 8
      sleepingx1, sleepingy2, sleepingz0, // vertex 4

      sleepingx0, sleepingy0, sleepingz0, // vertex 1
      sleepingx1, sleepingy0, sleepingz0, // vertex 3
      sleepingx0, sleepingy0, sleepingz1, // vertex 5

      sleepingx1, sleepingy0, sleepingz1, // vertex 7
      sleepingx1, sleepingy0, sleepingz0, // vertex 3
      sleepingx0, sleepingy0, sleepingz1, // vertex 5

      sleepingx0, sleepingy2, sleepingz0, // vertex 2
      sleepingx1, sleepingy2, sleepingz0, // vertex 4
      sleepingx0, sleepingy2, sleepingz1, // vertex 6

      sleepingx1, sleepingy2, sleepingz1, // vertex 8
      sleepingx1, sleepingy2, sleepingz0, // vertex 4
      sleepingx0, sleepingy2, sleepingz1, // vertex 6

      sleepingx0, sleepingy0, sleepingz1, // vertex 5
      sleepingx0, sleepingy2, sleepingz1, // vertex 6
      sleepingx1, sleepingy0, sleepingz1, // vertex 7

      sleepingx1, sleepingy0, sleepingz1, // vertex 7
      sleepingx0, sleepingy2, sleepingz1, // vertex 6
      sleepingx1, sleepingy2, sleepingz1  // vertex 8
    };

    GLfloat color_buffer_data3 [] = {
      1.0f, 204/255, 229/255, // color 1
      1.0f, 204/255, 229/255, // color 2
      1.0f, 204/255, 229/255, // color 3

      1.0f, 204/255, 229/255, // color 3
      1.0f, 204/255, 229/255, // color 2
      1.0f, 204/255, 229/255, // color 4

      1.0f, 204/255, 229/255, // color 1
      1.0f, 204/255, 229/255, // color 5
      1.0f, 204/255, 229/255, // color 2

      1.0f, 204/255, 229/255, // color 5
      1.0f, 204/255, 229/255, // color 6
      1.0f, 204/255, 229/255, // color 2

      1.0f, 204/255, 229/255, // color 3
      1.0f, 204/255, 229/255, // color 7
      1.0f, 204/255, 229/255, // color 4

      1.0f, 204/255, 229/255, // color 7
      1.0f, 204/255, 229/255, // color 8
      1.0f, 204/255, 229/255, // color 4

      0, 204/255, 102/205, // color 1
      0, 204/255, 102/205, // color 3
      0, 204/255, 102/205, // color 5

      0, 204/255, 102/205, // color 7
      0, 204/255, 102/205, // color 3
      0, 204/255, 102/205, // color 5

      0, 204/255, 102/205, // color 2
      0, 204/255, 102/205, // color 4
      0, 204/255, 102/205, // color 6

      0, 204/255, 102/205, // color 8
      0, 204/255, 102/205, // color 4
      0, 204/255, 102/205, // color 6

      1.0f, 204/255, 229/255, // color 5
      1.0f, 204/255, 229/255, // color 6
      1.0f, 204/255, 229/255, // color 7

      1.0f, 204/255, 229/255, // color 7
      1.0f, 204/255, 229/255, // color 6
      1.0f, 204/255, 229/255, // color 8
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    blockSleepingY = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data3, color_buffer_data3, GL_FILL);
}

int i;
void drawBridge(string type1, int id1)
{
  i = 0;
  while(i != 10)
  {
    int j = 0;
    while(j != 20)
    {
        string str = type[i][j];
        str += "Switch";
        if(exists[i][j] && id[i][j] == id1)
          if(str == type1)
            if(state[i][j] == "absent")
              state[i][j] = "present";
      j++;
    }
    i++;
  }
}

void updateBlockCoordinates()
{
  int flag = 0;
  i = 0;
  while(i != 4)
  {
    if(rotateDirection[i])
    {
      memset(rotateDirection, 0, sizeof(rotateDirection));
      if(blockState == "standing")
      {
        flag += 1;
        if(i >= 2)
        {
          blockState = "sleepingX";
          if(i != 3) blockX -= 2;
          else blockX += 1;
        }
        else
        {
          blockState = "sleepingY";
          if(i != 0) blockY -= 1;
          else blockY += 2;
        }
      }
      else if(blockState == "sleepingX")
      {
        if(flag == 0) {
          flag += 1;
          if(i >= 2)
          {
            blockState = "standing";
            if(i != 3) blockX -= 1; 
            else blockX += 2;
          }
          else
          {
              if(i == 0) blockY += 1;
              else if(i == 1)  blockY -= 1;
          }
        }
      }
      else if(flag == 0)
      {
        if(i >= 2)
        {
          if(i != 3)  blockX -= 1;
          else blockX += 1;
        }
        else
        {
          blockState = "standing";
            if(i != 0)  blockY -= 2;
            else  blockY += 1;
        }
      }
    }
    i += 1;
  }
  return;
}

void setCamera(int view, glm::vec3 * target, glm::vec3 * eye, glm::vec3 * up)
{
  int i, j;
  j = blockX + 7;
  i = 5 - blockY;
  float x = 0.0f, y = 0.0f, z = 0.5f, targetX = 0.0f, targetY = 0.0f, targetZ = 0.0f, upAngle, temp1, temp2;

  // In case Block is standing and view is NOT chase cam
  if(blockState == "standing")
    z = 1.5f;

  // Viewing/Chasing in +y direction (upwards)
  if(view == 2 || view == 6)
  {
    x = 0.5f;
    // Specific case for Chase Cam
    if(view == 6)
    {
      y = -3.0f;
      z = 4.0f;
    }

    targetX = ((float)blockX);
    targetX += x;
    while(exists[i--][j])
    targetY = 5-i;
  }

  // Viewing/Chasing in -y direction (downwards)
  else if(view == 3 || view == 7)
  {
    x = 0.5f;
    y = -1.0f;
    if(blockState == "sleepingY")
    {
      y = -2.0f;
      z = 4.0f;
    }

    // Specific case for Chase Cam
    if(view == 7)
    {
      y = 3.0f;
      z = 4.0f;
    }

    targetX = ((float)blockX) + x;
    while(exists[i++][j])
    targetY = 5-i;
  }

  // Viewing/Chasing in -x direction (leftwards)
  else if(view == 4 || view == 8)
  {
    y = -0.5f;

    // Specific case for Chase Cam
    if(view == 8)
    {
      x = 3.0f;
      z = 4.0f;
    }

    while(exists[i][j--])
    targetX = j-7;
    targetY = ((float)blockY) + y;
  }

  // Viewing/Chasing in +x direction (rightwards)
  else if(view == 5 || view == 9)
  {
    x = 1.0f;
    y = -0.5f;
    if(blockState == "sleepingX")
      x = 2.0f;

    // Specific case for Chase Cam
    if(view == 9)
    {
      x = -3.0f;
      z = 4.0f;
    }

    while(exists[i][j++])
    targetX = j - 7;
    targetY = ((float)blockY) + y;
  }

  * eye = glm::vec3 (((float)blockX)+x, ((float)blockY)+y, z);
  * target = glm::vec3 (targetX, targetY, targetZ);
  * up = glm::vec3 (0, 1, 0);

  // Helicopter View
  if(view == 10)
  {
    * eye = glm::vec3 (((float)blockX) + cos(angle*M_PI/180.0f)*5.0f, ((float)blockY) + sin(angle*M_PI/180.0f)*5.0f, 7.0f);
    * target = glm::vec3 (((float)blockX) + 0.5f, ((float)blockY) - 0.5f, 2.0f);

    temp1 = (((float)blockX) + cos(angle*M_PI/180.0f)*5.0f)*(((float)blockX) + cos(angle*M_PI/180.0f)*5.0f);
    temp2 = (((float)blockY) + sin(angle*M_PI/180.0f)*5.0f)*(((float)blockY) + sin(angle*M_PI/180.0f)*5.0f);
    temp1 = sqrt(temp1 + temp2);

    upAngle = 90.0f + ((float)atan2(7.0f, temp1));

    * up = glm::vec3 (cos(upAngle*M_PI/180.0f)*cos((angle + 180.0f)*M_PI/180.0f),
                      cos(upAngle*M_PI/180.0f)*sin((angle + 180.0f)*M_PI/180.0f),
                      sin(upAngle*M_PI/180.0f));
  }
}


/* Render the scene with openGL */
void draw (GLFWwindow* window, float x, float y, float w, float h)
{
    int fbwidth, fbheight, i, j;
    float angle = 0;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
    glViewport((int)(x*fbwidth), (int)(y*fbheight), (int)(w*fbwidth), (int)(h*fbheight));

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    // Eye - Location of camera. Don't change unless you are sure!!
    // glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
    glm::vec3 eye (0,0,5);
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (0, 0, 0);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 1, 0);

    // Compute Camera matrix (view)
    // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
    //  Don't change unless you are sure!!

    // Fixed camera for 2D (ortho) in XY plane
    Matrices.view = glm::lookAt(eye, target, up);

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projectionP * Matrices.view;

    if(!proj_type)
    {
      VP = Matrices.projectionO * Matrices.view;
    }
    else if(proj_type == 1)
    {
      eye = glm::vec3(0, -6, 7);
      target = glm::vec3(0, 0, 0);
      Matrices.view = glm::lookAt(eye, target, up);
      VP = Matrices.projectionP * Matrices.view;
    }
    else
    {
      setCamera(proj_type, &target, &eye, &up);
      Matrices.view = glm::lookAt(eye, target, up);
      VP = Matrices.projectionP * Matrices.view;
    }

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP;	// MVP = Projection * View * Model

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */

    if(levelUp)
    {
      resetTileState();
      createTiles();
      createBlock1();
      createBlock2();
      createBlock3();
      takingInput = false;
      return;
    }
    takingInput = true;

    // Draw Tiles
    for(i = 0; i < 10; i++)
    {
      for(j = 0; j < 20; j++)
      {
        if(exists[i][j] && state[i][j] == "present")
        {
          Matrices.model = glm::mat4(1.0f);
          glm::mat4 tileTranslation = glm::translate (glm::vec3((0.0f, 0.0f, z[i][j]))); // glTranslatef
          Matrices.model *= (tileTranslation);
          MVP = VP * Matrices.model;
          glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
          draw3DObject(tile[i][j]);
        }
      }
    }
    for(i = 0; i < 10; i++)
    {
      for(j = 0; j < 20; j++)
      {
        if(exists[i][j] && state[i][j] == "present")
        {
          Matrices.model = glm::mat4(1.0f);
          glm::mat4 tileTranslation = glm::translate (glm::vec3((0.0f, 0.0f, z[i][j]))); // glTranslatef
          Matrices.model *= (tileTranslation);
          MVP = VP * Matrices.model;
          glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
          draw3DObject(tileBorder[i][j]);
        }
      }
    }
    int flag = 0;

    // Draw Block
    Matrices.model = glm::mat4(1.0f);

    updateBlockCoordinates();

    glm::mat4 blockTranslation = glm::translate (glm::vec3((float)blockX, (float)blockY, 0.0f)); // glTranslatef
    Matrices.model *= (blockTranslation);
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    if(blockState == "standing")
    {
      draw3DObject(blockStanding);
      if (blockX == goalx && blockY == goaly)
      {
        levelUp = true;
        last_update_time = glfwGetTime();
        level++;
      }
      else if(!exists[5-blockY][blockX+7] ||
              type[5-blockY][blockX+7] == "fragile" ||
              state[5-blockY][blockX+7] == "absent")
      {
        gameOver = true;
      }

      else if(type[5-blockY][blockX+7] == "bridgeLightSwitch" ||
              type[5-blockY][blockX+7] == "bridgeHeavySwitch")
      {
        drawBridge(type[5-blockY][blockX+7], id[5-blockY][blockX+7]);
      }
    }
    else if(blockState == "sleepingX")
    {
      draw3DObject(blockSleepingX);
      if(!(blockX == goalx && blockY == goaly) &&
        (!(blockX + 1 == goalx && blockY == goaly)) &&
        ((!exists[5-blockY][blockX+7] || !exists[5-blockY][blockX+8]) ||
        (state[5-blockY][blockX+7] == "absent" || state[5-blockY][blockX+8] == "absent")))
      {
        gameOver = true;
      }
      else if(type[5-blockY][blockX+7] == "bridgeLightSwitch")
      {
        drawBridge(type[5-blockY][blockX+7], id[5-blockY][blockX+7]);
      }
      else if(type[5-blockY][blockX+8] == "bridgeLightSwitch")
      {
        drawBridge(type[5-blockY][blockX+8], id[5-blockY][blockX+8]);
      }
    }
    else
    {
      draw3DObject(blockSleepingY);
      if(!(blockX == goalx && blockY == goaly) &&
        (!(blockX == goalx && blockY - 1 == goaly)) &&
        ((!exists[5-blockY][blockX+7] || !exists[6-blockY][blockX+7]) ||
        (state[5-blockY][blockX+7] == "absent" || state[6-blockY][blockX+7] == "absent")))
        gameOver = true;
      else if(type[5-blockY][blockX+7] == "bridgeLightSwitch")
        drawBridge(type[5-blockY][blockX+7], id[5-blockY][blockX+7]);
      else if(type[6-blockY][blockX+7] == "bridgeLightSwitch")
        drawBridge(type[6-blockY][blockX+7], id[6-blockY][blockX+7]);
    }
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, scroll_callback);

    windowCopy = window;
    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models

  // Generate the VAO, VBOs, vertices data & copy into the array buffer
  createTiles ();
  createBlock1 ();
  createBlock2();
  createBlock3();

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (1.0f, 1.0f, 1.0f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
  int width = 550;
  int height = 550;
  rot["up"] = 0;
  rot["down"] = 1;
  rot["left"] = 2;
  rot["right"] = 3;
  mpg123_handle *mh;
  unsigned char * buffer;
  size_t buffer_size;
  size_t done;
  int i, bufferCount, err;

  int defaultDriver;
  ao_device *dev;
  
  ao_sample_format format;
  int channels, encoding;
  long rate;

  proj_type = 0;

    GLFWwindow* window = initGLFW(width, height);

  initialize();

	initGL (window, width, height);

  last_update_time = glfwGetTime();

  ao_initialize();
  
  defaultDriver = ao_default_driver_id();
  mpg123_init();
  mh = mpg123_new(NULL, &err);
  buffer_size = 4096;
  buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
  
  /* open the file and get the decoding format */
  mpg123_open(mh, "bomberman.mp3");
  mpg123_getformat(mh, &rate, &channels, &encoding);
  
  /* set the output format and open the output device */
  format.bits = mpg123_encsize(encoding) * BITS;
  format.rate = rate;
  format.channels = channels;
  format.byte_format = AO_FMT_NATIVE;
  format.matrix = 0;
  dev = ao_open_live(defaultDriver, &format, NULL);

    /* Draw in loop */
    while (!glfwWindowShouldClose(window) && !gameOver) {

      	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Update Scores
        
        // OpenGL Draw commands
        
        draw(window, 0, 0, 1, 1);
        /* Play sound */
        if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
        {
          ao_play(dev, (char *)buffer, done);
        }
        else
        {
          mpg123_seek(mh, 0, SEEK_SET);
        }

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= updateTime) { // atleast 1s elapsed since last frame
            // do something every 0.5 seconds ..
            if(levelUp)
            {
              levelUp = false;
            }
            last_update_time = current_time;
        }
    }

    printf("\n\nGame Over!\n______________________\n\nYou Final Score is %d\n\n", moveCount);
    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
