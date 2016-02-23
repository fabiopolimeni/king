#version 430 core

in vec2 TexCoords;
in vec4	VertColor;

out vec4 	FragColor;

uniform sampler2D Image;

void main()
{    
    FragColor = texture(Image, TexCoords) *  VertColor;
} 