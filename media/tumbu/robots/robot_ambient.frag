// Robot lighting, ambient pass: diffuse texture * ambient-occlusion texture * ambient light * material colour.
#include <OgreUnifiedShader.h>

SAMPLER2D(dMap, 0);
SAMPLER2D(aoMap, 1);

OGRE_UNIFORMS(
    uniform vec3 ambient;
    uniform vec4 matDif;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    gl_FragColor = texture2D(dMap, oUv) * texture2D(aoMap, oUv) * vec4(ambient, 1.0) * vec4(matDif.rgb, 1.0);
}
