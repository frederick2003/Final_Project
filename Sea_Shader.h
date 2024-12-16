#include <string>

static std::string Sea_Vertex_Shader = R"(
#version 330 core

uniform mat4 gvp;

const vec3 Pos[4] = vec3[4](
    vec3(-100.0f, 0.0f, -100.0f),
    vec3(100.0f, 0.0f, -100.0f),
    vec3(100.0f, 0.0f, 100.0f),
    vec3(-100.0f, 0.0f, 100.0f)
);

const int Indices[6] = int[6](0, 2, 1, 2, 0, 3);

void main() {
    int index = Indices[int(gl_VertexID)]; // Explicit cast to int
    vec4 vPos = vec4(Pos[index], 1.0);     // Use `index`, not `Index`
    gl_Position = gvp * vPos;             // Transform the vertex position
}
)";

static std::string Sea_Fragment_Shader = R"(
#version 330 core

layout(location = 0) out vec4 FragColour;
void main() {
    FragColour = vec4(0.0);
}
)";