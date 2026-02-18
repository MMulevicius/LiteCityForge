#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV; // will be unused for non-UV meshes (OK if VAO doesn't enable it)

uniform mat4 uVP;
uniform mat4 uModel;
uniform mat4 uLightSpace;

out vec2 vUV;
out vec3 vWorldPos;
out vec4 vLightSpacePos;

void main()
{
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;

    vUV = aUV; // if mesh has no UV, it’ll be 0s (fine)
    vLightSpacePos = uLightSpace * world;

    gl_Position = uVP * world;
}
