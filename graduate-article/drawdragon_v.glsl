#version 150
in vec3 position;
in vec3 normal;

uniform mat4 mvp;
uniform mat3 normal_matrix;
smooth out vec3 datas;
void main() {
    gl_Position = mvp * vec4(position, 1);
    datas.xy = position.xy;
    datas.z = abs(normalize(normal_matrix * normal).z);
}
