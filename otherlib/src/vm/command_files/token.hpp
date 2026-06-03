/**
 * \file vm/command_files/token.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_TOKEN_HPP
#define OTHERLIB_VM_COMMAND_FILES_TOKEN_HPP

#include <array>
#include <string>

#include "vm/diagnostics/vm_diagnostic.hpp"

namespace other {

  constexpr static inline std::array kOperators = {
    '+', '-', '*', '/',
    '=', '!', '<', '>', '&', '|',
    '^', '%', '$', '#', '@'
  };

  constexpr static inline std::array kPunctuation = {
    '(', ')', '{', '}', '[', ']', ';', ':', ',', '.', '"', '\''
  };

  enum token_type {
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_INTEGER_LITERAL,
    TOKEN_TYPE_FLOATING_POINT_LITERAL,
    TOKEN_TYPE_HEX_LITERAL,
    TOKEN_TYPE_STRING_LITERAL,

    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_ASTERISK,
    TOKEN_TYPE_SLASH,

    TOKEN_TYPE_EQUAL,
    TOKEN_TYPE_EQUAL_EQUAL,

    TOKEN_TYPE_BANG,

    TOKEN_TYPE_LESS,
    TOKEN_TYPE_LESS_EQUAL,

    TOKEN_TYPE_GREATER,
    TOKEN_TYPE_GREATER_EQUAL,

    TOKEN_TYPE_LOGICAL_AND,
    TOKEN_TYPE_LOGICAL_OR,

    TOKEN_TYPE_PERCENT,
    TOKEN_TYPE_CARET,

    TOKEN_TYPE_DOLLAR,
    TOKEN_TYPE_HASH,
    TOKEN_TYPE_AT,

    TOKEN_TYPE_LEFT_PAREN,
    TOKEN_TYPE_RIGHT_PAREN,
    TOKEN_TYPE_LEFT_BRACE,
    TOKEN_TYPE_RIGHT_BRACE,
    TOKEN_TYPE_LEFT_BRACKET,
    TOKEN_TYPE_RIGHT_BRACKET,

    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_DOT,
    TOKEN_TYPE_DOUBLE_QUOTE,
    TOKEN_TYPE_SINGLE_QUOTE,

    TOKEN_TYPE_KW_I8_TYPE,
    TOKEN_TYPE_KW_U8_TYPE,
    TOKEN_TYPE_KW_I16_TYPE,
    TOKEN_TYPE_KW_U16_TYPE,
    TOKEN_TYPE_KW_I32_TYPE,
    TOKEN_TYPE_KW_U32_TYPE,
    TOKEN_TYPE_KW_I64_TYPE,
    TOKEN_TYPE_KW_U64_TYPE,
    TOKEN_TYPE_KW_F32_TYPE,
    TOKEN_TYPE_KW_F64_TYPE,
    TOKEN_TYPE_KW_STRING_TYPE,
    TOKEN_TYPE_KW_ADDRESS_TYPE,
    TOKEN_TYPE_KW_BLOB_TYPE,
    TOKEN_TYPE_KW_USER_DEFINED_TYPE,

    // 0 table
    TOKEN_TYPE_KW_STOPDEV,
    TOKEN_TYPE_KW_DUMP,
    // 1 table
    TOKEN_TYPE_KW_WRITE,
    TOKEN_TYPE_KW_SET,
    TOKEN_TYPE_KW_CMP,
    TOKEN_TYPE_KW_CMPGT,
    TOKEN_TYPE_KW_CMPLT,
    TOKEN_TYPE_KW_AND,
    TOKEN_TYPE_KW_OR,
    TOKEN_TYPE_KW_XOR,
    TOKEN_TYPE_KW_LSHIFT,
    TOKEN_TYPE_KW_RSHIFT,
    // 2 table
    TOKEN_TYPE_KW_GOTO,
    TOKEN_TYPE_KW_JE,
    TOKEN_TYPE_KW_JNE,
    TOKEN_TYPE_KW_CALL,
    TOKEN_TYPE_KW_RET,
    TOKEN_TYPE_KW_SYSCALL,
    TOKEN_TYPE_KW_INVOKE,
    // 3 table
    TOKEN_TYPE_KW_ADD,
    TOKEN_TYPE_KW_SUB,
    TOKEN_TYPE_KW_MUL,
    TOKEN_TYPE_KW_DIV,
    TOKEN_TYPE_KW_MOD,

    TOKEN_TYPE_KW_R0,
    TOKEN_TYPE_KW_R1,
    TOKEN_TYPE_KW_R2,
    TOKEN_TYPE_KW_R3,
    TOKEN_TYPE_KW_R4,
    TOKEN_TYPE_KW_R5,
    TOKEN_TYPE_KW_R6,
    TOKEN_TYPE_KW_R7,
    TOKEN_TYPE_KW_R8,
    TOKEN_TYPE_KW_R9,
    TOKEN_TYPE_KW_RA,
    TOKEN_TYPE_KW_RB,
    TOKEN_TYPE_KW_RC,
    TOKEN_TYPE_KW_RD,
    TOKEN_TYPE_KW_RE,
    TOKEN_TYPE_KW_RF,
    TOKEN_TYPE_KW_RFLAG,

    TOKEN_TYPE_KW_BEGIN,
    TOKEN_TYPE_KW_END,
    TOKEN_TYPE_KW_OBJECT,
    TOKEN_TYPE_KW_ASSET,
    TOKEN_TYPE_KW_SCENE,
    TOKEN_TYPE_KW_MODEL_SOURCE,
    TOKEN_TYPE_KW_MODEL,
    TOKEN_TYPE_KW_ANIMATION,
    TOKEN_TYPE_KW_SCRIPT_SOURCE,
    TOKEN_TYPE_KW_SCRIPT,
    TOKEN_TYPE_KW_AUDIO,
    TOKEN_TYPE_KW_SCENE_OBJECT,
    TOKEN_TYPE_KW_INPUT_MAP,
    TOKEN_TYPE_KW_PIPELINE,
    TOKEN_TYPE_KW_PASS,
    TOKEN_TYPE_KW_SHADER,
    TOKEN_TYPE_KW_TEXTURE,
    TOKEN_TYPE_KW_BUFFER,
    TOKEN_TYPE_KW_TAG,
    TOKEN_TYPE_KW_DATA,

    TOKEN_TYPE_ADDRESS,
    TOKEN_TYPE_LABEL,
    TOKEN_TYPE_TAG,
    TOKEN_TYPE_DATA_VALUE,

    TOKEN_TYPE_SOURCE_START,
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_INVALID,
  };

  struct keyword_token {
    const std::string_view text;
    const token_type type;
    constexpr keyword_token(const std::string_view str, token_type type) : text(str), type(type) {}
  };

  constexpr inline size_t kNumKwTokens = TOKEN_TYPE_KW_DATA - TOKEN_TYPE_KW_I8_TYPE + 1;
  constexpr inline std::array<keyword_token, kNumKwTokens> kKeywordTokens = {
    keyword_token("byte", TOKEN_TYPE_KW_I8_TYPE),
    keyword_token("ubyte", TOKEN_TYPE_KW_U8_TYPE),
    keyword_token("int16", TOKEN_TYPE_KW_I16_TYPE),
    keyword_token("short", TOKEN_TYPE_KW_I16_TYPE),
    keyword_token("uint16", TOKEN_TYPE_KW_U16_TYPE),
    keyword_token("ushort", TOKEN_TYPE_KW_U16_TYPE),
    keyword_token("int32", TOKEN_TYPE_KW_I32_TYPE),
    keyword_token("int", TOKEN_TYPE_KW_I32_TYPE),
    keyword_token("uint32", TOKEN_TYPE_KW_U32_TYPE),
    keyword_token("uint", TOKEN_TYPE_KW_U32_TYPE),
    keyword_token("int64", TOKEN_TYPE_KW_I64_TYPE),
    keyword_token("long", TOKEN_TYPE_KW_I64_TYPE),
    keyword_token("uint64", TOKEN_TYPE_KW_U64_TYPE),
    keyword_token("ulong", TOKEN_TYPE_KW_U64_TYPE),
    keyword_token("float", TOKEN_TYPE_KW_F32_TYPE),
    keyword_token("float32", TOKEN_TYPE_KW_F32_TYPE),
    keyword_token("double", TOKEN_TYPE_KW_F64_TYPE),
    keyword_token("float64", TOKEN_TYPE_KW_F64_TYPE),
    keyword_token("string", TOKEN_TYPE_KW_STRING_TYPE),
    keyword_token("address", TOKEN_TYPE_KW_ADDRESS_TYPE),
    keyword_token("blob", TOKEN_TYPE_KW_BLOB_TYPE),
    keyword_token("user_type", TOKEN_TYPE_KW_USER_DEFINED_TYPE),

    keyword_token("stopdev", TOKEN_TYPE_KW_STOPDEV),
    keyword_token("dump", TOKEN_TYPE_KW_DUMP),
    keyword_token("write", TOKEN_TYPE_KW_WRITE),
    keyword_token("set", TOKEN_TYPE_KW_SET),
    keyword_token("cmp", TOKEN_TYPE_KW_CMP),
    keyword_token("cmpgt", TOKEN_TYPE_KW_CMPGT),
    keyword_token("cmplt", TOKEN_TYPE_KW_CMPLT),
    keyword_token("and", TOKEN_TYPE_KW_AND),
    keyword_token("or", TOKEN_TYPE_KW_OR),
    keyword_token("xor", TOKEN_TYPE_KW_XOR),
    keyword_token("lshift", TOKEN_TYPE_KW_LSHIFT),
    keyword_token("rshift", TOKEN_TYPE_KW_RSHIFT),

    keyword_token("goto", TOKEN_TYPE_KW_GOTO),
    keyword_token("je", TOKEN_TYPE_KW_JE),
    keyword_token("jne", TOKEN_TYPE_KW_JNE),
    keyword_token("call", TOKEN_TYPE_KW_CALL),
    keyword_token("ret", TOKEN_TYPE_KW_RET),
    keyword_token("syscall", TOKEN_TYPE_KW_SYSCALL),
    keyword_token("invoke", TOKEN_TYPE_KW_INVOKE),

    keyword_token("add", TOKEN_TYPE_KW_ADD),
    keyword_token("sub", TOKEN_TYPE_KW_SUB),
    keyword_token("mul", TOKEN_TYPE_KW_MUL),
    keyword_token("div", TOKEN_TYPE_KW_DIV),
    keyword_token("mod", TOKEN_TYPE_KW_MOD),

    keyword_token("r0", TOKEN_TYPE_KW_R0),
    keyword_token("r1", TOKEN_TYPE_KW_R1),
    keyword_token("r2", TOKEN_TYPE_KW_R2),
    keyword_token("r3", TOKEN_TYPE_KW_R3),
    keyword_token("r4", TOKEN_TYPE_KW_R4),
    keyword_token("r5", TOKEN_TYPE_KW_R5),
    keyword_token("r6", TOKEN_TYPE_KW_R6),
    keyword_token("r7", TOKEN_TYPE_KW_R7),
    keyword_token("r8", TOKEN_TYPE_KW_R8),
    keyword_token("r9", TOKEN_TYPE_KW_R9),
    keyword_token("ra", TOKEN_TYPE_KW_RA),
    keyword_token("rb", TOKEN_TYPE_KW_RB),
    keyword_token("rc", TOKEN_TYPE_KW_RC),
    keyword_token("rd", TOKEN_TYPE_KW_RD),
    keyword_token("re", TOKEN_TYPE_KW_RE),
    keyword_token("rf", TOKEN_TYPE_KW_RF),
    keyword_token("rflag", TOKEN_TYPE_KW_RFLAG),

    keyword_token("begin", TOKEN_TYPE_KW_BEGIN),
    keyword_token("end", TOKEN_TYPE_KW_END),
    keyword_token("object", TOKEN_TYPE_KW_OBJECT),
    keyword_token("asset", TOKEN_TYPE_KW_ASSET),
    keyword_token("scene", TOKEN_TYPE_KW_SCENE),
    keyword_token("pipeline", TOKEN_TYPE_KW_PIPELINE),
    keyword_token("pass", TOKEN_TYPE_KW_PASS),
    keyword_token("shader", TOKEN_TYPE_KW_SHADER),
    keyword_token("texture", TOKEN_TYPE_KW_TEXTURE),
    keyword_token("buffer", TOKEN_TYPE_KW_BUFFER),
    keyword_token("tag", TOKEN_TYPE_KW_TAG),
    keyword_token("data", TOKEN_TYPE_KW_DATA),
  };

  struct token {
    std::string text;
    token_type type;

    source_span source_view;

    token() : text(""), type(TOKEN_TYPE_INVALID), source_view{} {}
    token(token_type type, const std::string_view str, source_span source_view)
        : text(str), type(type), source_view(source_view) {}
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_TOKEN_HPP