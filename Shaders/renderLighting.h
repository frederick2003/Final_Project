#include <string>

static std::string lightingVertexShader = R"(
#version 330 core

// Input
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;

// Output data, to be interpolated for each fragment
out vec2 uv;
out vec3 worldPosition;
out vec3 worldNormal;
out vec4 FragPositionLightSpace;

uniform mat4 MVP;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceMatrix;

void main() {
    // Transform vertex position to clip space
    gl_Position = MVP * vec4(vertexPosition, 1.0);

    // Pass UV coordinates to the fragment shader
    uv = vertexUV;

    // Transform position to world space
    worldPosition = vec3(modelMatrix * vec4(vertexPosition, 1.0));

    // Transform normal to world space using the normal matrix
    worldNormal = normalize(normalMatrix * vertexNormal);

    FragPositionLightSpace = lightSpaceMatrix * vec4(worldPosition, 1.0);

}
)";


static std::string lightingFragmentShader = R"(
#version 330 core

// Input from the vertex shader
in vec3 worldNormal;
in vec3 worldPosition;
in vec2 uv;
in vec4 FragPositionLightSpace;


// Output color
out vec3 finalColor;

// Uniforms for light and material properties
uniform vec3 lightPosition;  // Light source position
uniform vec3 lightIntensity; // Light source intensity
uniform sampler2D textureSampler;
uniform sampler2D shadowMap;


const vec3 materialAmbient = vec3(0.2, 0.2, 0.2); // Ambient reflectivity

float ShadowCalculation(vec4 fragPosLightSpace, vec3 N, vec3 L)
{
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	vec2 uv = projCoords.xy * 0.5 + 0.5;
	uv = clamp(uv,0.0,1.0);
	//float closestDepth = texture(shadowMap, projCoords.xy).r;
	float closestDepth = texture(shadowMap, uv).r;
	// get depth of current fragment from light's perspective


	float currentDepth = projCoords.z;
	// check whether current frag pos is in shadow
	float bias = max(0.005 * (1.0 - dot(N, L)), 0.001); // Use dynamic bias

	return (currentDepth >= closestDepth + bias)  ? 0.6 : 1.0;
}

void main() {
    // Normalize input vectors
    vec3 N = normalize(worldNormal);                          // Normalized normal
    vec3 L = normalize(lightPosition - worldPosition);        // Light direction
    vec3 V = normalize(-worldPosition);                       // View direction


    // Distance between light and surface point
    float distance = distance(lightPosition, worldPosition);

    vec3 I = vec3(lightIntensity)/ vec3(4 * (3.1415926) * pow((distance),2.0f));

    vec3 basicCol = vec3(dot(N,L)) * texture(textureSampler, uv).rgb * I;

	float shadow = ShadowCalculation(FragPositionLightSpace,N,L);

    vec3 tonedColor = vec3(basicCol/ (1 + basicCol));

    float gamma = 1.0/2.2f;
	vec3 gammaCol = pow(vec3(tonedColor), vec3(gamma));

	vec3 exposure = vec3(1.3f);
	vec3 exposedCol = gammaCol * exposure;

	vec3 fragCol = exposedCol * shadow;

    finalColor = fragCol;
}
)";