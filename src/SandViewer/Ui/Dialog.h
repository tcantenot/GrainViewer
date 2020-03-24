// **************************************************
// Author : �lie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 �lie Michel.
// **************************************************

#pragma once

/**
 * When deriving from this, you may call
 * registerDialogForBehavior(DialogType, BehaviorType) for the dialog to be
 * automatically created when a behavior is associated to an object of the
 * scene.
 * In this case, the Dialog must define a function
 *     void setControlledBehavior(std::weak_ptr<BehaviorType>)
 */
class Dialog {
public:
	/**
	 * Draw panel (in right-hand side bar).
	 */
	virtual void draw() {}

	/**
	 * Draw on top of 3D render
	 * x, y, w, h is the viewport rect
	 */
	virtual void drawHandles(float x, float y, float w, float h) {}
};

template <typename T> struct DialogFactory { static std::shared_ptr<Dialog> MakeShared() { return std::make_shared<Dialog>(); } };
#define registerDialogForBehavior(DialogType, BehaviorType) template<> struct DialogFactory<BehaviorType> { static std::shared_ptr<DialogType> MakeShared() { return std::make_shared<DialogType>(); } };

