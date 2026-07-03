/**
 * \file renderer/colors.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_COLORS_HPP
#define OTHER_RENDERER_RENDERER_COLORS_HPP

#include <glm/vec4.hpp>

namespace other {
  namespace basic_colors {

    constexpr glm::vec4 kWhite{ 1, 1, 1, 1 };
    constexpr glm::vec4 kRed{ 1, 0, 0, 1 };
    constexpr glm::vec4 kGreen{ 0, 1, 0, 1 };
    constexpr glm::vec4 kBlue{ 0.2f, 0.4f, 1, 1 };
    constexpr glm::vec4 kYellow{ 1, 1, 0, 1 };

  }  // namespace basic_colors
}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_COLORS_HPP