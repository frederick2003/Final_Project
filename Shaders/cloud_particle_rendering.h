#include <string>

static std::string cloudVertexShader = R"(
#version 330 core

layout(location = 0) in vec3 vertexPosition;

uniform mat4 MVP;
uniform vec3 cameraRight;
uniform vec3 cameraUp;
uniform float particleAlpha;

out vec2 UV;
out float Alpha;

void main(){
    vec3 particleCenter = vec3(0,0,0);
    vec3 vertexPos = particleCenter + cameraRight * vertexPosition.x
    + cameraUp * vertexPosition.y;

    gl_Position = MVP * vec4(vertexPos, 1.0);
    UV = vertexPosition.xy + vec2(0.5, 0.5);
    Alpha = particleAlpha;
}
)";

static std::string cloudFragmentShader = R"(
#version 330 core

in vec2 UV;
in float Alpha;
out vec4 color;

uniform sampler2D textureSampler;
void main(){
    vec4 texColor = texture(textureSampler, UV);
    color = vec4(texColor.rgb, texColor.a * Alpha);

    float distFromCenter = length(UV - vec2(0.5, 0.5));
    color.a *= smoothstep(0.5, 0.3, distFromCenter);

    vec3 cloudColor = vec3(0.9, 0.9, 0.9);
    float depth = smoothstep(0.5, 0.0, distFromCenter);
    color.rgb = mix(color.rgb, cloudColor, depth * 0.6);
}
)";