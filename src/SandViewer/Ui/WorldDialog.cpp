#include <limits>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

#include "utils/guiutils.h"
#include "WorldDialog.h"
#include "Light.h"

void WorldDialog::draw()
{
	if (auto cont = m_cont.lock()) {
		if (ImGui::CollapsingHeader("World", ImGuiTreeNodeFlags_DefaultOpen)) {
			// World
			bool shadowMaps = cont->isShadowMapEnabled();
			ImGui::Checkbox("Shadow Maps (global toggle)", &shadowMaps);
			cont->setShadowMapEnabled(shadowMaps);

			/*
			int i = 0;
			for (auto& l : world.lights()) {
				ImGui::PushID(i);
				if (ImGui::CollapsingHeader(("Light " + std::to_string(i++)).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
					BeginDisable(!shadowMaps);
					bool shadowMap = l->hasShadowMap();
					ImGui::Checkbox("Shadow Maps", &shadowMap);
					l->setHasShadowMap(shadowMap);
					EndDisable(!shadowMaps);

					glm::vec3 position = l->position();
					ImGui::InputFloat3("Position", (float*)&position);
					l->setPosition(position);

					ImGui::ColorEdit3("Color", glm::value_ptr(l->color()));
				}
				ImGui::PopID();
			}
			*/
		}
	}
}
