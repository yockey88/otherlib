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
        return col.operator ImU32();
      }

      static inline ImU32 imvec4_to_hex(const ImVec4& color) {
        ImColor col{ color.x, color.y, color.z, color.w };
        return col.operator ImU32();
      }

      static inline ImU32 to_im_col(const glm::vec4& c) {
        return colors::rgba_to_hex(c);
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
      constexpr inline glm::vec4 kNodeBodyColor = hex_col_to_rgba(IM_COL32(59, 59, 59, 255));
      constexpr inline glm::vec4 kBasicNodeLinkColor = hex_col_to_rgba(IM_COL32(199, 199, 199, 255));
      constexpr inline glm::vec4 kDataLinkColor = hex_col_to_rgba(IM_COL32(39, 185, 242, 255));
      constexpr inline glm::vec4 kNodeCardStrokeColor = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
      constexpr inline glm::vec4 kNodeOutlineColor = hex_col_to_rgba(IM_COL32(107, 107, 107, 255));
      constexpr inline glm::vec4 kNodeTitleTextColor = hex_col_to_rgba(IM_COL32(230, 235, 243, 255));
      constexpr inline glm::vec4 kNodeEditorBackground = hex_col_to_rgba(IM_COL32(20, 20, 20, 255));

      //  Engine Modes
      constexpr inline glm::vec4 kModeStoppedBar = hex_col_to_rgba(IM_COL32(80, 80, 80, 255));
      constexpr inline glm::vec4 kModePlayingBar = hex_col_to_rgba(IM_COL32(34, 197, 94, 255));
      constexpr inline glm::vec4 kModePlayingTint = hex_col_to_rgba(IM_COL32(11, 42, 26, 255));

      //  Property Inspector  (grid-tool-inspector from theme reference)
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
        constexpr inline glm::vec4 kPropertyLabel = hex_col_to_rgba(IM_COL32(170, 175, 185, 255));
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

      //  Scene Hierarchy  (grid-tool-hierarchy from theme reference)
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

      //  Console  (grid-tool-console from theme reference)
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

      //  Scene Object Editor  (grid-asset-scene-object from theme reference)
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
        constexpr inline glm::vec4 kComponentLight = hex_col_to_rgba(IM_COL32(255, 230, 120, 200));
        constexpr inline glm::vec4 kComponentCamera = hex_col_to_rgba(IM_COL32(180, 180, 180, 200));
        constexpr inline glm::vec4 kComponentAnimation = hex_col_to_rgba(IM_COL32(40, 200, 185, 200));
        constexpr inline glm::vec4 kComponentRegistry = hex_col_to_rgba(IM_COL32(100, 150, 200, 200));
        constexpr inline glm::vec4 kComponentCustom = hex_col_to_rgba(IM_COL32(155, 160, 170, 200));

        /// asset slot states
        constexpr inline glm::vec4 kAssetSlotEmpty = hex_col_to_rgba(IM_COL32(60, 60, 60, 200));
        constexpr inline glm::vec4 kAssetSlotFilled = hex_col_to_rgba(IM_COL32(236, 158, 36, 160));
        constexpr inline glm::vec4 kAssetSlotInvalid = hex_col_to_rgba(IM_COL32(239, 68, 68, 160));
        constexpr inline glm::vec4 kAssetSlotDragHover = hex_col_to_rgba(IM_COL32(39, 185, 242, 200));
      }  // namespace scene_object

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