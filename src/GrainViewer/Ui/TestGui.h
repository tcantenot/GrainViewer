/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#pragma once

class Window;

#include <string>
#include <vector>

/**
 * Gui for tests, displaying a progress bar and some debug info
 */
class TestGui {
public:
	TestGui(std::shared_ptr<Window> window);
	~TestGui();

	void updateProgress(float progress);
	void addMessage(const std::string & msg);

	void render();

	// event callbacks
	void onResize(int width, int height);
	void onMouseButton(int button, int action, int mods);
	void onKey(int key, int scancode, int action, int mods);
	void onCursorPosition(double x, double y);

private:
	void setupCallbacks();

private:
	std::weak_ptr<Window> m_window;

	float m_windowWidth;
	float m_windowHeight;

	float m_progress = 0;
	std::vector<std::string> m_messages;

	bool m_isMouseMoveStarted;
};
