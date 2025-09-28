/**
 * \file project-write/main.cpp
 **/
#include <nlohmann/json.hpp>

#include "core/defines.hpp"

#include "object/scene_object.hpp"
#include "object/script_component.hpp"

#define OTHER_IMPLEMENTATION
#include "other.hpp"

namespace json = nlohmann;

constexpr const char* kUsageString =
  R"(Usage: project-writer <path-to-project-json>)";

std::vector<other::ref<other::assembly>> load_assemblies(const json::json& project_json);
void unload_assemblies(const std::vector<other::ref<other::assembly>>& assemblies);

void process_object(const json::json& obj_json, other::scene& scene);

std::vector<uint8_t> write_json_to_bytes(const json::json& j);
void write_bytes_to_file(const std::vector<uint8_t>& bytes, const other::filepath& output_path);

int main(int argc, char** argv) {
  other::initialize_other_environment();
  other::ref<other::assembly> other_assembly = other::subsystem<other::scripting_environment>::get()->load_dotnet_module("build/other-csharp/Debug/OtherCs.dll");

  exit_code res = other::SUCCESS;
  std::vector<other::ref<other::assembly>> testing_assemblies = {};
  try {
    if (argc < 2) {
      std::println(kUsageString);
      return other::FAILURE;
    }

    other::filepath project_file = argv[1];

    json::json project_json;
    {
      std::ifstream file_stream(project_file);
      if (!file_stream.is_open()) {
        throw std::runtime_error("Failed to open project JSON file: " + project_file.string());
      }

      file_stream >> project_json;
    }

    testing_assemblies = load_assemblies(project_json);
    auto bytes = write_json_to_bytes(project_json);

    other::filepath output_dir = project_file.parent_path();
    other::filepath filename_no_ext = project_file.stem();
    other::filepath output_file_path = output_dir / (filename_no_ext.string() + ".oscn");
    write_bytes_to_file(bytes, output_file_path);

    res = other::SUCCESS;
  } catch (const json::json::exception& e) {
    std::println("JSON error: {}", e.what());
    res = other::FAILURE;
  } catch (const std::runtime_error& e) {
    std::println("Runtime error: {}", e.what());
    res = other::FAILURE;
  } catch (...) {
    std::println("An unknown error occurred.");
    res = other::FAILURE;
  }

  unload_assemblies(testing_assemblies);

  other::subsystem<other::scripting_environment>::get()->unload_dotnet_module(other_assembly);
  other::shutdown_other_environment();
  return res;
}

std::vector<other::ref<other::assembly>> load_assemblies(const json::json& project_json) {
  std::vector<other::ref<other::assembly>> assemblies = {};

  auto dotnet_modules = project_json["dotnet-modules"];
  for (const auto& module_path : dotnet_modules) {
    std::string module_path_str = module_path["path"].get<std::string>();
    if (!std::filesystem::exists(module_path_str)) {
      std::println("   - Dotnet module path does not exist: {}", module_path_str);
      continue;
    }
    if (other::filepath(module_path_str).extension() != ".dll") {
      std::println("   - Dotnet module path is not a .dll file: {}", module_path_str);
      continue;
    }

    other::ref<other::assembly> module_assembly = other::subsystem<other::scripting_environment>::get()->load_dotnet_module(module_path_str);
    assemblies.push_back(module_assembly);
    std::println("   - Loaded dotnet module: {}", module_path_str);
  }

  return assemblies;
}

void unload_assemblies(const std::vector<other::ref<other::assembly>>& assemblies) {
  for (const auto& asm_ref : assemblies) {
    other::subsystem<other::scripting_environment>::get()->unload_dotnet_module(asm_ref);
    std::println("   - Unloaded dotnet module: {}", asm_ref->get_name());
  }
}

void process_object(const json::json& obj_json, other::scene& scene) {
  std::string name = obj_json["name"].get<std::string>();
  auto* obj = &scene.create_object(name);

  other::script_component* script_comp = nullptr;
  if (obj_json.contains("dotnet")) {
    auto dotnet_class = obj_json["dotnet"];
    if (!dotnet_class.contains("type-name")) {
      std::println("    - Dotnet class missing 'type-name' field for object '{}'", obj->name);
    } else {
      std::string type_name = dotnet_class["type-name"].get<std::string>();
      script_comp = scene.get_component<other::script_component>(obj);
      other::subsystem<other::scripting_environment>::get()->attach_dotnet_object(script_comp->script_object_id, type_name);
    }

    if (script_comp != nullptr && dotnet_class.contains("fields")) {
      other::script_object* script_obj = other::subsystem<other::scripting_environment>::get()->get_object(script_comp->script_object_id);

      auto fields = dotnet_class["fields"];
      for (const auto& field : fields) {
        try {
          std::string name = field["name"].get<std::string>();
          std::string type_str = field["type"].get<std::string>();
          other::value_type val_type = other::get_value_type_from_string(type_str);
          std::println("      - Setting field '{}' of type '{}' on object '{}'", name, type_str, obj->name);
          switch (val_type) {
            case other::value_type::OEBOOL: {
              bool val = field["value"].get<bool>();
              script_obj->dotnet_object->set_field<bool>(name, val);
              break;
            }
            case other::value_type::CHAR: {
              char val = field["value"].get<char>();
              script_obj->dotnet_object->set_field<char>(name, val);
              break;
            }
            case other::value_type::INT8: {
              int8_t val = field["value"].get<int8_t>();
              script_obj->dotnet_object->set_field<int8_t>(name, val);
              break;
            }
            case other::value_type::INT16: {
              int16_t val = field["value"].get<int16_t>();
              script_obj->dotnet_object->set_field<int16_t>(name, val);
              break;
            }
            case other::value_type::INT32: {
              int32_t val = field["value"].get<int32_t>();
              script_obj->dotnet_object->set_field<int32_t>(name, val);
              break;
            }
            case other::value_type::INT64: {
              int64_t val = field["value"].get<int64_t>();
              script_obj->dotnet_object->set_field<int64_t>(name, val);
              break;
            }
            case other::value_type::UINT8: {
              uint8_t val = field["value"].get<uint8_t>();
              script_obj->dotnet_object->set_field<uint8_t>(name, val);
              break;
            }
            case other::value_type::UINT16: {
              uint16_t val = field["value"].get<uint16_t>();
              script_obj->dotnet_object->set_field<uint16_t>(name, val);
              break;
            }
            case other::value_type::UINT32: {
              uint32_t val = field["value"].get<uint32_t>();
              script_obj->dotnet_object->set_field<uint32_t>(name, val);
              break;
            }
            case other::value_type::UINT64: {
              uint64_t val = field["value"].get<uint64_t>();
              script_obj->dotnet_object->set_field<uint64_t>(name, val);
              break;
            }
            case other::value_type::FLOAT: {
              float val = field["value"].get<float>();
              script_obj->dotnet_object->set_field<float>(name, val);
              break;
            }
            case other::value_type::DOUBLE: {
              double val = field["value"].get<double>();
              script_obj->dotnet_object->set_field<double>(name, val);
              break;
            }
            case other::value_type::STRING: {
              std::string val = field["value"].get<std::string>();
              script_obj->dotnet_object->set_field<std::string>(name, val);
              break;
            }
            default: {
              std::println("      - Unsupported field type '{}' for field '{}' on object '{}'", type_str, name, obj->name);
              break;
            }
          }

          script_obj->dotnet_object->invoke("DisplayInfo");

        } catch (...) {
          std::println("      - Failed to set field on object '{}'", obj->name);
          continue;
        }
      }
    }
  }
}

std::vector<uint8_t> write_json_to_bytes(const json::json& j) {
  std::string scene_name = j["name"].get<std::string>();
  other::scene scene1(scene_name);

  std::vector<other::scene_object*> scene_objects = {};
  auto object_list = j["objects"];

  for (const auto& obj_json : object_list) {
    process_object(obj_json, scene1);
  }

  std::vector<uint8_t> bytes = other::serialization::write_scene_to_bytes(scene1);
  std::println(" - Scene: {} [{} bytes]", scene_name, bytes.size());
  std::println("    - Objects: {}", scene1.get_object_count());
  return bytes;
}

void write_bytes_to_file(const std::vector<uint8_t>& bytes, const other::filepath& output_path) {
  std::ofstream output_file(output_path, std::ios::binary | std::ios::trunc);
  if (!output_file.is_open()) {
    std::cerr << "Failed to open output scene file for writing." << std::endl;
    return;
  }
  output_file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  std::println(" - Wrote scene [{} bytes] to {}", bytes.size(), output_path.string());
}