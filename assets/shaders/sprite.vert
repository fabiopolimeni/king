#version 430 core

#define UBO_PROJECTION	0
#define UBO_TEMPLATE	1
#define UBO_INSTANCE	2
#define UBO_DATA		3

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
	uint	DataID;
};

layout(binding = UBO_INSTANCE) uniform Instance
{
	Sprite Sprites[MAX_INSTANCES];
} Instances;

struct Model
{
	mat4 Transform;
	vec4 Color;
};

layout(binding = UBO_DATA) uniform Datum
{
	Model Models[MAX_INSTANCES];
} Data;

out vec2 TexCoords;
out vec4 VertColor;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	Sprite sprite = Instances.Sprites[gl_InstanceID];
	Model model = Data.Models[sprite.DataID];
	vec4 vertex = Templates.Vertices[sprite.TemplateID].XYUV[gl_VertexID % MAX_VERTICES];
	
	// To fragment shader
	VertColor = model.Color;
	TexCoords = vertex.zw;
	
	gl_Position = Projection.Ortho * model.Transform * vec4(vertex.xy, 0.0, 1.0);
}
