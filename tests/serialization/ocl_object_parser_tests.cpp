/**
 * \file ocl_object_parser_tests.cpp
 **/
#include "ocl/object_parser.hpp"
#include "ocl_tests.hpp"

namespace other {
  namespace {

    static const std::string kSimpleInput1 =
      R"(
    object pipeline my_pipeline
    begin
      name = "My Pipeline"
    end
    )";

    static const std::string kSimpleInput1NoName =
      R"(
    object pipeline 
    begin
      name = "My Pipeline"
    end
    )";

    static const std::string kSimpleInput2 =
      R"(
  object pipeline my_pipeline
  begin
    name = "My Pipeline"
    buffers = [
      buffer camera_buffer          : uniform_buffer dynamic @camera
      buffer model_buffer           : uniform_buffer dynamic @vertex
      buffer bone_buffer            : uniform_buffer dynamic @bone
      buffer point_light_buffer     : storage_buffer dynamic @point_light
      buffer direction_light_buffer : storage_buffer dynamic @direction_light
    ]
  end
    )";

  }  // namespace

  TEST_F(ocl_tests, parse_simple_object) {
    ocl_object_declaration expected_desc;
    expected_desc.object_type = "pipeline";
    expected_desc.object_name = "my_pipeline";

    ocl_object_declaration desc = parse_ocl_object(kSimpleInput1);
    EXPECT_EQ(desc.object_type, expected_desc.object_type);
    EXPECT_EQ(desc.object_name, expected_desc.object_name);
    // EXPECT_EQ(desc.body_tokens.size(), 3);
  }

  // TEST_F(ocl_tests, parse_simple_object2) {
  //   ocl_object_declaration expected_desc;
  //   expected_desc.object_type = "pipeline";
  //   expected_desc.object_name = "object_" + std::to_string(std::hash<std::string_view>{}(kSimpleInput1NoName));

  //   ocl_object_declaration desc = parse_ocl_object(kSimpleInput1NoName);
  //   EXPECT_EQ(desc.object_type, expected_desc.object_type);
  //   EXPECT_EQ(desc.object_name, expected_desc.object_name);
  //   EXPECT_EQ(desc.body_tokens.size(), 3);
  // }

  // TEST_F(ocl_tests, parse_pipeline_object) {
  //   ocl_object_declaration expected_desc;
  //   expected_desc.object_type = "pipeline";
  //   expected_desc.object_name = "my_pipeline";

  //   ocl_object_declaration desc = parse_ocl_object(kSimpleInput2);
  //   EXPECT_EQ(desc.object_type, expected_desc.object_type);
  //   EXPECT_EQ(desc.object_name, expected_desc.object_name);
  // }

}  // namespace other