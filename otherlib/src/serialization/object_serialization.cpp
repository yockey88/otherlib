/**
 * \file serialization/object_serialization.cpp
 **/

#include <sstream>

#include "serialization/byte_writer.hpp"
#include "serialization/serialization.hpp"

#include "script/script_object.hpp"
#include "script/scripting_environment.hpp"

#include "object/script_component.hpp"
#include "scene/scene.hpp"

namespace other {
  namespace serialization {

    std::vector<uint8_t> write_object_to_bytes(const scene& s, const scene_object& obj) {
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

      std::vector<uint8_t> bytes = {};
      byte_writer writer{};
      writer.write(obj.id);
      writer.write(s.get_transform(obj.id));
      // write_reflected_object(obj, bytes);
      // write_reflected_object(s.get_transform(obj.id), bytes);
      // write_value(parent_id, bytes);
      // write_list_with_2B_count(children_ids, bytes);
      // write_value(scripts->script_object_id, bytes);
      // bytes.append_range(write_attached_scripts_to_bytes(scripts));

      return bytes;
    }

  }  // namespace serialization
}  // namespace other