/**
 * \file ui/colors.hpp
 **/
#ifndef OTHERLIB_UI_COLORS_HPP
#define OTHERLIB_UI_COLORS_HPP

#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace other {
  namespace ui {
    namespace colors {

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
        static constexpr auto kNodeEditorBackground = ImVec4(0.03f, 0.03f, 0.03f, 0.6f);
        static constexpr auto kNodeHeaderColor = ImVec4(44.f / 255.f, 47.f / 255.f, 55.f / 255.f, 1.f);
        static constexpr auto kNodeBodyColor = ImVec4(25.f / 255.f, 26.f / 255.f, 31.f / 255.f, 1.f);

        static constexpr auto kBasicNodeLinkColor = ImVec4(75.f / 255.f, 77.f / 255.f, 99.f / 255.f, 1.f);
        // static constexpr auto kNodeOutlineColor = ImVec4(1.f, 1.f, 1.f, 0.2f);
      }  // namespace editor
      namespace console {
        static constexpr auto kConsoleBackground = ImVec4(0.03f, 0.03f, 0.03f, 0.6f);
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

    }  // namespace colors
  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_COLORS_HPP