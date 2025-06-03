/**
 * \file serialization/serialization_sandbox.cpp
 **/
#include <cstdint>
#include <fstream>

#include <flatbuffers/flexbuffers.h>
#include <glm/glm.hpp>

#include "core/defines.hpp"
#include "serialization/reflection.hpp"

#include "renderer/camera.hpp"

#include "driver/driver.hpp"


struct my_serializable {
  int a;
  float b;
  std::string c;
  glm::vec3 d;

  OTHER_REFLECTABLE(my_serializable);
};
OTHER_REFLECT(
  my_serializable,
  field(a, other::attr::serializable()),
  field(b, other::attr::serializable()),
  field(c, other::attr::serializable()),
  field(d, other::attr::serializable())
)
static_assert(other::reflected_type<my_serializable>, "my_serializable must be a reflected type");
static_assert(other::reflected_type<decltype(my_serializable::d)>, "glm::vec3 must be a reflected type");

template <typename T>
T read_object(const std::vector<uint8_t>& data) {
  auto buffer = flexbuffers::GetRoot(data.data(), data.size());
  auto outer_map = buffer.AsMap();
  CORE_LOG_INFO("Deserialized type hash: {}", outer_map["type-hash"].AsUInt64());
  CORE_LOG_INFO("Deserialized type name: {}", outer_map["type-name"].AsString().str());
  CORE_LOG_INFO("Deserialized number of fields: {}", outer_map["num-fields"].AsUInt64());

  T deserialized_obj;
  for_each(refl::reflect(deserialized_obj).members, [&](auto member) {
    if constexpr (refl::descriptor::has_attribute<other::attr::serializable>(member) &&
                  !refl::descriptor::is_function(member)) {
      std::string name = std::string{ member.name };
      flexbuffers::Reference reference = outer_map[name.c_str()];
      if (reference.IsNull()) {
        CORE_LOG_WARN("Field '{}' not found in serialized data.", name);
        return;
      }

      using member_t = std::remove_cvref_t<decltype(member(deserialized_obj))>;
      member(deserialized_obj) = other::get_field<member_t>(reference, std::string{ member.name });
    }
  });

  return deserialized_obj;
}

class OTHER_CLASS serialization_sandbox : public other::driver {
 public:
  serialization_sandbox(const other::config_table& config)
      : other::driver(config) {}

  void on_initialize() override {
  }

  void run() override {
    {
      std::vector<uint8_t> serialized_data;
      {
        my_serializable obj;
        obj.a = 42;
        obj.b = 3.14f;
        obj.c = "Hello, World!";
        obj.d = glm::vec3(1.0f, 2.0f, 3.0f);

        CORE_LOG_INFO("Original object: \n{}", other::type_data_handler<my_serializable>::as_string("obj", obj));

        serialized_data = other::type_data_handler<my_serializable>::as_bytes(obj);
        CORE_LOG_INFO("Serialized data size: {}", serialized_data.size());
        {
          std::ofstream of{ "artifacts/serialized_data.bin", std::ios::binary };
          of.write(reinterpret_cast<const char*>(serialized_data.data()), serialized_data.size());
          of.close();
        }
      }
      {
        auto buffer = flexbuffers::GetRoot(serialized_data);
        auto outer_map = buffer.AsMap();
        CORE_LOG_INFO("Deserialized type hash: {}", outer_map["type-hash"].AsUInt64());
        CORE_LOG_INFO("Deserialized type name: {}", outer_map["type-name"].AsString().str());
        CORE_LOG_INFO("Deserialized number of fields: {}", outer_map["num-fields"].AsUInt64());

        // my_serializable deserialized_obj = read_object<my_serializable>(serialized_data);
        my_serializable deserialized_obj = other::type_data_handler<my_serializable>::from_bytes(serialized_data);
        CORE_LOG_INFO("Deserialized object: \n{}", other::type_data_handler<my_serializable>::as_string("deserialized_obj", deserialized_obj));
      }
    }

    {
      std::vector<uint8_t> serialized_data;
      {
        other::camera cam;
        cam.euler_angles = glm::vec3(0.f, 0.f, 0.f);
        cam.fov = 45.f;
        cam.image_size = glm::vec2(1920.f, 1080.f);
        cam.samples_per_pixel = 1;
        cam.max_bounce_depth = 2;
        cam.defocus_angle = 0.1f;
        cam.focus_dist = 5.f;
        cam.world_up = glm::vec3(0.f, 1.f, 0.f);
        cam.aspect_ratio = 69.f;
        cam.image_width = 68.f;
        cam.sensitivity = 0.1f;
        cam.constrain_pitch = true;
        cam.look_from({ 0.f, 0.f, 5.f });
        cam.look_at({ 0.f, 0.f, 0.f });

        CORE_LOG_INFO("Original camera: \n{}", other::type_data_handler<other::camera>::as_string("cam", cam));
        serialized_data = other::type_data_handler<other::camera>::as_bytes(cam);

        CORE_LOG_INFO("Serialized camera data size: {}", serialized_data.size());
        {
          std::ofstream of{ "artifacts/camera_serialized_data2.bin", std::ios::binary };
          of.write(reinterpret_cast<const char*>(serialized_data.data()), serialized_data.size());
          of.close();
        }

        other::camera deserialized_cam = other::type_data_handler<other::camera>::from_bytes(serialized_data);
        CORE_LOG_INFO("Deserialized camera: \n{}", other::type_data_handler<other::camera>::as_string("deserialized_cam", deserialized_cam));
      }
    }

    {
      std::vector<uint8_t> unserialized_bytes;
      std::ifstream ifs("artifacts/serialized_data.bin", std::ios::binary);
      if (ifs) {
        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        unserialized_bytes.resize(size);
        ifs.read(reinterpret_cast<char*>(unserialized_bytes.data()), size);
        ifs.close();

        my_serializable deserialized_obj = read_object<my_serializable>(unserialized_bytes);
        CORE_LOG_INFO("Unserialized object: \n{}", other::type_data_handler<my_serializable>::as_string("deserialized_obj", deserialized_obj));
      } else {
        CORE_LOG_ERROR("Failed to open serialized data file.");
      }
    }

    {
      std::vector<uint8_t> unserialized_bytes;
      std::ifstream ifs("artifacts/camera_serialized_data2.bin", std::ios::binary);
      if (ifs) {
        ifs.seekg(0, std::ios::end);
        size_t size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        unserialized_bytes.resize(size);
        ifs.read(reinterpret_cast<char*>(unserialized_bytes.data()), size);
        ifs.close();

        other::camera deserialized_cam = read_object<other::camera>(unserialized_bytes);
        CORE_LOG_INFO("Unserialized camera: \n{}", other::type_data_handler<other::camera>::as_string("deserialized_cam", deserialized_cam));
      } else {
        CORE_LOG_ERROR("Failed to open serialized camera data file.");
      }
    }
  }

  void on_shutdown() override {
  }
};

OTHER_DRIVER(serialization_sandbox)