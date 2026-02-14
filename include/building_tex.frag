#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uUseTexture; 
uniform vec3 uColor;

void main()
{
    if (uUseTexture != 0)
        FragColor = texture(uTexture, vUV);
    else
        FragColor = vec4(uColor, 1.0);
}