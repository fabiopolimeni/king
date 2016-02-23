#version 430 core

#define UBO_PROJECTION	0
#define UBO_TEMPLATE	1
#define UBO_INSTANCE	2

#define MAX_VERTICES	6
#define MAX_TEMPLATES 	16
#define MAX_INSTANCES 	256

precision highp float;

layout(std140, column_major) uniform;
layout(std430, column_major) buffer;

layout(binding = UBO_PROJECTION) uniform Screen
{
	float InvWidth;
	float InvHeight;
}Frame;

struct Vertex
{
	vec4 XYUV[MAX_VERTICES];
};

layout(binding = UBO_TEMPLATE) uniform Template
{
	Vertex	Vertices[MAX_TEMPLATES];
} Templates;

struct Transform
{
	vec2 	Position;
	vec2	Scale;
	vec2	Rotation;
	uint	Layout;
	float	Padding;
};

layout(binding = UBO_INSTANCE) uniform Instance
{
	Transform Transforms[MAX_INSTANCES];
} Instances;

out vec2 TexCoords;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	Transform Sprite = Instances.Transforms[gl_InstanceID];
	vec4 Vertex = Templates.Vertices[Sprite.Layout].XYUV[gl_VertexID % MAX_VERTICES];
	vec2 VertPos = (Vertex.xy */* vec2(Frame.InvWidth, Frame.InvHeight) */ 2.0) - vec2(1.0);
	
	// Apply scale, rotation and traslation
	// We rotate around the centre, hence the translations
	VertPos += Sprite.Position;
	
	VertPos += vec2(0.5) * Sprite.Scale;
	VertPos = vec2(
		VertPos.x * Sprite.Rotation.y - VertPos.y * Sprite.Rotation.x,
		VertPos.x * Sprite.Rotation.x + VertPos.y * Sprite.Rotation.y
	);
	VertPos -= vec2(0.5) * Sprite.Scale;
	
	VertPos *= Sprite.Scale;
	
	TexCoords = Vertex.zw;
	
	// Debug
	//if (gl_InstanceID == 0) {
	//		 if (gl_VertexID == 0)  VertPos = vec2(0.0f, 1.0f);
	//	else if (gl_VertexID == 1)  VertPos = vec2(1.0f, 0.0f);
	//	else if (gl_VertexID == 2)  VertPos = vec2(0.0f, 0.0f);
	//	else if (gl_VertexID == 3)  VertPos = vec2(0.0f, 1.0f);
	//	else if (gl_VertexID == 4)  VertPos = vec2(1.0f, 1.0f);
	//	else if (gl_VertexID == 5)  VertPos = vec2(1.0f, 0.0f);
	//}

	gl_Position = vec4(VertPos, 0.0, 1.0);
}
