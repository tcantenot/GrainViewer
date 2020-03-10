// **************************************************
// Author : �lie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 �lie Michel.
// **************************************************

#pragma once

#include "Shader.h"

// TODO: take as parameter
constexpr GLsizei MAX_DISPLAY_WIDTH = 2560;
constexpr GLsizei MAX_DISPLAY_HEIGHT = 1600;

typedef struct {
	GLenum format;  // GL_RGBA32F, GL_RGBA32UI...
	GLenum attachement; // GL_COLOR_ATTACHMENT0, ...
} ColorLayerInfo;

class Framebuffer {
public:
	/**
	 * @param mipmapDepthBuffer Turn it on when implementing Hierarchical Z Buffer
	 */
	Framebuffer(size_t width = MAX_DISPLAY_WIDTH,
		        size_t height = MAX_DISPLAY_HEIGHT,
		        const std::vector<ColorLayerInfo> & colorLayerInfos = {},
		        bool mipmapDepthBuffer = false);
	~Framebuffer();

	void bind() const;

	GLuint depthTexture() const { return m_depthTexture; }
	GLuint colorTexture(size_t i) const { return m_colorTextures[i]; }
	size_t colorTextureCount() const { return m_colorTextures.size(); }

	GLuint raw() const { return m_framebufferId; }

	GLsizei width() const { return m_width; }
	GLsizei height() const { return m_height; }

	/**
	 * Use this with caution, it reallocates video memory
	 */
	void setResolution(size_t width, size_t height);

	void saveToPng(const std::string & filename);

	/**
	 * Save the mipmap levels of depth attachement to prefixXX.png
	 */
	void saveDepthMipMapsToPng(const std::string & prefix);

	GLsizei depthLevels() const;

private:
	void init();
	void destroy();

private:
	GLsizei m_width, m_height;
	std::vector<ColorLayerInfo> m_colorLayerInfos;

	GLsizei m_depthLevels;

	GLuint m_framebufferId;
	std::vector<GLuint> m_colorTextures;
	GLuint m_depthTexture;

	// Allocated only when the framebuffer is saved to file, assuming that if
	// it happens once, it is likely to hapen again
	std::vector<uint8_t> m_pixels;
};

