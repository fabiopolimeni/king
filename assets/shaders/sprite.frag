#version 430 core

in vec2 	TexCoords;
out vec4 	FragColor;

uniform sampler2D Image;

void main()
{    
    FragColor = vec4(TexCoords.x,TexCoords.y,0.0,1.0);// texture(Image, TexCoords);
} 