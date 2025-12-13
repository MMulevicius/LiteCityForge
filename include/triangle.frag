#version 330 core

in vec3 ourColor;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform sampler2D uTexture2;

void main()
{
    // plain textured quad
    FragColor = mix(texture(uTexture, TexCoord), texture(uTexture2, TexCoord), 0.2);

    // If you want to mix with vertex color instead:
    // FragColor = texture(uTexture, TexCoord) * vec4(ourColor, 1.0);
}
