/**
 * \file object/object_serialization.cpp
 **/
#include "object/object_serialization.hpp"

#include <sstream>

#include "serialization/serialization.hpp"

#include "script/script_object.hpp"
#include "script/scripting_environment.hpp"

#include "object/object_serialization.hpp"
#include "object/object_serialization_data.hpp"
#include "object/script_component.hpp"
#include "scene/scene.hpp"

namespace other {
  namespace serialization {

    ostd::vector<uint8_t> write_object_to_bytes(const scene& s, const scene_object& obj) {
      /**
       | <entity-data-length> | <object> | transform-length | <transform> | <parent id> | <num children> | <children ids> | <script-id> | <script data> | <component data> |
       |-------------------------------------------------------------------------------------------------------------------------------------------------------------------|
       | 8 bytes              |          | 8 bytes          |             | 8 bytes     | 2 bytes        |                | 8 bytes     |               |                  |
       **/
      // const script_component* scripts = s.get_component<script_component>(&obj);
      // OTHER_ASSERT(scripts != nullptr, "Scene object must have a script component to be serialized");
      // script_object* script_obj = subsystem<scripting_environment>::get()->get_object(scripts->script_object_id);
      // OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment.", scripts->script_object_id);

      // CORE_LOG_DEBUG("Serializing scene object '{}' (ID: {:#018x})", obj.name, obj.id);

      // const scene_object* parent = s.get_parent(&obj);
      // uint64_t parent_id = parent != nullptr ? parent->id : 0;

      // ostd::vector<uint64_t> children_ids = s.get_children_ids(obj.id);

      ostd::vector<uint8_t> bytes = {};
      // write_reflected_object(obj, bytes);
      // write_reflected_object(s.get_transform(obj.id), bytes);
      // write_value(parent_id, bytes);
      // write_list_with_2B_count(children_ids, bytes);
      // write_value(scripts->script_object_id, bytes);
      // bytes.append_range(write_attached_scripts_to_bytes(scripts));

      return bytes;
    }

    std::pair<parsed_scene_object, natural_t> parse_single_object(const std::span<const uint8_t> buffer) {
      parsed_scene_object obj = {};

      size_t cursor = 0;
      // obj.object = read_reflected_object<scene_object>(buffer, cursor);
      // obj.obj_transform = read_reflected_object<transform>(buffer, cursor);
      // obj.parent_id = read_value<uint64_t>(buffer, cursor);
      // obj.children_ids = read_list_with_2B_count<uint64_t>(buffer, cursor);
      // obj.script_object_id = read_value<integer_t>(buffer, cursor);
      // cursor += parse_attached_scripts_into_object(buffer.subspan(cursor), obj);

      /// \todo
      ///   - component data

      return { obj, cursor };
    }

    std::pair<ostd::vector<parsed_scene_object>, natural_t> parse_object_list(const std::span<const uint8_t> buffer, natural_t num_objects) {
      ostd::vector<parsed_scene_object> objects = {};
      objects.reserve(num_objects);

      size_t cursor = 0;
      for (natural_t o = 0; o < num_objects; ++o) {
        auto [obj, bytes_read] = parse_single_object(std::span{ buffer.data() + cursor, buffer.size() - cursor });
        objects.push_back(std::move(obj));
        cursor += bytes_read;
      }

      return { std::move(objects), cursor };
    }

    namespace detail {

      void write_dotnet_object(dotnet_object* dn_obj, ostd::vector<uint8_t>& bytes) {
        // if (dn_obj != nullptr) {
        //   write_value<uint16_t>((uint16_t)dn_obj->get_type_name().size(), bytes);
        //   write_string_value(dn_obj->get_type_name(), bytes);

        //   ostd::vector<uint8_t> data = dn_obj->serialize_to_bytes();
        //   write_value<uint16_t>((uint16_t)data.size(), bytes);
        //   bytes.append_range(data);
        // } else {
        //   /// two zero lengths
        //   write_value<uint16_t>(0x0000, bytes);
        //   write_value<uint16_t>(0x0000, bytes);
        // }
      }

      std::pair<parsed_scene_object::dotnet_object, natural_t> parse_dotnet_object(const std::span<const uint8_t> buffer) {
        size_t cursor = 0;

        parsed_scene_object::dotnet_object obj = {};

        // uint16_t type_name_len = read_value<uint16_t>(buffer, cursor);
        // if (type_name_len > 0) {
        //   obj.name = read_string_value(buffer, type_name_len, cursor);
        // }

        // uint16_t data_len = read_value<uint16_t>(buffer, cursor);
        // if (data_len > 0) {
        //   obj.dotnet_blob = read_bytes(buffer, data_len, cursor) | std::ranges::to<ostd::vector<uint8_t>>();
        // }

        return { obj, cursor };
      }

      std::pair<parsed_scene_object::python_object, natural_t> parse_python_object(const std::span<const uint8_t> buffer) {
        return { {}, 0 };
      }

      std::pair<parsed_scene_object::lua_object, natural_t> parse_lua_object(const std::span<const uint8_t> buffer) {
        return { {}, 0 };
      }

    }  // namespace detail

    ostd::vector<uint8_t> write_attached_scripts_to_bytes(const script_component* obj) {
      /**
       | dotnet | python | lua |
       |-----------------------|
       |        |        |     |

       dotnet:
       | type name len | name | data len | data |
       |----------------------------------------|
       | 2 bytes       |      | 2 bytes  | data |

       python:
       | |
       |-|
       | |

       lua:
       | |
       |-|
       | |
       **/
      ostd::vector<uint8_t> bytes = {};

      scripting_environment* env = subsystem<scripting_environment>::get();
      OTHER_ASSERT(env != nullptr, "Scripting environment subsystem not found");

      script_object* script_obj = env->get_object(obj->script_object_id);
      OTHER_ASSERT(script_obj != nullptr, "Script object with ID {} not found in scripting environment", obj->script_object_id);

      // detail::write_dotnet_object(script_obj->dotnet_object, bytes);
      // detail::write_python_object(script_obj->python_object, bytes);
      // detail::write_lua_object(script_obj->lua_object, bytes);

      return bytes;
    }

    natural_t parse_attached_scripts_into_object(const std::span<const uint8_t> buffer, parsed_scene_object& object) {
      size_t cursor = 0;

      // auto [dotnet_obj, bytes_read1] = detail::parse_dotnet_object(buffer);
      // cursor += bytes_read1;

      // auto [python_obj, bytes_read2] = detail::parse_python_object(buffer.subspan(cursor));
      // cursor += bytes_read2;

      // auto [lua_obj, bytes_read3] = detail::parse_lua_object(buffer.subspan(cursor));
      // cursor += bytes_read3;

      // object.dotnet_obj = dotnet_obj;
      // object.python_obj = python_obj;
      // object.lua_obj = lua_obj;

      return cursor;
    }

  }  // namespace serialization
}  // namespace other