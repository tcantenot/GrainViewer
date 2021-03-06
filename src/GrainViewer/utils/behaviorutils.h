/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
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

#include "ShaderProgram.h"
#include "ViewLayerMask.h"

#include "utils/jsonutils.h"
#include "utils/strutils.h"
#include "utils/ReflectionAttributes.h"

#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <refl.hpp>
#include <magic_enum.hpp>

#include <string>

// This file is more reflectutils.h than anything else actually

/**
 * Automatically deserialize properties using reflection.
 * The type T must have reflection enabled (see refl-cpp)
 */
template<typename T>
void autoDeserialize(const rapidjson::Value& json, T& properties) {
	// Deserialize properties using reflection
	for_each(refl::reflect(properties).members, [&](auto member) {
		using type = typename decltype(member)::value_type;
		std::string name = std::string(member.name);
		// bool, float, etc. properties (whatever is supported by jrOption
		if constexpr (
			std::is_same_v<type, bool>
			|| std::is_same_v<type, float>
			|| std::is_same_v<type, int>
			|| std::is_same_v<type, glm::vec3>
			|| std::is_same_v<type, glm::vec4>
			|| std::is_same_v<type, ViewLayerMask>)
		{
			jrOption(json, name, member(properties), member(properties));
		}
		// Enum properties
		// TODO: use magic_enum to load from strings
		else if constexpr (std::is_enum_v<type>) {
			if (json.HasMember(name.c_str()) && json[name.c_str()].IsString()) {
				std::string valueStr = json[name.c_str()].GetString();
				auto v = magic_enum::enum_cast<type>(valueStr);
				if (v.has_value()) {
					member(properties) = v.value();
				} else {
					ERR_LOG << "Invalid value '" << valueStr << "' for parameter '" << name << "'";
				}
			} else {
				int value = static_cast<int>(member(properties));
				jrOption(json, name, value, value);
				member(properties) = static_cast<typename decltype(member)::value_type>(value);
			}
		}
		else {
			ERR_LOG << "Unsupported type for property '" << member.name << "'";
		}
	});
}

/**
 * Automatically bind properties using reflection.
 * The type T must have reflection enabled (see refl-cpp)
 */
template<typename T>
void autoSetUniforms(const ShaderProgram& shader, const T& properties) {
	// Automatically bind properties using reflection
	for_each(refl::reflect(properties).members, [&](auto member) {
		using type = typename decltype(member)::value_type;
		// transform property name "foo" into uniform name "uFoo"
			// (ideally this transformation should be performed at compile time)
		std::string name = "u" + std::string(member.name);
		name[1] = toupper(name[1]);
		// whatever is supported by setUniform
		if constexpr (
			std::is_same_v<type, bool>
			|| std::is_same_v<type, float>
			|| std::is_same_v<type, int>
			|| std::is_same_v<type, glm::vec3>
			|| std::is_same_v<type, glm::vec4>)
		{
			shader.setUniform(name, member(properties));
		}
		else if constexpr (std::is_enum_v<type>)
		{
			shader.setUniform(name, static_cast<int>(member(properties)));
		}
		else if constexpr (std::is_same_v<type, ViewLayerMask>)
		{
			// not used by shaders
		}
	});
}

/**
 * Ideally this would be a constexpr function forking on refl-cpp's member.name meta-type
 */
std::string toDisplayName(const std::string& name);

/**
 * Automatically create imgui UI
 * The type T must have reflection enabled (see refl-cpp)
 */
template<typename T>
void autoUi(T& properties) {
	int id = 0; // for disambiguation in case several enums have identically named values
	for_each(refl::reflect(properties).members, [&](auto member) {
		using type = typename decltype(member)::value_type;
		std::string displayName = toDisplayName(std::string(member.name));

		float rangeMinimum = 0.0f;
		float rangeMaximum = 1.0f;
		if constexpr (refl::descriptor::has_attribute<ReflectionAttributes::Range>(member)) {
			constexpr auto range = refl::descriptor::get_attribute<ReflectionAttributes::Range>(member);
			rangeMinimum = range.minimum;
			rangeMaximum = range.maximum;
		}

		if constexpr (refl::descriptor::has_attribute<ReflectionAttributes::HideInDialog>(member)) {
			// skip
		}
		else if constexpr (std::is_same_v<type, bool>)
		{
			ImGui::Checkbox(displayName.c_str(), &member(properties));
		}
		else if constexpr (std::is_same_v<type, float>) {
			ImGui::SliderFloat(displayName.c_str(), &member(properties), rangeMinimum, rangeMaximum, "%.5f");
		}
		else if constexpr (std::is_same_v<type, int>) {
			ImGui::SliderInt(displayName.c_str(), &member(properties), static_cast<int>(rangeMinimum), static_cast<int>(rangeMaximum));
		}
		else if constexpr (std::is_same_v<type, glm::vec3>) {
			ImGui::SliderFloat3(displayName.c_str(), glm::value_ptr(member(properties)), rangeMinimum, rangeMaximum, "%.5f");
		}
		else if constexpr (std::is_same_v<type, glm::vec4>) {
			ImGui::SliderFloat4(displayName.c_str(), glm::value_ptr(member(properties)), rangeMinimum, rangeMaximum, "%.5f");
		}
		else if constexpr (std::is_enum_v<type>) {
			// assuming int enum
			int value = static_cast<int>(member(properties));
			ImGui::Text(("\n" + displayName + ":").c_str());
			ImGui::PushID(id++);
			constexpr auto options = magic_enum::enum_entries<type>();
			for (const auto& opt : options) {
				std::string optName = toDisplayName(std::string(opt.second));
				if (startsWith(optName, displayName)) {
					optName = optName.substr(displayName.size() + 1); // +1 for whitespace
				}
				ImGui::RadioButton(optName.c_str(), &value, static_cast<int>(opt.first));
			}
			ImGui::PopID();
			member(properties) = static_cast<type>(value);
		}
		else {
			ERR_LOG << "Unsupported type for property '" << member.name << "'";
		}
	});
}

// Misc utils (should end up somewhere else)
template <typename Enum>
constexpr Enum lastValue() {
	return magic_enum::enum_value<Enum>(magic_enum::enum_count<Enum>() - 1);
}


#define PROPERTIES_OPERATORS_DECL \
	bool operator==(const Properties& other); \
	bool operator!=(const Properties& other);

#define PROPERTIES_OPERATORS_DEF(Type) \
	bool Type::Properties::operator==(const Properties& other) { \
		bool isEqual = true; \
		for_each(refl::reflect(*this).members, [&](auto member) { \
			isEqual = isEqual && (member(*this) != member(other)); \
		}); \
		return isEqual; \
	} \
	bool Type::Properties::operator!=(const Properties& other) { \
		return !(*this == other); \
	}
