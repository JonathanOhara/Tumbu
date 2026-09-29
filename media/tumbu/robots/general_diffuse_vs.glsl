#version 120
// Robot lighting, per-light pass (GLSL port of general.cg diffuse_vs).
uniform mat4 wMat;
uniform mat4 wvpMat;
uniform vec4 spotlightDir;
attribute vec4 tangent;

varying vec2 uv;
varying vec4 wp;
varying vec3 n;
varying vec3 t;
varying vec3 b;
varying vec3 sdir;

void main()
{
    wp = wMat * gl_Vertex;
    gl_Position = wvpMat * gl_Vertex;
    uv = gl_MultiTexCoord0.xy;
    n = gl_Normal;
    t = tangent.xyz;
    b = cross(tangent.xyz, gl_Normal) * tangent.w;
    sdir = (wMat * spotlightDir).xyz; // spotlight direction in world space
}
