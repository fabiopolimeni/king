#pragma once

#include "GraphicsPipeline.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <queue>

class SpriteBatch
{

public:

	const static size_t	INDEX_NONE = -1;
	const static size_t	MAX_VERTICES = 6;
	const static size_t	MAX_TEMPLATES = 16;
	const static size_t	MAX_INSTANCES = 256;

	enum Uniform
	{
		eUBO_PROJECTION,
		eUBO_TEMPLATE,
		eUBO_INSTANCE,
		eUBO_MAX
	};

	struct Template
	{
		const glm::vec4	mVBO[MAX_VERTICES];
		const size_t	mTemplateId;
		
		static Template INVALID;
		static const size_t VBO_SIZE = sizeof(glm::vec4) * MAX_VERTICES;

		bool isValid() const {
			return mTemplateId != INDEX_NONE;
		}

		Template(const glm::vec4* vbo, const size_t id)
			: mTemplateId(id)
		{
			if (vbo) {
				memcpy(const_cast<glm::vec4*>(mVBO), vbo, VBO_SIZE);
			}
		}
	};

	// This doesn't store the transform
	// by design, as we don't want the
	// user to modify it directly.
	struct Instance
	{
		const Template&	mTemplate;
		const size_t	mInstanceId;

		static Instance INVALID;

		bool isValid() const {
			return mTemplate.isValid() && mInstanceId != INDEX_NONE;
		}
	};

	bool init(glm::vec2 win_dims, uint32_t texture_id,
		const char* vs_source, const char* fs_source,
		size_t max_templates, size_t max_sprites);

	void release();

	// Generates the VBO containing vertex positions and texture coordinates
	// @param atlas_offsets defined as x=left, y=top, z=right, w=bottom
	const Template& createTemplate(glm::vec4 atlas_offsets);

	// Add an instance of template to the sprite's batch
	// @return The ref index of the instance, SpriteKey.mTemplate == null otherwise
	const Instance addInstance(const Template& template_ref);

	// Update instance transform
	bool updateInstance(const Instance& sprite_ref,
		glm::vec2 position = glm::vec2(0.f),
		glm::vec2 scale = glm::vec2(1.f),
		float rotation = 0.f);

	// Flush pending uniform buffers
	void flushBuffers();

	// Draw all instances in once
	void draw() const;

private:

	struct Transform
	{
		glm::vec2			mPos;
		glm::vec2			mScale;
		glm::vec2			mRot;

		const glm::uint32	mTemplateId;
		const glm::float32	mPad;
	};

	GraphicsPipeline mGraphicsPipe;

	uint32_t	mTexId;

	uint32_t	mVAO;
	uint32_t	mUBO[eUBO_MAX];

	size_t	mMaxTemplates;
	size_t	mMaxInstances;

	std::vector<Template>	mTemplates;
	std::vector<Transform>	mInstances;

	// Dirty templates and instances
	std::queue<size_t>	mPendingTemplates;
	std::queue<size_t>	mPendingInstances;

	void fillTemplatesBuffer();
	void fillInstancesBuffer();
};
