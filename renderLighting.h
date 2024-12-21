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

uniform mat4 MVP;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;

void main() {
    // Transform vertex position to clip space
    gl_Position = MVP * vec4(vertexPosition, 1.0);

    // Pass UV coordinates to the fragment shader
    uv = vertexUV;

    // Transform position to world space
    worldPosition = vec3(modelMatrix * vec4(vertexPosition, 1.0));

    // Transform normal to world space using the normal matrix
    worldNormal = normalize(normalMatrix * vertexNormal);
}
)";

static std::string lightingFragmentShader = R"(
#version 330 core

// Input from the vertex shader
in vec3 worldNormal;
in vec3 worldPosition;
in vec2 uv;

// Output color
out vec4 finalColor;

// Uniforms for light and material properties
uniform vec3 lightPosition;  // Light source position
uniform vec3 lightIntensity; // Light source intensity
uniform sampler2D textureSampler; // Diffuse texture#

const vec3 materialAmbient = vec3(0.2, 0.2, 0.2); // Ambient reflectivity

void main() {
    // Normalize input vectors
    vec3 N = normalize(worldNormal);                          // Normalized normal
    vec3 L = normalize(lightPosition - worldPosition);        // Light direction
    vec3 V = normalize(-worldPosition);                       // View direction


    // Distance between light and surface point
    float distance = distance(lightPosition, worldPosition);

    vec3 I = vec3(lightIntensity)/ vec3(4 * (3.1415926) * pow((distance),2.0f));

    vec3 basicCol = vec3(dot(N,L)) * texture(textureSampler, uv).rgb * I;

    vec3 tonedColor = vec3(basicCol/ (1 + basicCol));

    float gamma = 1.0/2.2f;
	vec3 gammaCol = pow(vec3(tonedColor), vec3(gamma));


	vec3 exposure = vec3(1.3f);
	vec3 exposedCol = gammaCol * exposure;

    finalColor = vec4(exposedCol,1.0f);
}
)";
