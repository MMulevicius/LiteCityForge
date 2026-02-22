#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV; 

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

    // if mesh has no UV, it’ll be 0s (fine)
    vUV = aUV; 
    vLightSpacePos = uLightSpace * world;

    gl_Position = uVP * world;
}
