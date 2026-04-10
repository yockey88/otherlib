/**
 * \file ocl/object_parser.cpp
 **/
#include "ocl/object_parser.hpp"

#include <ranges>
#include <string_view>

#include "core/defines.hpp"
#include "core/logger.hpp"

#include "renderer/pipeline_definition.hpp"

#include "vm/command_files/token.hpp"

namespace other {
  namespace {

    bool is_primary_type(token_type type) {
      return type == TOKEN_TYPE_KW_ASSET ||
        type == TOKEN_TYPE_KW_OBJECT;
    }

    bool is_object_type_kw(token_type type) {
      return type == TOKEN_TYPE_KW_SCENE ||
        type == TOKEN_TYPE_KW_MODEL_SOURCE ||
        type == TOKEN_TYPE_KW_MODEL ||
        type == TOKEN_TYPE_KW_ANIMATION ||
        type == TOKEN_TYPE_KW_SCRIPT_SOURCE ||
        type == TOKEN_TYPE_KW_SCRIPT ||
        type == TOKEN_TYPE_KW_AUDIO ||
        type == TOKEN_TYPE_KW_SCENE_OBJECT ||
        type == TOKEN_TYPE_KW_INPUT_MAP ||
        type == TOKEN_TYPE_KW_PIPELINE;
    }

    bool tokenize_and_validate(const std::string_view input, std::vector<token>& out_tokens);
    std::pair<std::vector<token>, std::vector<token>> split_object_desc_and_body(const std::vector<token>& tokens);

  }  // namespace

  ocl_object_declaration parse_ocl_object(const std::string_view source) {
    std::vector<token> tokens = {};
    if (!tokenize_and_validate(source, tokens)) {
      CORE_LOG_ERROR("Failed to parse object: tokenization failed");
      return {};
    }
    CORE_LOG_DEBUG("Tokenization successful, source validated");

    auto [desc_tokens, body_tokens] = split_object_desc_and_body(tokens);
    CORE_LOG_TRACE("Object description and body split into {} and {} tokens", desc_tokens.size(), body_tokens.size());

    token pimary_type_token = desc_tokens[0];
    token secondary_type_token = desc_tokens[1];
    CORE_LOG_TRACE("  - primary token: {}", pimary_type_token.text);
    CORE_LOG_TRACE("  - secondary token: {}", secondary_type_token.text);

    token name = {};
    bool named = desc_tokens.size() == 3;
    if (named) {
      if (desc_tokens[2].type != TOKEN_TYPE_IDENTIFIER) {
        CORE_LOG_ERROR("Failed to parse object: expected identifier for object name");
        return {};
      }
      name = desc_tokens[2];
    } else {
      name = token(TOKEN_TYPE_IDENTIFIER, "object_" + std::to_string(std::hash<std::string_view>{}(source)), pimary_type_token.line_number, pimary_type_token.column_number);
    }
    CORE_LOG_TRACE("  - object name: {}", name.text);

    std::vector<token> type_body_tokens = body_tokens;
    switch (secondary_type_token.type) {
      case TOKEN_TYPE_KW_SCENE: break;
      case TOKEN_TYPE_KW_MODEL_SOURCE: break;
      case TOKEN_TYPE_KW_MODEL: break;
      case TOKEN_TYPE_KW_ANIMATION: break;
      case TOKEN_TYPE_KW_SCRIPT_SOURCE: break;
      case TOKEN_TYPE_KW_SCRIPT: break;
      case TOKEN_TYPE_KW_AUDIO: break;
      case TOKEN_TYPE_KW_SCENE_OBJECT: break;
      case TOKEN_TYPE_KW_INPUT_MAP: break;
      case TOKEN_TYPE_KW_PIPELINE: break;
      default:
        CORE_LOG_ERROR("Failed to parse object: unexpected object type");
        return {};
    }

    return {
      .object_type = secondary_type_token.text,
      .object_name = name.text,
      // .body_tokens = body_tokens,
    };
  }

  pipeline_definition parse_pipeline_body(const std::vector<token>& body_tokens) {
    /**
     * grammar of pipeline body:
     *
     **/
    return {};
  }

  pipeline_pass_definition parse_pipeline_pass_body(const std::vector<token>& tokens) {
    return {};
  }

  namespace {

    bool tokenize_and_validate(const std::string_view input, std::vector<token>& out_tokens) {
      auto tokens = ocmd_lexer(input).tokenize();

      /// object <type> <name>? begin <body> end
      /// so minimum tokens is 4: object <type> begin end
      if (tokens.size() < 4) {
        CORE_LOG_ERROR("Failed to parse object: not enough tokens");
        return false;
      }

      if (!is_primary_type(tokens[0].type)) {
        CORE_LOG_ERROR("Failed to parse object: expected 'object' keyword");
        return false;
      }

      if (!is_object_type_kw(tokens[1].type)) {
        CORE_LOG_ERROR("Failed to parse object: expected object type keyword");
        return false;
      }

      bool nameless = tokens[2].type == TOKEN_TYPE_KW_BEGIN;
      if (!nameless && (tokens[2].type != TOKEN_TYPE_IDENTIFIER && tokens[3].type != TOKEN_TYPE_KW_BEGIN)) {
        CORE_LOG_ERROR("Failed to parse object: expected identifier and 'begin' keyword");
        return false;
      }

      auto contains_end = std::ranges::find_if(tokens, [](const token& t) { return t.type == TOKEN_TYPE_KW_END; });
      if (contains_end == tokens.end()) {
        CORE_LOG_ERROR("Failed to parse object: expected 'end' keyword");
        return false;
      }

      out_tokens = std::move(tokens);
      return true;
    }

    std::pair<std::vector<token>, std::vector<token>> split_object_desc_and_body(const std::vector<token>& tokens) {
      auto desc_tokens =
        tokens |
        std::views::take_while([](const token& t) { return t.type != TOKEN_TYPE_KW_BEGIN; }) |
        std::ranges::to<std::vector>();

      auto body_tokens =
        tokens |
        std::views::drop_while([](const token& t) { return t.type != TOKEN_TYPE_KW_BEGIN; }) |
        std::views::drop(1) |
        std::views::take_while([](const token& t) { return t.type != TOKEN_TYPE_KW_END; }) |
        std::ranges::to<std::vector>();

      return { desc_tokens, body_tokens };
    }

  }  // namespace
}  // namespace other