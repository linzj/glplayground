#version 120
uniform sampler2D u_texture;
uniform ivec2 u_screenGeometry;
uniform float u_kernel[91];
const int c_blockSize = 91;

void main(void)
{
    int i;
    vec2 texcoord = gl_FragCoord.xy / vec2(u_screenGeometry);
    float toffset = 1.0 / u_screenGeometry.y;
    vec4 color = texture2D(u_texture, texcoord) * u_kernel[0];
    for (i = 1; i < c_blockSize; ++i) {
        color += texture2D(u_texture, texcoord - vec2(0.0, i * toffset)) * u_kernel[i];
    }
    gl_FragColor = color;
}
