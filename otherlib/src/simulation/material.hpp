/**
 * \file simulation/material.hpp
 **/
#ifndef OTHER_SIMULATION_MATERIAL_HPP
#define OTHER_SIMULATION_MATERIAL_HPP

#include "core/ref_counted.hpp"

namespace other {

  class material : public other::ref_counted {
   public:
    virtual ~material() = default;
  };

}  // namespace other

#endif  // OTHER_SIMULATION_MATERIAL_HPP