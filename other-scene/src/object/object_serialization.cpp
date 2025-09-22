/**
 * \file object/object_serialization.cpp
 **/
#include "object/object_serialization.hpp"

#include "serialization/serialization.hpp"

#include "script/script_object.hpp"
#include "script/scripting_environment.hpp"

#include "object/object_serialization_data.hpp"
#include "object/script_component.hpp"
#include "scene/scene.hpp"

namespace other {
  namespace serialization {

    std::vector<uint8_t> write_object_to_bytes(const scene& s, const scene_object& obj) {
      /**
       | <entity-data-length> | <object> | transform-length | <transform> | <parent id> | <num children> | <children ids> | <script-id> | <num scripts> | <script data> | <component data> |
       |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
       | 8 bytes              |          | 8 bytes          |             | parent id   | 2 bytes        |                | 8 bytes     |               | 2 bytes       | component data   |
       **/
      const script_component* scripts = s.get_component<script_component>(&obj);
      OTHER_ASSERT(scripts != nullptr, "Scene object must have a script component to be serialized");
      script_object* script_obj = subsystem<scripting_environment>::get()->get_object(scripts->script_object_id);
      OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment.", scripts->script_object_id);

      const scene_object* parent = s.get_parent(&obj);
      uint64_t parent_id = parent != nullptr ? parent->id : 0;

      std::vector<uint64_t> children_ids = s.get_children_ids(obj.id);

      std::vector<uint8_t> bytes = {};
      write_reflected_object(obj, bytes);
      write_reflected_object(s.get_transform(obj.id), bytes);
      write_value(parent_id, bytes);
      write_list_with_2B_count(children_ids, bytes);
      write_value(scripts->script_object_id, bytes);
      {
        std::vector<uint8_t> data = write_attached_scripts_to_bytes(scripts);
        bytes.append_range(data);
      }

      return bytes;
    }

    std::pair<parsed_scene_object, natural_t> parse_single_object(const std::span<const uint8_t> buffer) {
      parsed_scene_object obj = {};

      size_t cursor = 0;
      obj.object = read_reflected_object<scene_object>(buffer, cursor);
      obj.obj_transform = read_reflected_object<transform>(buffer, cursor);
      obj.parent_id = read_value<uint64_t>(buffer, cursor);
      obj.children_ids = read_list_with_2B_count<uint64_t>(buffer, cursor);
      obj.script_object_id = read_value<integer_t>(buffer, cursor);

      uint16_t num_scripts = read_value<uint16_t>(buffer, cursor);
      if (num_scripts > 0) {
        auto [scripts, bytes_read] = parse_attached_script_list(buffer.subspan(cursor), num_scripts);
        obj.attached_scripts = std::move(scripts);
        cursor += bytes_read;
      }

      /// \todo
      ///   - component data

      return { obj, cursor };
    }

    std::pair<std::vector<parsed_scene_object>, natural_t> parse_object_list(const std::span<const uint8_t> buffer, natural_t num_objects) {
      std::vector<parsed_scene_object> objects = {};
      objects.reserve(num_objects);

      size_t cursor = 0;
      for (natural_t o = 0; o < num_objects; ++o) {
        auto [obj, bytes_read] = parse_single_object(std::span{ buffer.data() + cursor, buffer.size() - cursor });
        objects.push_back(std::move(obj));
        cursor += bytes_read;
      }

      return { std::move(objects), cursor };
    }

    std::vector<uint8_t> write_attached_script_to_bytes(const parsed_scene_object::attached_script& script) {
      /**
       | type    | name length | name | data length | data |
       |---------------------------------------------------|
       | 1 bytes | 2 bytes     |      | 8 bytes     |      |
       **/
      std::vector<uint8_t> bytes = {};
      write_value(script.type, bytes);
      write_value<uint16_t>((uint16_t)script.name.size(), bytes);
      write_string_value(script.name, bytes);
      write_value<uint64_t>((uint64_t)script.data.size(), bytes);
      bytes.append_range(script.data);
      return bytes;
    }

    std::vector<uint8_t> write_attached_scripts_to_bytes(const script_component* obj) {
      std::vector<uint8_t> bytes = {};

      scripting_environment* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment subsystem not found");

      script_object* script_obj = env->get_object(obj->script_object_id);
      OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment", obj->script_object_id);

      uint16_t num_scripts = 0;
      if (script_obj->dotnet_object != nullptr) {
        ++num_scripts;
      }
      if (script_obj->python_object != nullptr) {
        // ++num_scripts;
      }
      // if (script_obj->lua_object != nullptr) {}

      write_value(num_scripts, bytes);

      if (script_obj->dotnet_object != nullptr) {
        write_value((uint8_t)parsed_scene_object::attached_script::DOTNET, bytes);
        write_value<uint16_t>((uint16_t)script_obj->dotnet_object->get_type_name().size(), bytes);
        write_string_value(script_obj->dotnet_object->get_type_name(), bytes);

        {
          std::vector<uint8_t> data = script_obj->dotnet_object->serialize_to_bytes();
          write_value<uint64_t>((uint64_t)data.size(), bytes);
          bytes.append_range(data);
        }
      }

      if (script_obj->python_object != nullptr) {
        // write_value((uint8_t)parsed_scene_object::attached_script::PYTHON, bytes);
        // write_value<uint16_t>((uint16_t)script_obj->python_object->name.size(), bytes);
        // write_string_value(script_obj->python_object->name, bytes);
        // write_value<uint64_t>(0, bytes);  /// \todo placeholder for data length
        /// \todo serialize python object into blob and write here
      }

      // if (script_obj->lua_object != nullptr) {
      //   parsed_scene_object::attached_script script_data = {};
      // }
      return bytes;
    }

    std::pair<parsed_scene_object::attached_script, natural_t> parse_single_attached_script(const std::span<const uint8_t> buffer) {
      parsed_scene_object::attached_script script = {};
      size_t cursor = 0;

      script.type = read_value<uint8_t>(buffer, cursor);

      uint16_t name_length = read_value<uint16_t>(buffer, cursor);
      script.name = read_string_value(buffer, name_length, cursor);

      uint64_t data_length = read_value<uint64_t>(buffer, cursor);
      OTHER_ASSERT(buffer.size() >= cursor + data_length, "Buffer too small to read script data of length {}", data_length);

      std::span<const uint8_t> data_span = buffer.subspan(cursor, data_length);
      script.data = std::vector<uint8_t>(data_span.begin(), data_span.end());
      cursor += data_length;
      return { script, cursor };
    }

    std::pair<std::vector<parsed_scene_object::attached_script>, natural_t> parse_attached_script_list(const std::span<const uint8_t> buffer, natural_t num_scripts) {
      std::vector<parsed_scene_object::attached_script> scripts = {};
      scripts.reserve(num_scripts);

      size_t cursor = 0;
      for (natural_t i = 0; i < num_scripts; ++i) {
        auto [script, bytes_read] = parse_single_attached_script(std::span{ buffer.data() + cursor, buffer.size() - cursor });
        scripts.push_back(std::move(script));
        cursor += bytes_read;
      }

      return { std::move(scripts), cursor };
    }

  }  // namespace serialization
}  // namespace other