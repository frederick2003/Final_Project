#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <render/shader.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glm/gtc/noise.hpp>

// In this lab we store our GLSL shaders as C++ string in a header file and load them directly instead of reading them from files
#include "axis_and_movement.h"
#include "Sea_Shader.h"
#include "SkyBox_Shaders.h"
#include "renderTextures.h"

static GLFWwindow *window;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static GLuint LoadTextureTileBox(const char *texture_file_path);

// OpenGL camera view parameters
//static glm::vec3 eye_center(-360.0, 60.0f, 300.0f);
//static glm::vec3 eye_center(0.0, 160.0f, -300.0f);
static glm::vec3 eye_center(-360.0, 480.0f, 300.0f);
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


    	programID = LoadShadersFromString(cubeVertexShader, cubeFragmentShader);
    	if (programID == 0)
    	{
    		std::cerr << "Failed to load shaders." << std::endl;
    	}
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

struct CliffWalls {
    // A structure for visualizing vertical cliff walls

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
    float wallHeight;

    CliffWalls(float height, float gridSize, int gridDivisions)
        : wallHeight(height), size(gridSize), divisions(gridDivisions) {}

    void initialize() {
        // Generate wall vertices, colors, and indices
        generate();

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

    void generate() {
        positions.clear();
        colors.clear();
        indices.clear();

        float step = size / divisions; // Grid cell size
        float halfSize = size / 2.0f;  // Center the walls relative to the ground

        // Wall 1: Along the Z-axis at X = -halfSize
        for (int i = 0; i <= divisions; ++i) {
            for (int j = 0; j <= divisions; ++j) {
                float y = -halfSize + i * step;
                float x = -halfSize;
                float z = -halfSize + j * step;

                // Interpolated height for Wall 1
                float heightWall1 = wallHeight;

                // Add vertices for Wall 1
                positions.push_back(glm::vec3(x, y, z)); // Bottom
                positions.push_back(glm::vec3(x, y + heightWall1, z)); // Top

            	glm::vec3 fixedColor(0.4f, 0.2f, 0.1f);
            	colors.push_back(fixedColor);
            	colors.push_back(fixedColor);
            }
        }

        // Wall 2: Along the X-axis at Z = -halfSize
        for (int i = 0; i <= divisions; ++i) {
            for (int j = 0; j <= divisions; ++j) {
                float y = -halfSize + i * step;
                float z = -halfSize;
                float x = -halfSize + j * step;

                // Interpolated height for Wall 2
                float heightWall2 = wallHeight;

                // Add vertices for Wall 2
                positions.push_back(glm::vec3(x, y, z)); // Bottom
                positions.push_back(glm::vec3(x, y + heightWall2, z)); // Top


            	glm::vec3 fixedColor(0.4f, 0.2f, 0.1f);
            	colors.push_back(fixedColor);
            	colors.push_back(fixedColor);
            }
        }

        // Generate indices for triangle mesh
        for (int i = 0; i < divisions; ++i) {
            for (int j = 0; j < divisions; ++j) {
                unsigned int baseIndex = i * (divisions + 1) * 2 + j * 2;

                // Triangle 1 for Wall 1
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex + 2);

                indices.push_back(baseIndex + 1);
                indices.push_back(baseIndex + 3);
                indices.push_back(baseIndex + 2);

                // Triangle 1 for Wall 2
                unsigned int wall2Offset = (divisions + 1) * (divisions + 1) * 2; // Offset for Wall 2
                indices.push_back(baseIndex + wall2Offset);
                indices.push_back(baseIndex + wall2Offset + 2);
                indices.push_back(baseIndex + wall2Offset + 1);

                indices.push_back(baseIndex + wall2Offset + 1);
                indices.push_back(baseIndex + wall2Offset + 2);
                indices.push_back(baseIndex + wall2Offset + 3);
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
};

struct infiniteSea {
	GLuint vertexArrayID, vertexBufferID, indexBufferID;
	GLuint programID, MVPmatrixID, cameraPosID, seaColorID, backgroundColorID, waveFrequencyID, waveAmplitudeID, gridSpacingID;

	float quadVertices[12] = {
		// Positions (X, Y, Z)
		-100.0f, 0.0f, -100.0f,
		 100.0f, 0.0f, -100.0f,
		-100.0f,  0.0f, 100.0f,
		 100.0f,  0.0f, 100.0f
	};
	unsigned int planeIndices[6] = {
		0, 2, 1,
		2, 3, 1
	};


	void initialize() {
		glGenVertexArrays(1, &vertexArrayID);
		glGenBuffers(1, &vertexBufferID);
		glGenBuffers(1, &indexBufferID);

		glBindVertexArray(vertexArrayID);

		// Vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

		// Vertex attributes
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0); // Unbind VAO

		programID = LoadShadersFromString(Sea_Vertex_Shader, Sea_Fragment_Shader);
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}
		else {
			std::cout << "Shaders loaded correctly." << std::endl;
		}
		MVPmatrixID = glGetUniformLocation(programID, "MVP");
		cameraPosID = glGetUniformLocation(programID, "cameraPosition");
		seaColorID = glGetUniformLocation(programID, "seaColor");
		backgroundColorID = glGetUniformLocation(programID, "backgroundColor");
		waveFrequencyID = glGetUniformLocation(programID, "waveFrequency");
		waveAmplitudeID = glGetUniformLocation(programID, "waveAmplitude");
		gridSpacingID = glGetUniformLocation(programID, "gridSpacing");
	}

	void render(const glm::mat4& cameraMatrix, const glm::vec3& cameraPositionMatrix) {
		glUseProgram(programID);

		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);

		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(cameraPositionMatrix.x, 0.0f, cameraPositionMatrix.z));

		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(MVPmatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniform2f(cameraPosID, cameraPositionMatrix.x, cameraPositionMatrix.y);
		glUniform3f(seaColorID, 0.0f, 0.3f, 0.7f); // Blue sea
		glUniform3f(backgroundColorID, 1.0f, 0.0, 0.0f); // Dark background
		glUniform1f(waveFrequencyID, 0.05f);
		glUniform1f(waveAmplitudeID, 0.05f);
		glUniform1f(gridSpacingID, 5.0f);

		// Draw the quad
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // Fullscreen quad with 4 vertices

		glBindVertexArray(0); // Disable attributes after drawing
	}

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);;
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteProgram(programID);
	}
};

struct skybox {
 GLuint skyboxVAO, skyboxVBO;
    GLuint cubemapTexture;
    GLuint programID;
    GLuint viewLoc, projectionLoc;


		float skyboxVertices[108] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	static GLuint loadCubemap(std::vector<std::string> faces) {
		GLuint textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		for (GLuint i = 0; i < faces.size(); i++) {
			int width, height, nrChannels;
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				stbi_image_free(data);
			} else {
				std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
				stbi_image_free(data);
			}
		}


		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


		return textureID;
	}

    skybox(std::vector<std::string> faces) {

        cubemapTexture = loadCubemap(faces);

        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);


        glBindVertexArray(skyboxVAO);


        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);


        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glBindVertexArray(0);  // Unbind VAO

		programID = LoadShadersFromString(SkyboxVertexShader, SkyboxFragmentShader);
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}
        viewLoc = glGetUniformLocation(programID, "view");
        projectionLoc = glGetUniformLocation(programID, "projection");

    }

	void render(glm::mat4 view, glm::mat4 projection) {
		glDepthFunc(GL_LEQUAL);
		glUseProgram(programID);

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(glm::mat3(view))));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

		if (viewLoc == -1 || projectionLoc == -1) { std::cerr << "Error: Could not find 'view' or 'projection' uniforms in the shader." << std::endl; }

		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindVertexArray(0);
		glUseProgram(0);

		glDepthFunc(GL_LESS);
	}
};

struct cliffWall {
	glm::vec3 position;			// Position of the cliff face
	glm::vec3 scale;			// Size of the face on each axis

	GLfloat vertex_buffer_data[12] = {
		// Face
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 0.5f, 1.0f,
		-1.0f, 0.5f, -1.0f
	};

	GLfloat colour_buffer_data[12] = {
		0.1f, 0.1f, 0.1f,
		0.1f, 0.1f, 0.1f,
		0.1f, 0.1f, 0.1f,
		0.1f, 0.1f, 0.1f
		};

	GLfloat uv_buffer_data[8] {
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f
	};


	GLuint index_buffer_data[6] {
		0,1,2, //First triangle
		2,3,0  //Second
	};

	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint colorBufferID;
	GLuint uvBufferID;
	GLuint TextureID;


	// Shader variable IDs
	GLuint textureSamplerID;
	GLuint mvpMatrixID;
	GLuint programID;

	void intialize(glm::vec3 position, glm::vec3 scale) {
		// Define scale of the box geometry
		this->position = position;
		this->scale = scale;

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
		glBufferData(GL_ARRAY_BUFFER, sizeof(colour_buffer_data), colour_buffer_data, GL_STATIC_DRAW);

		// Create an index buffer object to store the index data that defines triangle faces
		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

		// Create a VBO to store the UV data
		for (int i = 0; i < 4; ++i) uv_buffer_data[2*i] *= 2;
		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data,GL_STATIC_DRAW);


		programID = LoadShadersFromString(textureVertexShader, textureFragmentShader);
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		TextureID = LoadTextureTileBox("../Final_Project/Cliff_face.jpg");
		textureSamplerID = glGetUniformLocation(programID, "textureSampler");
		if ( mvpMatrixID== -1 || TextureID == -1 || textureSamplerID == -1 ) {
			std::cout << "Error loading shaders." << std::endl;

		}
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		// TODO: Model transform
		// ------------------------------------
		glm::mat4 modelMatrix = glm::mat4();
		// Translate the cliff to its position
		modelMatrix = glm::translate(modelMatrix, position);
		// Scale the cliff along each axis
		modelMatrix = glm::scale(modelMatrix, scale);
		// Rotate the cliff face
		modelMatrix = glm::rotate(modelMatrix,glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));

		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// Set textureSampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureID);
		glUniform1i(textureSamplerID, 0);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			6,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteTextures(1, &TextureID);
		glDeleteProgram(programID);
	}
};

struct Turf {
	glm::vec3 position;			// Position of the cliff face
	glm::vec3 scale;			// Size of the face on each axis

	GLfloat vertex_buffer_data[12] = {
		// Face
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f
	};

	GLfloat colour_buffer_data[12] = {
		0.1f, 0.1f, 0.1f,
		0.1f, 0.1f, 0.1f,
		0.1f, 0.1f, 0.1f,
		0.1f, 0.1f, 0.1f
		};

	GLfloat uv_buffer_data[8] {
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f
	};


	GLuint index_buffer_data[6] {
		0,1,2, //First triangle
		2,3,0  //Second
	};

	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint colorBufferID;
	GLuint uvBufferID;
	GLuint TextureID;


	// Shader variable IDs
	GLuint textureSamplerID;
	GLuint mvpMatrixID;
	GLuint programID;

	void intialize(glm::vec3 position, glm::vec3 scale) {
		// Define scale of the box geometry
		this->position = position;
		this->scale = scale;

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
		glBufferData(GL_ARRAY_BUFFER, sizeof(colour_buffer_data), colour_buffer_data, GL_STATIC_DRAW);

		// Create an index buffer object to store the index data that defines triangle faces
		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

		// Create a VBO to store the UV data
		for (int i = 0; i < 4; ++i) uv_buffer_data[2*i] *= 2;
		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data,GL_STATIC_DRAW);


		programID = LoadShadersFromString(textureVertexShader, textureFragmentShader);
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		TextureID = LoadTextureTileBox("../Final_Project/grass_texture.jpg");
		textureSamplerID = glGetUniformLocation(programID, "textureSampler");
		if ( mvpMatrixID== -1 || TextureID == -1 || textureSamplerID == -1 ) {
			std::cout << "Error loading shaders." << std::endl;

		}
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		// TODO: Model transform
		// ------------------------------------
		glm::mat4 modelMatrix = glm::mat4();
		// Translate the cliff to its position
		modelMatrix = glm::translate(modelMatrix, position);
		// Scale the cliff along each axis
		modelMatrix = glm::scale(modelMatrix, scale);
		// Rotate the cliff face
		modelMatrix = glm::rotate(modelMatrix,glm::radians(45.0f),glm::vec3(0.0f,1.0f,0.0f));

		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// Set textureSampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureID);
		glUniform1i(textureSamplerID, 0);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			6,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteTextures(1, &TextureID);
		glDeleteProgram(programID);
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
	window = glfwCreateWindow(1024, 768, "Final Project", NULL, NULL);
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


	//------------------------------------------------------------------
	// Initialize the ground
	Ground ground(200.0f, 40);
	ground.initialize(Ground::heightFunction);

	// ------------------------------------------------------------------
	// Initialize cliff wall
	cliffWall myCliff;
	myCliff.intialize(glm::vec3(80, 30, -100), glm::vec3(130, 35, 10));

	//--------------------------------------------------------------------------------------
	// Create a turf texture
	Turf myTurf;
	myTurf.intialize(glm::vec3(25, -2, -200), glm::vec3(100, 50, 100));

	// ------------------------------------------------------------------------------------
	// Prepare a perspective camera
	glm::float32 FoV = 30;
	glm::float32 zNear = 0.1f;
	glm::float32 zFar = 1000.0f;
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, zNear, zFar);

    // ------------------------------------

	skybox mySkybox({"../Final_Project/px.png", "../Final_Project/nx.png", "../Final_Project/py.png", "../Final_Project/ny.png","../Final_Project/pz.png", "../Final_Project/nz.png" });
	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// -------------------------------------------------------------------------------------
		// For convenience, we multiply the projection and view matrix together and pass a single matrix for rendering
		glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
		glm::mat4 vp = projectionMatrix * viewMatrix;


		//------------------------------------------------------------------------------
		//Render object to the screen
		mySkybox.render(viewMatrix,projectionMatrix);
		ground.render(vp);
		myTurf.render(vp);
		myCliff.render(vp);

		//------------------------------------------------------------------------------
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Destroy all objects created
	ground.cleanup();
	myCliff.cleanup();
	myTurf.cleanup();

	// Close OpenGL window and terminate GLFW
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
		eye_center.y += 20.0f;
		std::cout << "(" << eye_center.x <<"," << eye_center.y << "," << eye_center.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		if (eye_center.y>=40) {
			eye_center.y -= 20.0f;
		}
		std::cout << "(" << eye_center.x <<"," << eye_center.y << "," << eye_center.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.x -= 20.0f;
		std::cout << "(" << eye_center.x <<"," << eye_center.y << "," << eye_center.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.x += 20.0f;
		std::cout << "(" << eye_center.x <<"," << eye_center.y << "," << eye_center.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{

		eye_center.x += direction.x * 20.0f;
		eye_center.y += direction.y * 20.0f;
		eye_center.z += direction.z * 20.0f;
		std::cout << "(" << eye_center.x <<"," << eye_center.y << "," << eye_center.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		eye_center.x -= direction.x * 20.0f;
		eye_center.y -= direction.y * 20.0f;
		eye_center.z -= direction.z * 20.0f;
		std::cout << "(" << eye_center.x <<"," << eye_center.y << "," << eye_center.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static GLuint LoadTextureTileBox(const char *texture_file_path) {
	int w, h, channels;
	uint8_t* img = stbi_load(texture_file_path, &w, &h, &channels, 3);
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// To tile textures on a box, we set wrapping to repeat
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (img) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
		glGenerateMipmap(GL_TEXTURE_2D);
		std::cout << "Loaded" << texture_file_path << std::endl;
	} else {
		std::cout << "Failed to load texture " << texture_file_path << std::endl;
	}
	stbi_image_free(img);

	return texture;
}
