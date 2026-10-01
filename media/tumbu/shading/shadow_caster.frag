OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Depth-only shadow caster: the shadow map keeps only the depth, so the colour does not matter.
#include <OgreUnifiedShader.h>

MAIN_PARAMETERS
MAIN_DECLARATION
{
    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
