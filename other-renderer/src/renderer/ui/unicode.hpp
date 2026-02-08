/**
 * \file renderer/ui/unicode.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_UI_UNICODE_HPP
#define OTHER_RENDERER_RENDERER_UI_UNICODE_HPP

#include <imgui/imgui.h>

namespace other {
  namespace ui {
    namespace unicode {

      constexpr const char* kPromptSymbol = "\xe2\x9d\xaf";    // ❯ UTF-8
      constexpr const char* kStatusDot = "\xe2\x97\x8f";       // ● UTF-8
      constexpr const char* kEmDash = "\xe2\x80\x94";          // — UTF-8
      constexpr const char* kSearchIcon = "\xF0\x9F\x94\x8D";  // 🔍 UTF-8
      constexpr const char* kDownArrow = "\xe2\x96\xbe";       // ▾ UTF-8
      constexpr const char* kEyeIcon = "\xF0\x9F\x91\x81";     // 👁 UTF-8

      constexpr static ImWchar kUnicodeExtraRanges[] = {
        0x0020, 0x00FF,
        0x0100, 0xFFFF,  /// enough range to cover all unicode characters
        0
      };

    }  // namespace unicode
  }  // namespace ui
}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_UI_UNICODE_HPP