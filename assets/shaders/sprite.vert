#version 430 core

#define UBO_TEMPLATE	0
#define UBO_INSTANCE	1
#define UBO_PROJECTION	2

#define MAX_VERTICES	6
#define MAX_TEMPLATES 	16
#define MAX_INSTANCES 	256

precision highp float;

layout(std140, column_major) uniform;

uniform Projection
{
	mat4 Ortho;
};

uniform Template
{
	vec4	Vertices[MAX_VERTICES];
} Templates[MAX_TEMPLATES];

uniform Instance
{
	vec2 	Position;
	vec2	Scale;
	vec2	Rotation;
	uint	Template;
	float	Padding;
} Instances[MAX_INSTANCES];

out vec2 TexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	//uint TemplateId = Instances[gl_InstanceID].Template;
	//vec4 Vertex = Templates[TemplateId].Vertices[gl_VertexID % MAX_VERTICES];
	
	//TexCoords = Vertex.zw;
	//gl_Position = Projection * vec4(Vertex.xy, 0.0, 1.0);
}
