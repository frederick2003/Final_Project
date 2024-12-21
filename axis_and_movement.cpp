#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <render/shader.h>

#include <vector>
#include <iostream>
#include <random>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "axis_and_movement.h"
#include "SkyBox_Shaders.h"
#include "renderTextures.h"
#include "DepthShaders.h"
#include "renderLighting.h"
#include "debugQuadShaders.h"

static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static GLuint LoadTextureTileBox(const char *texture_file_path);
static void saveDepthTexture(GLuint fbo, std::string filename);
static void cursor_callback(GLFWwindow *window, double xpos, double ypos);
void renderQuad();


// Debug information for renderQuad() function
unsigned int quadVAO = 0;
unsigned int quadVBO;

// OpenGL camera view parameters
//static glm::vec3 eye_center(-560, 340.0, 300.0f);
//static glm::vec3 eye_center(300,300,300);
static glm::vec3 eye_center(2,300,3);
//static glm::vec3 eye_center(-178,227,0);
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

// View control
static float viewAzimuth = 0.0f;
static float viewPolar = 0.0f;
static float viewDistance = 300.0f;

static float FoV = 45.0f;
static float zNear = 600.0f;
static float zFar = 1500.0f;

// Lighting control
const glm::vec3 wave500(0.0f, 255.0f, 146.0f);
const glm::vec3 wave600(255.0f, 190.0f, 0.0f);
const glm::vec3 wave700(205.0f, 0.0f, 0.0f);

// Reduce the overall scaling factor to make the light slightly darker
static glm::vec3 lightIntensity = 30.0f * (8.0f * wave500 + 15.6f * wave600 + 18.4f * wave700);

static glm::vec3 lightPosition( 0,300,0);

// Shadow mapping
static glm::vec3 lightUp(0, 0, 1);
static int shadowMapWidth = 1024;
static int shadowMapHeight = 1024;

static float depthFoV = 140.0f;
static float depthNear = 5.0f;
static float depthFar = 700.5f;

// Helper flag and function to save depth maps for debugging
static bool saveDepth = true;

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

struct world_setup {
	glm::vec3 position;			// Position of the cliff face
	glm::vec3 scale;			// Size of the face on each axis

	GLfloat vertex_buffer_data[48] = {
		// Coords for cliff face
		-1.25f, -0.15f, -1.0f,
		-1.25f, -0.15f, 1.0f,
		-1.0f, 0.15f, 1.0f,
		-1.0f, 0.15f, -1.0f,

		// Coords for cliff plateau
		-1.0f, 0.15f, 1.0f,
		1.0f, 0.15f, 1.0f,
		1.0f, 0.15f, -1.0f,
		-1.0f, 0.15f, -1.0f,

		// Coords for sea
		-1.0f, -0.15f, -1.0f,
		-3.0f, -0.15f, -1.0f,
		-3.0f, -0.15f, 1.0f,

		// Coords for seafront footpath
		-0.9f, 0.152f, 1.0f,
		-0.8f, 0.152f, 1.0f,
		-0.8f, 0.152f, -1.0f,
		-0.9f, 0.152f, -1.0f
	};

	// TODO: set up vertex normals properly
	GLfloat normal_buffer_data[48] {
		-0.769,0.641,0,
		-0.769,0.641,0,
		-0.769,0.641,0,
		-0.769,0.641,0,

		0.0,1.0,0.0,
		0.0,1.0,0.0,
		0.0,1.0,0.0,
		0.0,1.0,0.0,

		0.0,1.0,0.0,
		0.0,1.0,0.0,
		0.0,1.0,0.0,

		0.0,1.0,0.0,
		0.0,1.0,0.0,
		0.0,1.0,0.0,
		0.0,1.0,0.0,
	};

	GLfloat uv_buffer_data[32] {
		0.0f, 1.0f, // cliff face
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f, // plateau
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f, // Sea
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f, // Seafront Footpath
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f
	};


	GLuint index_buffer_data[24] {
		0,1,2, //Cliff face
		2,3,0,

		4, 5, 6, // Plateau
		4, 6, 7,

		0, 9, 10, // Sea
		0, 10, 1,

		11,12,13, // Seafront Footpath
		13, 14, 11
	};

	glm::mat4 modelMatrix;
	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint uvBufferID;
	GLuint normalBufferID;
	GLuint TextureID, TextureID2, TextureID3, TextureID4;


	// Shader variable IDs
	GLuint normalMatrixID;
	GLuint textureSamplerID;
	GLuint mvpMatrixID;
	GLuint modelMatrixID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint programID;
	GLuint programID2;
	GLuint depthShaderID;

	GLuint simpleDepthShader;
	GLuint lightSpaceMatrixID;
	GLuint modelMatrixDepthID;
	GLuint shadowMapTextureID;
	GLuint LSM_ID;

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

		// Create an index buffer object to store the index data that defines triangle faces
		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

		// Create a VBO to store the UV data
		for (int i = 0; i < 18; ++i) {
			if(i>=7 && i<15) {
				uv_buffer_data[2*i] *=4;
			}
			else if (i>=21 && i<31) {
				uv_buffer_data[2*i] *=15;
			}
			else {
				uv_buffer_data[2*i] *=2;
			}
		}
		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data,GL_STATIC_DRAW);

		glGenBuffers(1, &normalBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);


		//TODO Make alterations to the shader to allow ligting and shadows.
		programID = LoadShadersFromString(textureVertexShader, textureFragmentShader);
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		//TODO set new shader for lighting
		programID2 = LoadShadersFromString(lightingVertexShader, lightingFragmentShader);
		if (programID2 == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}
		else {
			std::cerr << "loaded lighting shaders." << std::endl;
		}

		depthShaderID = LoadShadersFromString(depthVertexShader, depthFragmentShader);
		if (programID2 == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}
		else {
			std::cerr << "loaded lighting shaders." << std::endl;
		}
		modelMatrixDepthID = glGetUniformLocation(depthShaderID, "model");
		lightSpaceMatrixID = glGetUniformLocation(depthShaderID, "lightSpaceModel");
		/*
		normalMatrixID = glGetUniformLocation(programID, "normalMatrix");
		modelMatrixID = glGetUniformLocation(programID, "modelMatrix");
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		lightPositionID = glGetUniformLocation(programID, "lightPosition");
		lightIntensityID = glGetUniformLocation(programID, "lightIntensity");*/
		shadowMapTextureID = glGetUniformLocation(programID2, "shadowMap");
		LSM_ID = glGetUniformLocation(programID2, "lightSpaceMatrix");


		mvpMatrixID = glGetUniformLocation(programID2, "MVP");
		lightPositionID = glGetUniformLocation(programID2, "lightPosition");
		lightIntensityID = glGetUniformLocation(programID2, "lightIntensity");
		modelMatrixID = glGetUniformLocation(programID2, "modelMatrix");
		normalMatrixID = glGetUniformLocation(programID2, "normalMatrix");


		TextureID = LoadTextureTileBox("../Final_Project/Cliff_face.jpg");
		TextureID2 = LoadTextureTileBox("../Final_Project/grass_texture.jpg");
		TextureID3 = LoadTextureTileBox("../Final_Project/Ocean_Texture.jpg");
		TextureID4 = LoadTextureTileBox("../Final_Project/Footpath_Texture.jpg");

		//textureSamplerID = glGetUniformLocation(programID, "textureSampler");
		textureSamplerID = glGetUniformLocation(programID2, "textureSampler");

		if ( mvpMatrixID== -1 || textureSamplerID == -1  ) {
			std::cout << "Error loading shaders." << std::endl;
		}
	}

	void renderWithLight(glm::mat4 cameraMatrix, glm::mat4 lightSpaceMatrix) {
		glUseProgram(programID2);

		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);


		modelMatrix = glm::mat4(1.0f);
		// Translate the cliff to its position
		modelMatrix = glm::translate(modelMatrix, position);
		// Scale the cliff along each axis
		modelMatrix = glm::scale(modelMatrix, scale);
		// Rotate the cliff face
		modelMatrix = glm::rotate(modelMatrix,glm::radians(0.0f),glm::vec3(0.0f,1.0f,0.0f));

		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);

		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
		glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);


		// Set light data
		glUniform3fv(lightPositionID, 1, &lightPosition[0]);
		glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);
		// Bind shadow map texture
		glUniform1i(shadowMapTextureID, 0);


		//Set textureSampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureID);
		glUniform1i(textureSamplerID, 0);

		// Draw the cliff face
		glDrawElements(
			GL_TRIANGLES,      // mode
			6,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		// set texture and draw Plateau with Texture2
		glBindTexture(GL_TEXTURE_2D, TextureID2);
		glUniform1i(textureSamplerID, 0);
		glDrawElements(
			GL_TRIANGLES,
			6,
			GL_UNSIGNED_INT,
			(void*)(6 * sizeof(GLuint)));

		// set texture and draw sea with Texture3
		glBindTexture(GL_TEXTURE_2D, TextureID3);
		glUniform1i(textureSamplerID, 0);
		glDrawElements(
			GL_TRIANGLES,
			6,
			GL_UNSIGNED_INT,
			(void*)(12 * sizeof(GLuint)));

		// set texture and draw sea with Texture3
		glBindTexture(GL_TEXTURE_2D, TextureID4);
		glUniform1i(textureSamplerID, 0);
		glDrawElements(
			GL_TRIANGLES,
			6,
			GL_UNSIGNED_INT,
			(void*)(18 * sizeof(GLuint)));


		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	void render(glm::mat4 cameraMatrix) {
		// TODO change to a depth shader
		glUseProgram(programID);

		// TODO review to see if this line is necessary
		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		/*
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		*/

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		// TODO: Model transform
		// ------------------------------------
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		// Translate the cliff to its position
		modelMatrix = glm::translate(modelMatrix, position);
		// Scale the cliff along each axis
		modelMatrix = glm::scale(modelMatrix, scale);
		// Rotate the cliff face
		modelMatrix = glm::rotate(modelMatrix,glm::radians(0.0f),glm::vec3(0.0f,1.0f,0.0f));

		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		//Set textureSampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, TextureID);
		glUniform1i(textureSamplerID, 0);

		// Draw the cliff face
		glDrawElements(
			GL_TRIANGLES,      // mode
			6,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		// set texture and draw Plateau with Texture2
		glBindTexture(GL_TEXTURE_2D, TextureID2);
		glUniform1i(textureSamplerID, 0);
		glDrawElements(
			GL_TRIANGLES,
			6,
			GL_UNSIGNED_INT,
			(void*)(6 * sizeof(GLuint)));

		// set texture and draw sea with Texture3
		glBindTexture(GL_TEXTURE_2D, TextureID3);
		glUniform1i(textureSamplerID, 0);
		glDrawElements(
			GL_TRIANGLES,
			6,
			GL_UNSIGNED_INT,
			(void*)(12 * sizeof(GLuint)));

		// set texture and draw sea with Texture3
		glBindTexture(GL_TEXTURE_2D, TextureID4);
		glUniform1i(textureSamplerID, 0);
		glDrawElements(
			GL_TRIANGLES,
			6,
			GL_UNSIGNED_INT,
			(void*)(18 * sizeof(GLuint)));


		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		//glDisableVertexAttribArray(2);
	}

	void renderShadow(glm::mat4 lightSpaceMatrix) {
		glUseProgram(depthShaderID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		//TODO might have to change this to indentity matrix.
		glUniformMatrix4fv(modelMatrixDepthID, 1, GL_FALSE, &modelMatrix[0][0]);
		glUniformMatrix4fv(lightSpaceMatrixID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			24,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}


	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteTextures(1, &TextureID);
		glDeleteProgram(programID);
	}
};

struct Building {
	glm::vec3 position;		// Position of the box
	glm::vec3 scale;		// Size of the box in each axis

	GLfloat vertex_buffer_data[72] = {	// Vertex definition for a canonical box
		// Front face
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,

		// Back face
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		// Left face
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		// Right face
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,

		// Top face
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		// Bottom face
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
	};


	GLfloat normal_buffer_data[72] = {
		// Front face
		0.0f, 0.0f, 1.0f,  // Normal pointing out along the +Z axis
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		// Back face
		0.0f, 0.0f, -1.0f, // Normal pointing out along the -Z axis
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,

		// Left face
		-1.0f, 0.0f, 0.0f, // Normal pointing out along the -X axis
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,

		// Right face
		1.0f, 0.0f, 0.0f,  // Normal pointing out along the +X axis
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Top face
		0.0f, 1.0f, 0.0f,  // Normal pointing out along the +Y axis
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Bottom face
		0.0f, -1.0f, 0.0f, // Normal pointing out along the -Y axis
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
	};


	GLuint index_buffer_data[36] = {		// 12 triangle faces of a box
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		16, 17, 18,
		16, 18, 19,

		20, 21, 22,
		20, 22, 23,
	};

	GLfloat uv_buffer_data[48] = {
		// Front
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Back
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Left
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Right
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Top - we do not want texture the top
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		// Bottom - we do not want texture the bottom
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
	};


	glm::mat4 modelMatrix;

	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint colorBufferID;
	GLuint normalBufferID;
	GLuint uvBufferID;
	GLuint textureID;


	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint textureSamplerID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint lightSpaceMatrixID;
	GLuint normalMatrixID;
	GLuint modelMatrixID;
	GLuint modelMatrixDepthID;

	GLuint programID;
	GLuint programID2;
	GLuint depthShaderID;

	void initialize(glm::vec3 position, glm::vec3 scale) {
		// Define scale of the building geometry
		this->position = position;
		this->scale = scale;

		// Create a vertex array object
		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		// Create a vertex buffer object to store the vertex data
		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		// Create a vertex buffer object to store the UV data
		for (int i = 0; i < 24; ++i) uv_buffer_data[2*i+1] *= 5;
		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data,GL_STATIC_DRAW);

		// Create a vertex buffer storing Normal buffer data
		glGenBuffers(1, &normalBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);


		// Create an index buffer object to store the index data that defines triangle faces
		glGenBuffers(1, &indexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

		// Create and compile our GLSL program from the shaders
		programID = LoadShadersFromString(textureVertexShader,textureFragmentShader );
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		programID2 = LoadShadersFromString(lightingVertexShader,lightingFragmentShader );
		if (programID2 == 0)
		{
			std::cerr << "lighting shaders failed to load shaders." << std::endl;
		}

		depthShaderID = LoadShadersFromString(depthVertexShader,depthFragmentShader );
		if (depthShaderID == 0)
		{
			std::cerr << "Depth shaders ailed to load shaders." << std::endl;
		}

		//Light rendering shader uniforms
		mvpMatrixID = glGetUniformLocation(programID2, "MVP");
		lightPositionID = glGetUniformLocation(programID2, "lightPosition");
		lightIntensityID = glGetUniformLocation(programID2, "lightIntensity");
		modelMatrixID = glGetUniformLocation(programID2, "modelMatrix");
		normalMatrixID = glGetUniformLocation(programID2, "normalMatrix");

		//Depth rendering shader uniforms
		modelMatrixDepthID = glGetUniformLocation(depthShaderID, "model");
		lightSpaceMatrixID = glGetUniformLocation(depthShaderID, "lightSpaceMatrix");

		static std::vector<std::string> textures = {
			"../Final_Project/facade0.jpg",
			"../Final_Project/facade1.jpg",
			"../Final_Project/facade2.jpg",
			"../Final_Project/facade3.jpg",
			"../Final_Project/facade4.jpg",
			"../Final_Project/facade5.jpg"
		};

		// Static random number generator
		static std::random_device rd;  // Obtain a random number from hardware once
		static std::mt19937 gen(rd()); // Seed the generator once
		std::uniform_int_distribution<> distr(0, 5); // Define the range

		int random = distr(gen);  // Generate the random number
		textureID = LoadTextureTileBox("../Final_Project/futuristic.jpg"); // Load the selected texture, assuming LoadTextureTileBox accepts std::string

		// Get a handle for our "textureSampler" uniform
		//textureSamplerID = glGetUniformLocation(programID,"textureSampler");
		textureSamplerID = glGetUniformLocation(programID2,"textureSampler");
	}

	void renderWithLight(glm::mat4 cameraMatrix) {
		glUseProgram(programID2);

		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		modelMatrix = glm::mat4(1.0f);
		// Scale the cliff along each axis
		modelMatrix = glm::scale(modelMatrix, scale);
		// Translate the cliff to its position
		modelMatrix = glm::translate(modelMatrix, position);
		// Rotate the cliff face
		modelMatrix = glm::rotate(modelMatrix,glm::radians(0.0f),glm::vec3(0.0f,1.0f,0.0f));


		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);

		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
		glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);

		// Set light data
		glUniform3fv(lightPositionID, 1, &lightPosition[0]);
		glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);



		//Set textureSampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(textureSamplerID, 0);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			36,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);

		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		// TODO: Model transform
		// -----------------------
		glm::mat4 modelMatrix = glm::mat4();
		// Scale the box along each axis to make it look like a building
		modelMatrix = glm::scale(modelMatrix, scale);
		// Translate the box to it's position.
		modelMatrix = glm::translate(modelMatrix, position);

		// Set model-view-projection matrix
		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// Set textureSampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(textureSamplerID, 0);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			36,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);


		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	void renderShadow(glm::mat4 lightSpaceMatrix) {
		glUseProgram(depthShaderID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		//TODO might have to change this to indentity matrix.


		glUniformMatrix4fv(modelMatrixDepthID, 1, GL_FALSE, &modelMatrix[0][0]);
		glUniformMatrix4fv(lightSpaceMatrixID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			36,    			   // number of indices
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
		glDeleteTextures(1, &textureID);
		glDeleteProgram(programID);
	}
};

struct Lighting_Shadows {
	GLuint FBO;
	GLuint depthTexture;
	GLuint depthShader;
	GLuint lightSpaceMatrixID;

	GLuint programID;
	GLuint FragPositionLightSpaceID;
	GLuint normalMatrixID;
	GLuint mvpMatrixID;
	GLuint modelMatrixID;

	// light variables
	GLuint lightPositionID;
	GLuint lightIntensityID;

	// ID for shadowmap texture
	GLuint shadowMapLocation;

	GLuint simpleDepthShader;

	void initilize() {

		// Generate and bind the framebuffer.
		glGenFramebuffers(1, &FBO);

		// Generate the depth texture
		glGenTextures(1, &depthTexture);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Attach the depth texture to the framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

		// Ensure framebuffer completeness
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cerr << "Error: Framebuffer is not complete!" << std::endl;
		} else {
			std::cout << "Framebuffer is complete!" << std::endl;
		}

		// Verify depth texture attachment
		GLint attachedTexture;
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attachedTexture);
		if (attachedTexture == (GLint)depthTexture) {
			std::cout << "Depth texture is correctly attached to the framebuffer!" << std::endl;
		} else {
			std::cerr << "Error: Depth texture is not correctly attached to the framebuffer!" << std::endl;
		}

		// Disable colour buffer for this depth only FBO
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Check if the framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cerr << "Error: Framebuffer is not complete!" << std::endl;
		}

		simpleDepthShader = LoadShadersFromString(depthVertexShader, depthFragmentShader);

		if (simpleDepthShader == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		lightSpaceMatrixID = glGetUniformLocation(simpleDepthShader, "lightSpaceMatrix");

		// pass shadowMapTexture to the shader
		programID = LoadShadersFromString(lightingVertexShader, lightingFragmentShader);

		if(programID == 0) {
			std::cerr << "Failed to load shaders." << std::endl;
		}

		shadowMapLocation = glGetUniformLocation(programID, "shadowMap");
		glUniform1i(shadowMapLocation, 0);
	}

	// Render depth map
	void shadowMapPass() {
		float near_plane = 1.0f, far_plane = 7.5f;
		glm::mat4 lightProjection = glm::perspective(glm::radians(depthFoV),(float)windowWidth/windowHeight, depthNear, depthFar);
		glm::mat4 lightView = glm::lookAt(lightPosition, lookat, glm::vec3(0.0f, 0.0f, 1.0f)         // Up vector
		);
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;
		glUseProgram(simpleDepthShader);
		glUniformMatrix4fv(lightSpaceMatrixID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

		glBindFramebuffer(GL_FRAMEBUFFER,	FBO);
		glViewport(0,0, shadowMapWidth, shadowMapHeight); // set the width and height of the shadow map. same as window

		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
	}

	// Render scene as normal with shadow mapping (using depth map)
	void lightingMapPass(glm::mat4 cameraMatrix) {
		float near_plane = 1.0f, far_plane = 7.5f;
		glm::mat4 lightProjection = glm::perspective(glm::radians(depthFoV),(float)windowWidth/windowHeight, depthNear, depthFar);
		glm::mat4 lightView = glm::lookAt(
		lightPosition, // Light's position
		glm::vec3(-275.6f, 0.0f, -279.33f),   // Target position (looking at the origin)
		glm::vec3(0.0f, 0.0f, 1.0f)         // Up vector
		);
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		glBindBuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, windowWidth, windowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(programID);// Bind depth texture to texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniformMatrix4fv(FragPositionLightSpaceID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
		glm::mat4 mvp = cameraMatrix;
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
		glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);

		// Set light data
		glUniform3fv(lightPositionID, 1, &lightPosition[0]);
		glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);
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
	window = glfwCreateWindow(windowWidth, windowHeight, "Final Project", NULL, NULL);
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

	//glfwSetCursorPosCallback(window, cursor_callback);

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

	// ------------------------------------------------------------------
	// Initialize cliff wall
	world_setup myWorld;
	myWorld.intialize(glm::vec3(80, -10, -100), glm::vec3(200, 200, 200));

	Building myBuilding;
	myBuilding.initialize(glm::vec3(0, 1.4, 0), glm::vec3(16, 3*16, 16));

	Lighting_Shadows renderLight;
	renderLight.initilize();

	float near_plane = 1.0f, far_plane = 7.5f;
	glm::mat4 lightProjection = glm::perspective(glm::radians(depthFoV),(float)windowWidth/windowHeight, depthNear, depthFar);
	glm::mat4 lightView = glm::lookAt(
	lightPosition, // Light's position
	lookat,   // Target position (looking at the origin)
	glm::vec3(0.0f, 0.0f, 1.0f)         // Up vector
	);

	glm::mat4 lightSpaceMatrix = lightProjection * lightView;

	GLuint debug_QuadID;
	GLuint nearPlaneID;
	GLuint farPlaneID;
	debug_QuadID = LoadShadersFromString(debugVertexShader, debugFragmentShader);
	if (debug_QuadID == 0)
	{
		std::cerr << "Failed to load shaders." << std::endl;
	}
	nearPlaneID = glGetUniformLocation(debug_QuadID, "near_plane");
	farPlaneID = glGetUniformLocation(debug_QuadID, "far_plane");



	// ------------------------------------------------------------------------------------
	// Prepare a perspective camera
	glm::float32 FoV = 30;
	glm::float32 zNear = 0.1f;
	glm::float32 zFar = 1000.0f;
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);
	//glm::mat4 projectionMatrix(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);

	renderLight.shadowMapPass();
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
		if (saveDepth) {
			myWorld.renderShadow(lightSpaceMatrix);
			myBuilding.renderShadow(lightSpaceMatrix);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0,0,windowWidth, windowHeight);
			std::string filename = "../Final_Project/depth_camera.png";
			saveDepthTexture(renderLight.depthTexture, filename);
			std::cout << "Depth texture saved to " << filename << std::endl;
			saveDepth = false;
		}

		//------------------------------------------------------------------------------
		//Render object to the screen
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderLight.depthTexture);
		//mySkybox.render(viewMatrix,projectionMatrix);
		myWorld.renderWithLight(vp,lightSpaceMatrix);
		myBuilding.renderWithLight(vp);

		/*
		glUseProgram(debug_QuadID);
		glUniform1f(nearPlaneID, near_plane);
		glUniform1f(farPlaneID, far_plane);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderLight.depthTexture);
		renderQuad();
		*/





		//------------------------------------------------------------------------------
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Destroy all objects created
	myWorld.cleanup();
	myBuilding.cleanup();
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

	if (key == GLFW_KEY_Y && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		lightPosition.y+= 1.0;
		std::cout << "(" << lightPosition.x <<"," << lightPosition.y << "," << lightPosition.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_T && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		lightPosition.y-= 1.0;
		std::cout << "(" << lightPosition.x <<"," << lightPosition.y << "," << lightPosition.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_G && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		lightPosition.x+= 1.0;
		std::cout << "(" << lightPosition.x <<"," << lightPosition.y << "," << lightPosition.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_F && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		lightPosition.x-= 1.0;
		std::cout << "(" << lightPosition.x <<"," << lightPosition.y << "," << lightPosition.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_H && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		lightPosition.z+= 1.0;
		std::cout << "(" << lightPosition.x <<"," << lightPosition.y << "," << lightPosition.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_J && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		lightPosition.z-= 1.0;
		std::cout << "(" << lightPosition.x <<"," << lightPosition.y << "," << lightPosition.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{

			eye_center.y -= 20.0f;

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

static void saveDepthTexture(GLuint fbo, std::string filename) {
	int width = shadowMapWidth;
	int height = shadowMapHeight;
	if (shadowMapWidth == 0 || shadowMapHeight == 0) {
		width = windowWidth;
		height = windowHeight;
	}
	int channels = 3;

	std::vector<float> depth(width * height);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glReadBuffer(GL_DEPTH_COMPONENT);
	glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	std::vector<unsigned char> img(width * height * 3);
	for (int i = 0; i < width * height; ++i) img[3*i] = img[3*i+1] = img[3*i+2] = depth[i] * 255;

	stbi_write_png(filename.c_str(), width, height, channels, img.data(), width * channels);
}

void renderQuad() {
	if (quadVAO == 0)
	{
		float quadVertices[20] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

}

static void cursor_callback(GLFWwindow *window, double xpos, double ypos) {
	// Ensure cursor coordinates are within the window
	xpos = glm::clamp(xpos, 0.0, (double)windowWidth - 1.0);
	ypos = glm::clamp(ypos, 0.0, (double)windowHeight - 1.0);


	// Normalize to [0, 1]
	float x = xpos / windowWidth;
	float y = ypos / windowHeight;

	// To [-1, 1] and flip y up
	x = x * 2.0f - 1.0f;
	y = 1.0f - y * 2.0f;

	const float scale = 2.0f * tan(glm::radians(FoV / 2.0f)) * viewDistance; // FOV scaling
	lightPosition.x = x * scale + eye_center.x; // Offset based on camera
	lightPosition.y = y * scale + eye_center.y; // Offset based on camera
	lightPosition.z = eye_center.z - viewDistance; // Adjust depth

	std::cout << lightPosition.x << " " << lightPosition.y << " " << lightPosition.z << std::endl;
}
