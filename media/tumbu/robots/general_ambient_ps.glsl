#version 120
// Robot lighting, ambient pass (GLSL port of general.cg ambient_ps).
uniform vec3 ambient;
uniform vec4 matDif;
uniform sampler2D dMap;
uniform sampler2D aoMap;
varying vec2 uv;

void main()
{
    gl_FragColor = texture2D(dMap, uv) * texture2D(aoMap, uv) * vec4(ambient, 1.0) * vec4(matDif.rgb, 1.0);
}
