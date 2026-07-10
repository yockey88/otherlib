/**
 * \file neural/layer.hpp-
 **/
#ifndef TENSORLIB_NEURAL_LAYER_HPP
#define TENSORLIB_NEURAL_LAYER_HPP

#include <concepts>

#include "math/matrix.hpp"
#include "math/vector.hpp"

#include "neural/activation.hpp"

namespace other {

  template <natural_t R, natural_t C>
  struct layer {
    matrix<R, C> weights;
    vector<R> bias;

    // derivative_fn_t<R> activation_fn = &sigmoid;

    // layer(const matrix<R, C>& w, const vector<R>& b, derivative_fn_t<R> activation_fn = &sigmoid)
    //     : weights(w), bias(b), activation_fn(activation_fn) {}

    // vector<R> activate(const vector<C>& input) const {
    //   vector<R> product = matrix_vector_product(weights, input);
    //   vector<R> result = vector_sum(product, bias);
    //   return activation_fn(result);
    // }
  };

  template <natural_t R, natural_t C>
  layer(const matrix<R, C>&, const vector<R>&) -> layer<R, C>;

}  // namespace other

#endif  // TENSORLIB_NEURAL_LAYER_HPP