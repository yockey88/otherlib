/**
 * \file tests/serialization/scene_document_tests.cpp
 *
 * document-level scene persistence: the field codec, component codecs (payload<->toml),
 * and the .oscn/.oscnb readers/writers — no live scene, no engine boot beyond minimal.
 **/
#include <gtest/gtest.h>

#include "other_test.hpp"

#include "object/grid_component.hpp"
#include "object/transform.hpp"
#include "serialization/component_codec.hpp"
#include "serialization/scene_document.hpp"
#include "serialization/scene_field_codec.hpp"
#include "serialization/scene_serializer.hpp"

namespace other {

  class scene_document_tests : public other_test {};

  using namespace serialization;
  namespace fc = serialization::field_codec;

  TEST_F(scene_document_tests, field_codec_scalar_roundtrip) {
    ostd::vector<uint8_t> bytes = {};
    fc::encode_value(true, bytes);
    fc::encode_value(uint32_t{ 42 }, bytes);
    fc::encode_value(3.25f, bytes);
    fc::encode_value(std::string{ "hello" }, bytes);
    fc::encode_value(glm::vec3{ 1.f, 2.f, 3.f }, bytes);
    fc::encode_value(glm::quat{ 0.5f, 0.5f, 0.5f, 0.5f }, bytes);

    size_t offset = 0;
    bool b = false;
    uint32_t u = 0;
    float f = 0.f;
    std::string s = "";
    glm::vec3 v = {};
    glm::quat q = {};
    EXPECT_TRUE(fc::decode_value(bytes, offset, b));
    EXPECT_TRUE(fc::decode_value(bytes, offset, u));
    EXPECT_TRUE(fc::decode_value(bytes, offset, f));
    EXPECT_TRUE(fc::decode_value(bytes, offset, s));
    EXPECT_TRUE(fc::decode_value(bytes, offset, v));
    EXPECT_TRUE(fc::decode_value(bytes, offset, q));
    EXPECT_EQ(offset, bytes.size());

    EXPECT_EQ(b, true);
    EXPECT_EQ(u, 42u);
    EXPECT_FLOAT_EQ(f, 3.25f);
    EXPECT_EQ(s, "hello");
    EXPECT_EQ(v, glm::vec3(1.f, 2.f, 3.f));
    EXPECT_EQ(q.w, 0.5f);
    EXPECT_EQ(q.x, 0.5f);
  }

  TEST_F(scene_document_tests, field_codec_container_and_reflected_roundtrip) {
    ostd::vector<uint8_t> bytes = {};
    const std::vector<std::string> names = { "Simulation", "SimulationView" };
    fc::encode_value(names, bytes);

    grid_component grid = {};
    grid.coordinate_system = GRID_COORDINATES_CYLINDRICAL;
    grid.extent = 40;
    grid.layer_spacing = 5.f;
    grid.line_color = glm::vec4(0.1f, 0.2f, 0.3f, 1.f);
    fc::encode_value(grid, bytes);

    size_t offset = 0;
    std::vector<std::string> decoded_names = {};
    EXPECT_TRUE(fc::decode_value(bytes, offset, decoded_names));
    EXPECT_EQ(decoded_names, names);

    grid_component decoded_grid = {};
    EXPECT_TRUE(fc::decode_value(bytes, offset, decoded_grid));
    EXPECT_EQ(decoded_grid.coordinate_system, (uint32_t)GRID_COORDINATES_CYLINDRICAL);
    EXPECT_EQ(decoded_grid.extent, 40u);
    EXPECT_FLOAT_EQ(decoded_grid.layer_spacing, 5.f);
    EXPECT_EQ(decoded_grid.line_color, glm::vec4(0.1f, 0.2f, 0.3f, 1.f));
    EXPECT_EQ(offset, bytes.size());
  }

  TEST_F(scene_document_tests, unknown_fields_skip_forward) {
    /// stream = one unknown field, then a real grid field: the reader must skip the
    ///  unknown frame and still apply the known one
    ostd::vector<uint8_t> bytes = {};
    fc::write_raw<natural_t>(FNV(std::string{ "field_from_the_future" }), bytes);
    fc::encode_value(std::string{ "??" }, bytes);
    fc::write_raw<natural_t>(FNV(std::string{ "extent" }), bytes);
    fc::encode_value(uint32_t{ 99 }, bytes);

    grid_component grid = {};
    ostd::vector<std::string> warnings = {};
    EXPECT_TRUE(fc::decode_reflected(std::span<const uint8_t>(bytes.data(), bytes.size()), grid, &warnings));
    EXPECT_EQ(grid.extent, 99u);
    EXPECT_TRUE(warnings.empty());
  }

  TEST_F(scene_document_tests, mismatched_field_encoding_warns_and_continues) {
    /// "extent" carrying a string body must not abort the remaining fields
    ostd::vector<uint8_t> bytes = {};
    fc::write_raw<natural_t>(FNV(std::string{ "extent" }), bytes);
    fc::encode_value(std::string{ "not a number" }, bytes);
    fc::write_raw<natural_t>(FNV(std::string{ "sector_count" }), bytes);
    fc::encode_value(uint32_t{ 24 }, bytes);

    grid_component grid = {};
    ostd::vector<std::string> warnings = {};
    EXPECT_TRUE(fc::decode_reflected(std::span<const uint8_t>(bytes.data(), bytes.size()), grid, &warnings));
    EXPECT_EQ(grid.extent, grid_component{}.extent);  /// untouched
    EXPECT_EQ(grid.sector_count, 24u);
    EXPECT_EQ(warnings.size(), 1u);
  }

  namespace {

    scene_document build_test_document() {
      const component_codec* transform_codec = find_component_codec("transform");
      const component_codec* grid_codec = find_component_codec("grid");
      const component_codec* script_codec = find_component_codec("script");
      OTHER_ASSERT(transform_codec != nullptr && grid_codec != nullptr && script_codec != nullptr, "builtin codecs missing");

      ostd::vector<std::string> warnings = {};
      const toml::table transform_fields = toml::parse(R"(
        local_position = [1.0, 2.0, 3.0]
        local_scale = [2.0, 2.0, 2.0]
        local_rotation_quat = [1.0, 0.0, 0.0, 0.0]
      )");
      const toml::table grid_fields = toml::parse(R"(
        coordinate_system = 3
        extent = 40
        layer_spacing = 5.0
      )");
      const toml::table script_fields = toml::parse(R"(
        behaviors = ["Simulation", "SimulationView"]
      )");

      scene_document doc = {};
      doc.name = "doc-test";
      doc.script = "hooks.lua";
      doc.clear_color = glm::vec4(0.1f, 0.2f, 0.3f, 1.f);

      object_record parent = {};
      parent.file_id = 7;
      parent.name = "Parent";
      parent.tags = { "main-camera", "sun" };
      parent.components.push_back({ .key_hash = transform_codec->key_hash, .payload = transform_codec->payload_from_toml(transform_fields, warnings) });
      parent.components.push_back({ .key_hash = script_codec->key_hash, .payload = script_codec->payload_from_toml(script_fields, warnings) });
      doc.objects.push_back(std::move(parent));

      object_record child = {};
      child.file_id = 9;
      child.parent_file_id = 7;
      child.name = "Child";
      child.visible = false;
      child.components.push_back({ .key_hash = grid_codec->key_hash, .payload = grid_codec->payload_from_toml(grid_fields, warnings) });
      doc.objects.push_back(std::move(child));

      OTHER_ASSERT(warnings.empty(), "test document construction produced warnings");
      return doc;
    }

    void expect_documents_equal(const scene_document& a, const scene_document& b) {
      EXPECT_EQ(a.schema_version, b.schema_version);
      EXPECT_EQ(a.name, b.name);
      EXPECT_EQ(a.script, b.script);
      EXPECT_EQ(a.clear_color, b.clear_color);
      ASSERT_EQ(a.objects.size(), b.objects.size());
      for (size_t i = 0; i < a.objects.size(); ++i) {
        const object_record& oa = a.objects[i];
        const object_record& ob = b.objects[i];
        EXPECT_EQ(oa.file_id, ob.file_id) << "object " << i;
        EXPECT_EQ(oa.parent_file_id, ob.parent_file_id) << "object " << i;
        EXPECT_EQ(oa.name, ob.name) << "object " << i;
        EXPECT_EQ(oa.visible, ob.visible) << "object " << i;
        EXPECT_EQ(oa.tags, ob.tags) << "object " << i;
        ASSERT_EQ(oa.components.size(), ob.components.size()) << "object " << i;
        for (size_t c = 0; c < oa.components.size(); ++c) {
          EXPECT_EQ(oa.components[c].key_hash, ob.components[c].key_hash) << "object " << i << " component " << c;
          EXPECT_EQ(oa.components[c].payload, ob.components[c].payload) << "object " << i << " component " << c;
        }
      }
    }

  }  // namespace

  TEST_F(scene_document_tests, binary_roundtrip) {
    const scene_document doc = build_test_document();
    const ostd::vector<uint8_t> bytes = write_scene_binary(doc);

    scene_parse_result parsed = parse_scene_binary(bytes);
    ASSERT_TRUE(parsed.success()) << parsed.error;
    EXPECT_TRUE(parsed.warnings.empty());
    expect_documents_equal(doc, *parsed.document);
  }

  TEST_F(scene_document_tests, toml_roundtrip) {
    const scene_document doc = build_test_document();
    const std::string text = write_scene_toml(doc);

    scene_parse_result parsed = parse_scene_toml(text);
    ASSERT_TRUE(parsed.success()) << parsed.error;
    EXPECT_TRUE(parsed.warnings.empty()) << parsed.warnings.front();
    expect_documents_equal(doc, *parsed.document);
  }

  TEST_F(scene_document_tests, binary_rejects_garbage) {
    const std::array<uint8_t, 6> garbage = { 'N', 'O', 'P', 'E', 0, 0 };
    EXPECT_FALSE(parse_scene_binary(garbage).success());

    /// a valid document truncated mid-stream must fail, never crash
    const ostd::vector<uint8_t> bytes = write_scene_binary(build_test_document());
    for (const size_t cut : { bytes.size() / 4, bytes.size() / 2, bytes.size() - 1 }) {
      EXPECT_FALSE(parse_scene_binary(std::span<const uint8_t>(bytes.data(), cut)).success()) << "cut at " << cut;
    }
  }

  TEST_F(scene_document_tests, toml_structural_errors) {
    EXPECT_FALSE(parse_scene_toml("this is not toml [").success());
    EXPECT_FALSE(parse_scene_toml("[not-a-scene]\nx = 1").success());

    /// child before parent violates the parents-precede-children contract
    constexpr std::string_view kOrphan = R"([scene]
name = "x"

[[objects]]
id = 2
parent = 7
name = "Child"
)";
    EXPECT_FALSE(parse_scene_toml(kOrphan).success());

    /// unknown component tables warn but do not fail
    constexpr std::string_view kUnknownComponent = R"([scene]
name = "x"

[[objects]]
id = 1
name = "A"

[objects.components.wobbulator]
strength = 11
)";
    scene_parse_result parsed = parse_scene_toml(kUnknownComponent);
    ASSERT_TRUE(parsed.success()) << parsed.error;
    EXPECT_EQ(parsed.warnings.size(), 1u);
    EXPECT_TRUE(parsed.document->objects.front().components.empty());
  }

  TEST_F(scene_document_tests, toml_ids_optional_for_hand_authoring) {
    constexpr std::string_view kNoIds = R"([scene]
name = "x"

[[objects]]
name = "A"

[[objects]]
name = "B"
)";
    scene_parse_result parsed = parse_scene_toml(kNoIds);
    ASSERT_TRUE(parsed.success()) << parsed.error;
    ASSERT_EQ(parsed.document->objects.size(), 2u);
    EXPECT_NE(parsed.document->objects[0].file_id, 0u);
    EXPECT_NE(parsed.document->objects[1].file_id, 0u);
    EXPECT_NE(parsed.document->objects[0].file_id, parsed.document->objects[1].file_id);
  }

  TEST_F(scene_document_tests, transform_payload_to_toml_emits_member_names) {
    const component_codec* transform_codec = find_component_codec("transform");
    ASSERT_NE(transform_codec, nullptr);

    ostd::vector<std::string> warnings = {};
    const toml::table fields = toml::parse("local_position = [4.0, 5.0, 6.0]");
    const ostd::vector<uint8_t> payload = transform_codec->payload_from_toml(fields, warnings);

    toml_writer w;
    w.table("t");
    transform_codec->payload_to_toml(payload, w, "t");
    const std::string text = w.str();

    const toml::table parsed = toml::parse(text);
    const toml::array* position = parsed.at_path("t.local_position").as_array();
    ASSERT_NE(position, nullptr);
    EXPECT_DOUBLE_EQ(position->get(0)->value_or<double>(0.0), 4.0);
    EXPECT_DOUBLE_EQ(position->get(1)->value_or<double>(0.0), 5.0);
    EXPECT_DOUBLE_EQ(position->get(2)->value_or<double>(0.0), 6.0);
  }

}  // namespace other
