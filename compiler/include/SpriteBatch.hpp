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

	const static size_t INDEX_NONE = -1;
	const static size_t INSTANCE_VERTS = 6;

	enum Uniform
	{
		eUBO_TEMPLATE,
		eUBO_INSTANCE,
		eUBO_PROJECTION,
		eUBO_MAX
	};

	struct Template
	{
		typedef std::array<glm::vec4, INSTANCE_VERTS> vbo_t;
		const vbo_t&	mVBO;
		const size_t	mTemplateId;
		
		static Template INVALID;

		bool isValid() const {
			return mTemplateId != INDEX_NONE;
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

	bool init(uint32_t texture_id, const char* vs_source, const char* fs_source,
		size_t max_templates, size_t max_sprites, glm::mat4 projection);

	void release();

	// Generates the VBO containing vertex positions and texture coordinates
	// @param atlas_offsets defined as x=left, y=top, z=right, w=bottom
	Template create(glm::vec4 atlas_offsets);

	// Add an instance of template to the sprite's batch
	// @return The ref index of the instance, SpriteKey.mTemplate == null otherwise
	Instance add(const Template& template_ref);

	// Update instance transform
	bool update(const Instance& sprite_ref, glm::vec2 position, glm::vec2 scale, float rotation);

	// Flush pending uniform buffers
	void flush();

	// Draw all instances in once
	void draw() const;

private:

	struct Transform
	{
		glm::vec2		mPos;
		glm::vec2		mScale;
		float			mRot;

		const uint32_t	mTemplateId;
		const glm::vec2	mPad;
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
