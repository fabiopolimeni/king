#version 430 core

#define TEMPLATE	0
#define INSTANCE	1
#define PROJECTION	2

#define MAX_VERTICES	6
#define MAX_TEMPLATES 	16
#define MAX_INSTANCES 	256

precision highp float;

layout(std140, column_major) uniform;

layout(binding = PROJECTION) uniform projection
{
	mat4	Ortho;
} Projection;

layout(binding = INSTANCE) uniform instance
{
	vec2 	Position;
	vec2	Scale;
	float	Rotation;
	uint	Template;
	vec2	Padding;
} Instances[MAX_INSTANCES];

layout(binding = TEMPLATE) uniform template
{
	vec4 	Vertices[MAX_VERTICES];
} Templates[MAX_TEMPLATES];

out vec2 TexCoords;

void main()
{
	uint TemplateId = Instances[gl_InstanceId].Template;
	vec4 Vertex = Templates[TemplateId].Vertices[gl_VertexId % MAX_VERTICES];
	TexCoords = Vertex.zw;
	gl_Position = Projection.Ortho * vec4(Vertex.xy, 0.0, 1.0);
}
