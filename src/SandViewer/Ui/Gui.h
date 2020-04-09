// **************************************************
// Author : �lie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 �lie Michel.
// **************************************************

#pragma once

class Window;
class Scene;
class Dialog;

#include <string>
#include <vector>
#include <memory>

class Gui {
public:
	Gui(std::shared_ptr<Window> window);
	~Gui();

	void setScene(std::shared_ptr<Scene> scene);

	// Call these resp. before and after loading the scene
	void  beforeLoading();
	void  afterLoading();

	void update();
	void render();

	// event callbacks
	void onResize(int width, int height);
	void onMouseButton(int button, int action, int mods);
	void onKey(int key, int scancode, int action, int mods);
	void onCursorPosition(double x, double y);

private:
	void setupCallbacks();
	void setupDialogs();
	void updateImGui();

private:
	struct DialogGroup {
		std::string title;
		std::vector<std::shared_ptr<Dialog>> dialogs;
		bool enabled = false;
	};

private:
	template<typename DialogType, typename ControllerType>
	void addDialogGroup(std::string title, ControllerType controller)
	{
		DialogGroup group;
		group.title = title;
		auto dialog = std::make_shared<DialogType>();
		dialog->setController(controller);
		group.dialogs.push_back(dialog);
		m_dialogGroups.push_back(group);
	}

private:
	std::weak_ptr<Window> m_window;
	std::shared_ptr<Scene> m_scene;
	std::vector<DialogGroup> m_dialogGroups;

	float m_startTime;

	float m_windowWidth;
	float m_windowHeight;
	bool m_showPanel = true;
	float m_panelWidth = 300.f;

	bool m_imguiFocus;
	bool m_isMouseMoveStarted;
	int m_isControlPressed = 0;
};
