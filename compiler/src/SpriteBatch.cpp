#include "SpriteBatch.hpp"
#include "ShaderCompiler.hpp"
#include "OGL.hpp"
#include "format.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <cassert>
#include <cmath>
#include <algorithm>
#include <memory>
#include <utility>
#include <exception>

#define BUFFER_TYPE GL_UNIFORM_BUFFER
//#define BUFFER_TYPE GL_SHADER_STORAGE_BUFFER

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

	gl::uint32 initBuffer(gl::enumerator buff_type, int32_t size, bool dynamic)
	{
		gl::int32 buffer_offeset(0);
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &buffer_offeset);

		gl::int32 max_buffer_size(0);
		glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_buffer_size);

		gl::int32 required_buffer_size = (gl::int32)nextMultipleOf(size, buffer_offeset);
		const auto buffer_size = std::min(required_buffer_size, max_buffer_size);

		if (buffer_size < required_buffer_size) {
			throw std::runtime_error(fmt::format(
				"Cannot create buffer of size {} bytes, maximum allowed {} bytes\n",
				required_buffer_size, max_buffer_size));
		}

		gl::uint32 ubo;
		glGenBuffers(1, &ubo);
		glBindBuffer(buff_type, ubo);

		auto buffer_usage = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
		glBufferData(buff_type, buffer_size, nullptr, buffer_usage);
		glBindBuffer(buff_type, 0);

		return ubo;
	}

	void updateBuffer(gl::enumerator buff_type, gl::uint32 ubo, uint8_t* buffer, gl::int32 offset, gl::sizei size)
	{
		if (size > 0) {

			// uniform buffer object needs to be bound with std140 layout attribute
			glBindBuffer(buff_type, ubo);
			uint8_t* buffer_ptr = (uint8_t*)glMapBufferRange(buff_type, offset,
				size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

			// copy memory into the buffer
			memcpy(buffer_ptr, buffer, size);

			// make sure the uniform buffer is uploaded
			glUnmapBuffer(buff_type);
		}
	}

	void fillBuffer(gl::enumerator buff_type, gl::uint32 ubo, uint8_t* buffer, gl::sizei size)
	{
		if (size > 0) {

			// uniform buffer object needs to be bound with std140 layout attribute
			glBindBuffer(buff_type, ubo);
			uint8_t* buffer_ptr = (uint8_t*)glMapBufferRange(buff_type, 0,
				size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

			// copy memory into the buffer
			memcpy(buffer_ptr, buffer, size);

			// make sure the uniform buffer is uploaded
			glUnmapBuffer(buff_type);
		}
	}

	void releaseBuffer(gl::uint32& ubo)
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
	mUBO[eUBO_PROJECTION] = initBuffer(BUFFER_TYPE, sizeof(projection), false);
	fillBuffer(BUFFER_TYPE, mUBO[eUBO_PROJECTION], (uint8_t*)&projection[0], sizeof(projection));

	mMaxTemplates = max_templates;
	mUBO[eUBO_TEMPLATE] = initBuffer(BUFFER_TYPE, mMaxTemplates * Template::VBO_SIZE, false);
	
	mMaxInstances = max_sprites;
	mUBO[eUBO_INSTANCE] = initBuffer(BUFFER_TYPE, mMaxInstances * sizeof(Instance), true);
	mUBO[eUBO_DATA] = initBuffer(BUFFER_TYPE, mMaxInstances * sizeof(Data), true);

	// Create the vertex array for the draw command
	mVAO = initVAO();

	// Record texture id
	mTexId = texture_id;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mTexId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return mGraphicsPipe.isValid() && mVAO &&
		mUBO[eUBO_TEMPLATE] &&	mUBO[eUBO_INSTANCE] &&	mUBO[eUBO_PROJECTION];
}

void SpriteBatch::release()
{
	mGraphicsPipe.destroy();

	for (size_t ui = 0; ui < eUBO_MAX; ++ui) {
		releaseBuffer(mUBO[ui]);
	}

	releaseVAO(mVAO);
}

void SpriteBatch::fillTemplatesBuffer()
{
	// Refill the whole buffer with new data
	if (!bDirtyTemplates) {
		return;
	}

	const size_t n_templates = mTemplates.size();
	const size_t buffer_size = n_templates * Template::VBO_SIZE;
	std::unique_ptr<uint8_t> templates_buffer(new uint8_t[buffer_size]);

	for (size_t i = 0; i < n_templates; ++i)
	{
		const size_t offset = i * Template::VBO_SIZE;
		memcpy(templates_buffer.get() + offset,
			mTemplates[i].mVBO, Template::VBO_SIZE);
	}

	fillBuffer(BUFFER_TYPE, mUBO[eUBO_TEMPLATE], templates_buffer.get(), buffer_size);
	bDirtyTemplates = false;
}

void SpriteBatch::fillInstancesBuffer()
{
	// Refill the whole buffer with new data
	if (!bDirtyInstances) {
		return;
	}

	// Update instances buffer
	{
		const size_t elem_size = sizeof(Instance);
		const size_t n_instances = mInstances.size();
		const size_t instance_buffer_size = n_instances * elem_size;
		std::unique_ptr<uint8_t> instance_buffer(new uint8_t[instance_buffer_size]);

		for (size_t i = 0; i < n_instances; ++i)
		{
			const size_t offset = i * elem_size;
			memcpy(instance_buffer.get() + offset, mInstances[i].get(), elem_size);
		}

		fillBuffer(BUFFER_TYPE, mUBO[eUBO_INSTANCE], instance_buffer.get(), instance_buffer_size);
	}

	// Update transform buffer
	{
		const size_t elem_size = sizeof(Data);
		const size_t n_transform = mData.size();
		const size_t transform_buffer_size = n_transform * elem_size;
		std::unique_ptr<uint8_t> transform_buffer(new uint8_t[transform_buffer_size]);

		for (size_t i = 0; i < n_transform; ++i)
		{
			const size_t offset = i * elem_size;
			memcpy(transform_buffer.get() + offset, &mData[i], elem_size);
		}

		fillBuffer(BUFFER_TYPE, mUBO[eUBO_DATA], transform_buffer.get(), transform_buffer_size);
	}

	bDirtyInstances = false;
}

void SpriteBatch::flushBuffers()
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

	// bind buffers
	for (gl::uint32 ui = 0; ui < eUBO_MAX; ++ui) {
		glBindBufferBase(BUFFER_TYPE, ui, mUBO[ui]);
	}

	// bind texture
	glBindTextures(0, 1, &mTexId);

	// bind the vertex array buffer, which is required
	glBindVertexArray(mVAO);

	// buffer vertex count
	glDrawArraysInstanced(GL_TRIANGLES, 0, MAX_VERTICES, mData.size());
}

SpriteBatch::Template SpriteBatch::Template::INVALID = {
	nullptr, INDEX_NONE
};

SpriteBatch::Instance SpriteBatch::Instance::INVALID = {
	INDEX_NONE, INDEX_NONE
};


glm::vec2 SpriteBatch::getInstancePosition(const std::shared_ptr<Instance>& instance) const
{
	assert(instance->isValid());
	glm::mat4 transformation = mData[instance->mDataId].mTransform;

	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transformation, scale, rotation, translation, skew, perspective);

	return glm::vec2(translation);
}

glm::vec2 SpriteBatch::getInstanceSize(const std::shared_ptr<Instance>& instance) const
{
	assert(instance->isValid());
	glm::mat4 transformation = mData[instance->mDataId].mTransform;

	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transformation, scale, rotation, translation, skew, perspective);

	return glm::vec2(scale);
}

float SpriteBatch::getInstanceRotation(const std::shared_ptr<Instance>& instance) const
{
	assert(instance->isValid());
	glm::mat4 transformation = mData[instance->mDataId].mTransform;

	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transformation, scale, rotation, translation, skew, perspective);

	return glm::angle(rotation);
}

glm::vec4 SpriteBatch::getInstanceColor(const std::shared_ptr<Instance>& instance) const
{
	assert(instance->isValid());
	return mData[instance->mDataId].mColor;
}

const SpriteBatch::Template& SpriteBatch::getInstanceTemplate(const std::shared_ptr<Instance>& instance) const
{
	assert(instance->isValid());
	return mTemplates[instance->mTemplateId];
}

const SpriteBatch::Template& SpriteBatch::createTemplate(glm::vec4 atlas_offsets)
{
	float left = atlas_offsets.x;
	float top = atlas_offsets.y;
	float right = atlas_offsets.z;
	float bottom = atlas_offsets.w;

	// Interleaved array of vertex
	// positions and texture coordinates
	const glm::vec4 vbo[MAX_VERTICES] = {
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
		Template new_template(vbo, mTemplates.size());
		mTemplates.push_back(new_template);

		bDirtyTemplates = true;
		return mTemplates.back();
	}

	return SpriteBatch::Template::INVALID;
}

bool SpriteBatch::swapInstanceTemplate(std::shared_ptr<Instance>& instance, const Template & new_template)
{
	if (new_template.isValid() && instance->isValid())
	{
		assert(new_template.mTemplateId < mTemplates.size());
		instance->mTemplateId = new_template.mTemplateId;

		bDirtyInstances = true;
		return true;
	}

	return false;
}

std::shared_ptr<SpriteBatch::Instance> SpriteBatch::addInstance(const Template& template_ref)
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
			if (mData.size() < mMaxInstances)
			{
				mInstances.push_back(std::make_shared<Instance>(template_ref.mTemplateId, mData.size()));
				mData.push_back({ glm::mat4(0.0f), glm::vec4(1.0f) });

				bDirtyInstances = true;
				return mInstances.back();
			}
		}
	}

	// return an invalid object
	return std::make_shared<Instance>(SpriteBatch::Instance::INVALID);
}

bool SpriteBatch::updateInstance(const std::shared_ptr<Instance>& instance_ref,
	glm::vec2 position, glm::vec2 scale, glm::vec4 color, float rotation)
{
	Instance* instance_ptr = instance_ref.get();
	const auto data_id = instance_ptr->mDataId;
	if (instance_ptr->isValid() && mData.size() > data_id)
	{
		glm::mat4 model;

		// rotate around the centre
		model = glm::translate(model, glm::vec3(position, 0.0f));
		model = glm::translate(model, glm::vec3(0.5f * scale.x, 0.5f * scale.y, 0.0f));
		model = glm::rotate(model, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::translate(model, glm::vec3(-0.5f * scale.x, -0.5f * scale.y, 0.0f));
		model = glm::scale(model, glm::vec3(scale, 1.0f));

		mData[data_id].mTransform = model;
		mData[data_id].mColor = color;

		bDirtyInstances = true;
		return true;
	}

	return false;
}

void SpriteBatch::removeInstance(const std::shared_ptr<Instance>& sprite_ref)
{
	// find this reference in the list of our instances
	auto instance = std::find(mInstances.begin(), mInstances.end(), sprite_ref);
	if (instance != std::end(mInstances))
	{
		assert((*instance)->isValid());
		assert((*instance) == mInstances[(*instance)->mDataId]);
		assert((*instance)->mDataId < mData.size());
		assert(mData.size() == mInstances.size());

		// swap the instance pointed by Instance::mDataId
		// in the data array with last one
		auto data_id = (*instance)->mDataId;
		auto swap_id = mData.size() - 1;
		
		// swap algorithm only if not last instance
		if (data_id != swap_id)
		{
			// swap data
			mData[data_id] = mData[swap_id];

			// swap instances
			mInstances[data_id] = mInstances[swap_id];

			// force instance to update its data reference
			mInstances[data_id]->mDataId = data_id;
		}

		// pop from data and instance from the lists
		mData.pop_back();
		mInstances.pop_back();

		bDirtyInstances = true;
	}
}
