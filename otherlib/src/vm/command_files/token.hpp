/**
 * \file vm/command_files/token.hpp
 **/
#ifndef OTHERLIB_VM_COMMAND_FILES_TOKEN_HPP
#define OTHERLIB_VM_COMMAND_FILES_TOKEN_HPP

#include <array>
#include <string>

namespace other {

  constexpr static inline std::array kOperators = {
    '+', '-', '*', '/',
    '=', '!', '<', '>', '&', '|',
    '^', '%', '$', '#', '@'
  };

  constexpr static inline std::array kPunctuation = {
    '(', ')', '{', '}', '[', ']', ';', ':', ',', '.', '"', '\''
  };

  constexpr static inline std::array kOcmdKeywords = {
    // g0
    "stopdev",
    "dump",
    "dumpx",

    // g1
    "write",
    "load",
    "set",
    "iwrite",
    "cmp",
    "cmpgt",
    "cmplt",
    "and",
    "or",
    "xor",
    "lshift",
    "rshift",

    // g2
    "goto",
    "jmp",
    "je",
    "jne",
    "call",
    "ret",
    "retx",

    // g3
    "add",
    "sub",
    "mul",
    "div",
    "mod",

    // g4
    "loadscn",

    /// data type keywords
    "byte",
    "ubyte",
    "int16",
    "short",
    "uint16",
    "ushort",
    "int32",
    "int",
    "uint32",
    "uint",
    "int64",
    "long",
    "uint64",
    "ulong",
    "float",
    "float32",
    "double",
    "float64",
    "string",
    "blob",
    "user_type",

    /// register keywords
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "r6",
    "r7",
    "r8",
    "r9",
    "ra",
    "rb",
    "rc",
    "rd",
    "re",
    "rf",
    "rflag",

    /// other
    "end",
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
    TOKEN_TYPE_KW_BLOB_TYPE,
    TOKEN_TYPE_KW_USER_DEFINED_TYPE,

    TOKEN_TYPE_KW_STOPDEV,
    TOKEN_TYPE_KW_DUMP,
    TOKEN_TYPE_KW_DUMPX,
    TOKEN_TYPE_KW_WRITE,
    TOKEN_TYPE_KW_LOAD,
    TOKEN_TYPE_KW_SET,
    TOKEN_TYPE_KW_IWRITE,
    TOKEN_TYPE_KW_CMP,
    TOKEN_TYPE_KW_CMPGT,
    TOKEN_TYPE_KW_CMPLT,
    TOKEN_TYPE_KW_AND,
    TOKEN_TYPE_KW_OR,
    TOKEN_TYPE_KW_XOR,
    TOKEN_TYPE_KW_LSHIFT,
    TOKEN_TYPE_KW_RSHIFT,
    TOKEN_TYPE_KW_GOTO,
    TOKEN_TYPE_KW_JMP,
    TOKEN_TYPE_KW_JE,
    TOKEN_TYPE_KW_JNE,
    TOKEN_TYPE_KW_CALL,
    TOKEN_TYPE_KW_RET,
    TOKEN_TYPE_KW_RETX,
    TOKEN_TYPE_KW_ADD,
    TOKEN_TYPE_KW_SUB,
    TOKEN_TYPE_KW_MUL,
    TOKEN_TYPE_KW_DIV,
    TOKEN_TYPE_KW_MOD,
    TOKEN_TYPE_KW_LOADSCN,

    TOKEN_TYPE_KW_END,

    TOKEN_TYPE_REGISTER,
    TOKEN_TYPE_ADDRESS,
    TOKEN_TYPE_LABEL,
    TOKEN_TYPE_TAG,
    TOKEN_TYPE_DATA_VALUE,

    TOKEN_TYPE_EOF,
    TOKEN_TYPE_INVALID,
  };

  struct token {
    std::string text;
    token_type type;

    size_t line_number;
    size_t column_number;

    token()
        : text(""), type(TOKEN_TYPE_INVALID), line_number(0), column_number(0) {}
    token(token_type type, const std::string_view str, size_t line, size_t col)
        : text(str), type(type), line_number(line), column_number(col) {}
  };

}  // namespace other

#endif  // OTHERLIB_VM_COMMAND_FILES_TOKEN_HPP