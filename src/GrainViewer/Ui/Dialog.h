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

#include <memory>

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

template <typename T>
struct DialogFactory {
	static std::shared_ptr<Dialog> MakeShared() { return std::make_shared<Dialog>(); }
};

#define registerDialogForBehavior(DialogType, BehaviorType) \
template<> \
struct DialogFactory<BehaviorType> { \
	static std::shared_ptr<DialogType> MakeShared() { return std::make_shared<DialogType>(); } \
};
