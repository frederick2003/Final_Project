#ifndef ANIMATIONSHADERS_H
#define ANIMATIONSHADERS_H
#include <string>

static std::string animationVertexShader = R"(
#version 330 core

// Input
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;
layout(location = 3) in vec4 joints;    // Joint indices
layout(location = 4) in vec4 weights;   // Joint weights


// Output data, to be interpolated for each fragment
out vec3 worldPosition;
out vec3 worldNormal;
out vec2 worldUV;

uniform mat4 MVP;
const int MAX_JOINTS = 128;
uniform mat4 jointMatrices[MAX_JOINTS];  // Array of joint matrices

void main() {
    // Initialize transformed position and normal
    vec4 skinnedPosition = vec4(0.0);
    vec3 skinnedNormal = vec3(0.0);

    // Apply skinning transformation
    for (int i = 0; i < 4; i++) {  // Assuming max 4 joints per vertex
                                   int jointIndex = int(joints[i]);
                                   float weight = weights[i];

                                   // Skip if weight is zero
                                   if (weight > 0.0) {
                                       // Transform position and normal by joint matrix
                                       skinnedPosition += jointMatrices[jointIndex] * vec4(vertexPosition, 1.0) * weight;

                                       // Transform normal by joint matrix (excluding translation)
                                       //mat3 normalMatrix = transpose(inverse(mat3(jointMatrices[jointIndex])));
                                       mat3 jointRotation = mat3(jointMatrices[jointIndex]);
                                       skinnedNormal += jointRotation * vertexNormal * weight;
                                   }
    }

    // Transform vertex
    gl_Position = MVP * skinnedPosition;

    // World-space geometry
    worldPosition = skinnedPosition.xyz;
    worldNormal = normalize(skinnedNormal);
    worldUV = vertexUV;
}


)";

static std::string animationFragmentShader = R"(
#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;

out vec3 finalColor;

uniform vec3 lightPosition;
uniform vec3 lightIntensity;

void main()
{
	// Lighting
	vec3 lightDir = lightPosition - worldPosition;
	float lightDist = dot(lightDir, lightDir);
	lightDir = normalize(lightDir);
	vec3 v = lightIntensity * clamp(dot(lightDir, worldNormal), 0.0, 1.0) / lightDist;

	// Tone mapping
	v = v / (1.0 + v);

	// Gamma correction
	finalColor = pow(v, vec3(1.0 / 2.2));
}

)";

#endif //ANIMATIONSHADERS_H
