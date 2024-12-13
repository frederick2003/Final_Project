#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/shader.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <glm/gtc/noise.hpp>

// In this lab we store our GLSL shaders as C++ string in a header file and load them directly instead of reading them from files
#include "axis_and_movement.h"

static GLFWwindow *window;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// OpenGL camera view parameters
static glm::vec3 eye_center(300.0f, 300.0f, 300.0f);
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

// View control
static float viewAzimuth = 0.0f;
static float viewPolar = 0.0f;
static float viewDistance = 300.0f;

struct AxisXYZ {
    // A structure for visualizing the global 3D coordinate system

	GLfloat vertex_buffer_data[18] = {
		// X axis
		0.0, 0.0f, 0.0f,
		100.0f, 0.0f, 0.0f,

		// Y axis
		0.0f, 0.0f, 0.0f,
		0.0f, 100.0f, 0.0f,

		// Z axis
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 100.0f,
	};

	GLfloat color_buffer_data[18] = {
		// X, red
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Y, green
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Z, blue
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
	};

	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint colorBufferID;

	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint programID;

	void initialize() {
		// Create a vertex array object
		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		// Create a vertex buffer object to store the vertex data
		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		// Create a vertex buffer object to store the color data
		glGenBuffers(1, &colorBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(color_buffer_data), color_buffer_data, GL_STATIC_DRAW);

		// Create and compile our GLSL program from the shaders
		programID = LoadShadersFromString(cubeVertexShader, cubeFragmentShader);
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		// Get a handle for our "MVP" uniform
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glm::mat4 mvp = cameraMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        // Draw the lines
        glDrawArrays(GL_LINES, 0, 6);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
	}

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteProgram(programID);
	}
};


struct Ground {
    // A structure for visualizing a meshed floor

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<unsigned int> indices;

    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint colorBufferID;
    GLuint indexBufferID;

    GLuint mvpMatrixID;
    GLuint programID;

    float size;
    int divisions;

    Ground(float gridSize, int gridDivisions) : size(gridSize), divisions(gridDivisions) {}

    void initialize(float (*heightFunction)(float, float)) {
        // Generate floor vertices, colors, and indices
        generate(heightFunction);

        // Create and bind vertex array object
        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);

        // Create and bind vertex buffer object
        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);

        // Create and bind color buffer object
        glGenBuffers(1, &colorBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_STATIC_DRAW);

        // Create and bind index buffer object
        glGenBuffers(1, &indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Create and compile shader program
        programID = glCreateProgram(); // Replace with actual shader program setup
        mvpMatrixID = glGetUniformLocation(programID, "MVP");
    }

    void generate(float (*heightFunction)(float, float)) {
        positions.clear();
        colors.clear();
        indices.clear();

        float step = size / divisions; // Grid cell size
        float halfSize = size / 2.0f;  // Center the grid at the origin

        // Generate vertices and colors
        for (int i = 0; i <= divisions; ++i) {
            for (int j = 0; j <= divisions; ++j) {
                float x = -halfSize + j * step;
                float z = -halfSize + i * step;
                float y = heightFunction(x, z);

                // Add position
                positions.push_back(glm::vec3(x, y, z));

            	// Set color based on height (higher is lighter, lower is darker)
            	float brightness = glm::clamp((y + size / 2) / size, 0.0f, 1.0f); // Normalize brightness to [0, 1]
            	glm::vec3 color = glm::vec3(0.0f, 0.0f , brightness);
            	colors.push_back(color);
            }
        }

        // Generate indices for triangle mesh
        for (int i = 0; i < divisions; ++i) {
            for (int j = 0; j < divisions; ++j) {
                unsigned int topLeft = i * (divisions + 1) + j;
                unsigned int topRight = topLeft + 1;
                unsigned int bottomLeft = (i + 1) * (divisions + 1) + j;
                unsigned int bottomRight = bottomLeft + 1;

                // Triangle 1
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // Triangle 2
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
    }

    void render(const glm::mat4& cameraMatrix) {
        glUseProgram(programID);

        glBindVertexArray(vertexArrayID);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glm::mat4 mvp = cameraMatrix;
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, (void*)0);

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }

    void cleanup() {
        glDeleteBuffers(1, &vertexBufferID);
        glDeleteBuffers(1, &colorBufferID);
        glDeleteBuffers(1, &indexBufferID);
        glDeleteVertexArrays(1, &vertexArrayID);
        glDeleteProgram(programID);
    }

    static float heightFunction(float x, float z) {
    	return glm::perlin(glm::vec2(x * 0.1f, z * 0.1f)) * 7.5f; // Scale noise and height
    }
};

int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Lab 1", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to open a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	// Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	// Background
	glClearColor(0.2f, 0.2f, 0.2f, 0.f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// A coordinate system
    AxisXYZ debugAxes;
    debugAxes.initialize();

	// Initialize the ground
	Ground ground(200.0f, 40);
	ground.initialize(Ground::heightFunction);


	// TODO: Prepare a perspective camera
	// ------------------------------------
	glm::float32 FoV = 30;
	glm::float32 zNear = 0.1f;
	glm::float32 zFar = 1000.0f;
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, zNear, zFar);

    // ------------------------------------

	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// TODO: Set camera view matrix
		// ------------------------------------
        glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
		// ------------------------------------

		// For convenience, we multiply the projection and view matrix together and pass a single matrix for rendering
		glm::mat4 vp = projectionMatrix * viewMatrix;

		// Visualize the global axes
        debugAxes.render(vp);
		ground.render(vp);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Close OpenGL window and terminate GLFW
	ground.cleanup();
	glfwTerminate();

	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	glm::vec3 direction = glm::normalize(lookat - eye_center);
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		std::cout << "Reset." << std::endl;
	}

	if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.y += 2.0f;
	}

	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.y -= 2.0f;
	}

	if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.x += 2.0f;
		std::cout << eye_center.x << std::endl;
	}

	if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.x -= 2.0f;
	}

	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{

		eye_center.x += direction.x * 2.0f;
		eye_center.y += direction.y * 2.0f;
		eye_center.z += direction.z * 2.0f;
	}

	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.x -= direction.x * 2.0f;
		eye_center.y -= direction.y * 2.0f;
		eye_center.z -= direction.z * 2.0f;
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}
