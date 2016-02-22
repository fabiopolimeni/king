#include "SpriteBatch.hpp"
#include "ShaderCompiler.hpp"
#include "OGL.hpp"

#include <cassert>
#include <cmath>
#include <algorithm>
#include <memory>
#include <utility>

namespace
{
	// there are several version of this function and optimisations we can apply
	// knowing before hand if x86/x64 architecture, or using the BSR instruction.
	// this part of the code in not used in a time sensitive place, hence this
	// solution is portable and sufficient.
	inline size_t nextPowerOfTwo(size_t a)
	{
		return size_t(1) << (size_t)ceil(log2(a));
	}

	// we assume here that m is power of two
	inline size_t nextMultipleOf(size_t n, size_t m)
	{
		const size_t r = m ? n % m : 0;
		return (r == 0) ? n : n + m - r;
	}

	const float PI = 3.1415927f;
	float deg2Rad(float degrees)
	{
		return degrees * PI / 180.0f;
	}

	float rad2Deg(float radians)
	{
		return radians * 180.0f / PI;
	}

	gl::uint32 initUBO(int32_t size, bool dynamic)
	{
		gl::int32 uniform_buffer_offeset(0);
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniform_buffer_offeset);

		gl::int32 max_uniform_buffer_size(0);
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_uniform_buffer_size);

		gl::int32 buffer_size = (gl::int32)nextMultipleOf(size, uniform_buffer_offeset);
		const auto uniform_buffer_size = std::min(buffer_size, max_uniform_buffer_size);

		if (uniform_buffer_size < buffer_size) {
			return 0;
		}

		gl::uint32 ubo;
		glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferData(GL_UNIFORM_BUFFER, uniform_buffer_size, NULL,
			dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		return ubo;
	}

	void updateUBO(gl::uint32 ubo, uint8_t* buffer, gl::int32 offset, gl::sizei size)
	{
		// uniform buffer object needs to be bound with std140 layout attribute
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		uint8_t* buffer_ptr = (uint8_t*)glMapBufferRange(GL_UNIFORM_BUFFER, offset,
			size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

		// copy memory into the buffer
		memcpy(buffer_ptr, buffer, size);

		// make sure the uniform buffer is uploaded
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	}

	void fillUBO(gl::uint32 ubo, uint8_t* buffer, gl::sizei size)
	{
		// uniform buffer object needs to be bound with std140 layout attribute
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		uint8_t* buffer_ptr = (uint8_t*)glMapBufferRange(GL_UNIFORM_BUFFER, 0,
			size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

		// copy memory into the buffer
		memcpy(buffer_ptr, buffer, size);

		// make sure the uniform buffer is uploaded
		glUnmapBuffer(GL_UNIFORM_BUFFER);
	}

	void releaseUBO(gl::uint32& ubo)
	{
		assert(glIsBuffer(ubo));
		glDeleteBuffers(1, &ubo);
		ubo = 0;
	}

	gl::uint32 initVAO()
	{
		gl::uint32 vao;
		glGenVertexArrays(1, &vao);
		return vao;
	}

	void releaseVAO(gl::uint32& vao)
	{
		assert(glIsBuffer(vao));
		glDeleteBuffers(1, &vao);
		vao = 0;
	}
}

bool SpriteBatch::init(glm::mat4 projection, uint32_t texture_id,
	const char * vs_source, const char * fs_source,
	size_t max_templates, size_t max_sprites)
{
	std::array<const char*, GraphicsPipeline::StageType::eST_MAX> filestages = {
		vs_source, nullptr, nullptr, nullptr, fs_source, nullptr
	};

	// Build shader program
	mGraphicsPipe = ShaderCompiler::buildFromFiles(filestages);

	// Create uniform buffers
	mMaxTemplates = max_templates;
	mUBO[eUBO_TEMPLATE] = initUBO(mMaxTemplates * sizeof(Template::vbo_t), false);
	
	mMaxInstances = max_sprites;
	mUBO[eUBO_INSTANCE] = initUBO(mMaxInstances * sizeof(Transform), true);

	mUBO[eUBO_PROJECTION] = initUBO(sizeof(projection), false);
	fillUBO(mUBO[eUBO_PROJECTION], (uint8_t*)&projection[0], sizeof(projection));

	// Create the vertex array for the draw command
	mVAO = initVAO();

	// Record texture id
	mTexId = texture_id;

	return mGraphicsPipe.isValid() && mVAO &&
		mUBO[eUBO_TEMPLATE] &&	mUBO[eUBO_INSTANCE] &&	mUBO[eUBO_PROJECTION];
}

void SpriteBatch::release()
{
	mGraphicsPipe.destroy();
	releaseUBO(mUBO[eUBO_TEMPLATE]);
	releaseUBO(mUBO[eUBO_INSTANCE]);
	releaseVAO(mVAO);
}

void SpriteBatch::fillTemplatesBuffer()
{
	// Refill the whole buffer with new data
	if (mPendingTemplates.empty()) {
		return;
	}

	const size_t n_templates = mTemplates.size();
	const size_t buffer_size = n_templates * sizeof(Template::vbo_t);
	std::unique_ptr<uint8_t> templates_buffer(new uint8_t[buffer_size]);

	for (size_t i = 0; i < n_templates; ++i)
	{
		const size_t offset = i * sizeof(Template::vbo_t);
		memcpy(templates_buffer.get() + offset,
			mTemplates[i].mVBO.data(), sizeof(Template::vbo_t));
	}

	fillUBO(mUBO[eUBO_TEMPLATE], templates_buffer.get(), buffer_size);
	
	// Clear pending templates
	while (!mPendingTemplates.empty()) {
		mPendingTemplates.pop();
	}
}

void SpriteBatch::fillInstancesBuffer()
{
	// Refill the whole buffer with new data
	if (mPendingInstances.empty()) {
		return;
	}

	const size_t n_instances = mInstances.size();
	const size_t buffer_size = n_instances * sizeof(Transform);
	std::unique_ptr<uint8_t> instances_buffer(new uint8_t[buffer_size]);

	for (size_t i = 0; i < n_instances; ++i)
	{
		const size_t offset = i * sizeof(Transform);
		memcpy(instances_buffer.get() + offset,
			&mInstances[i], sizeof(Transform));
	}

	fillUBO(mUBO[eUBO_INSTANCE], instances_buffer.get(), buffer_size);

	// Clear pending instance transforms
	while (!mPendingInstances.empty()) {
		mPendingInstances.pop();
	}
}

void SpriteBatch::flush()
{
	// Update pending templates
	fillTemplatesBuffer();

	// Update pending instance transformations
	fillInstancesBuffer();
}

void SpriteBatch::draw() const
{
	// bind shader programs
	glBindProgramPipeline(mGraphicsPipe.getPipeId());

	// bind uniform buffers
	glBindBufferBase(GL_UNIFORM_BUFFER, Uniform::eUBO_TEMPLATE, mUBO[Uniform::eUBO_TEMPLATE]);
	glBindBufferBase(GL_UNIFORM_BUFFER, Uniform::eUBO_INSTANCE, mUBO[Uniform::eUBO_INSTANCE]);
	glBindBufferBase(GL_UNIFORM_BUFFER, Uniform::eUBO_PROJECTION, mUBO[Uniform::eUBO_PROJECTION]);

	// bind texture
	glBindTextures(0, 1, &mTexId);

	// bind the vertex array buffer, which is required
	glBindVertexArray(mVAO);

	// buffer vertex count
	glDrawArraysInstanced(GL_TRIANGLES, 0, MAX_VERTICES, mInstances.size());
}

SpriteBatch::Template SpriteBatch::Template::INVALID = {
	std::array<glm::vec4, MAX_VERTICES>(), INDEX_NONE
};

SpriteBatch::Instance SpriteBatch::Instance::INVALID = {
	SpriteBatch::Template::INVALID, INDEX_NONE
};

bool SpriteBatch::update(const Instance & instance_ref, glm::vec2 position, glm::vec2 scale, float rotation)
{
	if (instance_ref.isValid() && mInstances.size() > instance_ref.mInstanceId)
	{
		mInstances[instance_ref.mInstanceId].mPos = position;
		mInstances[instance_ref.mInstanceId].mScale = scale;
		
		// Store sin and cos respectively in x and y of the rotation vector
		float theta = deg2Rad(rotation);
		mInstances[instance_ref.mInstanceId].mRot = glm::vec2(sin(theta), cos(theta));

		mPendingInstances.emplace(instance_ref.mInstanceId);
		return true;
	}

	return false;
}

SpriteBatch::Template SpriteBatch::create(glm::vec4 atlas_offsets)
{
	float left = atlas_offsets.x;
	float top = atlas_offsets.y;
	float right = atlas_offsets.z;
	float bottom = atlas_offsets.w;

	// Interleaved array of vertex
	// positions and texture coordinates
	const std::array<glm::vec4, MAX_VERTICES> vbo = {
		glm::vec4(0.0f, 1.0f, left, top),
		glm::vec4(1.0f, 0.0f, right, bottom),
		glm::vec4(0.0f, 0.0f, left, bottom),

		glm::vec4(0.0f, 1.0f, left, top),
		glm::vec4(1.0f, 1.0f, right, top),
		glm::vec4(1.0f, 0.0f, right, bottom)
	};

	// we can't add more than the maximum templates
	if (mTemplates.size() < mMaxTemplates)
	{
		Template new_template = { vbo, mTemplates.size() };
		mTemplates.emplace_back(new_template);
		mPendingTemplates.emplace(new_template.mTemplateId);
		return new_template;
	}

	return SpriteBatch::Template::INVALID;
}

SpriteBatch::Instance SpriteBatch::add(const Template& template_ref)
{
	if (template_ref.isValid())
	{
		// find the template associated with this instance
		auto sprite_template = std::find_if(mTemplates.begin(), mTemplates.end(),
			[&template_ref](const Template& t) {
			return t.mTemplateId == template_ref.mTemplateId;
		});

		// if found the template
		if (sprite_template != std::end(mTemplates))
		{
			// we can't add more than the maximum instances
			if (mInstances.size() < mMaxInstances)
			{
				Instance new_instance = { (*sprite_template), mInstances.size() };
				Transform new_transform = { glm::vec2(0.f), glm::vec2(1.f), glm::vec2(0.f), (*sprite_template).mTemplateId, 0.0f };
				mInstances.emplace_back(new_transform);
				return new_instance;
			}
		}
	}

	// return an invalid object
	return SpriteBatch::Instance::INVALID;
}
