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
        static constexpr auto kNodeEditorBackground = ImVec4(0.03f, 0.03f, 0.03f, 0.7f);
        static constexpr auto kNodeHeaderColor = ImVec4(44.f / 255.f, 47.f / 255.f, 55.f / 255.f, 1.f);
        static constexpr auto kNodeBodyColor = ImVec4(25.f / 255.f, 26.f / 255.f, 31.f / 255.f, 1.f);

        static constexpr auto kBasicNodeLinkColor = ImVec4(75.f / 255.f, 77.f / 255.f, 99.f / 255.f, 1.f);
        // static constexpr auto kNodeOutlineColor = ImVec4(1.f, 1.f, 1.f, 0.2f);
      }  // namespace editor

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