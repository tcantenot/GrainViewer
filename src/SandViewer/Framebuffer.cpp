// **************************************************
// Author : �lie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 �lie Michel.
// **************************************************

#include <cmath>
#include <algorithm>

#include "Framebuffer.h"
#include "Logger.h"
#include "ResourceManager.h"

using namespace std;


Framebuffer::Framebuffer(size_t width, size_t height, const vector<ColorLayerInfo> & colorLayerInfos, bool mipmapDepthBuffer)
	: m_width(static_cast<GLsizei>(width))
	, m_height(static_cast<GLsizei>(height))
	, m_colorLayerInfos(colorLayerInfos)
	, m_depthLevels(mipmapDepthBuffer ? static_cast<GLsizei>(1 + floor(log2(std::max(m_width, m_height)))) : 1)
{
	init();
}

Framebuffer::~Framebuffer() {
	destroy();
}

void Framebuffer::init() {
	glCreateFramebuffers(1, &m_framebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);

	if (!m_colorLayerInfos.empty()) {
		m_colorTextures.resize(m_colorLayerInfos.size());
		glCreateTextures(GL_TEXTURE_2D, static_cast<GLsizei>(m_colorTextures.size()), &m_colorTextures[0]);
	}

	for (size_t k = 0; k < m_colorLayerInfos.size(); ++k) {
		glTextureStorage2D(m_colorTextures[k], 1, m_colorLayerInfos[k].format, m_width, m_height);
		glTextureParameteri(m_colorTextures[k], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(m_colorTextures[k], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glNamedFramebufferTexture(m_framebufferId, m_colorLayerInfos[k].attachement, m_colorTextures[k], 0);
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &m_depthTexture);
	glTextureStorage2D(m_depthTexture, m_depthLevels, GL_DEPTH_COMPONENT24, m_width, m_height);
	glNamedFramebufferTexture(m_framebufferId, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);

	glTextureParameteri(m_depthTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(m_depthTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(m_depthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(m_depthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (m_depthLevels > 1) {
		glGenerateTextureMipmap(m_depthTexture);
	}

	if (m_colorLayerInfos.empty()) {
		glNamedFramebufferDrawBuffer(m_framebufferId, GL_NONE);
	}
	else {
		vector<GLenum> drawBuffers(m_colorLayerInfos.size());
		for (size_t k = 0; k < m_colorLayerInfos.size(); ++k) {
			drawBuffers[k] = m_colorLayerInfos[k].attachement;
		}
		glNamedFramebufferDrawBuffers(m_framebufferId, static_cast<GLsizei>(drawBuffers.size()), &drawBuffers[0]);
	}

	if (glCheckNamedFramebufferStatus(m_framebufferId, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		ERR_LOG << "Framebuffer not complete!";
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroy() {
	glDeleteFramebuffers(1, &m_framebufferId);
}

void Framebuffer::bind() const {
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferId);
}

void Framebuffer::setResolution(size_t width, size_t height)
{
	if (width == m_width && height == m_height) return;
	width = std::min(std::max((size_t)1, width), (size_t)4096);
	height = std::min(std::max((size_t)1, height), (size_t)4096);
	DEBUG_LOG << "Resizing framebuffer to (" << width << "x" << height << ")";
	m_width = static_cast<GLsizei>(width);
	m_height = static_cast<GLsizei>(height);
	destroy();
	init();
}

void Framebuffer::saveToPng(const std::string & filename)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebufferId);
	m_pixels.resize(4 * width() * height());
	void* pixels = static_cast<void*>(m_pixels.data());
	GLsizei bufSize = static_cast<GLsizei>(m_pixels.size() * sizeof(uint8_t));
	glReadnPixels(0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, bufSize, pixels);
	ResourceManager::saveImage_libpng(filename, width(), height(), pixels);
}

void Framebuffer::saveDepthMipMapsToPng(const std::string & prefix)
{
	m_pixels.resize(4 * width() * height());
	void* pixels = static_cast<void*>(m_pixels.data());
	GLsizei bufSize = static_cast<GLsizei>(m_pixels.size() * sizeof(uint8_t));

	glReadnPixels(0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, bufSize, pixels);

	GLsizei levelWidth = m_width;
	GLsizei levelHeight = m_height;
	for (GLint level = 0; level < depthLevels() - 2; ++level) {
		glGetTextureSubImage(depthTexture(), level, 0, 0, 0, levelWidth, levelHeight, 1, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, levelWidth * levelHeight, pixels);
		ResourceManager::saveImage_libpng(prefix + std::to_string(level) + ".png", levelWidth, levelHeight, pixels);
		levelWidth = levelWidth / 2 + levelWidth % 2;
		levelHeight = levelHeight / 2 + levelHeight % 2;
	}
}

GLsizei Framebuffer::depthLevels() const {
	return m_depthLevels;
}