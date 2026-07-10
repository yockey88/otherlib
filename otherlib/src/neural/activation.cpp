/**
 * \file neural/activation.cpp
 **/
#include "neural/activation.hpp"

namespace tensor {
  namespace neural {

    dyn_vector sigmoid(const dyn_vector& v) {
      dyn_vector res{ v.size };
      for (natural_t i = 0; i < v.size; ++i) {
        res(i) = detail::sigmoid(v(i));
      }
      return res;
    }

    dyn_vector sigmoid_derivative(const dyn_vector& v) {
      dyn_vector res{ v.size };
      for (natural_t i = 0; i < v.size; ++i) {
        res(i) = detail::sigmoid(v(i)) * (1 - detail::sigmoid(v(i)));
      }
      return res;
    }

    dyn_vector relu(const dyn_vector& v) {
      dyn_vector res{ v.size };
      for (natural_t i = 0; i < v.size; ++i) {
        res(i) = detail::relu(v(i));
      }
      return res;
    }

    dyn_vector relu_derivative(const dyn_vector& v) {
      dyn_vector res{ v.size };
      for (natural_t i = 0; i < v.size; ++i) {
        res(i) = v(i) > 0 ? 1.f : 0.f;
      }
      return res;
    }

  }  // namespace neural
}  // namespace tensor
