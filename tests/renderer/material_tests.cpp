/**
 * \file tests/renderer/material_tests.cpp
 *
 * contract: .omat parsing is pure cpu + error-result based (never asserts); material_layout
 *  computes std430 offsets/element size, and packing folds defaults, values, warnings, tints
 **/
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "gpu_resource/material.hpp"
#include "renderer/material_layout.hpp"
#include "renderer/pipeline_definition.hpp"

#include "other_test.hpp"

namespace other {

  class material_tests : public other_test {
   protected:
    filepath temp_dir;

    void SetUp() override {
      other_test::SetUp();
      temp_dir = std::filesystem::temp_directory_path() / "other-material-tests";
      std::filesystem::remove_all(temp_dir);
      std::filesystem::create_directories(temp_dir);
    }

    void TearDown() override {
      other_test::TearDown();
      std::error_code ec;
      std::filesystem::remove_all(temp_dir, ec);
    }

    filepath write_file(const std::string_view name, const std::string_view contents) const {
      const filepath path = temp_dir / name;
      std::ofstream out(path);
      out << contents;
      return path;
    }

    /// the standard layout as default-pipeline.toml declares it
    static material_layout standard_layout() {
      material_layout layout;
      layout.params = {
        { .name = "base_color", .kind = material_value::kind::VEC4, .default_value = material_value::from(glm::vec4(1.f)) },
        { .name = "emissive_color", .kind = material_value::kind::VEC3, .default_value = material_value::from(glm::vec3(0.f)) },
        { .name = "roughness", .kind = material_value::kind::F32, .default_value = material_value::from(1.f) },
        { .name = "metalness", .kind = material_value::kind::F32, .default_value = material_value::from(0.f) },
      };
      layout.texture_slots = {
        { .name = "base_color", .uniform = "OE_mat_base_color_map", .unit = 8 },
        { .name = "normal", .uniform = "OE_mat_normal_map", .unit = 9 },
      };
      layout.finalize();
      return layout;
    }

    static float float_at(std::span<const uint8_t> blob, uint32_t offset) {
      float v = 0.f;
      std::memcpy(&v, blob.data() + offset, sizeof(float));
      return v;
    }
  };

  TEST_F(material_tests, parse_omat_round_trip) {
    const filepath path = write_file("hull-plating.omat",
                                     "asset-type = \"material\"\n"
                                     "name = \"hull-plating\"\n"
                                     "\n"
                                     "[params]\n"
                                     "base_color = [0.82, 0.85, 0.9, 1.0]\n"
                                     "roughness = 0.35\n"
                                     "sector = 3\n"
                                     "glow = true\n"
                                     "offset = [0.5, 0.25]\n"
                                     "\n"
                                     "[textures]\n"
                                     "base_color = \"../textures/hull_albedo.png\"\n"
                                     "normal = \"../textures/hull_n.png\"\n");

    const material_parse_result result = parse_material_toml(path);
    ASSERT_TRUE(result.success()) << result.error;
    EXPECT_TRUE(result.warnings.empty());

    const material& mat = *result.mat;
    EXPECT_EQ(mat.name, "hull-plating");
    ASSERT_EQ(mat.params.size(), 5u);

    const material_value& base_color = mat.params.at(FNV("base_color"));
    EXPECT_EQ(base_color.value_kind, material_value::kind::VEC4);
    EXPECT_FLOAT_EQ(base_color.data.x, 0.82f);
    EXPECT_FLOAT_EQ(base_color.data.w, 1.f);

    EXPECT_EQ(mat.params.at(FNV("roughness")).value_kind, material_value::kind::F32);
    EXPECT_FLOAT_EQ(mat.params.at(FNV("roughness")).data.x, 0.35f);
    EXPECT_EQ(mat.params.at(FNV("sector")).value_kind, material_value::kind::I32);
    EXPECT_EQ(mat.params.at(FNV("glow")).value_kind, material_value::kind::B32);
    EXPECT_EQ(mat.params.at(FNV("offset")).value_kind, material_value::kind::VEC2);

    ASSERT_EQ(mat.texture_paths.size(), 2u);
    EXPECT_EQ(mat.texture_paths.at(FNV("base_color")), "../textures/hull_albedo.png");
    EXPECT_EQ(mat.texture_paths.at(FNV("normal")), "../textures/hull_n.png");
    /// slot hashes resolve at asset load, not parse — the parser is engine-free
    EXPECT_TRUE(mat.texture_hashes.empty());
  }

  TEST_F(material_tests, parse_malformed_errors_not_aborts) {
    const filepath path = write_file("broken.omat", "this is = { not [ valid toml");
    const material_parse_result broken = parse_material_toml(path);
    EXPECT_FALSE(broken.success());
    EXPECT_FALSE(broken.error.empty());

    const material_parse_result missing = parse_material_toml(temp_dir / "does-not-exist.omat");
    EXPECT_FALSE(missing.success());
    EXPECT_FALSE(missing.error.empty());

    /// unsupported value shapes are per-param warnings, not load failures
    const filepath odd = write_file("odd.omat",
                                    "[params]\n"
                                    "good = 1.0\n"
                                    "bad = \"a string is not a param value\"\n");
    const material_parse_result partial = parse_material_toml(odd);
    ASSERT_TRUE(partial.success()) << partial.error;
    EXPECT_EQ(partial.mat->params.size(), 1u);
    ASSERT_EQ(partial.warnings.size(), 1u);
    EXPECT_NE(partial.warnings[0].find("bad"), std::string::npos);
  }

  TEST_F(material_tests, layout_std430_offsets) {
    const material_layout layout = standard_layout();

    /// vec4 @0, vec3 aligns to 16, roughness packs into the vec3 pad @28, metalness @32,
    /// struct rounds to the max member alignment -> 48-byte stride
    ASSERT_EQ(layout.params.size(), 4u);
    EXPECT_EQ(layout.params[0].offset, 0u);
    EXPECT_EQ(layout.params[1].offset, 16u);
    EXPECT_EQ(layout.params[2].offset, 28u);
    EXPECT_EQ(layout.params[3].offset, 32u);
    EXPECT_EQ(layout.element_size, 48u);
    ASSERT_TRUE(layout.base_color_offset.has_value());
    EXPECT_EQ(*layout.base_color_offset, 0u);

    /// scalars pack to 4, vec2 aligns to 8
    material_layout mixed;
    mixed.params = {
      { .name = "a", .kind = material_value::kind::F32, .default_value = material_value::from(0.f) },
      { .name = "b", .kind = material_value::kind::VEC2, .default_value = material_value::from(glm::vec2(0.f)) },
      { .name = "c", .kind = material_value::kind::F32, .default_value = material_value::from(0.f) },
    };
    mixed.finalize();
    EXPECT_EQ(mixed.params[0].offset, 0u);
    EXPECT_EQ(mixed.params[1].offset, 8u);
    EXPECT_EQ(mixed.params[2].offset, 16u);
    EXPECT_EQ(mixed.element_size, 24u);
    EXPECT_FALSE(mixed.base_color_offset.has_value());
  }

  TEST_F(material_tests, pack_defaults_then_values) {
    const material_layout layout = standard_layout();
    ostd::vector<uint8_t> blob(layout.element_size);

    layout.pack(nullptr, blob);
    EXPECT_FLOAT_EQ(float_at(blob, 0), 1.f);    // base_color.r default
    EXPECT_FLOAT_EQ(float_at(blob, 16), 0.f);   // emissive.r default
    EXPECT_FLOAT_EQ(float_at(blob, 28), 1.f);   // roughness default
    EXPECT_FLOAT_EQ(float_at(blob, 32), 0.f);   // metalness default

    material mat;
    mat.params[FNV("base_color")] = material_value::from(glm::vec4(0.25f, 0.5f, 0.75f, 1.f));
    mat.params[FNV("roughness")] = material_value::from(0.35f);
    layout.pack(&mat, blob);
    EXPECT_FLOAT_EQ(float_at(blob, 0), 0.25f);
    EXPECT_FLOAT_EQ(float_at(blob, 4), 0.5f);
    EXPECT_FLOAT_EQ(float_at(blob, 28), 0.35f);
    /// params the material doesn't set keep the layout defaults
    EXPECT_FLOAT_EQ(float_at(blob, 32), 0.f);
  }

  TEST_F(material_tests, pack_unknown_param_warns_once) {
    const material_layout layout = standard_layout();
    ostd::vector<uint8_t> blob(layout.element_size);

    material mat;
    mat.params[FNV("sparkle")] = material_value::from(1.f);
    mat.param_names[FNV("sparkle")] = "sparkle";
    /// kind mismatch is skipped-with-warning too: the layout default wins
    mat.params[FNV("roughness")] = material_value::from(glm::vec3(1.f));
    mat.param_names[FNV("roughness")] = "roughness";

    ostd::vector<std::string> warned;
    layout.pack(&mat, blob, [&](std::string_view param, std::string_view) { warned.emplace_back(param); });

    ASSERT_EQ(warned.size(), 2u);
    EXPECT_TRUE(std::ranges::contains(warned, "sparkle"));
    EXPECT_TRUE(std::ranges::contains(warned, "roughness"));
    EXPECT_FLOAT_EQ(float_at(blob, 28), 1.f);  // roughness stayed default
  }

  TEST_F(material_tests, fold_base_color_tint) {
    const material_layout layout = standard_layout();
    ostd::vector<uint8_t> blob(layout.element_size);
    layout.pack(nullptr, blob);

    layout.fold_base_color_tint(blob, glm::vec4(0.5f, 0.25f, 1.f, 1.f));
    EXPECT_FLOAT_EQ(float_at(blob, 0), 0.5f);
    EXPECT_FLOAT_EQ(float_at(blob, 4), 0.25f);
    EXPECT_FLOAT_EQ(float_at(blob, 8), 1.f);
    /// non-base_color params never fold
    EXPECT_FLOAT_EQ(float_at(blob, 28), 1.f);
  }

  TEST_F(material_tests, default_pipeline_layout_derivation) {
    /// the shipped pipeline TOML is the ABI anchor: its layout must derive the element size the
    ///  GLSL block was written against (48 std430) and size the per-draw binding from it
    const pipeline_definition def = read_pipeline_definition_from_file(filepath{ "resources/editor-assets/default-pipeline.toml" });
    ASSERT_TRUE(def.materials.has_value());
    EXPECT_EQ(def.materials->element_size, 48u);
    EXPECT_EQ(def.materials->instance_capacity, 100u);
    ASSERT_EQ(def.materials->texture_slots.size(), 2u);
    EXPECT_EQ(def.materials->texture_slots[0].unit, 8u);
    EXPECT_EQ(def.materials->texture_slots[1].unit, 9u);

    const auto pass = std::ranges::find(def.passes, "geometry-pass", &pipeline_pass_definition::name);
    ASSERT_NE(pass, def.passes.end());
    const auto binding = std::ranges::find_if(pass->bindings, [](const frame_binding_definition& bd) {
      return bd.tag.value() == resource_tag::kMaterialTag;
    });
    ASSERT_NE(binding, pass->bindings.end());
    EXPECT_EQ(binding->element_size, def.materials->element_size * def.materials->instance_capacity);
  }

}  // namespace other
