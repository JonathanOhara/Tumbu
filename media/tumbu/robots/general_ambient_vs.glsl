#version 120
// Robot lighting, ambient pass (GLSL port of general.cg ambient_vs).
uniform mat4 wvpMat;
varying vec2 uv;

void main()
{
    gl_Position = wvpMat * gl_Vertex;
    uv = gl_MultiTexCoord0.xy;
}
