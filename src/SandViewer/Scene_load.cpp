#include <iostream>
#include <fstream>

#include <rapidjson/document.h>

#include "Logger.h"
#include "ResourceManager.h"
#include "RuntimeObject.h"
#include "utils/strutils.h"
#include "utils/fileutils.h"
#include "Scene.h"
#include "ShaderPool.h"
#include "EnvironmentVariables.h"
#include "BehaviorRegistry.h"
#include "Behavior.h"

bool Scene::load(const std::string & filename)
{
	clear();
	m_filename = filename;

	EnvironmentVariables env;
	env.baseFile = fs::path(m_filename).stem().string();

	rapidjson::Document d;
	bool valid;
	std::ifstream in(filename);

	if (!in.is_open()) {
		ERR_LOG << filename << ": Unable to read";
		return false;
	}

	LOG << "Loading scene from JSON file " << filename << "...";
	ResourceManager::setResourceRoot(baseDir(filename));

	std::string json;
	std::string line;
	while (getline(in, line)) {
		json += line;
	}

	if (d.Parse(json.c_str()).HasParseError()) {
		ERR_LOG << "rapidjson: " << d.GetParseError();
		ERR_LOG << "Parse error while reading JSON file " << filename;
		return false;
	}

	valid = d.IsObject() && d.HasMember("augen");
	if (!valid) { ERR_LOG << "JSON scene must be an object with a field called 'augen'."; return false; }

	rapidjson::Value& root = d["augen"];

	if (root.HasMember("shaders")) {
		if (!ShaderPool::Deserialize(root["shaders"])) {
			return false;
		}
	}

	if (root.HasMember("deferredShader")) {
		if (!m_deferredShader.deserialize(root["deferredShader"])) {
			return false;
		}
	}

	if (!m_world.deserialize(root)) { // look at both root["world"] and root["lights"]
		return false;
	}
	m_world.start();

	if (root.HasMember("cameras")) {
		auto& cameras = root["cameras"];
		if (!cameras.IsArray()) { ERR_LOG << "cameras field must be an array."; return false; }

		m_viewportCameraIndex = 0;
		for (rapidjson::SizeType i = 0; i < cameras.Size(); i++) {
			auto& cameraJson = cameras[i];

			auto camera = std::make_shared<TurntableCamera>();
			camera->deserialize(cameraJson, env, m_animationManager);
			m_cameras.push_back(camera);
		}
	}
	else {
		m_cameras.push_back(std::make_shared<TurntableCamera>());
		m_viewportCameraIndex = 0;
	}

	if (root.HasMember("objects")) {
		auto& objects = root["objects"];
		if (!objects.IsArray()) { ERR_LOG << "objects field must be an array."; return false; }

		for (rapidjson::SizeType i = 0; i < objects.Size(); i++) {
			auto& o = objects[i];

			auto obj = std::make_shared<RuntimeObject>();

			if (o.HasMember("name") && o["name"].IsString()) {
				obj->name = o["name"].GetString();
			}

			// Components
			std::vector<std::shared_ptr<Behavior>> behaviors;
			const auto& d = o["behaviors"];
			behaviors.reserve(static_cast<size_t>(d.Size()));
			for (rapidjson::SizeType i = 0; i < d.Size(); i++) {
				const rapidjson::Value& behaviorJson = d[i];
				if (!(behaviorJson.HasMember("type") && behaviorJson["type"].IsString())) {
					continue;
				}
				const std::string & type = behaviorJson["type"].GetString();
				std::shared_ptr<Behavior> b;
				BehaviorRegistry::addBehavior(b, obj, type);
				if (b) {
					b->deserialize(behaviorJson, env, m_animationManager);
					if (behaviorJson.HasMember("enabled") && behaviorJson["enabled"].IsBool()) {
						b->setEnabled(behaviorJson["enabled"].GetBool());
					}
					behaviors.push_back(b);
				}
				else {
					ERR_LOG << "Unknown behavior type: " << type;
				}
			}

			obj->start();
			m_objects.push_back(obj);
		}
	}

	if (root.HasMember("scene")) {
		auto& scene = root["scene"];
		if (scene.HasMember("quitAfterFrame")) {
			if (scene["quitAfterFrame"].IsInt()) {
				m_quitAfterFrame = scene["quitAfterFrame"].GetInt();
			} else {
				WARN_LOG << "'quitAfterFrame' field of 'scene' must be an integer";
			}
		}
	}

	const glm::vec2 & res = viewportCamera()->resolution();
	DEBUG_LOG << "reso: " << res.x << ", " << res.y;
	m_deferredShader.setResolution(static_cast<int>(res.x), static_cast<int>(res.y));

	reloadShaders();

	DEBUG_LOG << "Loading done.";

	return true;
}
