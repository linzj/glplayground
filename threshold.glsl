#version 120

uniform float u_maxValue;
uniform ivec2 u_screenGeometry;
uniform sampler2D u_textureOrig;
uniform sampler2D u_textureBlur;

void main(void)
{
    vec2 texcoord = gl_FragCoord.xy / vec2(u_screenGeometry);
    vec3 colorOrig = texture2D(u_textureOrig, texcoord).rgb;
    vec3 colorBlur = texture2D(u_textureBlur, texcoord).rgb;
    vec3 result;
    result.r = colorOrig.r > colorBlur.r ? u_maxValue : 0.0;
    result.g = colorOrig.g > colorBlur.g ? u_maxValue : 0.0;
    result.b = colorOrig.b > colorBlur.b ? u_maxValue : 0.0;
    gl_FragColor = vec4(result, 1.0);
}
