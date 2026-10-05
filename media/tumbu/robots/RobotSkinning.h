// Hardware skinning of the robot parts, shared by robot_toon.vert, robot_outline.vert, robot_aura.vert and
// robot_shadow_caster.vert. The programs declare includes_skeletal_animation (robots.program), so Ogre leaves the mesh
// on the GPU and only sends the bone matrices each frame (world_matrix_array_3x4, three rows per 3x4 matrix), instead of
// skinning every animating part on the CPU and uploading it again (slow on Direct3D 11, whose shadow buffers are
// staging resources; CLAUDE.md "Rendering pitfalls").
//
// The including shader puts ROBOT_SKIN_INPUTS among its vertex inputs. The bone array is a plain global uniform
// (OGRE_UNIFORMS only matters for Vulkan, which the game does not use).

// Ogre 14 sends the bones in object space (MeshManager::getBonesUseObjectSpace, on by default): boneMatrices holds one
// matrix per bone the mesh uses (blend index i = matrix i), and the entity's world matrix comes apart in robotWorld
// (world_matrix). Room for 64 bones x 3 rows; the robot skeletons have 50.
uniform vec4 boneMatrices[192];
uniform mat4 robotWorld;

// Ogre stores the bone indices as four unsigned bytes: Direct3D 11 reads them as integers (R8G8B8A8_UINT), OpenGL
// as floats.
#if defined(OGRE_HLSL)
#define ROBOT_SKIN_INPUTS IN(uvec4 blendIndices, BLENDINDICES) IN(vec4 blendWeights, BLENDWEIGHT)
#else
#define ROBOT_SKIN_INPUTS IN(vec4 blendIndices, BLENDINDICES) IN(vec4 blendWeights, BLENDWEIGHT)
#endif

// The blended 3x4 object-space matrix of a vertex (rows row0..row2), from up to four bones.
// Ogre keeps the four largest weights of a vertex and makes them sum to 1, but stores only as many as the mesh needs
// (1 to 4 floats): a component the mesh does not have reads as 0, except the last one, which reads as 1. So when the
// four values add up to more than 1.5, the fourth is not a real weight.
void robotSkinMatrix(vec4 indices, vec4 weights, out vec4 row0, out vec4 row1, out vec4 row2)
{
    if (weights.x + weights.y + weights.z + weights.w > 1.5)
        weights.w = 0.0;
    row0 = vec4_splat(0.0);
    row1 = vec4_splat(0.0);
    row2 = vec4_splat(0.0);
    for (int k = 0; k < 4; k++)
    {
        int bone = int(indices[k]) * 3;
        row0 += boneMatrices[bone] * weights[k];
        row1 += boneMatrices[bone + 1] * weights[k];
        row2 += boneMatrices[bone + 2] * weights[k];
    }
}

// World position of a vertex: skinned in object space, then the entity's world matrix.
vec3 robotSkinPoint(vec4 row0, vec4 row1, vec4 row2, vec4 p)
{
    vec4 skinned = vec4(dot(row0, p), dot(row1, p), dot(row2, p), 1.0);
    return mul(robotWorld, skinned).xyz;
}

// Directions (normal, tangent): robot parts are scaled uniformly, so the same matrix turns them.
vec3 robotSkinDirection(vec4 row0, vec4 row1, vec4 row2, vec3 d)
{
    vec3 skinned = vec3(dot(row0.xyz, d), dot(row1.xyz, d), dot(row2.xyz, d));
    return mul(robotWorld, vec4(skinned, 0.0)).xyz;
}
