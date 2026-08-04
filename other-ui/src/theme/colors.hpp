/**
 * \file renderer/ui/colors.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_UI_COLORS_HPP
#define OTHER_RENDERER_RENDERER_UI_COLORS_HPP

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
        return col.operator ImU32();
      }

      static inline ImU32 imvec4_to_hex(const ImVec4& color) {
        ImColor col{ color.x, color.y, color.z, color.w };
        return col.operator ImU32();
      }

      static inline ImU32 to_im_col(const glm::vec4& c) {
        return colors::rgba_to_hex(c);
      }

      static inline ImU32 im_col_with_multiplier(const ImColor& col, float factor) {
        const ImVec4& color_val = col.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(color_val.x, color_val.y, color_val.z, hue, sat, val);
        return ImColor::HSV(hue, sat, std::min(val * factor, 1.f));
      }

      static inline ImU32 im_col_with_saturation_multiplier(const ImColor& col, float factor) {
        const ImVec4& col_raw = col.Value;
        float hue, sat, val;
        ImGui::ColorConvertRGBtoHSV(col_raw.x, col_raw.y, col_raw.z, hue, sat, val);
        return ImColor::HSV(hue, std::min(sat * factor, 1.0f), val);
      }

      static inline glm::vec4 color_with_multiplier(const glm::vec4& col, float factor) {
        ImU32 mod_col = im_col_with_multiplier({ col.r, col.g, col.b, col.a }, factor);
        return hex_col_to_rgba(mod_col);
      }

      static inline glm::vec4 color_with_saturation_multiplier(const glm::vec4& col, float factor) {
        ImU32 mod_col = im_col_with_saturation_multiplier({ col.r, col.g, col.b, col.a }, factor);
        return hex_col_to_rgba(mod_col);
      }

      static inline glm::vec4 mute_by_factor(const glm::vec4& col, float factor) {
        return color_with_saturation_multiplier(col, factor);
      }

      //  Pure Hues
      constexpr inline glm::vec4 kRed = hex_col_to_rgba(IM_COL32(255, 0, 0, 255));
      constexpr inline glm::vec4 kBalancedRed = hex_col_to_rgba(IM_COL32(255, 50, 50, 255));
      constexpr inline glm::vec4 kGreen = hex_col_to_rgba(IM_COL32(0, 255, 0, 255));
      constexpr inline glm::vec4 kBalancedGreen = hex_col_to_rgba(IM_COL32(50, 255, 50, 255));
      constexpr inline glm::vec4 kBlue = hex_col_to_rgba(IM_COL32(0, 0, 255, 255));
      constexpr inline glm::vec4 kBalancedBlue = hex_col_to_rgba(IM_COL32(50, 50, 255, 255));
      constexpr inline glm::vec4 kPurple = hex_col_to_rgba(IM_COL32(128, 0, 128, 255));
      constexpr inline glm::vec4 kBalancedPurple = hex_col_to_rgba(IM_COL32(128, 50, 128, 255));
      constexpr inline glm::vec4 kYellow = hex_col_to_rgba(IM_COL32(255, 210, 0, 255));
      constexpr inline glm::vec4 kCyan = hex_col_to_rgba(IM_COL32(0, 210, 220, 255));
      constexpr inline glm::vec4 kOrange = hex_col_to_rgba(IM_COL32(242, 130, 7, 255));
      constexpr inline glm::vec4 kWhite = hex_col_to_rgba(IM_COL32(255, 255, 255, 255));
      constexpr inline glm::vec4 kBlack = hex_col_to_rgba(IM_COL32(0, 0, 0, 255));

      //  Background Ramp
      constexpr inline glm::vec4 kBG0 = hex_col_to_rgba(IM_COL32(20, 20, 20, 255));
      constexpr inline glm::vec4 kBG1 = hex_col_to_rgba(IM_COL32(25, 25, 25, 255));
      constexpr inline glm::vec4 kBG2 = hex_col_to_rgba(IM_COL32(35, 35, 35, 255));
      constexpr inline glm::vec4 kBG3 = hex_col_to_rgba(IM_COL32(44, 44, 44, 255));
      constexpr inline glm::vec4 kBG4 = hex_col_to_rgba(IM_COL32(54, 54, 54, 255));

      //  Surfaces & Containers
      constexpr inline glm::vec4 kPanel = hex_col_to_rgba(IM_COL32(49, 49, 49, 255));
      constexpr inline glm::vec4 kPanelHover = hex_col_to_rgba(IM_COL32(56, 56, 56, 255));
      constexpr inline glm::vec4 kPanelActive = hex_col_to_rgba(IM_COL32(62, 62, 62, 255));
      constexpr inline glm::vec4 kPopup = hex_col_to_rgba(IM_COL32(59, 59, 59, 255));
      constexpr inline glm::vec4 kField = hex_col_to_rgba(IM_COL32(14, 14, 14, 255));
      constexpr inline glm::vec4 kFieldHover = hex_col_to_rgba(IM_COL32(18, 18, 18, 255));
      constexpr inline glm::vec4 kFieldFocused = hex_col_to_rgba(IM_COL32(22, 22, 22, 255));
      constexpr inline glm::vec4 kTitleBar = hex_col_to_rgba(IM_COL32(16, 16, 16, 255));
      constexpr inline glm::vec4 kTitleBarActive = hex_col_to_rgba(IM_COL32(22, 22, 22, 255));
      constexpr inline glm::vec4 kMenuBar = hex_col_to_rgba(IM_COL32(25, 25, 25, 255));
      constexpr inline glm::vec4 kStatusBar = hex_col_to_rgba(IM_COL32(20, 20, 20, 255));
      constexpr inline glm::vec4 kTab = hex_col_to_rgba(IM_COL32(30, 30, 30, 255));
      constexpr inline glm::vec4 kTabHover = hex_col_to_rgba(IM_COL32(40, 40, 40, 255));
      constexpr inline glm::vec4 kTabActive = hex_col_to_rgba(IM_COL32(49, 49, 49, 255));
      constexpr inline glm::vec4 kTabUnfocused = hex_col_to_rgba(IM_COL32(22, 22, 22, 255));
      constexpr inline glm::vec4 kScrollBar = hex_col_to_rgba(IM_COL32(50, 50, 50, 255));
      constexpr inline glm::vec4 kScrollBarHover = hex_col_to_rgba(IM_COL32(70, 70, 70, 255));
      constexpr inline glm::vec4 kScrollBarActive = hex_col_to_rgba(IM_COL32(90, 90, 90, 255));
      constexpr inline glm::vec4 kTooltipBG = hex_col_to_rgba(IM_COL32(30, 30, 30, 240));

      //  Borders
      constexpr inline glm::vec4 kBorderSubtle = hex_col_to_rgba(IM_COL32(46, 46, 46, 255));
      constexpr inline glm::vec4 kBorder = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
      constexpr inline glm::vec4 kBorderStrong = hex_col_to_rgba(IM_COL32(107, 107, 107, 255));

      //  Text
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

      //  Accent & Brand
      constexpr inline glm::vec4 kAccent = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));
      constexpr inline glm::vec4 kAccentHover = hex_col_to_rgba(IM_COL32(248, 176, 60, 255));
      constexpr inline glm::vec4 kAccentActive = hex_col_to_rgba(IM_COL32(210, 140, 28, 255));
      constexpr inline glm::vec4 kAccentWarm = hex_col_to_rgba(IM_COL32(217, 122, 29, 255));
      constexpr inline glm::vec4 kAccentWarmHover = hex_col_to_rgba(IM_COL32(240, 154, 60, 255));
      constexpr inline glm::vec4 kAccentCool = hex_col_to_rgba(IM_COL32(43, 212, 180, 255));
      constexpr inline glm::vec4 kAccentCoolHover = hex_col_to_rgba(IM_COL32(94, 234, 212, 255));
      constexpr inline glm::vec4 kAccentCoolAlt = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kAccentCoolAltHover = hex_col_to_rgba(IM_COL32(88, 203, 246, 255));
      constexpr inline glm::vec4 kHighlight = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));

      //  Selection & Focus
      constexpr inline glm::vec4 kOutlineFocus = hex_col_to_rgba(IM_COL32(58, 175, 255, 255));
      constexpr inline glm::vec4 kOutlineAlt = hex_col_to_rgba(IM_COL32(120, 100, 200, 255));
      constexpr inline glm::vec4 kSelection = hex_col_to_rgba(IM_COL32(59, 40, 22, 255));
      constexpr inline glm::vec4 kSelectionHover = hex_col_to_rgba(IM_COL32(59, 40, 22, 200));
      constexpr inline glm::vec4 kSelectionSoft = hex_col_to_rgba(IM_COL32(59, 40, 22, 153));

      //  Semantic Status
      constexpr inline glm::vec4 kInfo = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kInfoBG = hex_col_to_rgba(IM_COL32(15, 40, 55, 180));
      constexpr inline glm::vec4 kSuccess = hex_col_to_rgba(IM_COL32(74, 222, 128, 255));
      constexpr inline glm::vec4 kSuccessBG = hex_col_to_rgba(IM_COL32(15, 45, 25, 180));
      constexpr inline glm::vec4 kWarning = hex_col_to_rgba(IM_COL32(245, 158, 11, 255));
      constexpr inline glm::vec4 kWarningBG = hex_col_to_rgba(IM_COL32(50, 35, 10, 180));
      constexpr inline glm::vec4 kError = hex_col_to_rgba(IM_COL32(239, 68, 68, 255));
      constexpr inline glm::vec4 kErrorBG = hex_col_to_rgba(IM_COL32(50, 15, 15, 180));
      constexpr inline glm::vec4 kDataType = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kDataTypeAlt = hex_col_to_rgba(IM_COL32(43, 212, 180, 255));
      constexpr inline glm::vec4 kDataValue = hex_col_to_rgba(IM_COL32(242, 183, 116, 255));

      //  Buttons
      constexpr inline glm::vec4 kButton = hex_col_to_rgba(IM_COL32(55, 55, 55, 255));
      constexpr inline glm::vec4 kButtonHover = hex_col_to_rgba(IM_COL32(70, 70, 70, 255));
      constexpr inline glm::vec4 kButtonActive = hex_col_to_rgba(IM_COL32(85, 85, 85, 255));
      constexpr inline glm::vec4 kButtonPrimary = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));
      constexpr inline glm::vec4 kButtonPrimaryHover = hex_col_to_rgba(IM_COL32(248, 176, 60, 255));
      constexpr inline glm::vec4 kButtonPrimaryActive = hex_col_to_rgba(IM_COL32(210, 140, 28, 255));
      constexpr inline glm::vec4 kButtonDanger = hex_col_to_rgba(IM_COL32(180, 45, 45, 255));
      constexpr inline glm::vec4 kButtonDangerHover = hex_col_to_rgba(IM_COL32(210, 60, 60, 255));
      constexpr inline glm::vec4 kButtonDangerActive = hex_col_to_rgba(IM_COL32(160, 35, 35, 255));

      //  Sliders & Controls
      constexpr inline glm::vec4 kSliderGrab = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));
      constexpr inline glm::vec4 kSliderGrabActive = hex_col_to_rgba(IM_COL32(255, 180, 60, 255));
      constexpr inline glm::vec4 kCheckMark = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));

      //  Headers & Separators
      constexpr inline glm::vec4 kHeader = hex_col_to_rgba(IM_COL32(47, 47, 47, 255));
      constexpr inline glm::vec4 kHeaderHover = hex_col_to_rgba(IM_COL32(60, 60, 60, 255));
      constexpr inline glm::vec4 kHeaderActive = hex_col_to_rgba(IM_COL32(70, 70, 70, 255));
      constexpr inline glm::vec4 kResizeGrip = hex_col_to_rgba(IM_COL32(80, 80, 80, 50));
      constexpr inline glm::vec4 kResizeGripHover = hex_col_to_rgba(IM_COL32(120, 120, 120, 120));
      constexpr inline glm::vec4 kResizeGripActive = hex_col_to_rgba(IM_COL32(236, 158, 36, 180));
      constexpr inline glm::vec4 kSeparator = hex_col_to_rgba(IM_COL32(46, 46, 46, 255));
      constexpr inline glm::vec4 kSeparatorHover = hex_col_to_rgba(IM_COL32(100, 100, 100, 255));
      constexpr inline glm::vec4 kSeparatorActive = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));

      //  Docking
      constexpr inline glm::vec4 kDockingPreview = hex_col_to_rgba(IM_COL32(236, 158, 36, 100));
      constexpr inline glm::vec4 kDockingEmptyBG = hex_col_to_rgba(IM_COL32(14, 14, 14, 255));

      //  Console
      constexpr inline glm::vec4 kConsoleBG = hex_col_to_rgba(IM_COL32(8, 8, 8, 153));
      constexpr inline glm::vec4 kConsolePrompt = hex_col_to_rgba(IM_COL32(217, 122, 29, 255));
      constexpr inline glm::vec4 kConsoleCommand = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kConsoleOutput = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
      constexpr inline glm::vec4 kConsoleHint = hex_col_to_rgba(IM_COL32(155, 160, 170, 255));
      constexpr inline glm::vec4 kConsoleInfo = hex_col_to_rgba(IM_COL32(193, 208, 193, 255));
      constexpr inline glm::vec4 kConsoleWarning = hex_col_to_rgba(IM_COL32(245, 158, 11, 255));
      constexpr inline glm::vec4 kConsoleError = hex_col_to_rgba(IM_COL32(239, 68, 68, 255));

      //  Node Editor (canvas-level)
      constexpr inline glm::vec4 kNodeHeaderColor = hex_col_to_rgba(IM_COL32(46, 46, 46, 255));
      constexpr inline glm::vec4 kNodeHeaderHoverColor = hex_col_to_rgba(IM_COL32(60, 60, 60, 255));
      constexpr inline glm::vec4 kNodeBodyColor = hex_col_to_rgba(IM_COL32(59, 59, 59, 255));
      constexpr inline glm::vec4 kNodeTextColor = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
      constexpr inline glm::vec4 kBasicNodeLinkColor = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
      constexpr inline glm::vec4 kInputPinColor = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kOutputPinColor = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));
      constexpr inline glm::vec4 kDataLinkColor = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kNodeCardStrokeColor = hex_col_to_rgba(IM_COL32(107, 107, 107, 255));
      constexpr inline glm::vec4 kNodeOutlineColor = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
      constexpr inline glm::vec4 kNodeTitleTextColor = hex_col_to_rgba(IM_COL32(230, 235, 243, 255));
      constexpr inline glm::vec4 kNodeEditorBackground = hex_col_to_rgba(IM_COL32(20, 20, 20, 255));

      //  Engine Modes
      constexpr inline glm::vec4 kModeStoppedBar = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
      constexpr inline glm::vec4 kModePlayingBar = hex_col_to_rgba(IM_COL32(34, 197, 94, 255));
      constexpr inline glm::vec4 kModePlayingTint = hex_col_to_rgba(IM_COL32(11, 42, 26, 255));

      //  Property Inspector
      namespace inspector {
        constexpr inline glm::vec4 kBG = hex_col_to_rgba(IM_COL32(28, 28, 28, 255));
        constexpr inline glm::vec4 kBorder = hex_col_to_rgba(IM_COL32(46, 46, 46, 255));

        /// object header bar
        constexpr inline glm::vec4 kObjectHeaderBG = hex_col_to_rgba(IM_COL32(35, 35, 37, 255));
        constexpr inline glm::vec4 kObjectName = hex_col_to_rgba(IM_COL32(230, 235, 243, 255));
        constexpr inline glm::vec4 kObjectTag = hex_col_to_rgba(IM_COL32(155, 160, 170, 255));
        constexpr inline glm::vec4 kObjectID = hex_col_to_rgba(IM_COL32(100, 100, 100, 255));

        /// component section headers
        constexpr inline glm::vec4 kComponentHeaderBG = hex_col_to_rgba(IM_COL32(38, 38, 40, 255));
        constexpr inline glm::vec4 kComponentHeaderHover = hex_col_to_rgba(IM_COL32(45, 45, 48, 255));
        constexpr inline glm::vec4 kComponentHeaderText = hex_col_to_rgba(IM_COL32(210, 215, 225, 255));

        /// component body
        constexpr inline glm::vec4 kComponentBody = hex_col_to_rgba(IM_COL32(30, 30, 32, 255));
        constexpr inline glm::vec4 kComponentSeparator = hex_col_to_rgba(IM_COL32(46, 46, 46, 255));

        /// property rows
        constexpr inline glm::vec4 kPropertyLabel = hex_col_to_rgba(IM_COL32(176, 105, 5, 255));
        constexpr inline glm::vec4 kPropertyValueText = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
        constexpr inline glm::vec4 kPropertyRowHover = hex_col_to_rgba(IM_COL32(40, 40, 42, 255));
        constexpr inline glm::vec4 kPropertyRowAlt = hex_col_to_rgba(IM_COL32(26, 26, 28, 255));
        constexpr inline glm::vec4 kPropertySeparator = hex_col_to_rgba(IM_COL32(38, 38, 38, 255));

        /// input fields
        constexpr inline glm::vec4 kFieldBG = hex_col_to_rgba(IM_COL32(14, 14, 14, 255));
        constexpr inline glm::vec4 kFieldBorder = hex_col_to_rgba(IM_COL32(50, 50, 50, 255));
        constexpr inline glm::vec4 kFieldBorderFocused = hex_col_to_rgba(IM_COL32(236, 158, 36, 200));
        constexpr inline glm::vec4 kFieldBorderError = hex_col_to_rgba(IM_COL32(239, 68, 68, 200));

        /// vector field axis tints (left-border accent)
        constexpr inline glm::vec4 kVecFieldX = hex_col_to_rgba(IM_COL32(160, 45, 45, 255));
        constexpr inline glm::vec4 kVecFieldY = hex_col_to_rgba(IM_COL32(45, 140, 45, 255));
        constexpr inline glm::vec4 kVecFieldZ = hex_col_to_rgba(IM_COL32(45, 80, 160, 255));
        constexpr inline glm::vec4 kVecFieldW = hex_col_to_rgba(IM_COL32(130, 130, 130, 255));

        /// add-component button
        constexpr inline glm::vec4 kAddComponentBG = hex_col_to_rgba(IM_COL32(40, 40, 42, 255));
        constexpr inline glm::vec4 kAddComponentHover = hex_col_to_rgba(IM_COL32(55, 45, 28, 255));
        constexpr inline glm::vec4 kAddComponentText = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));

        /// markers
        constexpr inline glm::vec4 kOverrideMarker = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
        constexpr inline glm::vec4 kModifiedMarker = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));
        constexpr inline glm::vec4 kResetButton = hex_col_to_rgba(IM_COL32(180, 180, 180, 200));
        constexpr inline glm::vec4 kResetButtonHover = hex_col_to_rgba(IM_COL32(255, 255, 255, 255));
      }  // namespace inspector
      //  Scene Hierarchy
      namespace hierarchy {
        constexpr inline glm::vec4 kBG = hex_col_to_rgba(IM_COL32(25, 25, 25, 255));
        constexpr inline glm::vec4 kBorder = hex_col_to_rgba(IM_COL32(46, 46, 46, 255));

        /// item text
        constexpr inline glm::vec4 kItemText = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
        constexpr inline glm::vec4 kItemTextSelected = hex_col_to_rgba(IM_COL32(230, 235, 243, 255));
        constexpr inline glm::vec4 kItemTextDisabled = hex_col_to_rgba(IM_COL32(100, 100, 100, 255));

        /// item backgrounds
        constexpr inline glm::vec4 kItemHover = hex_col_to_rgba(IM_COL32(45, 45, 48, 255));
        constexpr inline glm::vec4 kItemSelected = hex_col_to_rgba(IM_COL32(55, 40, 22, 255));
        constexpr inline glm::vec4 kItemSelectedUnfocused = hex_col_to_rgba(IM_COL32(40, 40, 42, 255));

        /// tree structure
        constexpr inline glm::vec4 kIndentGuide = hex_col_to_rgba(IM_COL32(46, 46, 46, 120));
        constexpr inline glm::vec4 kExpandArrow = hex_col_to_rgba(IM_COL32(140, 140, 140, 255));
        constexpr inline glm::vec4 kExpandArrowHover = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));

        /// drag & drop
        constexpr inline glm::vec4 kDropTarget = hex_col_to_rgba(IM_COL32(39, 185, 242, 160));
        constexpr inline glm::vec4 kDropTargetLine = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
        constexpr inline glm::vec4 kDragPreview = hex_col_to_rgba(IM_COL32(35, 35, 37, 200));

        /// visibility / lock icons
        constexpr inline glm::vec4 kIconVisible = hex_col_to_rgba(IM_COL32(180, 180, 180, 255));
        constexpr inline glm::vec4 kIconHidden = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
        constexpr inline glm::vec4 kIconLocked = hex_col_to_rgba(IM_COL32(200, 80, 80, 255));

        /// search bar
        constexpr inline glm::vec4 kSearchBG = hex_col_to_rgba(IM_COL32(14, 14, 14, 255));
        constexpr inline glm::vec4 kSearchBorder = hex_col_to_rgba(IM_COL32(50, 50, 50, 255));
        constexpr inline glm::vec4 kSearchBorderFocused = hex_col_to_rgba(IM_COL32(236, 158, 36, 200));
        constexpr inline glm::vec4 kSearchMatch = hex_col_to_rgba(IM_COL32(236, 158, 36, 100));
      }  // namespace hierarchy
      //  Console
      namespace console {
        /// backgrounds
        constexpr inline glm::vec4 kBG = hex_col_to_rgba(IM_COL32(8, 8, 8, 230));
        constexpr inline glm::vec4 kBGAlt = hex_col_to_rgba(IM_COL32(12, 12, 12, 230));
        constexpr inline glm::vec4 kBorder = hex_col_to_rgba(IM_COL32(40, 40, 40, 255));
        constexpr inline glm::vec4 kScrollGutter = hex_col_to_rgba(IM_COL32(16, 16, 16, 255));
        /// prompt
        constexpr inline glm::vec4 kPromptSymbol = hex_col_to_rgba(IM_COL32(217, 122, 29, 255));
        constexpr inline glm::vec4 kPromptText = hex_col_to_rgba(IM_COL32(230, 235, 243, 255));
        constexpr inline glm::vec4 kPromptCursor = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));
        constexpr inline glm::vec4 kPromptBG = hex_col_to_rgba(IM_COL32(14, 14, 14, 255));
        constexpr inline glm::vec4 kPromptBorder = hex_col_to_rgba(IM_COL32(50, 40, 25, 255));

        /// log level colors
        constexpr inline glm::vec4 kCommand = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
        constexpr inline glm::vec4 kCommandHistory = hex_col_to_rgba(IM_COL32(39, 185, 242, 140));
        constexpr inline glm::vec4 kOutput = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
        constexpr inline glm::vec4 kTrace = hex_col_to_rgba(IM_COL32(107, 107, 107, 255));
        constexpr inline glm::vec4 kDebug = hex_col_to_rgba(IM_COL32(140, 140, 140, 255));
        constexpr inline glm::vec4 kInfo = hex_col_to_rgba(IM_COL32(193, 208, 193, 255));
        constexpr inline glm::vec4 kWarning = hex_col_to_rgba(IM_COL32(245, 158, 11, 255));
        constexpr inline glm::vec4 kError = hex_col_to_rgba(IM_COL32(239, 68, 68, 255));
        constexpr inline glm::vec4 kFatal = hex_col_to_rgba(IM_COL32(255, 40, 40, 255));
        constexpr inline glm::vec4 kHint = hex_col_to_rgba(IM_COL32(155, 160, 170, 255));

        /// autocomplete popup
        constexpr inline glm::vec4 kAutocompleteBG = hex_col_to_rgba(IM_COL32(22, 22, 22, 245));
        constexpr inline glm::vec4 kAutocompleteBorder = hex_col_to_rgba(IM_COL32(55, 55, 55, 255));
        constexpr inline glm::vec4 kAutocompleteSelected = hex_col_to_rgba(IM_COL32(50, 35, 18, 255));
        constexpr inline glm::vec4 kAutocompleteMatch = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));
        constexpr inline glm::vec4 kAutocompleteText = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
        constexpr inline glm::vec4 kAutocompleteDesc = hex_col_to_rgba(IM_COL32(130, 130, 130, 255));

        /// metadata
        constexpr inline glm::vec4 kTimestamp = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
        constexpr inline glm::vec4 kSource = hex_col_to_rgba(IM_COL32(100, 100, 100, 255));

        /// filter toggles
        constexpr inline glm::vec4 kFilterInactive = hex_col_to_rgba(IM_COL32(60, 60, 60, 255));
        constexpr inline glm::vec4 kFilterHover = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));

        /// inline result highlights
        constexpr inline glm::vec4 kResultValue = hex_col_to_rgba(IM_COL32(242, 190, 112, 255));
        constexpr inline glm::vec4 kResultType = hex_col_to_rgba(IM_COL32(112, 190, 242, 255));
      }  // namespace console

      namespace asset_browser {
        constexpr inline glm::vec4 kBG = hex_col_to_rgba(IM_COL32(25, 25, 25, 255));
        constexpr inline glm::vec4 kBorder = hex_col_to_rgba(IM_COL32(46, 46, 46, 255));

        // grid cards
        constexpr inline glm::vec4 kGridCardBG = hex_col_to_rgba(IM_COL32(35, 35, 37, 255));
        constexpr inline glm::vec4 kGridCardHover = hex_col_to_rgba(IM_COL32(45, 45, 48, 255));
        constexpr inline glm::vec4 kGridCardSelected = hex_col_to_rgba(IM_COL32(55, 40, 22, 255));
        constexpr inline glm::vec4 kGridCardBorder = hex_col_to_rgba(IM_COL32(50, 50, 52, 255));
        constexpr inline glm::vec4 kGridCardBorderSelected = hex_col_to_rgba(IM_COL32(236, 158, 36, 200));

        // thumbnails
        constexpr inline glm::vec4 kThumbnailBG = hex_col_to_rgba(IM_COL32(20, 20, 20, 255));

        // text
        constexpr inline glm::vec4 kFileName = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
        constexpr inline glm::vec4 kFileNameSelected = hex_col_to_rgba(IM_COL32(230, 235, 243, 255));

        // breadcrumbs
        constexpr inline glm::vec4 kBreadcrumbText = hex_col_to_rgba(IM_COL32(155, 160, 170, 255));
        constexpr inline glm::vec4 kBreadcrumbSeparator = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
        constexpr inline glm::vec4 kBreadcrumbCurrent = hex_col_to_rgba(IM_COL32(230, 235, 243, 255));

        // filter pills
        constexpr inline glm::vec4 kFilterInactive = hex_col_to_rgba(IM_COL32(60, 60, 60, 255));
        constexpr inline glm::vec4 kFilterHover = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));

        // directory tree
        constexpr inline glm::vec4 kDirTreeBG = hex_col_to_rgba(IM_COL32(20, 20, 20, 255));
        constexpr inline glm::vec4 kDirItemHover = hex_col_to_rgba(IM_COL32(45, 45, 48, 255));
        constexpr inline glm::vec4 kDirItemSelected = hex_col_to_rgba(IM_COL32(59, 40, 22, 255));

        // status bar
        constexpr inline glm::vec4 kStatusBarBG = hex_col_to_rgba(IM_COL32(20, 20, 20, 255));
        constexpr inline glm::vec4 kStatusText = hex_col_to_rgba(IM_COL32(107, 107, 107, 255));

        // search field
        constexpr inline glm::vec4 kSearchFieldBG = hex_col_to_rgba(IM_COL32(14, 14, 14, 255));
        constexpr inline glm::vec4 kSearchFieldBorder = hex_col_to_rgba(IM_COL32(50, 50, 50, 255));
        constexpr inline glm::vec4 kSearchFieldFocused = hex_col_to_rgba(IM_COL32(236, 158, 36, 200));
      }  // namespace asset_browser
      namespace asset {
        constexpr inline glm::vec4 kTexture = hex_col_to_rgba(IM_COL32(218, 165, 32, 255));
        constexpr inline glm::vec4 kModelSource = hex_col_to_rgba(IM_COL32(90, 130, 180, 255));
        constexpr inline glm::vec4 kModel = hex_col_to_rgba(IM_COL32(65, 160, 235, 255));
        constexpr inline glm::vec4 kAnimation = hex_col_to_rgba(IM_COL32(40, 200, 185, 255));
        constexpr inline glm::vec4 kScriptSource = hex_col_to_rgba(IM_COL32(148, 195, 58, 255));
        constexpr inline glm::vec4 kScript = hex_col_to_rgba(IM_COL32(60, 190, 110, 255));
        constexpr inline glm::vec4 kAudio = hex_col_to_rgba(IM_COL32(210, 80, 170, 255));
        constexpr inline glm::vec4 kScene = hex_col_to_rgba(IM_COL32(225, 140, 50, 255));
        constexpr inline glm::vec4 kMaterial = hex_col_to_rgba(IM_COL32(190, 100, 220, 255));
        // constexpr inline glm::vec4 kSceneObject = hex_col_to_rgba(IM_COL32(235, 120, 90, 255));
        constexpr inline glm::vec4 kFolder = hex_col_to_rgba(IM_COL32(217, 122, 29, 255));
        constexpr inline glm::vec4 kUnknown = hex_col_to_rgba(IM_COL32(107, 107, 107, 255));
      }  // namespace asset
      namespace texture {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(218, 165, 32, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(240, 190, 60, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(218, 165, 32, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(42, 32, 10, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(218, 165, 32, 140));

        constexpr inline glm::vec4 kCheckerLight = hex_col_to_rgba(IM_COL32(50, 50, 50, 255));
        constexpr inline glm::vec4 kCheckerDark = hex_col_to_rgba(IM_COL32(35, 35, 35, 255));

        constexpr inline glm::vec4 kChannelR = hex_col_to_rgba(IM_COL32(220, 60, 60, 255));
        constexpr inline glm::vec4 kChannelG = hex_col_to_rgba(IM_COL32(60, 200, 60, 255));
        constexpr inline glm::vec4 kChannelB = hex_col_to_rgba(IM_COL32(60, 120, 220, 255));
        constexpr inline glm::vec4 kChannelA = hex_col_to_rgba(IM_COL32(200, 200, 200, 255));

        constexpr inline glm::vec4 kMipSlider = hex_col_to_rgba(IM_COL32(218, 165, 32, 200));
        constexpr inline glm::vec4 kZoomBorder = hex_col_to_rgba(IM_COL32(218, 165, 32, 120));
        constexpr inline glm::vec4 kPixelGridLine = hex_col_to_rgba(IM_COL32(80, 80, 80, 60));
      }  // namespace texture
      namespace model_source {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(90, 130, 180, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(120, 158, 205, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(90, 130, 180, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(18, 26, 36, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(90, 130, 180, 140));
      }  // namespace model_source
      namespace model {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(65, 160, 235, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(95, 185, 250, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(65, 160, 235, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(16, 32, 48, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(65, 160, 235, 140));
      }  // namespace model
      namespace animation {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(40, 200, 185, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(70, 225, 210, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(40, 200, 185, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(14, 38, 35, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(40, 200, 185, 140));
      }  // namespace animation
      namespace script_source {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(148, 195, 58, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(170, 215, 80, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(148, 195, 58, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(28, 38, 16, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(148, 195, 58, 140));

        /// syntax highlighting
        constexpr inline glm::vec4 kSyntaxKeyword = hex_col_to_rgba(IM_COL32(198, 120, 221, 255));
        constexpr inline glm::vec4 kSyntaxString = hex_col_to_rgba(IM_COL32(152, 195, 121, 255));
        constexpr inline glm::vec4 kSyntaxNumber = hex_col_to_rgba(IM_COL32(209, 154, 102, 255));
        constexpr inline glm::vec4 kSyntaxComment = hex_col_to_rgba(IM_COL32(92, 99, 112, 255));
        constexpr inline glm::vec4 kSyntaxType = hex_col_to_rgba(IM_COL32(97, 175, 239, 255));
        constexpr inline glm::vec4 kSyntaxFunction = hex_col_to_rgba(IM_COL32(86, 182, 194, 255));
        constexpr inline glm::vec4 kSyntaxOperator = hex_col_to_rgba(IM_COL32(190, 190, 190, 255));

        /// language badges
        constexpr inline glm::vec4 kLuaBadge = hex_col_to_rgba(IM_COL32(0, 0, 128, 255));
        constexpr inline glm::vec4 kDotnetBadge = hex_col_to_rgba(IM_COL32(104, 33, 122, 255));

        /// compile status
        constexpr inline glm::vec4 kCompileSuccess = hex_col_to_rgba(IM_COL32(74, 222, 128, 255));
        constexpr inline glm::vec4 kCompileError = hex_col_to_rgba(IM_COL32(239, 68, 68, 255));
        constexpr inline glm::vec4 kCompileWarning = hex_col_to_rgba(IM_COL32(245, 158, 11, 255));

        /// line numbers
        constexpr inline glm::vec4 kLineNumber = hex_col_to_rgba(IM_COL32(90, 95, 100, 255));
        constexpr inline glm::vec4 kLineNumberActive = hex_col_to_rgba(IM_COL32(148, 195, 58, 200));
      }  // namespace script_source
      namespace script {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(60, 190, 110, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(85, 215, 135, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(60, 190, 110, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(16, 36, 24, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(60, 190, 110, 140));

        /// field visibility dots
        constexpr inline glm::vec4 kExposedProperty = hex_col_to_rgba(IM_COL32(60, 190, 110, 200));
        constexpr inline glm::vec4 kBoundProperty = hex_col_to_rgba(IM_COL32(39, 185, 242, 200));
        constexpr inline glm::vec4 kUnboundProperty = hex_col_to_rgba(IM_COL32(140, 140, 140, 160));
        constexpr inline glm::vec4 kOverriddenValue = hex_col_to_rgba(IM_COL32(236, 158, 36, 200));
      }  // namespace script
      namespace audio {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(210, 80, 170, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(230, 110, 195, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(210, 80, 170, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(40, 18, 34, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(210, 80, 170, 140));
      }  // namespace audio
      namespace scene {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(225, 140, 50, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(245, 165, 75, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(225, 140, 50, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(42, 28, 14, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(225, 140, 50, 140));
      }  // namespace scene
      namespace asset_editor {
        constexpr inline glm::vec4 kBG = hex_col_to_rgba(IM_COL32(25, 25, 25, 255));
        constexpr inline glm::vec4 kPanel = hex_col_to_rgba(IM_COL32(35, 35, 35, 255));
        constexpr inline glm::vec4 kField = hex_col_to_rgba(IM_COL32(14, 14, 14, 255));
        constexpr inline glm::vec4 kFieldHover = hex_col_to_rgba(IM_COL32(18, 18, 18, 255));
        constexpr inline glm::vec4 kBorder = hex_col_to_rgba(IM_COL32(55, 55, 55, 255));
        constexpr inline glm::vec4 kHeaderBG = hex_col_to_rgba(IM_COL32(30, 30, 30, 255));
        constexpr inline glm::vec4 kText = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
        constexpr inline glm::vec4 kTextMuted = hex_col_to_rgba(IM_COL32(140, 140, 140, 255));
        constexpr inline glm::vec4 kPropertyLabel = hex_col_to_rgba(IM_COL32(170, 175, 185, 255));
        constexpr inline glm::vec4 kPropertySep = hex_col_to_rgba(IM_COL32(42, 42, 42, 255));
        constexpr inline glm::vec4 kDirtyMarker = hex_col_to_rgba(IM_COL32(236, 158, 36, 255));
        constexpr inline glm::vec4 kReadOnlyOverlay = hex_col_to_rgba(IM_COL32(0, 0, 0, 40));
      }  // namespace asset_editor
      namespace texture_editor {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(218, 165, 32, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(238, 185, 55, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(218, 165, 32, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(40, 32, 18, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(218, 165, 32, 140));
        constexpr inline glm::vec4 kCheckerLight = hex_col_to_rgba(IM_COL32(60, 60, 60, 255));
        constexpr inline glm::vec4 kCheckerDark = hex_col_to_rgba(IM_COL32(40, 40, 40, 255));
        constexpr inline glm::vec4 kChannelR = hex_col_to_rgba(IM_COL32(220, 60, 60, 255));
        constexpr inline glm::vec4 kChannelG = hex_col_to_rgba(IM_COL32(60, 200, 60, 255));
        constexpr inline glm::vec4 kChannelB = hex_col_to_rgba(IM_COL32(60, 100, 220, 255));
        constexpr inline glm::vec4 kChannelA = hex_col_to_rgba(IM_COL32(180, 180, 180, 255));
      }  // namespace texture_editor
      namespace script_source_editor {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(148, 195, 58, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(170, 215, 80, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(148, 195, 58, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(28, 38, 16, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(148, 195, 58, 140));
        constexpr inline glm::vec4 kSyntaxKeyword = hex_col_to_rgba(IM_COL32(198, 120, 221, 255));
        constexpr inline glm::vec4 kSyntaxString = hex_col_to_rgba(IM_COL32(152, 195, 121, 255));
        constexpr inline glm::vec4 kSyntaxNumber = hex_col_to_rgba(IM_COL32(209, 154, 102, 255));
        constexpr inline glm::vec4 kSyntaxComment = hex_col_to_rgba(IM_COL32(92, 99, 112, 255));
        constexpr inline glm::vec4 kSyntaxType = hex_col_to_rgba(IM_COL32(97, 175, 239, 255));
        constexpr inline glm::vec4 kSyntaxFunction = hex_col_to_rgba(IM_COL32(86, 182, 194, 255));
        constexpr inline glm::vec4 kSyntaxOperator = hex_col_to_rgba(IM_COL32(190, 190, 190, 255));
        constexpr inline glm::vec4 kLuaBadge = hex_col_to_rgba(IM_COL32(0, 0, 128, 255));
        constexpr inline glm::vec4 kDotnetBadge = hex_col_to_rgba(IM_COL32(104, 33, 122, 255));
        constexpr inline glm::vec4 kCompileSuccess = hex_col_to_rgba(IM_COL32(74, 222, 128, 255));
        constexpr inline glm::vec4 kCompileError = hex_col_to_rgba(IM_COL32(239, 68, 68, 255));
        constexpr inline glm::vec4 kCompileWarning = hex_col_to_rgba(IM_COL32(245, 158, 11, 255));
        constexpr inline glm::vec4 kLineNumber = hex_col_to_rgba(IM_COL32(90, 95, 100, 255));
        constexpr inline glm::vec4 kLineNumberActive = hex_col_to_rgba(IM_COL32(148, 195, 58, 200));
      }  // namespace script_source_editor
      namespace script_editor {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(60, 190, 110, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(85, 215, 135, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(60, 190, 110, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(16, 36, 24, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(60, 190, 110, 140));
        constexpr inline glm::vec4 kExposedProperty = hex_col_to_rgba(IM_COL32(60, 190, 110, 200));
        constexpr inline glm::vec4 kBoundProperty = hex_col_to_rgba(IM_COL32(39, 185, 242, 200));
        constexpr inline glm::vec4 kUnboundProperty = hex_col_to_rgba(IM_COL32(140, 140, 140, 160));
        constexpr inline glm::vec4 kOverriddenValue = hex_col_to_rgba(IM_COL32(236, 158, 36, 200));
      }  // namespace script_editor
      //  Scene Object Editor
      namespace scene_object {
        constexpr inline glm::vec4 kSignature = hex_col_to_rgba(IM_COL32(235, 120, 90, 255));
        constexpr inline glm::vec4 kSignatureHover = hex_col_to_rgba(IM_COL32(250, 148, 118, 255));
        constexpr inline glm::vec4 kSignatureMuted = hex_col_to_rgba(IM_COL32(235, 120, 90, 100));
        constexpr inline glm::vec4 kHeaderTint = hex_col_to_rgba(IM_COL32(45, 24, 18, 255));
        constexpr inline glm::vec4 kBorderAccent = hex_col_to_rgba(IM_COL32(235, 120, 90, 140));

        /// per-component-type signature dots
        constexpr inline glm::vec4 kComponentTransform = hex_col_to_rgba(IM_COL32(65, 160, 235, 200));
        constexpr inline glm::vec4 kComponentRenderer = hex_col_to_rgba(IM_COL32(218, 165, 32, 200));
        constexpr inline glm::vec4 kComponentPhysics = hex_col_to_rgba(IM_COL32(140, 100, 200, 200));
        constexpr inline glm::vec4 kComponentScript = hex_col_to_rgba(IM_COL32(60, 190, 110, 200));
        constexpr inline glm::vec4 kComponentAudio = hex_col_to_rgba(IM_COL32(210, 80, 170, 200));
        constexpr inline glm::vec4 kComponentPointLight = hex_col_to_rgba(IM_COL32(255, 230, 120, 200));
        constexpr inline glm::vec4 kComponentDirectionLight = hex_col_to_rgba(IM_COL32(255, 200, 100, 200));
        constexpr inline glm::vec4 kComponentCamera = hex_col_to_rgba(IM_COL32(180, 180, 180, 200));
        constexpr inline glm::vec4 kComponentGrid = hex_col_to_rgba(IM_COL32(120, 205, 235, 200));
        constexpr inline glm::vec4 kComponentAnimation = hex_col_to_rgba(IM_COL32(40, 200, 185, 200));
        constexpr inline glm::vec4 kComponentRegistry = hex_col_to_rgba(IM_COL32(100, 150, 200, 200));
        constexpr inline glm::vec4 kComponentCustom = hex_col_to_rgba(IM_COL32(155, 160, 170, 200));

        /// asset slot states
        constexpr inline glm::vec4 kAssetSlotEmpty = hex_col_to_rgba(IM_COL32(60, 60, 60, 200));
        constexpr inline glm::vec4 kAssetSlotDragHover = hex_col_to_rgba(IM_COL32(39, 185, 242, 200));
        constexpr inline glm::vec4 kAssetSlotLoading = hex_col_to_rgba(IM_COL32(218, 165, 32, 200));
        constexpr inline glm::vec4 kAssetSlotFilled = hex_col_to_rgba(IM_COL32(236, 158, 36, 160));
        constexpr inline glm::vec4 kAssetSlotInvalid = hex_col_to_rgba(IM_COL32(239, 68, 68, 160));
      }  // namespace scene_object

    }  // namespace colors
  }  // namespace ui
}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_UI_COLORS_HPP