#pragma once

#include "GraphicsPipeline.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <queue>
#include <memory>

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
		eUBO_TRANSFORM,
		eUBO_MAX
	};

	struct Template
	{
		const glm::vec4	mVBO[MAX_VERTICES];
		const size_t	mTemplateId;
		
		static Template INVALID;
		static const size_t VBO_SIZE = sizeof(glm::vec4) * MAX_VERTICES;

		inline bool isValid() const {
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
		const size_t	mTemplateId;
		const size_t	mTransformId;

		static Instance INVALID;

		inline bool isValid() const {
			return mTemplateId != INDEX_NONE && mTransformId != INDEX_NONE;
		}

		inline Instance(const size_t template_id, const size_t transform_id)
			: mTemplateId(template_id), mTransformId(transform_id)
		{
		}
	};

	bool init(glm::mat4 projection, uint32_t texture_id,
		const char* vs_source, const char* fs_source,
		size_t max_templates, size_t max_sprites);

	void release();

	// Generates the VBO containing vertex positions and texture coordinates
	// @param atlas_offsets defined as x=left, y=top, z=right, w=bottom
	const Template& createTemplate(glm::vec4 atlas_offsets);

	// Add an instance of template to the sprite's batch
	// @return The ref index of the instance, SpriteKey.mTemplate == null otherwise
	std::shared_ptr<Instance> addInstance(const Template& template_ref);

	// Remove the instance from the set and decrement the control pointer
	void removeInstance(const std::shared_ptr<Instance>& sprite_ref);

	// Update instance transform
	bool updateInstance(const std::shared_ptr<Instance>& sprite_ref,
		glm::vec2 position = glm::vec2(0.f),
		glm::vec2 scale = glm::vec2(1.f),
		glm::vec4 color = glm::vec4(1.f),
		float rotation = 0.f);

	// Flush pending uniform buffers
	void flushBuffers();

	// Draw all instances in once
	void draw() const;

private:

	GraphicsPipeline mGraphicsPipe;

	uint32_t	mTexId;
	uint32_t	mVAO;
	uint32_t	mUBO[eUBO_MAX];

	size_t	mMaxTemplates;
	size_t	mMaxInstances;

	struct Transform
	{
		glm::mat4 mTransform;
		glm::vec4 mColor;
	};

	std::vector<Template>					mTemplates;
	std::vector<Transform>					mTransforms;
	std::vector<std::shared_ptr<Instance>>	mInstances;

	// Dirty templates and instances
	std::queue<size_t>	mPendingTemplates;
	std::queue<size_t>	mPendingInstances;

	void fillTemplatesBuffer();
	void fillInstancesBuffer();
};
