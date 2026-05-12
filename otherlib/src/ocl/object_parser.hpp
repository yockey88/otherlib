/**
 * \file ocl/object_parser.hpp
 **/
#ifndef OTHERLIB_OCL_OBJECT_PARSER_HPP
#define OTHERLIB_OCL_OBJECT_PARSER_HPP

#include <string>

#include "input/input_action.hpp"

#include "renderer/pipeline_definition.hpp"

#include "vm/command_files/lexer.hpp"

namespace other {

  struct ocl_object_declaration {
    std::string object_type;
    std::string object_name;
  };

  struct ocl_object_definition {
    using property_value = std::pair<std::string, std::string>;
    std::vector<property_value> properties;
  };

  struct ocl_object {
    ocl_object_declaration declaration;
    ocl_object_definition definition;
  };

  ocl_object_declaration parse_ocl_object(const std::string_view source);

}  // namespace other

#endif  // OTHERLIB_OCL_OBJECT_PARSER_HPP