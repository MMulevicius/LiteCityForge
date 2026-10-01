#version 330 core
out vec4 FragColor;

uniform vec3 uColor;

void main()
{   
    //dark road color
    FragColor = vec4(uColor, 1.0); 
}