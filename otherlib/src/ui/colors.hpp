/**
 * \file ui/colors.hpp
 **/
#ifndef OTHERLIB_UI_COLORS_HPP
#define OTHERLIB_UI_COLORS_HPP

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "glm/fwd.hpp"

namespace other {
  namespace ui {
    namespace colors {

      constexpr static inline glm::vec4 hex_col_to_rgba(ImU32 hex_col) {
        ImColor col{ hex_col };
        return glm::vec4{
          col.Value.x,
          col.Value.y,
          col.Value.z,
          col.Value.w
        };
      }

      constexpr static inline ImVec4 rgba_to_imvec4(const glm::vec4& color) {
        return ImVec4{ color.r, color.g, color.b, color.a };
      }

      constexpr static inline ImVec4 hex_to_imvec4(ImU32 hex_col) {
        return rgba_to_imvec4(hex_col_to_rgba(hex_col));
      }

      static inline ImU32 rgba_to_hex(const glm::vec4& color) {
        ImColor col{ color.r, color.g, color.b, color.a };
        return col;
      }

      constexpr inline glm::vec4 kRed = hex_col_to_rgba(IM_COL32(255, 0, 0, 255));
      constexpr inline glm::vec4 kBalancedRed = hex_col_to_rgba(IM_COL32(255, 50, 50, 255));
      constexpr inline glm::vec4 kGreen = hex_col_to_rgba(IM_COL32(0, 255, 0, 255));
      constexpr inline glm::vec4 kBalancedGreen = hex_col_to_rgba(IM_COL32(50, 255, 50, 255));
      constexpr inline glm::vec4 kBlue = hex_col_to_rgba(IM_COL32(0, 0, 255, 255));
      constexpr inline glm::vec4 kBalancedBlue = hex_col_to_rgba(IM_COL32(50, 50, 255, 255));
      constexpr inline glm::vec4 kPurple = hex_col_to_rgba(IM_COL32(128, 0, 128, 255));
      constexpr inline glm::vec4 kBalancedPurple = hex_col_to_rgba(IM_COL32(128, 50, 128, 255));

      constexpr inline glm::vec4 kBG0 = hex_col_to_rgba(IM_COL32(20, 20, 20, 255));
      constexpr inline glm::vec4 kBG1 = hex_col_to_rgba(IM_COL32(25, 25, 25, 255));
      constexpr inline glm::vec4 kBG2 = hex_col_to_rgba(IM_COL32(35, 35, 35, 255));
      constexpr inline glm::vec4 kBG3 = hex_col_to_rgba(IM_COL32(44, 44, 44, 255));

      constexpr inline glm::vec4 kPanel = hex_col_to_rgba(IM_COL32(49, 49, 49, 255));
      constexpr inline glm::vec4 kPopup = hex_col_to_rgba(IM_COL32(59, 59, 59, 255));
      constexpr inline glm::vec4 kField = hex_col_to_rgba(IM_COL32(14, 14, 14, 255));

      constexpr inline glm::vec4 kBorderSubtle = hex_col_to_rgba(IM_COL32(46, 46, 46, 255));
      constexpr inline glm::vec4 kBorder = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
      constexpr inline glm::vec4 kBorderStrong = hex_col_to_rgba(IM_COL32(107, 107, 107, 255));

      constexpr inline glm::vec4 kTextBright = hex_col_to_rgba(IM_COL32(230, 235, 243, 255));
      constexpr inline glm::vec4 kText = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
      constexpr inline glm::vec4 kTextMuted = hex_col_to_rgba(IM_COL32(155, 160, 170, 255));
      constexpr inline glm::vec4 kTextDisabled = hex_col_to_rgba(IM_COL32(107, 107, 107, 255));
      constexpr inline glm::vec4 kTextFriendlyAlert = hex_col_to_rgba(IM_COL32(255, 165, 0, 255));
      constexpr inline glm::vec4 kTextUnfriendAlert = hex_col_to_rgba(IM_COL32(255, 69, 0, 255));
      constexpr inline glm::vec4 kTextError = hex_col_to_rgba(IM_COL32(230, 51, 51, 255));
      constexpr inline glm::vec4 kTextValueType = hex_col_to_rgba(IM_COL32(112, 190, 242, 255));
      constexpr inline glm::vec4 kTextValueData = hex_col_to_rgba(IM_COL32(242, 190, 112, 255));
      constexpr inline glm::vec4 kFriendlyErrorRed = hex_col_to_rgba(IM_COL32(230, 51, 51, 255));

      constexpr inline glm::vec4 kAccent = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));
      constexpr inline glm::vec4 kAccentWarm = hex_col_to_rgba(IM_COL32(217, 122, 29, 255));
      constexpr inline glm::vec4 kAccentWarmHover = hex_col_to_rgba(IM_COL32(240, 154, 60, 255));
      constexpr inline glm::vec4 kAccentCool = hex_col_to_rgba(IM_COL32(43, 212, 180, 255));
      constexpr inline glm::vec4 kAccentCoolHover = hex_col_to_rgba(IM_COL32(94, 234, 212, 255));
      constexpr inline glm::vec4 kAccentCoolAlt = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kAccentCoolAltHover = hex_col_to_rgba(IM_COL32(88, 203, 246, 255));

      constexpr inline glm::vec4 kHighlight = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));

      constexpr inline glm::vec4 kOutlineFocus = hex_col_to_rgba(IM_COL32(58, 175, 255, 255));
      constexpr inline glm::vec4 kOutlineAlt = hex_col_to_rgba(IM_COL32(120, 100, 200, 255));

      constexpr inline glm::vec4 kSelection = hex_col_to_rgba(IM_COL32(59, 40, 22, 255));
      constexpr inline glm::vec4 kSelectionHover = hex_col_to_rgba(IM_COL32(59, 40, 22, 200));
      constexpr inline glm::vec4 kSelectionSoft = hex_col_to_rgba(IM_COL32(59, 40, 22, 153));

      constexpr inline glm::vec4 kInfo = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kSuccess = hex_col_to_rgba(IM_COL32(74, 222, 128, 255));
      constexpr inline glm::vec4 kWarning = hex_col_to_rgba(IM_COL32(245, 158, 11, 255));
      constexpr inline glm::vec4 kError = hex_col_to_rgba(IM_COL32(239, 68, 68, 255));

      constexpr inline glm::vec4 kDataType = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kDataTypeAlt = hex_col_to_rgba(IM_COL32(43, 212, 180, 255));
      constexpr inline glm::vec4 kDataValue = hex_col_to_rgba(IM_COL32(242, 183, 116, 255));

      constexpr inline glm::vec4 kConsoleBG = hex_col_to_rgba(IM_COL32(8, 8, 8, 153));
      constexpr inline glm::vec4 kConsolePrompt = hex_col_to_rgba(IM_COL32(217, 122, 29, 255));
      constexpr inline glm::vec4 kConsoleCommand = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kConsoleOutput = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
      constexpr inline glm::vec4 kConsoleHint = hex_col_to_rgba(IM_COL32(155, 160, 170, 255));
      constexpr inline glm::vec4 kConsoleInfo = hex_col_to_rgba(IM_COL32(193, 208, 193, 255));
      constexpr inline glm::vec4 kConsoleWarning = hex_col_to_rgba(IM_COL32(245, 158, 11, 255));
      constexpr inline glm::vec4 kConsoleError = hex_col_to_rgba(IM_COL32(239, 68, 68, 255));

      constexpr inline glm::vec4 kModeStoppedBar = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
      constexpr inline glm::vec4 kModePlayingBar = hex_col_to_rgba(IM_COL32(34, 197, 94, 255));
      constexpr inline glm::vec4 kModePlayingTint = hex_col_to_rgba(IM_COL32(11, 42, 26, 255));

      constexpr inline glm::vec4 kNodeHeaderColor = hex_col_to_rgba(IM_COL32(46, 46, 46, 255));
      constexpr inline glm::vec4 kNodeBodyColor = hex_col_to_rgba(IM_COL32(59, 59, 59, 255));
      constexpr inline glm::vec4 kBasicNodeLinkColor = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
      constexpr inline glm::vec4 kDataLinkColor = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kNodeCardStrokeColor = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
      constexpr inline glm::vec4 kNodeOutlineColor = hex_col_to_rgba(IM_COL32(107, 107, 107, 255));
      constexpr inline glm::vec4 kNodeTitleTextColor = hex_col_to_rgba(IM_COL32(230, 235, 243, 255));
      constexpr inline glm::vec4 kNodeEditorBackground = hex_col_to_rgba(IM_COL32(20, 20, 20, 255));

    }  // namespace colors
    namespace _colors {

      constexpr auto kFriendlyErrorRed = IM_COL32(230, 51, 51, 255);
      constexpr auto kUnfriendlyErrorRed = IM_COL32(255, 0, 0, 255);

      constexpr auto kTextFriendlyAlert = IM_COL32(255, 165, 0, 255);
      constexpr auto kTextUnfriendlyAlert = IM_COL32(255, 69, 0, 255);
      constexpr auto kFireOrange = IM_COL32(242, 130, 7, 255);

      constexpr auto kAccent = IM_COL32(236, 158, 36, 255);
      constexpr auto kHighlight = IM_COL32(39, 185, 242, 255);
      constexpr auto kText = IM_COL32(192, 192, 192, 255);
      constexpr auto kTextBright = IM_COL32(210, 210, 210, 255);
      constexpr auto kTextDark = IM_COL32(128, 128, 128, 255);
      /// slightly orangish
      constexpr auto kTextValueData = IM_COL32(242, 190, 112, 255);
      /// slightly bluish
      constexpr auto kTextValueType = IM_COL32(112, 190, 242, 255);
      constexpr auto kTextError = IM_COL32(230, 51, 51, 255);

      // /* ?? */ constexpr auto nice_blue = IM_COL32(83 , 232 , 254 , 255);
      // constexpr auto compliment = IM_COL32(78 , 151 , 166 , 255);
      // constexpr auto background = IM_COL32(36 , 36 , 36 , 255);
      // constexpr auto background_dark = IM_COL32(26 , 26 , 26 , 255);
      // constexpr auto title_bar = IM_COL32(21 , 21 , 21 , 255);
      // constexpr auto title_bar_orange = IM_COL32(186 , 66 , 30 , 255);
      // constexpr auto title_bar_green = IM_COL32(18 , 88 , 30 , 255);
      // constexpr auto title_bar_red = IM_COL32(185 , 30 , 30 , 255);
      // constexpr auto property_field = IM_COL32(15 , 15 , 15 , 255);
      // constexpr auto muted = IM_COL32(77, 77, 77, 255);
      // constexpr auto group_header = IM_COL32(47, 47, 47, 255);
      // constexpr auto selection = IM_COL32(237, 192, 119, 255);
      // constexpr auto selection_muted  = IM_COL32(237, 201, 142, 23);
      // constexpr auto background_popup = IM_COL32(50, 50, 50, 255);
      // constexpr auto valid_prefab = IM_COL32(82, 179, 222, 255);
      // constexpr auto invalid_prefab = IM_COL32(222, 43, 43, 255);
      // constexpr auto missing_mesh = IM_COL32(230, 102, 76, 255);
      // constexpr auto mesh_not_set = IM_COL32(250, 101, 23, 255);

      static constexpr glm::vec4 kRed{ 1.f, 0, 0, 1.f };
      static constexpr glm::vec4 kBalancedRed{ 1.f, 50.f / 255.f, 50.f / 255.f, 1.f };
      static constexpr glm::vec4 kGreen{ 0, 1.f, 0, 1.f };
      static constexpr glm::vec4 kBalancedGreen{ 50.f / 255.f, 1.f, 50.f / 255.f, 1.f };
      static constexpr glm::vec4 kBlue{ 0, 0, 1.f, 1.f };
      static constexpr glm::vec4 kBalancedBlue{ 50.f / 255.f, 50.f / 255.f, 1.f, 1.f };
      static constexpr glm::vec4 kPurple{ 0.5f, 0, 0.5f, 1.f };
      static constexpr glm::vec4 kBalancedPurple{ 0.5f, 0.25f, 0.5f, 1.f };

      namespace ig {
        static constexpr auto kLightGrayBackGround = IM_COL32(60, 60, 60, 255);
        static constexpr auto kDarkGrayBackGround = IM_COL32(30, 30, 30, 255);

        static constexpr auto kRed = IM_COL32(255, 0, 0, 255);
        static constexpr auto kBalancedRed = IM_COL32(255, 50, 50, 255);
        static constexpr auto kGreen = IM_COL32(0, 255, 0, 255);
        static constexpr auto kBalancedGreen = IM_COL32(50, 255, 50, 255);
        static constexpr auto kBlue = IM_COL32(0, 0, 255, 255);
        static constexpr auto kBalancedBlue = IM_COL32(50, 50, 255, 255);

        static constexpr auto kYellow = IM_COL32(255, 255, 0, 255);
      }  // namespace ig

      namespace editor {
        static constexpr auto kNodeEditorBackground = ImVec4(7.65 / 255.f, 7.65 / 255.f, 7.65 / 255.f, 153.f / 255.f);
        static constexpr auto kNodeHeaderColor = ImVec4(44.f / 255.f, 47.f / 255.f, 55.f / 255.f, 1.f);
        static constexpr auto kNodeBodyColor = ImVec4(25.f / 255.f, 26.f / 255.f, 31.f / 255.f, 1.f);

        static constexpr auto kBasicNodeLinkColor = ImVec4(75.f / 255.f, 77.f / 255.f, 99.f / 255.f, 1.f);
        // static constexpr auto kNodeOutlineColor = ImVec4(1.f, 1.f, 1.f, 0.2f);
      }  // namespace editor
      namespace console {
        /*
          console background: rgba(8, 8, 8, 0.6)
        */
        static constexpr auto kConsoleBackground = ImVec4(7.65 / 255.f, 7.65 / 255.f, 7.65 / 255.f, 153.f / 255.f);
        static constexpr auto kHeaderColor = ImVec4(44.f / 255.f, 47.f / 255.f, 55.f / 255.f, 1.f);
        static constexpr auto kBodyColor = ImVec4(25.f / 255.f, 26.f / 255.f, 31.f / 255.f, 1.f);

        static constexpr auto kConsoleCommandText = IM_COL32(112, 190, 242, 255);
        static constexpr auto kConsoleOutputText = IM_COL32(192, 192, 192, 255);
        static constexpr auto kConsoleDebugText = IM_COL32(128, 128, 128, 255);
        static constexpr auto kConsoleInfoText = IM_COL32(192, 210, 192, 255);
        static constexpr auto kConsoleWarningText = IM_COL32(242, 190, 112, 255);
        static constexpr auto kConsoleErrorText = IM_COL32(230, 51, 51, 255);
      }  // namespace console

      // IK IK :'(
      namespace ai {
        /*
        Nodes
        Header fill: rgb(44, 47, 55)
        Body fill: rgb(25, 26, 31)
        Title text (primary): rgb(230, 235, 243)
        Subtitle/label text (muted): rgb(155, 160, 170)
        Card stroke (subtle outline): rgb(255, 255, 255) (low opacity in the mockup)
        Shadow/inner outlines: rgb(0, 0, 0) (blurred, low opacity)
        Outline: rgb(120, 100, 200) (semi-transparent)

        Pins
        Execution pin: rgb(243, 156, 18)
        Data input pin: rgb(74, 144, 226) (blue)
        Data output pin: rgb(113, 201, 112) (green)
        Pin rim/outline: rgb(0, 0, 0) (subtle)

        Links (Wires)
        Execution link: rgb(243, 156, 18) (amber)
        Data link: rgb(113, 201, 112) (green)
        */
        static constexpr auto kNodeHeaderColor = IM_COL32(44, 47, 55, 255);
        static constexpr auto kNodeBodyColor = IM_COL32(25, 26, 31, 255);
        static constexpr auto kNodeTitleTextColor = IM_COL32(230, 235, 243, 255);
        static constexpr auto kNodeSubtitleTextColor = IM_COL32(155, 160, 170, 255);
        static constexpr auto kNodeCardStrokeColor = IM_COL32(255, 255, 255, 50);
        static constexpr auto kNodeShadowColor = IM_COL32(0, 0, 0, 50);
        static constexpr auto kNodeOutlineColor = IM_COL32(120, 100, 200, 75);

        static constexpr auto kExecutionPinColor = IM_COL32(243, 156, 18, 255);
        static constexpr auto kDataInputPinColor = IM_COL32(74, 144, 226, 255);
        static constexpr auto kDataOutputPinColor = IM_COL32(113, 201, 112, 255);
        static constexpr auto kPinRimColor = IM_COL32(0, 0, 0, 50);

        static constexpr auto kExecutionLinkColor = IM_COL32(243, 156, 18, 255);
        static constexpr auto kDataLinkColor = IM_COL32(113, 201, 112, 255);

      }  // namespace ai
    }  // namespace _colors
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_COLORS_HPP