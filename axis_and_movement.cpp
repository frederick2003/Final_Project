#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <render/shader.h>

#include <vector>
#include <iostream>
#include <random>

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define _USE_MATH_DEFINES
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


#include "../Final_Project/Shaders/SkyBox_Shaders.h"
#include "../Final_Project/Shaders/renderTextures.h"
#include "../Final_Project/Shaders/DepthShaders.h"
#include "../Final_Project/Shaders/renderLighting.h"
#include "../Final_Project/Shaders/debugQuadShaders.h"
#include "../Final_Project/Shaders/animationShaders.h"




#include <iomanip>
#define BUFFER_OFFSET(i) ((char *)NULL + (i))



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
static glm::vec3 eye_center(56.8179,509.257,372.0650);
static glm::vec3 lookat(77,150,-85);
static glm::vec3 up(0, 1, 0);

// View control
static float viewAzimuth = 0.0f;
static float viewPolar = 0.0f;
static float viewDistance = 300.0f;

static float FoV = 33.0f;
static float zNear = 1.0f;
static float zFar = 1000.0f;

// Lighting control
const glm::vec3 wave500(0.0f, 255.0f, 146.0f);
const glm::vec3 wave600(255.0f, 190.0f, 0.0f);
const glm::vec3 wave700(205.0f, 0.0f, 0.0f);

// Reduce the overall scaling factor to make the light slightly darker
static glm::vec3 lightIntensity = 2500.0f * (8.0f * wave500 + 15.6f * wave600 + 18.4f * wave700);
static glm::vec3 lightPosition( 20,900,-95);
static glm::vec3 lightLookat(20,0,-95);

// Shadow mapping
static glm::vec3 lightUp(0, 0, 1);
int shadowMapWidth = 0;
int shadowMapHeight = 0;

static float depthFoV = 25.0f;
static float depthNear = 300.0f;
static float depthFar = 3000.0f;

//Animation
// Animation
static bool playAnimation = true;
static float playbackSpeed = 2.0f;

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

		modelMatrix = glm::mat4(1.0f);
		// Translate the cliff to its position
		modelMatrix = glm::translate(modelMatrix, position);
		// Scale the cliff along each axis
		modelMatrix = glm::scale(modelMatrix, scale);

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
		for (int i = 24; i < 32; i += 2) {
			uv_buffer_data[i] *= 15; // Scale the 'u' coordinate for repetition
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
		lightSpaceMatrixID = glGetUniformLocation(depthShaderID, "lightSpaceMatrix");
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


		TextureID = LoadTextureTileBox("../Final_Project/Textures/Cliff_face.jpg");
		TextureID2 = LoadTextureTileBox("../Final_Project/Textures/grass_texture.jpg");
		TextureID3 = LoadTextureTileBox("../Final_Project/Textures/Ocean_Texture.jpg");
		TextureID4 = LoadTextureTileBox("../Final_Project/Textures/Footpath_Texture.jpg");

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

		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);

		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
		glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);
		glUniformMatrix4fv(LSM_ID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);



		// Set light data
		glUniform3fv(lightPositionID, 1, &lightPosition[0]);
		glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);
		// Bind shadow map texture
		glUniform1i(shadowMapTextureID, 1);


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

		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

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

	void orthographicRenderShadow(glm::mat4 lightSpaceMatrix) {
		glUseProgram(depthShaderID);

		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

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
	GLuint LSM_ID;
	GLuint shadowMapTextureID;

	GLuint programID;
	GLuint programID2;
	GLuint depthShaderID;

	void initialize(glm::vec3 position, glm::vec3 scale) {
		// Define scale of the building geometry
		this->position = position;
		this->scale = scale;


		modelMatrix = glm::mat4(1.0f);
		// Translate the cliff to its position
		modelMatrix = glm::translate(modelMatrix, position);
		// Scale the cliff along each axis
		modelMatrix = glm::scale(modelMatrix, scale);

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
		LSM_ID = glGetUniformLocation(programID2, "lightSpaceMatrix");
		shadowMapTextureID = glGetUniformLocation(programID2, "shadowMap");





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
		textureID = LoadTextureTileBox("../Final_Project/Textures/futuristic.jpg"); // Load the selected texture, assuming LoadTextureTileBox accepts std::string

		// Get a handle for our "textureSampler" uniform
		//textureSamplerID = glGetUniformLocation(programID,"textureSampler");
		textureSamplerID = glGetUniformLocation(programID2,"textureSampler");
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

		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);

		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
		glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);
		glUniformMatrix4fv(LSM_ID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);


		// Set light data
		glUniform3fv(lightPositionID, 1, &lightPosition[0]);
		glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);

		glUniform1i(shadowMapTextureID, 1);


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

		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);


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

	void orthographicRenderShadows(glm::mat4 lightSpaceMatrix) {
		glUseProgram(depthShaderID);

		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);


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

	void initialize() {

		// Generate and bind the framebuffer.
		glGenFramebuffers(1, &FBO);

		// Generate the depth texture
		glGenTextures(1, &depthTexture);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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
		modelMatrixID = glGetUniformLocation(simpleDepthShader, "model");

		// pass shadowMapTexture to the shader
		programID = LoadShadersFromString(lightingVertexShader, lightingFragmentShader);

		if(programID == 0) {
			std::cerr << "Failed to load shaders." << std::endl;
		}
	}

	void initializeOrtho() {
		// Generate and bind the framebuffer.
		glGenFramebuffers(1, &FBO);

		// Generate the depth texture
		glGenTextures(1, &depthTexture);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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
		modelMatrixID = glGetUniformLocation(simpleDepthShader, "model");

		// pass shadowMapTexture to the shader
		programID = LoadShadersFromString(lightingVertexShader, lightingFragmentShader);

		if(programID == 0) {
			std::cerr << "Failed to load shaders." << std::endl;
		}

		shadowMapLocation = glGetUniformLocation(programID, "shadowMap");
		glUniform1i(shadowMapLocation, 0);
	}

	// Render depth map
	void shadowMapPass(glm::mat4 lightSpaceMatrix) {
		glUseProgram(simpleDepthShader);
		glUniformMatrix4fv(lightSpaceMatrixID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

		glBindFramebuffer(GL_FRAMEBUFFER,	FBO);
		glViewport(0,0, shadowMapWidth, shadowMapHeight);

		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
	}

	// Render scene as normal with shadow mapping (using depth map)
	void lightingMapPass(glm::mat4 cameraMatrix) {
		float near_plane = 1.0f, far_plane = 500.5f;
		glm::mat4 lightProjection = glm::perspective(glm::radians(depthFoV),(float)windowWidth/windowHeight, depthNear, depthFar);
		glm::mat4 lightView = glm::lookAt(
		lightPosition, // Light's position
		glm::vec3(-275.6f, 0.0f, -279.33f),   // Target position (looking at the origin)
		lightUp
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

struct HorizontalPlane {
	glm::vec3 position;  // Position of the plane
    glm::vec3 scale;     // Scale of the plane

    // Vertex buffer data for the plateau only
    GLfloat vertex_buffer_data[12] = {
        -1.0f, 0.15f, 1.0f,
         1.0f, 0.15f, 1.0f,
         1.0f, 0.15f, -1.0f,
        -1.0f, 0.15f, -1.0f
    };

    GLfloat normal_buffer_data[12] = {
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    GLfloat uv_buffer_data[8] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f
    };

    GLuint index_buffer_data[6] = {
        0, 1, 2,
        2, 3, 0
    };

    glm::mat4 modelMatrix;
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint indexBufferID;
    GLuint uvBufferID;
    GLuint normalBufferID;
    GLuint TextureID;

    GLuint normalMatrixID;
    GLuint textureSamplerID;
    GLuint mvpMatrixID;
    GLuint modelMatrixID;
    GLuint lightPositionID;
    GLuint lightIntensityID;
    GLuint programID;
    GLuint depthShaderID;
    GLuint lightSpaceMatrixID;
    GLuint lightSpaceMatrixID2;
    GLuint modelMatrixDepthID;
    GLuint shadowMapTextureID;
    GLuint LSM_ID;

    void initialize(glm::vec3 position, glm::vec3 scale) {
        this->position = position;
        this->scale = scale;

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
        modelMatrix = glm::scale(modelMatrix, scale);

        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);

        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

        glGenBuffers(1, &normalBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);

        glGenBuffers(1, &uvBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

        glGenBuffers(1, &indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

        depthShaderID = LoadShadersFromString(depthVertexShader, depthFragmentShader);
        programID = LoadShadersFromString(lightingVertexShader, lightingFragmentShader);
        TextureID = LoadTextureTileBox("../Final_Project/Textures/grass_texture.jpg");

        if (programID == 0 || depthShaderID == 0) {
            std::cerr << "Failed to load shaders." << std::endl;
        }
        if (TextureID == 0) {
            std::cerr << "Failed to load texture." << std::endl;
        }

        mvpMatrixID = glGetUniformLocation(programID, "MVP");
        modelMatrixID = glGetUniformLocation(programID, "modelMatrix");
        normalMatrixID = glGetUniformLocation(programID, "normalMatrix");
    	lightSpaceMatrixID2 = glGetUniformLocation(programID, "lightSpaceMatrix");

        lightPositionID = glGetUniformLocation(programID, "lightPosition");
        lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
        textureSamplerID = glGetUniformLocation(programID, "textureSampler");
    	shadowMapTextureID = glGetUniformLocation(programID, "shadowMap");


    	modelMatrixDepthID = glGetUniformLocation(depthShaderID, "model");
    	lightSpaceMatrixID = glGetUniformLocation(depthShaderID, "lightSpaceMatrix");
    }


	void renderShadow(glm::mat4 lightSpaceMatrix) {
		glUseProgram(depthShaderID);

		glBindVertexArray(vertexArrayID);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		glUniformMatrix4fv(modelMatrixDepthID, 1, GL_FALSE, &modelMatrix[0][0]);
		glUniformMatrix4fv(lightSpaceMatrixID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

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

	void orthographicRenderShadow(glm::mat4 lightSpaceMatrix) {
    	glUseProgram(depthShaderID);

    	glBindVertexArray(vertexArrayID);

    	glEnableVertexAttribArray(0);
    	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    	glEnableVertexAttribArray(1);
    	glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
    	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    	glEnableVertexAttribArray(2);
    	glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

    	glUniformMatrix4fv(modelMatrixDepthID, 1, GL_FALSE, &modelMatrix[0][0]);
    	glUniformMatrix4fv(lightSpaceMatrixID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

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

	void renderWithLight(glm::mat4 cameraMatrix, glm::mat4 lightSpaceMatrix, glm::vec3 lightIntensity, glm::vec3 lightPosition) {
		glUseProgram(programID);
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

		glm::mat4 mvp = cameraMatrix * modelMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);

		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
		glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);
		glUniformMatrix4fv(lightSpaceMatrixID2, 1, GL_FALSE, &lightSpaceMatrix[0][0]);



		// Set light data
		glUniform3fv(lightPositionID, 1, &lightPosition[0]);
		glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);
		// Bind shadow map texture
		glUniform1i(shadowMapTextureID, 1);


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

struct MyBot {
	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint jointMatricesID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint programID;

	const int MAX_JOINTS = 128;  // Maximum number of joints supported

	tinygltf::Model model;

	// Each VAO corresponds to each mesh primitive in the GLTF model
	struct PrimitiveObject {
		GLuint vao;
		std::map<int, GLuint> vbos;
	};
	std::vector<PrimitiveObject> primitiveObjects;

	// Skinning
	struct SkinObject {
		// Transforms the geometry into the space of the respective joint
		std::vector<glm::mat4> inverseBindMatrices;

		// Transforms the geometry following the movement of the joints
		std::vector<glm::mat4> globalJointTransforms;

		// Combined transforms
		std::vector<glm::mat4> jointMatrices;
	};
	std::vector<SkinObject> skinObjects;

	// Animation
	struct SamplerObject {
		std::vector<float> input;
		std::vector<glm::vec4> output;
		int interpolation;
	};
	struct ChannelObject {
		int sampler;
		std::string targetPath;
		int targetNode;
	};
	struct AnimationObject {
		std::vector<SamplerObject> samplers;	// Animation data
	};
	std::vector<AnimationObject> animationObjects;

	glm::mat4 getNodeTransform(const tinygltf::Node& node) {
		glm::mat4 transform(1.0f);

		if (node.matrix.size() == 16) {
			transform = glm::make_mat4(node.matrix.data());
		} else {
			if (node.translation.size() == 3) {
				transform = glm::translate(transform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
			}
			if (node.rotation.size() == 4) {
				glm::quat q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
				transform *= glm::mat4_cast(q);
			}
			if (node.scale.size() == 3) {
				transform = glm::scale(transform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
			}
		}
		return transform;
	}

	void computeLocalNodeTransform(const tinygltf::Model& model,
		int nodeIndex,
		std::vector<glm::mat4> &localTransforms)
	{
		const tinygltf::Node& node = model.nodes[nodeIndex];

		glm::mat4 localTransform = glm::mat4(1.0f); // Start with the identity matrix

		// Step 1: Check for the `matrix` property
		if (!node.matrix.empty()) {
			// Use the provided matrix directly if it exists
			localTransform = glm::make_mat4(node.matrix.data());
		} else {
			// Step 2: Apply translation, rotation, and scale
			glm::vec3 translation(0.0f);
			glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
			glm::vec3 scale(1.0f);

			if (!node.translation.empty()) {
				translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
			}

			if (!node.rotation.empty()) {
				rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
			}

			if (!node.scale.empty()) {
				scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
			}

			// Step 3: Combine translation, rotation, and scale into a single transform
			localTransform = glm::translate(glm::mat4(1.0f), translation) *
							 glm::mat4_cast(rotation) *
							 glm::scale(glm::mat4(1.0f), scale);
		}

		// Step 4: Store the computed local transform
		localTransforms[nodeIndex] = localTransform;
	}


	void computeGlobalNodeTransform(const tinygltf::Model& model,
		const std::vector<glm::mat4> &localTransforms,
		int nodeIndex, const glm::mat4& parentTransform,
		std::vector<glm::mat4> &globalTransforms)
	{
		if (nodeIndex < 0 || nodeIndex >= model.nodes.size() ||
			nodeIndex >= localTransforms.size() ||
			nodeIndex >= globalTransforms.size()) {
			return; // Or handle error appropriately
			}

		// Step 1: Compute the global transform for the current node
		glm::mat4 globalTransform = parentTransform * localTransforms[nodeIndex];
		globalTransforms[nodeIndex] = globalTransform;

		// Step 2: Iterate over the children of the current node
		const tinygltf::Node& node = model.nodes[nodeIndex];
		for (int childIndex : node.children) {
			// Recursively compute global transforms for child nodes
			computeGlobalNodeTransform(model, localTransforms, childIndex, globalTransform, globalTransforms);
		}
	}

	std::vector<SkinObject> prepareSkinning(const tinygltf::Model &model) {
		std::vector<SkinObject> skinObjects;

		// In our Blender exporter, the default number of joints that may influence a vertex is set to 4, just for convenient implementation in shaders.

		for (size_t i = 0; i < model.skins.size(); i++) {
			SkinObject skinObject;

			const tinygltf::Skin &skin = model.skins[i];

			// Read inverseBindMatrices
			const tinygltf::Accessor &accessor = model.accessors[skin.inverseBindMatrices];
			assert(accessor.type == TINYGLTF_TYPE_MAT4);
			const tinygltf::BufferView &bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
			const float *ptr = reinterpret_cast<const float *>(
            	buffer.data.data() + accessor.byteOffset + bufferView.byteOffset);

			skinObject.inverseBindMatrices.resize(accessor.count);
			for (size_t j = 0; j < accessor.count; j++) {
				float m[16];
				memcpy(m, ptr + j * 16, 16 * sizeof(float));
				skinObject.inverseBindMatrices[j] = glm::make_mat4(m);
			}

			assert(skin.joints.size() == accessor.count);

			skinObject.globalJointTransforms.resize(skin.joints.size());
			skinObject.jointMatrices.resize(skin.joints.size());

			// ----------------------------------------------
			// TODO: your code here to compute joint matrices
			// Initialize local transforms for all nodes
			std::vector<glm::mat4> localTransforms(model.nodes.size(), glm::mat4(1.0f));

			// Compute local transforms for joints
			for (int jointIndex : skin.joints) {
				computeLocalNodeTransform(model, jointIndex, localTransforms);
			}

			// Initialize joint matrices for bind pose
			for (size_t j = 0; j < skin.joints.size(); j++) {
				int jointIndex = skin.joints[j];
				skinObject.globalJointTransforms[j] = localTransforms[jointIndex];
				skinObject.jointMatrices[j] = skinObject.globalJointTransforms[j] * skinObject.inverseBindMatrices[j];
			}
			// ----------------------------------------------
			skinObjects.push_back(skinObject);
		}
		return skinObjects;
	}

	int findKeyframeIndex(const std::vector<float>& times, float animationTime)
	{
		int left = 0;
		int right = times.size() - 1;

		while (left <= right) {
			int mid = (left + right) / 2;

			if (mid + 1 < times.size() && times[mid] <= animationTime && animationTime < times[mid + 1]) {
				return mid;
			}
			else if (times[mid] > animationTime) {
				right = mid - 1;
			}
			else { // animationTime >= times[mid + 1]
				left = mid + 1;
			}
		}

		// Target not found
		return times.size() - 2;
	}

	std::vector<AnimationObject> prepareAnimation(const tinygltf::Model &model)
	{
		std::vector<AnimationObject> animationObjects;
		for (const auto &anim : model.animations) {
			AnimationObject animationObject;

			for (const auto &sampler : anim.samplers) {
				SamplerObject samplerObject;

				const tinygltf::Accessor &inputAccessor = model.accessors[sampler.input];
				const tinygltf::BufferView &inputBufferView = model.bufferViews[inputAccessor.bufferView];
				const tinygltf::Buffer &inputBuffer = model.buffers[inputBufferView.buffer];

				assert(inputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
				assert(inputAccessor.type == TINYGLTF_TYPE_SCALAR);

				// Input (time) values
				samplerObject.input.resize(inputAccessor.count);

				const unsigned char *inputPtr = &inputBuffer.data[inputBufferView.byteOffset + inputAccessor.byteOffset];
				const float *inputBuf = reinterpret_cast<const float*>(inputPtr);

				// Read input (time) values
				int stride = inputAccessor.ByteStride(inputBufferView);
				for (size_t i = 0; i < inputAccessor.count; ++i) {
					samplerObject.input[i] = *reinterpret_cast<const float*>(inputPtr + i * stride);
				}

				const tinygltf::Accessor &outputAccessor = model.accessors[sampler.output];
				const tinygltf::BufferView &outputBufferView = model.bufferViews[outputAccessor.bufferView];
				const tinygltf::Buffer &outputBuffer = model.buffers[outputBufferView.buffer];

				assert(outputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				const unsigned char *outputPtr = &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset];
				const float *outputBuf = reinterpret_cast<const float*>(outputPtr);

				int outputStride = outputAccessor.ByteStride(outputBufferView);

				// Output values
				samplerObject.output.resize(outputAccessor.count);

				for (size_t i = 0; i < outputAccessor.count; ++i) {

					if (outputAccessor.type == TINYGLTF_TYPE_VEC3) {
						memcpy(&samplerObject.output[i], outputPtr + i * 3 * sizeof(float), 3 * sizeof(float));
					} else if (outputAccessor.type == TINYGLTF_TYPE_VEC4) {
						memcpy(&samplerObject.output[i], outputPtr + i * 4 * sizeof(float), 4 * sizeof(float));
					} else {
						std::cout << "Unsupport accessor type ..." << std::endl;
					}

				}

				animationObject.samplers.push_back(samplerObject);
			}

			animationObjects.push_back(animationObject);
		}
		return animationObjects;
	}

	void updateAnimation(
		const tinygltf::Model &model,
		const tinygltf::Animation &anim,
		const AnimationObject &animationObject,
		float time,
		std::vector<glm::mat4> &nodeTransforms)
	{
		// There are many channels so we have to accumulate the transforms
		for (const auto &channel : anim.channels) {

			int targetNodeIndex = channel.target_node;
			const auto &sampler = anim.samplers[channel.sampler];

			// Access output (value) data for the channel
			const tinygltf::Accessor &outputAccessor = model.accessors[sampler.output];
			const tinygltf::BufferView &outputBufferView = model.bufferViews[outputAccessor.bufferView];
			const tinygltf::Buffer &outputBuffer = model.buffers[outputBufferView.buffer];

			// Calculate current animation time (wrap if necessary)
			const std::vector<float> &times = animationObject.samplers[channel.sampler].input;
			float animationTime = fmod(time, times.back());
			int keyframeIndex = findKeyframeIndex(times, animationTime);

			const unsigned char *outputPtr = &outputBuffer.data[outputBufferView.byteOffset + outputAccessor.byteOffset];
			const float *outputBuf = reinterpret_cast<const float*>(outputPtr);
			int nextKeyframeIndex = keyframeIndex + 1;

			// Get the previous and next keyframe times
			float previousTime = times[keyframeIndex];
			float nextTime = times[nextKeyframeIndex];
			float t = (animationTime - previousTime) / (nextTime - previousTime);


			if (channel.target_path == "translation") {
				glm::vec3 translation0, translation1;
				memcpy(&translation0, outputPtr + keyframeIndex * 3 * sizeof(float), 3 * sizeof(float));
				memcpy(&translation1, outputPtr + nextKeyframeIndex * 3 * sizeof(float), 3 * sizeof(float));
				glm::vec3 translation;
				if (sampler.interpolation == "LINEAR") {
					// Perform linear interpolation
					translation = glm::mix(translation0, translation1, t);
				} else if (sampler.interpolation == "STEP") {
					// Use the current keyframe's value
					translation = translation0;
				}
				else {
					std::cout << "Unsupport interpolation type ..." << std::endl;
				}

				nodeTransforms[targetNodeIndex] = glm::translate(nodeTransforms[targetNodeIndex], translation);
			} else if (channel.target_path == "rotation") {
				glm::quat rotation0, rotation1;
				memcpy(&rotation0, outputPtr + keyframeIndex * 4 * sizeof(float), 4 * sizeof(float));
				memcpy(&rotation1, outputPtr + nextKeyframeIndex * 4 * sizeof(float), 4 * sizeof(float));
				glm::quat rotation;
				if (sampler.interpolation == "LINEAR") {
					// Perform spherical linear interpolation (slerp) for smooth rotation
					rotation = glm::slerp(rotation0, rotation1, t);
				}
				else if (sampler.interpolation == "STEP") {
					rotation = rotation0;
				}
				else {
					std::cout << "Unsupport interpolation type ..." << std::endl;
					return;
				}

				nodeTransforms[targetNodeIndex] *= glm::mat4_cast(rotation);
			} else if (channel.target_path == "scale") {
				glm::vec3 scale0, scale1;
				memcpy(&scale0, outputPtr + keyframeIndex * 3 * sizeof(float), 3 * sizeof(float));
				memcpy(&scale1, outputPtr + nextKeyframeIndex * 3 * sizeof(float), 3 * sizeof(float));
				glm::vec3 scale;
				if (sampler.interpolation == "LINEAR") {
					// Perform linear interpolation for smooth scaling transitions
					scale = glm::mix(scale0, scale1, t);
				}

				else if (sampler.interpolation == "STEP") {
					scale = scale0;
				}
				else {
					std::cout << "Unsupported interpolation type ..." << std::endl;
					return;
				}
				nodeTransforms[targetNodeIndex] = glm::scale(nodeTransforms[targetNodeIndex], scale);
			}
		}
	}

	void updateSkinning(const tinygltf::Model &model, const std::vector<glm::mat4> &nodeTransforms) {
		for (size_t skinIndex = 0; skinIndex < skinObjects.size(); ++skinIndex) {
			SkinObject &skinObject = skinObjects[skinIndex];
			const tinygltf::Skin &skin = model.skins[skinIndex]; // Access the corresponding skin

			size_t numJoints = skin.joints.size();
			skinObject.jointMatrices.resize(numJoints);

			for (size_t i = 0; i < numJoints; ++i) {
				int jointIndex = skin.joints[i];

				// Validate joint index
				if (jointIndex < 0 || jointIndex >= static_cast<int>(nodeTransforms.size())) {
					std::cerr << "Invalid joint index: " << jointIndex << std::endl;
					continue;
				}

				// Populate globalJointTransforms using nodeTransforms
				skinObject.globalJointTransforms[i] = nodeTransforms[jointIndex];

				// Combine with the inverse bind matrix to compute joint matrix
				skinObject.jointMatrices[i] =
					skinObject.globalJointTransforms[i] *
					skinObject.inverseBindMatrices[i];
			}
		}
	}


	std::vector<int> computeNodeParents(const tinygltf::Model& model) {
		std::vector<int> nodeParents(model.nodes.size(), -1); // Initialize with -1 (root nodes)

		for (size_t i = 0; i < model.nodes.size(); ++i) {
			const tinygltf::Node& node = model.nodes[i];
			for (int childIndex : node.children) {
				nodeParents[childIndex] = i; // Set the parent of the child
			}
		}

		return nodeParents;
	}

// Complete skeletal animation update function with missing edge cases handled
void update(float time) {
    // Early return if no animations or models exist
    if (model.animations.empty() || model.skins.empty()) {
        return;
    }

    // Handle animation and skin data
    const tinygltf::Animation& animation = model.animations[0];
    const AnimationObject& animationObject = animationObjects[0];
    const tinygltf::Skin& skin = model.skins[0];

    // Step 1: Initialize and compute local transforms for all nodes
    std::vector<glm::mat4> nodeTransforms(model.nodes.size(), glm::mat4(1.0f));
    updateAnimation(model, animation, animationObject, time, nodeTransforms);

    // Step 2: Compute parent relationships once
    std::vector<int> nodeParents = computeNodeParents(model);

    // Step 3: Process each skin object
    for (SkinObject& skinObject : skinObjects) {
        const size_t numJoints = skin.joints.size();

        // Validate joint data
        if (skinObject.globalJointTransforms.size() != numJoints ||
            skinObject.inverseBindMatrices.size() != numJoints) {
            // Handle error or resize vectors
            skinObject.globalJointTransforms.resize(numJoints);
            skinObject.inverseBindMatrices.resize(numJoints);
        }

        // Compute global transforms using parent hierarchy
        for (size_t i = 0; i < numJoints; ++i) {
            const int jointIndex = skin.joints[i];

            // Validate joint index
            if (jointIndex < 0 || jointIndex >= static_cast<int>(model.nodes.size())) {
                continue;  // Skip invalid joint
            }

            const int parentIndex = nodeParents[jointIndex];

            if (parentIndex == -1) {
                skinObject.globalJointTransforms[i] = nodeTransforms[jointIndex];
            } else {
                // Find parent's transform index in the joints array
                int parentTransformIndex = -1;
                for (size_t j = 0; j < numJoints; ++j) {
                    if (skin.joints[j] == parentIndex) {
                        parentTransformIndex = j;
                        break;
                    }
                }

                if (parentTransformIndex != -1) {
                    skinObject.globalJointTransforms[i] =
                        skinObject.globalJointTransforms[parentTransformIndex] *
                        nodeTransforms[jointIndex];
                } else {
                    // Parent isn't in joint list, use local transform
                    skinObject.globalJointTransforms[i] = nodeTransforms[jointIndex];
                }
            }
        }

        // Step 4: Compute final joint matrices for GPU skinning
        skinObject.jointMatrices.resize(numJoints);
        for (size_t i = 0; i < numJoints; ++i) {
            skinObject.jointMatrices[i] =
                skinObject.globalJointTransforms[i] *
                skinObject.inverseBindMatrices[i];
        }
    }
}

	bool loadModel(tinygltf::Model &model, const char *filename) {
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
		if (!warn.empty()) {
			std::cout << "WARN: " << warn << std::endl;
		}

		if (!err.empty()) {
			std::cout << "ERR: " << err << std::endl;
		}

		if (!res)
			std::cout << "Failed to load glTF: " << filename << std::endl;
		else
			std::cout << "Loaded glTF: " << filename << std::endl;

		return res;
	}

	void initialize() {
		// Modify your path if needed
		if (!loadModel(model, "../Final_Project/model/bot/bot.gltf")) {
			return;
		}

		// Prepare buffers for rendering
		primitiveObjects = bindModel(model);

		// Prepare joint matrices
		skinObjects = prepareSkinning(model);

		// Prepare animation data
		animationObjects = prepareAnimation(model);

		// Create and compile our GLSL program from the shaders
		programID = LoadShadersFromString(animationVertexShader, animationFragmentShader);
		if (programID == 0)
		{
			std::cerr << "Failed to load shaders." << std::endl;
		}

		// Get a handle for GLSL variables
		mvpMatrixID = glGetUniformLocation(programID, "MVP");
		lightPositionID = glGetUniformLocation(programID, "lightPosition");
		lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
		jointMatricesID = glGetUniformLocation(programID, "jointMatrices");
	}

	void bindMesh(std::vector<PrimitiveObject> &primitiveObjects,
				tinygltf::Model &model, tinygltf::Mesh &mesh) {

		std::map<int, GLuint> vbos;
		for (size_t i = 0; i < model.bufferViews.size(); ++i) {
			const tinygltf::BufferView &bufferView = model.bufferViews[i];

			int target = bufferView.target;

			if (bufferView.target == 0) {
				// The bufferView with target == 0 in our model refers to
				// the skinning weights, for 25 joints, each 4x4 matrix (16 floats), totaling to 400 floats or 1600 bytes.
				// So it is considered safe to skip the warning.
				//std::cout << "WARN: bufferView.target is zero" << std::endl;
				continue;
			}

			const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];
			GLuint vbo;
			glGenBuffers(1, &vbo);
			glBindBuffer(target, vbo);
			glBufferData(target, bufferView.byteLength,
						&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);

			vbos[i] = vbo;
		}

		// Each mesh can contain several primitives (or parts), each we need to
		// bind to an OpenGL vertex array object
		for (size_t i = 0; i < mesh.primitives.size(); ++i) {

			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			GLuint vao;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			for (auto &attrib : primitive.attributes) {
				tinygltf::Accessor accessor = model.accessors[attrib.second];
				int byteStride =
					accessor.ByteStride(model.bufferViews[accessor.bufferView]);
				glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);

				int size = 1;
				if (accessor.type != TINYGLTF_TYPE_SCALAR) {
					size = accessor.type;
				}

				int vaa = -1;
				if (attrib.first.compare("POSITION") == 0) vaa = 0;
				if (attrib.first.compare("NORMAL") == 0) vaa = 1;
				if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
				if (attrib.first.compare("JOINTS_0") == 0) vaa = 3;
				if (attrib.first.compare("WEIGHTS_0") == 0) vaa = 4;
				if (vaa > -1) {
					glEnableVertexAttribArray(vaa);
					glVertexAttribPointer(vaa, size, accessor.componentType,
										accessor.normalized ? GL_TRUE : GL_FALSE,
										byteStride, BUFFER_OFFSET(accessor.byteOffset));
				} else {
					std::cout << "vaa missing: " << attrib.first << std::endl;
				}
			}

			// Record VAO for later use
			PrimitiveObject primitiveObject;
			primitiveObject.vao = vao;
			primitiveObject.vbos = vbos;
			primitiveObjects.push_back(primitiveObject);

			glBindVertexArray(0);
		}
	}

	void bindModelNodes(std::vector<PrimitiveObject> &primitiveObjects,
						tinygltf::Model &model,
						tinygltf::Node &node) {
		// Bind buffers for the current mesh at the node
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			bindMesh(primitiveObjects, model, model.meshes[node.mesh]);
		}

		// Recursive into children nodes
		for (size_t i = 0; i < node.children.size(); i++) {
			assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
			bindModelNodes(primitiveObjects, model, model.nodes[node.children[i]]);
		}
	}

	std::vector<PrimitiveObject> bindModel(tinygltf::Model &model) {
		std::vector<PrimitiveObject> primitiveObjects;

		const tinygltf::Scene &scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
			bindModelNodes(primitiveObjects, model, model.nodes[scene.nodes[i]]);
		}

		return primitiveObjects;
	}

	void drawMesh(const std::vector<PrimitiveObject> &primitiveObjects,
				tinygltf::Model &model, tinygltf::Mesh &mesh) {

		for (size_t i = 0; i < mesh.primitives.size(); ++i)
		{
			GLuint vao = primitiveObjects[i].vao;
			std::map<int, GLuint> vbos = primitiveObjects[i].vbos;

			glBindVertexArray(vao);

			tinygltf::Primitive primitive = mesh.primitives[i];
			tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indexAccessor.bufferView));

			glDrawElements(primitive.mode, indexAccessor.count,
						indexAccessor.componentType,
						BUFFER_OFFSET(indexAccessor.byteOffset));

			glBindVertexArray(0);
		}
	}

	void drawModelNodes(const std::vector<PrimitiveObject>& primitiveObjects,
						tinygltf::Model &model, tinygltf::Node &node) {
		// Draw the mesh at the node, and recursively do so for children nodes
		if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
			drawMesh(primitiveObjects, model, model.meshes[node.mesh]);
		}
		for (size_t i = 0; i < node.children.size(); i++) {
			drawModelNodes(primitiveObjects, model, model.nodes[node.children[i]]);
		}
	}
	void drawModel(const std::vector<PrimitiveObject>& primitiveObjects,
				tinygltf::Model &model) {
		// Draw all nodes
		const tinygltf::Scene &scene = model.scenes[model.defaultScene];
		for (size_t i = 0; i < scene.nodes.size(); ++i) {
			drawModelNodes(primitiveObjects, model, model.nodes[scene.nodes[i]]);
		}
	}

	void render(glm::mat4 cameraMatrix) {
		glUseProgram(programID);

		// Set camera
		glm::mat4 mvp = cameraMatrix;
		glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);

		// -----------------------------------------------------------------
		// TODO: Set animation data for linear blend skinning in shader
		// -----------------------------------------------------------------
		// For each mesh that needs to be skinned
		for (size_t i = 0; i < skinObjects.size(); ++i) {
			const SkinObject& skin = skinObjects[i];

			// Validate number of joints doesn't exceed shader limit
			size_t numJoints = std::min(skin.jointMatrices.size(),
									   static_cast<size_t>(MAX_JOINTS));

			// Pass joint matrices to shader
			glUniformMatrix4fv(jointMatricesID,
							  numJoints,  // number of matrices
							  GL_FALSE,   // don't transpose
							  glm::value_ptr(skin.jointMatrices[0])); // pointer to first matrix
		}
		// -----------------------------------------------------------------

		// Set light data
		glUniform3fv(lightPositionID, 1, &lightPosition[0]);
		glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);

		// Draw the GLTF model
		drawModel(primitiveObjects, model);
	}

	void cleanup() {
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


	// Prepare shadow map size for shadow mapping. Usually this is the size of the window itself, but on some platforms like Mac this can be 2x the size of the window. Use glfwGetFramebufferSize to get the shadow map size properly.
	glfwGetFramebufferSize(window, &shadowMapWidth, &shadowMapHeight);
	std::cout << "Shadow Map Width: " << shadowMapWidth << std::endl;
	std::cout << "Shadow Map Height: " << shadowMapHeight << std::endl;

	// Background
	glClearColor(0.2f, 0.2f, 0.2f, 0.f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);


	// Our 3D character
	MyBot bot;
	bot.initialize();

	// Initialize cliff wall
	world_setup myWorld;
	myWorld.intialize(glm::vec3(80, -10, -100), glm::vec3(200, 200, 200));

	HorizontalPlane myPlane;
	myPlane.initialize(glm::vec3(80, -10, -100), glm::vec3(200, 200, 200));

	Building myBuilding;
	myBuilding.initialize(glm::vec3(0, 120, 0), glm::vec3(32, 3*32, 32));

	Lighting_Shadows renderLight;
	renderLight.initialize();



	float near_plane = 1.0f, far_plane = 50.0f;
	// Define the orthographic frustum bounds
	float left = -200.0f;
	float right = 200.0f;
	float bottom = -200.0f;
	float top = 200.0f;
	glm::mat4 orthographicLightProjection = glm::ortho(left, right, bottom, top, near_plane, far_plane);
	glm::mat4 lightProjection = glm::perspective(glm::radians(depthFoV),(float)windowWidth/windowHeight, depthNear, depthFar);
	glm::mat4 lightView = glm::lookAt (lightPosition,lightLookat, lightUp);

	glm::mat4 lightSpaceMatrix = lightProjection * lightView;
	glm::mat4 orthographicLSM = orthographicLightProjection * lightView;;

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
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);

	// Time and frame rate tracking
	static double lastTime = glfwGetTime();
	float time = 0.0f;			// Animation time
	float fTime = 0.0f;			// Time for measuring fps
	unsigned long frames = 0;

	renderLight.shadowMapPass(lightSpaceMatrix);
    // ------------------------------------
	skybox mySkybox({"../Final_Project/Textures/px.png", "../Final_Project/Textures/nx.png", "../Final_Project/Textures/py.png", "../Final_Project/Textures/ny.png","../Final_Project/Textures/pz.png", "../Final_Project/Textures/nz.png" });
	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Update states for animation
		double currentTime = glfwGetTime();
		float deltaTime = float(currentTime - lastTime);
		lastTime = currentTime;

		if (playAnimation) {
			time += deltaTime * playbackSpeed;
			bot.update(time);
		}

		// -------------------------------------------------------------------------------------
		// For convenience, we multiply the projection and view matrix together and pass a single matrix for rendering
		glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
		glm::mat4 vp = projectionMatrix * viewMatrix;
		bot.render(vp);
		// FPS tracking
		// Count number of frames over a few seconds and take average
		frames++;
		fTime += deltaTime;
		if (fTime > 2.0f) {
			float fps = frames / fTime;
			frames = 0;
			fTime = 0;

			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << "Final Project | Frames per second (FPS): " << fps;
			glfwSetWindowTitle(window, stream.str().c_str());
		}
		//------------------------------------------------------------------------------
		if (saveDepth) {
			myBuilding.renderShadow(lightSpaceMatrix);
			myWorld.renderShadow(lightSpaceMatrix);
			//myPlane.renderShadow(lightSpaceMatrix);
			glBindFramebuffer(GL_FRAMEBUFFER, renderLight.depthTexture);
			glViewport(0,0,windowWidth, windowHeight);
			std::string filename = "../Final_Project/depth_camera.png";
			saveDepthTexture(renderLight.depthTexture, filename);
			std::cout << "Depth texture saved to " << filename << std::endl;
			saveDepth = false;
		}

		//------------------------------------------------------------------------------


		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, renderLight.depthTexture);
		mySkybox.render(viewMatrix,projectionMatrix);
		myBuilding.renderWithLight(vp, lightSpaceMatrix);
		myWorld.renderWithLight(vp,lightSpaceMatrix);
		//myPlane.renderWithLight(vp,lightSpaceMatrix,lightIntensity, lightPosition);

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
	bot.cleanup();
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
