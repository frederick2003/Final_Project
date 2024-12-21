#include <string>

static std::string depthVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 lightSpaceModel;

void main()
{
    gl_Position = lightSpaceModel * model * vec4(aPos, 1.0);
}
)";

static std::string depthFragmentShader = R"(
#version 330 core

void main()
{
    // gl_FragDepth = gl_FragCoord.z;
}
)";