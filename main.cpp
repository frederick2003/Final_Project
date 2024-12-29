#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <unordered_set>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/norm.hpp>
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
#include "../Final_Project/Shaders/cloud_particle_rendering.h"
#include <iomanip>
#define BUFFER_OFFSET(i) ((char *)NULL + (i))



static GLFWwindow *window;
static int windowWidth = 1024;
static int windowHeight = 768;
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static GLuint LoadTextureTileBox(const char *texture_file_path);
static void saveDepthTexture(GLuint fbo, std::string filename);
static void cursor_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void setupMouseCallbacks(GLFWwindow* window);

// Global variables for Mouse drag functionality
static bool mousePressed = false;
static double lastX = 0.0;
static double lastY = 0.0;
static float mouseSensitivity = 0.5f;

// Debug information for renderQuad() function
unsigned int quadVAO = 0;
unsigned int quadVBO;

// OpenGL camera view parameters
//static glm::vec3 eye_center(623.182,530.257,-372.065);
static glm::vec3 eye_center(239.534,200.552,-227.14);
static glm::vec3 lookat(0,0,0);
static glm::vec3 up(0, 1, 0);

// View control
static float viewAzimuth = 0.0f;
static float viewPolar = 0.0f;
static float viewDistance = 300.0f;

static float FoV = 40.0f;
static float zNear = 1.0f;
static float zFar = 1500.0f;

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

// Animation
static bool playAnimation = true;
static float playbackSpeed = 2.0f;

// Helper flag and function to save depth maps for debugging
static bool saveDepth = true;

// Struct defining a particle to be used in a cloud system.
struct CloudParticle {
	glm::vec3 position;
	glm::vec3 velocity;
	float size;
	float alpha;
	float life;
};

// A struct defining a cloud system of cloud particles.
struct CloudSystem{
	std::vector<CloudParticle> particles;
	GLuint vertexArrayID, vertexBufferID;
	GLuint textureID, shaderID;
	GLuint mvpID, cameraRightID, cameraUpID, texSamplerID, alphaID;

	const int NUM_PARTICLES = 2000;
	const float CLOUD_HEIGHT_MIN = 150.0f; // Adjust based on mountain height
	const float CLOUD_HEIGHT_MAX = 200.0f;
	const float CLOUD_RADIUS = 300.0f;     // How far clouds spread from center

    const GLfloat vertices[12] = {
    	-0.5f, -0.5f, 0.0f,
    	0.5f, -0.5f, 0.0f,
    	0.5f,  0.5f, 0.0f,
    	-0.5f,  0.5f, 0.0f,
    };

	void initialize() {
		shaderID = LoadShadersFromString(cloudVertexShader, cloudFragmentShader);

		// Get uniform locations
		mvpID = glGetUniformLocation(shaderID, "MVP");
		cameraRightID = glGetUniformLocation(shaderID, "cameraRight");
		cameraUpID = glGetUniformLocation(shaderID, "cameraUp");
		texSamplerID = glGetUniformLocation(shaderID, "textureSampler");
		alphaID = glGetUniformLocation(shaderID, "particleAlpha");

		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		//TODO add a cloud texture.
		textureID = LoadTextureTileBox("../Final_Project/Textures/Cloud.png");

		initializeParticles();
	}

	void initializeParticles() {
		particles.clear();
		particles.reserve(NUM_PARTICLES);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
		std::uniform_real_distribution<float> radiusDist(0.0f, CLOUD_RADIUS);
		std::uniform_real_distribution<float> heightDist(CLOUD_HEIGHT_MIN, CLOUD_HEIGHT_MAX);
		std::uniform_real_distribution<float> sizeDist(20.0f, 40.0F);
		std::uniform_real_distribution<float> lifeDist(5.0f, 10.0f );

		// Sets up each particle to be used in our cloud effect, each particle is generated with a degree of randomness.
		for(int i = 0; i < NUM_PARTICLES; i++) {
			float angle = angleDist(gen);
			float radius = radiusDist(gen);
			float height = heightDist(gen);

			CloudParticle particle;
			particle.position = glm::vec3(
				180.0f + radius * cos(angle),
				height,
				-100.0f + radius * sin(angle)
				);

			particle.velocity = glm::vec3(
				cos(angle) * 2.0f,
				0.0f,
				sin(angle) * 2.0f
				);

			particle.size = sizeDist(gen);
			particle.alpha = 0.3f;
			particle.life = lifeDist(gen);

			particles.push_back(particle);
		}
	}

	void update(float deltaTime) {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
		std::uniform_real_distribution<float> radiusDist(0.0f, CLOUD_RADIUS);
		std::uniform_real_distribution<float> heightDist(CLOUD_HEIGHT_MIN, CLOUD_HEIGHT_MAX);
		std::uniform_real_distribution<float> sizeDist(20.0f, 40.0F);
		std::uniform_real_distribution<float> lifeDist(5.0f, 10.0f );

		// Iterate though each element in the container.
		for(auto& particle : particles) {
			particle.life -= deltaTime;

			if (particle.life <= 0.0f) {
				//Reset the particle to new random state.
				float angle = angleDist(gen);
				float radius = radiusDist(gen);
				float height = heightDist(gen);

				particle.position = glm::vec3(
				180.0f + radius * cos(angle),
				height,
				-100.0f + radius * sin(angle)
				);

				particle.velocity = glm::vec3(
					cos(angle) * 2.0f,
					0.0f,
					sin(angle) * 2.0f
					);

				particle.size = sizeDist(gen);
				particle.alpha = 0.3f;
				particle.life = lifeDist(gen);
			}
			else {
				// update particle position
				particle.position += particle.velocity * deltaTime;

				// adjust the particle's alpha based on life.
				particle.alpha = std::min(0.3f, particle.life * 0.1f);
			}
		}
	}

	void render(const glm::mat4& ViewProjection, const glm::vec3& cameraPos, const glm::vec3& lookat = glm::vec3(0.0f), const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
		glUseProgram(shaderID);

		//Enable blending to achieve transparent effect.
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// Disable depth writing.
		glDepthMask(GL_FALSE);

		// Extract view matrix for proper camera vectors
		glm::mat4 viewMatrix = glm::lookAt(cameraPos, lookat, up);
		glm::vec3 cameraRight = glm::normalize(glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]));
		glm::vec3 cameraUp = glm::normalize(glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(texSamplerID, 0);

		glBindVertexArray(vertexArrayID);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		// Sort particles by distance to camera (back to front)
		std::sort(particles.begin(), particles.end(),
			[cameraPos](const CloudParticle& a, const CloudParticle& b) {
				return glm::length2(a.position - cameraPos) > glm::length2(b.position - cameraPos);
			});

		// Render each particle
		for (const auto& particle : particles) {
			glm::mat4 model = glm::translate(glm::mat4(1.0f), particle.position);
			model = glm::scale(model, glm::vec3(particle.size));

			glm::mat4 MVP = ViewProjection * model;
			glUniformMatrix4fv(mvpID, 1, GL_FALSE, &MVP[0][0]);
			glUniform1f(alphaID, particle.alpha);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		// Reset OpenGL state
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisableVertexAttribArray(0);
	}

	void cleanup() {
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteVertexArrays(1, &vertexBufferID);
		glDeleteTextures(1, &textureID);
		glDeleteProgram(shaderID);
	}
};

// Struct defining the mountain surrounding the scene.
struct Mountain {
    glm::vec3 position;
    glm::vec3 scale;
	float rotationAngle = glm::radians(90.0f);        // Rotation angle in degrees (converted to radians)
	glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Rotation around Y-axis

    std::vector<GLfloat> vertex_buffer_data;
    std::vector<GLfloat> normal_buffer_data;
    std::vector<GLfloat> uv_buffer_data;

    // OpenGL buffers and IDs
    glm::mat4 modelMatrix;
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint normalBufferID;
    GLuint uvBufferID;
    GLuint textureID;

    // Shader variables
    GLuint mvpMatrixID;
    GLuint modelMatrixID;
    GLuint viewMatrixID;
    GLuint normalMatrixID;
    GLuint lightPositionID;
    GLuint lightIntensityID;
    GLuint textureSamplerID;
    GLuint programID;
    GLuint depthShaderID;
    GLuint shadowMapTextureID;
    GLuint lightSpaceMatrixID;
    GLuint LSM_ID;

    // Mountain generation parameters
    const int SEGMENTS = 20;  // Number of segments per edge
    const float BASE_HEIGHT = 0.15f;
    const float MAX_HEIGHT = 2.0f;

    float perlinNoise(float x, float y) {
        // Simple noise function for height variation
        return sin(x * 0.1f) * cos(y * 0.1f) * 0.5f +
               sin(x * 0.2f + y * 0.3f) * 0.25f;
    }

    float getHeight(float x, float z, float edgeDistance) {
        // Create height variation that smoothly decreases as we move away from the edge
        float noise = perlinNoise(x * 10.0f, z * 10.0f);
        float height = MAX_HEIGHT * (1.0f - edgeDistance) * (0.8f + 0.2f * noise);
        return std::max(BASE_HEIGHT, height);
    }

    void generateMountainGeometry() {
        // Clear existing data
        vertex_buffer_data.clear();
        normal_buffer_data.clear();
        uv_buffer_data.clear();

        std::vector<std::vector<glm::vec3>> vertices;
        std::vector<std::vector<float>> heights;

        // Generate grid of vertices
        for (int i = 0; i <= SEGMENTS; i++) {
            std::vector<glm::vec3> row;
            std::vector<float> heightRow;
            float z = -1.0f + (2.0f * i / SEGMENTS);

            for (int j = 0; j <= SEGMENTS; j++) {
                float x = -1.0f + (2.0f * j / SEGMENTS);

                // Calculate distance from edges
                float distFromLeft = abs(x + 1.0f);
                float distFromRight = abs(x - 1.0f);
                float distFromBack = abs(z - 1.0f);

                // Find minimum distance to any edge
                float edgeDistance = std::min({distFromLeft, distFromRight, distFromBack});
                edgeDistance = std::min(1.0f, edgeDistance * 2.0f);  // Scale distance for sharper falloff

                // Generate height based on edge distance and noise
                float height = getHeight(x, z, edgeDistance);

                // Keep base height for points not near edges
                if (edgeDistance > 0.8f) {
                    height = BASE_HEIGHT;
                }

                // Store vertex and height
                row.push_back(glm::vec3(x, height, z));
                heightRow.push_back(height);
            }
            vertices.push_back(row);
            heights.push_back(heightRow);
        }

        // Generate triangles
        for (int i = 0; i < SEGMENTS; i++) {
            for (int j = 0; j < SEGMENTS; j++) {
                glm::vec3 v1 = vertices[i][j];
                glm::vec3 v2 = vertices[i+1][j];
                glm::vec3 v3 = vertices[i][j+1];
                glm::vec3 v4 = vertices[i+1][j+1];

                // Calculate normals
                glm::vec3 normal1 = glm::normalize(glm::cross(v2 - v1, v3 - v1));
                glm::vec3 normal2 = glm::normalize(glm::cross(v4 - v2, v3 - v2));

                // First triangle
                vertex_buffer_data.insert(vertex_buffer_data.end(), {
                    v1.x, v1.y, v1.z,
                    v2.x, v2.y, v2.z,
                    v3.x, v3.y, v3.z
                });

                // Second triangle
                vertex_buffer_data.insert(vertex_buffer_data.end(), {
                    v2.x, v2.y, v2.z,
                    v4.x, v4.y, v4.z,
                    v3.x, v3.y, v3.z
                });

                // Normals for first triangle
                normal_buffer_data.insert(normal_buffer_data.end(), {
                    normal1.x, normal1.y, normal1.z,
                    normal1.x, normal1.y, normal1.z,
                    normal1.x, normal1.y, normal1.z
                });

                // Normals for second triangle
                normal_buffer_data.insert(normal_buffer_data.end(), {
                    normal2.x, normal2.y, normal2.z,
                    normal2.x, normal2.y, normal2.z,
                    normal2.x, normal2.y, normal2.z
                });

                // UV coordinates
                float texU1 = static_cast<float>(j) / SEGMENTS;
                float texU2 = static_cast<float>(j + 1) / SEGMENTS;
                float texV1 = static_cast<float>(i) / SEGMENTS;
                float texV2 = static_cast<float>(i + 1) / SEGMENTS;

                // UVs for first triangle
                uv_buffer_data.insert(uv_buffer_data.end(), {
                	texU1, texV1,
					texU1, texV2,
					texU2, texV1
                });

                // UVs for second triangle
                uv_buffer_data.insert(uv_buffer_data.end(), {
                	texU1, texV2,
                	texU2, texV2,
                	texU2, texV1
                });
            }
        }
    }

    void initialize(glm::vec3 position, glm::vec3 scale) {
        this->position = position;
        this->scale = scale;

        // Generate mountain geometry
        generateMountainGeometry();

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, position);
    	modelMatrix = glm::rotate(modelMatrix, rotationAngle, rotationAxis);
        modelMatrix = glm::scale(modelMatrix, scale);

        // Create and bind VAO
        glGenVertexArrays(1, &vertexArrayID);
        glBindVertexArray(vertexArrayID);

        // Create and bind vertex buffer
        glGenBuffers(1, &vertexBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glBufferData(GL_ARRAY_BUFFER, vertex_buffer_data.size() * sizeof(GLfloat),
                    vertex_buffer_data.data(), GL_STATIC_DRAW);

        // Create and bind normal buffer
        glGenBuffers(1, &normalBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glBufferData(GL_ARRAY_BUFFER, normal_buffer_data.size() * sizeof(GLfloat),
                    normal_buffer_data.data(), GL_STATIC_DRAW);

        // Create and bind UV buffer
        glGenBuffers(1, &uvBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glBufferData(GL_ARRAY_BUFFER, uv_buffer_data.size() * sizeof(GLfloat),
                    uv_buffer_data.data(), GL_STATIC_DRAW);

        // Load shaders
        programID = LoadShadersFromString(lightingVertexShader, lightingFragmentShader);
        depthShaderID = LoadShadersFromString(depthVertexShader, depthFragmentShader);

        // Get uniform locations
        mvpMatrixID = glGetUniformLocation(programID, "MVP");
        modelMatrixID = glGetUniformLocation(programID, "modelMatrix");
        normalMatrixID = glGetUniformLocation(programID, "normalMatrix");
        lightPositionID = glGetUniformLocation(programID, "lightPosition");
        lightIntensityID = glGetUniformLocation(programID, "lightIntensity");
        textureSamplerID = glGetUniformLocation(programID, "textureSampler");
        shadowMapTextureID = glGetUniformLocation(programID, "shadowMap");
        LSM_ID = glGetUniformLocation(programID, "lightSpaceMatrix");

        // Load mountain texture
        textureID = LoadTextureTileBox("../Final_Project/Textures/mountain_texture2.jpg");
    }

    void renderWithLight(glm::mat4 cameraMatrix, glm::mat4 lightSpaceMatrix) {
        glUseProgram(programID);
        glBindVertexArray(vertexArrayID);

        // Enable vertex attributes
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // Set uniforms
        glm::mat4 mvp = cameraMatrix * modelMatrix;
        glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
        glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
        glUniformMatrix4fv(LSM_ID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
        glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);

        glUniform3fv(lightPositionID, 1, &lightPosition[0]);
        glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);
        glUniform1i(shadowMapTextureID, 1);

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(textureSamplerID, 0);

        // Draw triangles
        glDrawArrays(GL_TRIANGLES, 0, vertex_buffer_data.size() / 3);

        // Disable vertex attributes
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

        glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);
        glUniformMatrix4fv(lightSpaceMatrixID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

        glDrawArrays(GL_TRIANGLES, 0, vertex_buffer_data.size() / 3);

        glDisableVertexAttribArray(0);
    }

    void cleanup() {
        glDeleteBuffers(1, &vertexBufferID);
        glDeleteBuffers(1, &normalBufferID);
        glDeleteBuffers(1, &uvBufferID);
        glDeleteVertexArrays(1, &vertexArrayID);
        glDeleteTextures(1, &textureID);
        glDeleteProgram(programID);
        glDeleteProgram(depthShaderID);
    }
};

// Struct defining a metro stop object.
struct metro_stop {
	glm::vec3 position;		// Position
	glm::vec3 scale;		// Size
	float rotationAngle;   // rotation angle in degrees
	glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Rotation around Y-axis

	GLfloat vertex_buffer_data[120] {
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

		// Back face (interior)
		0.99f, -0.99f, -0.99f,
	   -0.99f, -0.99f, -0.99f,
	   -0.99f,  0.99f, -0.99f,
		0.99f,  0.99f, -0.99f,

		// Left face (interior)
	   -0.99f, -0.99f, -0.99f,
	   -0.99f, -0.99f,  0.99f,
	   -0.99f,  0.99f,  0.99f,
	   -0.99f,  0.99f, -0.99f,

		// Right face (interior)
		0.99f, -0.99f,  0.99f,
		0.99f, -0.99f, -0.99f,
		0.99f,  0.99f, -0.99f,
		0.99f,  0.99f,  0.99f,

		// Top face (interior)
	   -0.99f,  0.99f,  0.99f,
		0.99f,  0.99f,  0.99f,
		0.99f,  0.99f, -0.99f,
	   -0.99f,  0.99f, -0.99f,

		// Bottom face (interior)
	   -0.99f, -0.99f, -0.99f,
		0.99f, -0.99f, -0.99f,
		0.99f, -0.99f,  0.99f,
	   -0.99f, -0.99f,  0.99f,
	};

	GLfloat normal_buffer_data[120] {
		// Back face
		0.0f, 0.0f, -1.0f,
	   0.0f, 0.0f, -1.0f,
	   0.0f, 0.0f, -1.0f,
	   0.0f, 0.0f, -1.0f,

	   // Left face
	   -1.0f, 0.0f, 0.0f,
	   -1.0f, 0.0f, 0.0f,
	   -1.0f, 0.0f, 0.0f,
	   -1.0f, 0.0f, 0.0f,

	   // Right face
	   1.0f, 0.0f, 0.0f,
	   1.0f, 0.0f, 0.0f,
	   1.0f, 0.0f, 0.0f,
	   1.0f, 0.0f, 0.0f,

	   // Top face
	   0.0f, 1.0f, 0.0f,
	   0.0f, 1.0f, 0.0f,
	   0.0f, 1.0f, 0.0f,
	   0.0f, 1.0f, 0.0f,

	   // Bottom face
	  0.0f, -1.0f, 0.0f,
	  0.0f, -1.0f, 0.0f,
	  0.0f, -1.0f, 0.0f,
	  0.0f, -1.0f, 0.0f,

		0.0f,  0.0f,  1.0f,
		0.0f,  0.0f,  1.0f,
		0.0f,  0.0f,  1.0f,
		0.0f,  0.0f,  1.0f,

		// Left face (interior)
		1.0f,  0.0f,  0.0f,
		1.0f,  0.0f,  0.0f,
		1.0f,  0.0f,  0.0f,
		1.0f,  0.0f,  0.0f,

		// Right face (interior)
	   -1.0f,  0.0f,  0.0f,
	   -1.0f,  0.0f,  0.0f,
	   -1.0f,  0.0f,  0.0f,
	   -1.0f,  0.0f,  0.0f,

		// Top face (interior)
		0.0f, -1.0f,  0.0f,
		0.0f, -1.0f,  0.0f,
		0.0f, -1.0f,  0.0f,
		0.0f, -1.0f,  0.0f,

		// Bottom face (interior)
		0.0f,  1.0f,  0.0f,
		0.0f,  1.0f,  0.0f,
		0.0f,  1.0f,  0.0f,
		0.0f,  1.0f,  0.0f,
   };

	GLuint index_buffer_data[60] = {
		0, 1, 2,//B
		0, 2, 3,

		4, 5, 6,//L
		4, 6, 7,

		8, 9, 10,//R
		8, 10, 11,

		12, 13, 14,//T
		12, 14, 15,

		16, 17, 18,//B
		16, 18, 19,

		// Back face (interior)
		20, 21, 22,
		20, 22, 23,

		// Left face (interior)
		24, 25, 26,
		24, 26, 27,

		// Right face (interior)
		28, 29, 30,
		28, 30, 31,

		// Top face (interior)
		32, 33, 34,
		32, 34, 35,

		// Bottom face (interior)
		36, 37, 38,
		36, 38, 39,
	};

	GLfloat uv_buffer_data[80] = {

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
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
	GLuint textureID, textureID2;


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

	GLuint programID2;
	GLuint depthShaderID;

	void initialize(glm::vec3 position, glm::vec3 scale, float rotationAngle) {
		// Define scale of the building geometry
		this->position = position;
		this->scale = scale;
		this->rotationAngle = rotationAngle;
		std::string textureLocation = "../Final_Project/Textures/Metro_Sides.jpg";
		std::string textureLocation2 = "../Final_Project/Textures/roof_tex.jpg";


		modelMatrix = glm::mat4(1.0f);
		// Translate the cliff to its position
		modelMatrix = glm::translate(modelMatrix, position);
		// Rotate
		modelMatrix = glm::rotate(modelMatrix, rotationAngle, rotationAxis);
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
		textureSamplerID = glGetUniformLocation(programID2,"textureSampler");




		//Depth rendering shader uniforms
		modelMatrixDepthID = glGetUniformLocation(depthShaderID, "model");
		lightSpaceMatrixID = glGetUniformLocation(depthShaderID, "lightSpaceMatrix");

		textureID = LoadTextureTileBox(textureLocation.c_str());
		textureID2 = LoadTextureTileBox(textureLocation2.c_str());
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

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);


		//Set textureSampler to use texture unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(textureSamplerID, 0);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			18,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glBindTexture(GL_TEXTURE_2D, textureID2);
		glUniform1i(textureSamplerID, 0);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			12,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)(18 * sizeof(GLuint))           // element array buffer offset
		);


		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(textureSamplerID, 0);
		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			30,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)(30 * sizeof(GLuint))           // element array buffer offset
		);

		glEnable(GL_CULL_FACE);


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
			30,    			   // number of indices
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
		glDeleteProgram(programID2);
	}
};

// Struct defining a sports centers.
struct sports_center {

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
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		// Back face
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,

		// Left face
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,

		// Right face
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Top face
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Bottom face
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,

	};


	GLuint index_buffer_data[36] = {		// 12 triangle faces of a box
		0, 1, 2,//F
		0, 2, 3,

		4, 5, 6,//B
		4, 6, 7,

		8, 9, 10,//L
		8, 10, 11,

		12, 13, 14,//R
		12, 14, 15,

		16, 17, 18,//T
		16, 18, 19,

		20, 21, 22,//B
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
		// Top
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Bottom
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
	GLuint textureID, textureID2;


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
		std::string textureLocation = "../Final_Project/Textures/brick_wall_text.jpg";
		std::string textureLocation2 = "../Final_Project/Textures/basketball_court.jpg";


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
		textureSamplerID = glGetUniformLocation(programID2,"textureSampler");

		//Depth rendering shader uniforms
		modelMatrixDepthID = glGetUniformLocation(depthShaderID, "model");
		lightSpaceMatrixID = glGetUniformLocation(depthShaderID, "lightSpaceMatrix");

		textureID = LoadTextureTileBox(textureLocation.c_str());
		textureID2 = LoadTextureTileBox(textureLocation2.c_str());
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
			24,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glBindTexture(GL_TEXTURE_2D, textureID2);
		glUniform1i(textureSamplerID, 0);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			12,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)(24 * sizeof(GLuint))           // element array buffer offset
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

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteTextures(1, &textureID);
		glDeleteProgram(programID2);
	}
};

// Standard skybox struct.
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

// A struct drawing world attributes, road textures, footpath textures etc.
struct world_attributes{

	glm::vec3 position;		// Position of the box
	glm::vec3 scale;		// Size of the box in each axis

	GLfloat vertex_buffer_data[84] = {	// Vertex definition for a canonical box
		// Coords for first Footpath
		-0.7f, 0.152f, 0.8f, //0
		-0.3f, 0.152f, 0.8f, //1
		-0.3f, 0.152f, 0.7f, //2
		-0.7f, 0.152f, 0.7f, //3
		//coords for second footpath
		-0.5f, 0.152f, 0.4f, //4
		-0.3f, 0.152f, 0.4f, //5
		-0.3f, 0.152f, 0.3f, //6
		-0.5f, 0.152f, 0.3f, //7

		-0.7f, 0.152f, -0.5f, //8
		-0.3f, 0.152f, -0.5f, //9
		-0.3f, 0.152f, -0.6f, //10
		-0.7f, 0.152f, -0.6f, //11

		-0.7f, 0.152f, -0.8f, //12
		-0.3f, 0.152f, -0.8f, //13
		-0.3f, 0.152f, -0.9f, //14
		-0.7f, 0.152f, -0.9f, //15

		// Connecting Footpath (new)
		-0.3f, 0.152f, 1.0f,  //16
		0.0f, 0.152f, 1.0f,  //17
		0.0f, 0.152f, 0.0f,  //18
		-0.3f, 0.152f, 0.0f,   //19

		// Coords for central plateau road
		-0.7f, 0.152f, 0.0f,  //20
		1.0f, 0.152f, 0.0f,  //21
		1.0f, 0.152f, -0.3f, //22
		-0.7f, 0.152f, -0.3f,  //23

		// second connecting footpath
		-0.3f,0.152f,-0.3f, //24
		0.0f,0.152f,-0.3f, //25
		0.0f,0.152f,-1.0f, //26
		-0.3f,0.152f,-1.0f //27
	};

	GLfloat normal_buffer_data[84] = {
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
	};


	GLuint index_buffer_data[42] = {
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

		24, 25, 26,
		24, 26, 27
	};

	GLfloat uv_buffer_data[56] = {
		// Front
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
	};


	glm::mat4 modelMatrix;

	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint normalBufferID;
	GLuint uvBufferID;
	GLuint TextureID, roadTextureID;


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
	    	for (int i = 0; i < 24; ++i) {
	    		// Textures need to be rotated
	    		float u = uv_buffer_data[2 * i];
	    		float v = uv_buffer_data[2 * i + 1];

	    		uv_buffer_data[2 * i] = 1.0f - v; // New U
	    		uv_buffer_data[2 * i + 1] = u;    // New V
	    	}

        glGenBuffers(1, &uvBufferID);
        glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);

        glGenBuffers(1, &indexBufferID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer_data), index_buffer_data, GL_STATIC_DRAW);

        depthShaderID = LoadShadersFromString(depthVertexShader, depthFragmentShader);
        programID = LoadShadersFromString(lightingVertexShader, lightingFragmentShader);
        TextureID = LoadTextureTileBox("../Final_Project/Textures/footpath_text.jpg");
	    roadTextureID = LoadTextureTileBox("../Final_Project/Textures/Road_text.jpg");

        if (programID == 0 || depthShaderID == 0) {
            std::cerr << "Failed to load shaders." << std::endl;
        }
        if (TextureID == 0) {
            std::cerr << "Failed to load texture." << std::endl;
        }

        mvpMatrixID = glGetUniformLocation(programID, "MVP");
        modelMatrixID = glGetUniformLocation(programID, "modelMatrix");
        normalMatrixID = glGetUniformLocation(programID, "normalMatrix");
    	LSM_ID = glGetUniformLocation(programID, "lightSpaceMatrix");

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
				42,    			   // number of indices
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

	    	// Draw the footpaths
	    	glDrawElements(
				GL_TRIANGLES,      // mode
				30,    			   // number of indices
				GL_UNSIGNED_INT,   // type
				(void*)0           // element array buffer offset
			);

			glBindTexture(GL_TEXTURE_2D, roadTextureID);
			glUniform1i(textureSamplerID, 0);

		glDrawElements(
			GL_TRIANGLES,      // mode
			6,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)(30 * sizeof(GLuint))           // element array buffer offset
		);

		glBindTexture(GL_TEXTURE_2D, TextureID);
		glUniform1i(textureSamplerID, 0);

		glDrawElements(
		GL_TRIANGLES,      // mode
		6,    			   // number of indices
		GL_UNSIGNED_INT,   // type
		(void*)(36 * sizeof(GLuint))           // element array buffer offset
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

// Struct to set up the main features of the scene, the sea, cliff and plateau.
struct world_setup {
	glm::vec3 position;
	glm::vec3 scale;

	struct SeaVertex {
		glm::vec3 position;
		glm::vec2 texCoord;
		glm::vec3 normal;
	};

	std::vector<SeaVertex> seaVertices;
	std::vector<GLuint> seaIndices;
	GLuint seaVAO, seaVBO, seaEBO;
	float seaTime = 0.0f;

	const int SEA_GRID_SIZE = 64;
	const float SEA_EXTEND_OUT = 2000.0f;
	const float SEA_EXTEND_SIDE = 3000.0f;
	const float CLIFF_BASE_X = 0.0f;
	const float TEXTURE_REPEAT = 50.0f;


	GLfloat vertex_buffer_data[24] = {
		// Coords for cliff face
		-1.25f, -0.15f, -1.0f,
		-1.25f, -0.15f, 1.0f,
		-1.0f, 0.15f, 1.0f,
		-1.0f, 0.15f, -1.0f,

		// Coords for plateau
		-1.0f, 0.15f, 1.0f,
		1.0f, 0.15f, 1.0f,
		1.0f, 0.15f, -1.0f,
		-1.0f, 0.15f, -1.0f,
	};

	GLfloat normal_buffer_data[24] {
		-0.769,0.641,0,
		-0.769,0.641,0,
		-0.769,0.641,0,
		-0.769,0.641,0,

		0.0,1.0,0.0,
		0.0,1.0,0.0,
		0.0,1.0,0.0,
		0.0,1.0,0.0,
	};

	GLfloat uv_buffer_data[16] {
		0.0f, 1.0f, // cliff face
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f, // plateau
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
	};


	GLuint index_buffer_data[12] {
		0,1,2, //Cliff face
		2,3,0,

		4, 5, 6, // Plateau
		4, 6, 7,
	};

	glm::mat4 modelMatrix;
	// OpenGL buffers
	GLuint vertexArrayID;
	GLuint vertexBufferID;
	GLuint indexBufferID;
	GLuint uvBufferID;
	GLuint normalBufferID;
	GLuint TextureID, TextureID2, TextureID3;


	// Shader variable IDs
	GLuint normalMatrixID;
	GLuint textureSamplerID;
	GLuint mvpMatrixID;
	GLuint modelMatrixID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint programID2;
	GLuint depthShaderID;


	GLuint simpleDepthShader;
	GLuint lightSpaceMatrixID;
	GLuint modelMatrixDepthID;
	GLuint shadowMapTextureID;
	GLuint LSM_ID;

	void intialize(glm::vec3 position, glm::vec3 scale) {
		this->position = position;
		this->scale = scale;

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
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

		for(int i = 8; i < 16; i+=2) {
			uv_buffer_data[i] *= 20;
			uv_buffer_data[i+1] *= 20;
		}

		glGenBuffers(1, &uvBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data,GL_STATIC_DRAW);

		glGenBuffers(1, &normalBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(normal_buffer_data), normal_buffer_data, GL_STATIC_DRAW);

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

		textureSamplerID = glGetUniformLocation(programID2, "textureSampler");

		if ( mvpMatrixID== -1 || textureSamplerID == -1  ) {
			std::cout << "Error loading shaders." << std::endl;
		}
        //creates data to be used in the creation and rendering of the sea.
		initializeCliffSea();
	}

	void renderWithLight(glm::mat4 cameraMatrix, glm::mat4 lightSpaceMatrix) {
		static float lastTime = glfwGetTime();
		float currentTime = glfwGetTime();
		float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		updateCliffSea(deltaTime);

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

		// Render the new animated sea mesh
		glBindTexture(GL_TEXTURE_2D, TextureID3);
		glUniform1i(textureSamplerID, 0);
		renderCliffSea(cameraMatrix, lightSpaceMatrix);

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
			12,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
	}

	void initializeCliffSea() {
    seaVertices.clear();
    seaIndices.clear();

    // Calculate grid spacings
    float gridSpacingOut = SEA_EXTEND_OUT / (SEA_GRID_SIZE - 1);
    float gridSpacingSide = SEA_EXTEND_SIDE / (SEA_GRID_SIZE - 1);

    // Generate vertices with higher density near cliff
    for (int z = 0; z < SEA_GRID_SIZE; z++) {
        for (int x = 0; x < SEA_GRID_SIZE; x++) {
            SeaVertex vertex;

            // Use exponential distribution for x-coordinates (outward from cliff)
            float xProgress = static_cast<float>(x) / (SEA_GRID_SIZE - 1);
            float xPos = CLIFF_BASE_X - SEA_EXTEND_OUT * (1.0f - exp(-3.0f *(1.0f - xProgress)));

            // Use a wider range for z-coordinates (lateral spread)
            float zProgress = static_cast<float>(z) / (SEA_GRID_SIZE - 1);
            // Center the z-coordinate range around the cliff
            float zPos = -SEA_EXTEND_SIDE/2 + zProgress * SEA_EXTEND_SIDE;

            vertex.position = glm::vec3(xPos, -0.15f, zPos);

            // Calculate texture coordinates with more detail near cliff
            vertex.texCoord.x = (1.0f - xProgress) * TEXTURE_REPEAT;
            vertex.texCoord.y = zProgress * TEXTURE_REPEAT;

            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);

            seaVertices.push_back(vertex);
        }
    }

    // Generate indices for triangle strips
    for (int z = 0; z < SEA_GRID_SIZE - 1; z++) {
        for (int x = 0; x < SEA_GRID_SIZE - 1; x++) {
            GLuint topLeft = z * SEA_GRID_SIZE + x;
            GLuint topRight = topLeft + 1;
            GLuint bottomLeft = (z + 1) * SEA_GRID_SIZE + x;
            GLuint bottomRight = bottomLeft + 1;

            seaIndices.push_back(topLeft);
            seaIndices.push_back(bottomLeft);
            seaIndices.push_back(topRight);

            seaIndices.push_back(topRight);
            seaIndices.push_back(bottomLeft);
            seaIndices.push_back(bottomRight);
        }
    }

    // Create and set up OpenGL buffers
    glGenVertexArrays(1, &seaVAO);
    glBindVertexArray(seaVAO);

    glGenBuffers(1, &seaVBO);
    glBindBuffer(GL_ARRAY_BUFFER, seaVBO);
    glBufferData(GL_ARRAY_BUFFER, seaVertices.size() * sizeof(SeaVertex), seaVertices.data(), GL_DYNAMIC_DRAW);

    glGenBuffers(1, &seaEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, seaEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, seaIndices.size() * sizeof(GLuint), seaIndices.data(), GL_STATIC_DRAW);

    // Set up vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SeaVertex), (void*)offsetof(SeaVertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SeaVertex), (void*)offsetof(SeaVertex, texCoord));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(SeaVertex), (void*)offsetof(SeaVertex, normal));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

	void updateCliffSea(float deltaTime) {
    seaTime += deltaTime;

    for (size_t i = 0; i < seaVertices.size(); i++) {
        SeaVertex& vertex = seaVertices[i];

        // Calculate distance from cliff base for wave scaling
        float distFromCliff = abs(vertex.position.x - CLIFF_BASE_X);
        float waveScale = exp(-distFromCliff / 500.0f); // Waves diminish with distance

        // Calculate distance from center for lateral wave scaling
        float distFromCenter = abs(vertex.position.z);
        float lateralScale = exp(-distFromCenter / 1000.0f); // Waves diminish with lateral distance

        // Combine both scaling factors
        float combinedScale = waveScale * (0.7f + 0.3f * lateralScale);

        // Composite wave function
        float x = vertex.position.x;
        float z = vertex.position.z;
        float height = 0.0f;

        // Base wave formation parameters
        const float BASE_SEA_LEVEL = -0.15f;
        const float MAX_WAVE_HEIGHT = 0.1f;

        // Function to create a shaped wave peak
        auto createWavePeak = [](float phase, float peakWidth = 1.0f) {
            // Create sharper peaks using power and smoothstep
            float base = sin(phase);
            float shaped = base * base * base * base;  // Sharpen peaks
            return shaped * glm::smoothstep(0.0f, 1.0f, 0.5f + base * 0.5f);
        };

        // Create multiple wave groups moving at different speeds and directions
        // Wave Group 1 - Large primary waves
        {
            float waveLength = 80.0f;
            float speed = 0.4f;
            float direction = 0.8f;  // Angle relative to cliff
            float phase = (x * direction + z * (1.0f - direction)) / waveLength + seaTime * speed;
            height += createWavePeak(phase) * 0.08f;
        }

        // Wave Group 2 - Medium waves at different angle
        {
            float waveLength = 60.0f;
            float speed = 0.3f;
            float direction = 0.6f;
            float phase = (x * direction - z * (1.0f - direction)) / waveLength + seaTime * speed;
            height += createWavePeak(phase, 1.5f) * 0.06f;
        }

        // Wave Group 3 - Smaller, faster waves
        {
            float waveLength = 40.0f;
            float speed = 0.5f;
            float direction = 0.4f;
            float phase = (x * direction + z * (1.0f - direction)) / waveLength + seaTime * speed;
            height += createWavePeak(phase, 2.0f) * 0.04f;
        }

        // Add subtle surface variation
        height += sin(x * 0.05f + z * 0.05f + seaTime * 0.8f) * 0.01f;

        // Apply scaling and ensure waves stay within bounds
        height *= waveScale;
        height = glm::clamp(height, -MAX_WAVE_HEIGHT, MAX_WAVE_HEIGHT);
        vertex.position.y = BASE_SEA_LEVEL + height;

        // Calculate wave normal based on the final wave shape
        float heightScale = height / MAX_WAVE_HEIGHT; // Normalize height for normal calculation
        float dx = heightScale * waveScale * 0.4f;   // Scale normal based on wave height and distance
        float dz = heightScale * waveScale * 0.3f;
        vertex.normal = glm::normalize(glm::vec3(-dx, 1.0f, -dz));

        // Scale waves based on combined distance factor
        height *= combinedScale;

        // Clamp the height to prevent waves from going above cliff base
        height = glm::clamp(height, -MAX_WAVE_HEIGHT, MAX_WAVE_HEIGHT);
        vertex.position.y = BASE_SEA_LEVEL + height;

        // Calculate normal based on wave height gradients
        float Dx = cos(x * 0.01f + z * 0.01f + seaTime * 0.5f) * 0.015f * combinedScale;
        float Dz = cos(z * 0.02f + seaTime * 0.8f) * 0.02f * combinedScale;
        vertex.normal = glm::normalize(glm::vec3(-Dx, 1.0f, -Dz));
    }

    // Update buffer
    glBindBuffer(GL_ARRAY_BUFFER, seaVBO);
    glBufferData(GL_ARRAY_BUFFER, seaVertices.size() * sizeof(SeaVertex), seaVertices.data(), GL_DYNAMIC_DRAW);
}

	void renderCliffSea(glm::mat4 cameraMatrix, glm::mat4 lightSpaceMatrix) {
    glUseProgram(programID2);
    glBindVertexArray(seaVAO);

    glm::mat4 mvp = cameraMatrix * modelMatrix;
    glUniformMatrix4fv(mvpMatrixID, 1, GL_FALSE, &mvp[0][0]);
    glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
    glUniformMatrix3fv(normalMatrixID, 1, GL_FALSE, &normalMatrix[0][0]);
    glUniformMatrix4fv(LSM_ID, 1, GL_FALSE, &lightSpaceMatrix[0][0]);

    glUniform3fv(lightPositionID, 1, &lightPosition[0]);
    glUniform3fv(lightIntensityID, 1, &lightIntensity[0]);
    glUniform1i(shadowMapTextureID, 1);

    glDrawElements(GL_TRIANGLES, seaIndices.size(), GL_UNSIGNED_INT, (void*)0);

    glBindVertexArray(0);
}

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteTextures(1, &TextureID);
		glDeleteProgram(programID2);
	}
};

// Struct defining a building model that can be sclaed and positioned in the scene.
struct Building {
	glm::vec3 position;
	glm::vec3 scale;

	GLfloat vertex_buffer_data[72] = {
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
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,

		// Back face
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,

		// Left face
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,

		// Right face
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		// Top face
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		// Bottom face
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
	};


	GLuint index_buffer_data[36] = {
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
		// Top
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		// Bottom
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
	GLuint textureID, heliTextureID;


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

	GLuint programID2;
	GLuint depthShaderID;

	void initialize(glm::vec3 position, glm::vec3 scale, std::string textureLocation) {
		// Define scale of the building geometry
		this->position = position;
		this->scale = scale;

		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix = glm::scale(modelMatrix, scale);

		// Create a vertex array object
		glGenVertexArrays(1, &vertexArrayID);
		glBindVertexArray(vertexArrayID);

		// Create a vertex buffer object to store the vertex data
		glGenBuffers(1, &vertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_buffer_data), vertex_buffer_data, GL_STATIC_DRAW);

		// Create a vertex buffer object to store the UV data
		for (int i = 0; i < 24; ++i) {
			if(i<16) {
				uv_buffer_data[2*i+1] *= 5;
			}
		}
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

		textureID = LoadTextureTileBox(textureLocation.c_str()); // Load the selected texture, assuming LoadTextureTileBox accepts std::string
		heliTextureID = LoadTextureTileBox("../Final_Project/Textures/helicopter.png");

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
			24,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)0           // element array buffer offset
		);

		glBindTexture(GL_TEXTURE_2D, heliTextureID);
		glUniform1i(textureSamplerID, 0);

		// Draw the box
		glDrawElements(
			GL_TRIANGLES,      // mode
			12,    			   // number of indices
			GL_UNSIGNED_INT,   // type
			(void*)(24 * sizeof(GLuint))          // element array buffer offset
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

	void cleanup() {
		glDeleteBuffers(1, &vertexBufferID);
		glDeleteBuffers(1, &colorBufferID);
		glDeleteBuffers(1, &indexBufferID);
		glDeleteVertexArrays(1, &vertexArrayID);
		glDeleteBuffers(1, &uvBufferID);
		glDeleteTextures(1, &textureID);
		glDeleteProgram(programID2);
	}
};

// FBO object used in the shadow mapping process.
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

// glTF parser and animator similar to lab 4 used to take gltf files and render them in the scene.
struct MyBot {
	// Shader variable IDs
	GLuint mvpMatrixID;
	GLuint jointMatricesID;
	GLuint lightPositionID;
	GLuint lightIntensityID;
	GLuint programID;
	glm::vec3 position;
	glm::vec3 scale;

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

	void initialize(glm::vec3 position, glm::vec3 scale) {
		this->position = position;
		this->scale = scale;
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

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix = glm::scale(modelMatrix, scale);
		// Set camera
		glm::mat4 mvp = cameraMatrix * modelMatrix;
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
	setupMouseCallbacks(window);

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

	// Initialize the main object in the scene. Sea, cliff, plateau.
	world_setup myWorld;
	myWorld.intialize(glm::vec3(80, 0, -100), glm::vec3(200, 200, 200));

	// Define world attributes to be used in the scene. Footpaths roads etc.
	world_attributes myAttributes;
	myAttributes.initialize(glm::vec3(80, 0, -100), glm::vec3(200, 200, 200));

	// Add particle system for cloud effect.
	CloudSystem myCloudSystem;
	myCloudSystem.initialize();

	//Define mountain to used in the scene.
	Mountain myMountain;
	myMountain.initialize(glm::vec3(180, -2, -100), glm::vec3(375, 200, 300));

	// Our 3D character
	MyBot bot, bot2;
	bot.initialize(glm::vec3(200, 62.0f, 10), glm::vec3(0.25f));
	bot2.initialize(glm::vec3(200, 62.0f, -200), glm::vec3(0.25f));

	metro_stop myMetro, myMetro2;
	myMetro.initialize(glm::vec3(50, 50.0f, 80), glm::vec3(20, 20, 20), glm::radians(180.0f));
	myMetro2.initialize(glm::vec3(50.0f, 50.0f, -280.0f), glm::vec3(20, 20, 20), glm::radians(0.0f));

	sports_center myCenter, myCenter2;
	myCenter.initialize(glm::vec3(200, 50.0f, 10), glm::vec3(3*20, 20, 3*20));
	myCenter2.initialize(glm::vec3(200, 50.0f, -230), glm::vec3(3*20, 20, 3*20));

	// define building to be used in the scene.
	Building myBuilding,myBuilding2, myBuilding3, myBuilding4;
	myBuilding.initialize(glm::vec3(-80, 125, 60), glm::vec3(32, 3*32, 32), "../Final_Project/Textures/futuristic.jpg");
	myBuilding2.initialize(glm::vec3(-65, 95, -30), glm::vec3(45, 2*32, 35), "../Final_Project/Textures/House.jpg");
	myBuilding3.initialize(glm::vec3(-80, 110, -200), glm::vec3(25, 2.5*32, 30), "../Final_Project/Textures/Apartment.jpg");
	myBuilding4.initialize(glm::vec3(-80, 140, -260), glm::vec3(30, 3.5*32, 28), "../Final_Project/Textures/Apartment_texture.jpg");

	// Initialize the skybox to encapsulate the scene.
	skybox mySkybox({"../Final_Project/Textures/px.png", "../Final_Project/Textures/nx.png", "../Final_Project/Textures/py.png", "../Final_Project/Textures/ny.png","../Final_Project/Textures/pz.png", "../Final_Project/Textures/nz.png" });

	// initialize the fbo used in rendering of shadows.
	Lighting_Shadows renderLight;
	renderLight.initialize();

	float near_plane = 1.0f, far_plane = 50.0f;
	glm::mat4 lightProjection = glm::perspective(glm::radians(depthFoV),(float)windowWidth/windowHeight, depthNear, depthFar);
	glm::mat4 lightView = glm::lookAt (lightPosition,lightLookat, lightUp);
	// Prepare a perspective camera
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;



	// Time and frame rate tracking
	static double lastTime = glfwGetTime();
	float time = 0.0f;			// Animation time
	float fTime = 0.0f;			// Time for measuring fps
	unsigned long frames = 0;

	renderLight.shadowMapPass(lightSpaceMatrix);
    // ------------------------------------
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
			bot2.update(time);
		}

		// -------------------------------------------------------------------------------------
		// For convenience, we multiply the projection and view matrix together and pass a single matrix for rendering
		glm::mat4 viewMatrix = glm::lookAt(eye_center, lookat, up);
		glm::mat4 vp = projectionMatrix * viewMatrix;
		bot.render(vp);
		bot2.render(vp);
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
			myBuilding2.renderShadow(lightSpaceMatrix);
			myBuilding3.renderShadow(lightSpaceMatrix);
			myBuilding4.renderShadow(lightSpaceMatrix);
			myWorld.renderShadow(lightSpaceMatrix);
			myAttributes.renderShadow(lightSpaceMatrix);
			myCenter.renderShadow(lightSpaceMatrix);
			myMetro.renderShadow(lightSpaceMatrix);
			myMetro2.renderShadow(lightSpaceMatrix);
			myCenter2.renderShadow(lightSpaceMatrix);
			myMountain.renderShadow(lightSpaceMatrix);
			glBindFramebuffer(GL_FRAMEBUFFER, renderLight.depthTexture);
			glViewport(0,0,windowWidth, windowHeight);
			std::string filename = "../Final_Project/depth_camera.png";
			saveDepthTexture(renderLight.depthTexture, filename);
			std::cout << "Depth texture saved to " << filename << std::endl;
			saveDepth = false;
		}

		//------------------------------------------------------------------------------
		myCloudSystem.update(deltaTime);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, renderLight.depthTexture);
		myBuilding.renderWithLight(vp, lightSpaceMatrix);
		myBuilding2.renderWithLight(vp, lightSpaceMatrix);
		myBuilding3.renderWithLight(vp, lightSpaceMatrix);
		myBuilding4.renderWithLight(vp, lightSpaceMatrix);
		myWorld.renderWithLight(vp,lightSpaceMatrix);
		myAttributes.renderWithLight(vp,lightSpaceMatrix,lightIntensity,lightPosition);
		myCenter.renderWithLight(vp, lightSpaceMatrix);
		myCenter2.renderWithLight(vp, lightSpaceMatrix);
		myMetro.renderWithLight(vp,lightSpaceMatrix);
		myMetro2.renderWithLight(vp,lightSpaceMatrix);
		myMountain.renderWithLight(vp,lightSpaceMatrix);
		myCloudSystem.render(vp,eye_center, lookat, up);
		mySkybox.render(viewMatrix,projectionMatrix);
		//------------------------------------------------------------------------------
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Destroy all objects created
	myWorld.cleanup();
	myBuilding.cleanup();
	myBuilding2.cleanup();
	myBuilding3.cleanup();
	myBuilding4.cleanup();
	bot.cleanup();
	myMountain.cleanup();
	myCenter.cleanup();
	myCenter2.cleanup();
	myMetro.cleanup();
	myMetro2.cleanup();
	myAttributes.cleanup();

	glfwSetCursorPosCallback(window, nullptr);
	glfwSetMouseButtonCallback(window, nullptr);
	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	glm::vec3 direction = glm::normalize(lookat - eye_center);
	const float MIN_DISTANCE = 50.0f;
	const float MOVE_SPEED = 20.0f;

	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		// Calculate new potential eye position
		glm::vec3 newEyePos = eye_center + direction * MOVE_SPEED;
		float newDistance = glm::length(lookat - newEyePos);

		if (newDistance < MIN_DISTANCE) {
			// Move lookat forward along with the camera
			lookat += direction * (MIN_DISTANCE-newDistance);
		}
		eye_center = newEyePos;
		std::cout << "(" << eye_center.x <<"," << eye_center.y << "," << eye_center.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
		glm::vec3 newEyePos = eye_center - direction * MOVE_SPEED;
		float newDistance = glm::length(lookat - newEyePos);
		if (newDistance < MIN_DISTANCE) {
			lookat -= direction * (MIN_DISTANCE-newDistance);
		}
		eye_center = newEyePos;
		std::cout << "(" << eye_center.x <<"," << eye_center.y << "," << eye_center.z << ")" << std::endl;
	}
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		std::cout << "Reset." << std::endl;
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

	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		glm::vec3 potentialMove = direction * 20.0f;
		float newY = eye_center.y + potentialMove.y;
		float newX = eye_center.x + potentialMove.x;
		float newZ = eye_center.z + potentialMove.z;

		// adjust y bounds
		if (newY >= 40.0f && newY <= 210.0f && newX >= -100.0f && newX <= 260.0f) {
			eye_center += potentialMove;
		} else {
			if (newY < 40.0f) {
				eye_center.y = 40.0f; // Clamp Y to the minimum value
			}
			else if (newY > 210.0f) {
				eye_center.y = 210.0f; // Clamp Y to the minimum value
			}
		}
		// adjust x bounds
		if (newX < -100.0f) {
			eye_center.x = -100.0f;
		} else if(newX > 260.0f) {
			eye_center.x = 260.0f;
		}
		// adjust z bounds
		if (newZ < -270.0f) {
			eye_center.z = -270.0f;
		} else if(newZ > 80.0f) {
			eye_center.z = 80.f;
		}
		std::cout << "(" << eye_center.x <<"," << eye_center.y << "," << eye_center.z << ")" << std::endl;
	}

	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		glm::vec3 potentialMove = direction * 20.0f;
		float newY = eye_center.y - potentialMove.y;
		float newX = eye_center.x - potentialMove.x;
		float newZ = eye_center.z - potentialMove.z;

		if (newY >= 40.0f && newY <= 210.0f && newX >= -100.0f && newX <= 260.0f) {
			eye_center -= potentialMove;
		} else {
			if (newY < 40.0f) {
				eye_center.y = 40.0f; // Clamp Y to the minimum value
			}  else if (newY > 210.0f) {
				eye_center.y = 210.0f; // Clamp Y to the minimum value
			}
		}
		// adjust x bounds
		if (newX < -100.0f) {
			eye_center.x = -100.0f;
		} else if(newX > 260.0f) {
			eye_center.x = 260.0f;
		}

		// adjust z bounds
		if (newZ < -270.0f) {
			eye_center.z = -270.0f;
		} else if(newZ > 80.0f) {
			eye_center.z = 80.f;
		}
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

static void cursor_callback(GLFWwindow *window, double xpos, double ypos) {
	if (mousePressed) {
		// Calculate cursor movement
		float xoffset = static_cast<float>(xpos - lastX);
		float yoffset = static_cast<float>(lastY - ypos);

		// Apply sensitivity
		xoffset *= -mouseSensitivity;
		yoffset *= -mouseSensitivity;

		float currentDistance = glm::length(lookat-eye_center);
		const float MIN_DISTANCE = 50.0f;

		//update lookat position
		glm::vec3 direction = glm::normalize(lookat-eye_center);
		glm::vec3 right = glm::normalize(glm::cross(direction,up));

		glm::vec3 newLookat = lookat + right * xoffset + up * yoffset;
		float newDistance = glm::length(newLookat-eye_center);

		if (newDistance < MIN_DISTANCE) {
			glm::vec3 forward = direction * (MIN_DISTANCE - newDistance);
			newLookat += forward;
			lookat = newLookat;
		}
		else {
			lookat = newLookat;
		}
		lastX = xpos;
		lastY = ypos;

	}
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	if(button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			mousePressed = true;
			glfwGetCursorPos(window, &lastX, &lastY);
		}
		else if(action == GLFW_RELEASE) {
			mousePressed = false;
		}
	}
}

void setupMouseCallbacks(GLFWwindow* window) {
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_callback);
}
