#version 150
smooth in vec3 datas;
out vec4 color;
#define COLOR_FREQ 10.0
#define ALPHA_FREQ 30.0
#define Alpha 0.6
void main() {
#if 1
    float xWorldPos = datas.x;
    float yWorldPos = datas.y;
    float diffuse = datas.z;

    float i = floor(xWorldPos * COLOR_FREQ);
    // float j = floor(yWorldPos * ALPHA_FREQ);
    color.rgb = (mod(i, 2.0) == 0) ? vec3(.4, .85, .0) : vec3(1.0);
    //color.a = (fmod(j, 2.0) == 0) ? Alpha : 0.2;
    color.a = 1;

    color.rgb *= diffuse;
#else
    color = vec4(1, 1, 1, 1);
#endif
}
