#version 430 core

#define UBO_PROJECTION	0
#define UBO_TEMPLATE	1
#define UBO_INSTANCE	2
#define UBO_TRANSFORM	3

#define MAX_VERTICES	6
#define MAX_TEMPLATES 	16
#define MAX_INSTANCES 	256

precision highp float;

layout(std140, row_major) uniform;
layout(std430, column_major) buffer;

layout(binding = UBO_PROJECTION) uniform Matrix
{
	mat4 Ortho;
}Projection;

struct Vertex
{
	vec4 XYUV[MAX_VERTICES];
};

layout(binding = UBO_TEMPLATE) uniform Template
{
	Vertex	Vertices[MAX_TEMPLATES];
} Templates;

struct Sprite
{
	uint	TemplateID;
	uint	TransformID;
};

layout(binding = UBO_INSTANCE) uniform Instance
{
	Sprite Sprites[MAX_INSTANCES];
} Instances;

layout(binding = UBO_TRANSFORM) uniform Transform
{
	mat4 Models[MAX_INSTANCES];
} Transforms;

out vec2 TexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	Sprite sprite = Instances.Sprites[gl_InstanceID];
	mat4 model = Transforms.Models[sprite.TransformID];
	vec4 vertex = Templates.Vertices[sprite.TemplateID].XYUV[gl_VertexID % MAX_VERTICES];
	
	TexCoords = vertex.zw;
	gl_Position = Projection.Ortho * model * vec4(vertex.xy, 0.0, 1.0);
}
