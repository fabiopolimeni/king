#version 430 core

in vec2 TexCoords;
in vec4	VertColor;

out vec4 	FragColor;

uniform sampler2D Image;

void main()
{
	vec4 TexColor = texture(Image, TexCoords);
    FragColor = vec4(TexColor.xyz, TexColor.x) *  VertColor;
}