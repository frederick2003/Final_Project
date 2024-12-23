#include <string>

static std::string textureVertexShader = R"(
    #version 330 core

    // Input
    layout(location = 0) in vec3 vertexPosition;
    layout(location = 1) in vec3 vertexColor;
    layout(location = 2) in vec2 vertexUV;

    // Output data, to be interpolated for each fragment
    out vec3 color;
    out vec2 uv;


    // Matrix for vertex transformation
    uniform mat4 MVP;

    void main() {
        // Transform vertex
        gl_Position =  MVP * vec4(vertexPosition, 1);

        // Pass vertex color to the fragment shader
        color = vertexColor;
        uv=vertexUV;
}
)";

static std::string textureFragmentShader = R"(
    #version 330 core

    in vec3 color;
    in vec2 uv;

    uniform sampler2D textureSampler;

    out vec3 finalColor;

    void main()
    {
	    finalColor = texture(textureSampler,uv).rgb;
    }
)";
