/**
 * \file neural/activation.hpp
 **/
#ifndef OTHERLIB_NEURAL_ACTIVATION_HPP
#define OTHERLIB_NEURAL_ACTIVATION_HPP

#include <utility>

#include "core/defines.hpp"
#include "math/dynamic_matrix.hpp"
#include "math/dynamic_vector.hpp"
#include "math/vector.hpp"

namespace other {
  namespace detail {

    // constexpr bool isnan(real_t x) {
    //   return !(x == x);
    // }

    // Sigmoid activation function
    constexpr real_t sigmoid(real_t x) {
      return 1.0f / (1.0f + std::exp(-x));
    }

    constexpr real_t sigmoid_derivative(real_t x) {
      return sigmoid(x) * (1.0f - sigmoid(x));
    }

    constexpr real_t relu(real_t x) {
      return x > 0 ? x : 0;
    }

    constexpr real_t relu_derivative(real_t x) {
      return x > 0 ? 1.f : 0.f;
    }

  }  // namespace detail

  template <natural_t N, typename Fn, size_t... I>
    requires std::is_invocable_r_v<real_t, Fn, real_t>
  constexpr std::array<real_t, N> activate_vector_element(const vector<N>& v, Fn&& fn, std::index_sequence<I...>) {
    return std::array<real_t, N>{ std::invoke(std::forward<Fn>(fn), v[I])... };
  }

  template <natural_t N>
  constexpr vector<N> sigmoid(const vector<N>& v) {
    return vector<N>{ activate_vector_element(v, &detail::sigmoid, std::make_index_sequence<N>{}) };
  }

  template <natural_t N>
  constexpr vector<N> sigmoid_derivative(const vector<N>& v) {
    return vector<N>{ activate_vector_element(v, [](real_t x) { return x * (1 - x); }, std::make_index_sequence<N>{}) };
  }

  template <natural_t N>
  constexpr vector<N> relu(const vector<N>& v) {
    return vector<N>{ activate_vector_element(v, &detail::relu, std::make_index_sequence<N>{}) };
  }

  template <natural_t N>
  constexpr vector<N> relu_derivative(const vector<N>& v) {
    return vector<N>{ activate_vector_element(v, [](real_t x) { return x > 0 ? 1.f : 0.f; }, std::make_index_sequence<N>{}) };
  }

  // template <natural_t N>
  // constexpr vector<N> clipping(const vector<N>& v, real_t min, real_t max) {
  //   return vector<N>{ activate_vector_element(v, [min, max](real_t x) { return std::clamp(x, min, max); }, std::make_index_sequence<N>{}) };
  // }

  // template <natural_t N>
  // constexpr vector<N> activate(const vector<N>& v) {
  //   return vector<N>{ activate_vector_element(v, [](real_t x) -> real_t { return x <= 0.f ? -1.f : 1.f; }, std::make_index_sequence<N>{}) };
  // }

  using nn_matrix = dynamic_matrix<real_t>;
  using nn_vector = dynamic_vector;

  nn_vector sigmoid(const nn_vector& v);
  nn_vector sigmoid_derivative(const nn_vector& v);

  nn_vector relu(const nn_vector& v);
  nn_vector relu_derivative(const nn_vector& v);

  // nn_vector activate(nn_vector& v);

  using derivative_fn_t = nn_vector (*)(const nn_vector&);
  using nn_activation_fn_t = nn_vector (*)(const nn_vector&);

  /// \todo replace these functions with a computation graph
  struct layer_activation {
    nn_activation_fn_t activation_function = nullptr;
    derivative_fn_t derivative_function = nullptr;
  };

  namespace layer_defns {

    constexpr inline layer_activation sigmoid_layer = { &sigmoid, &sigmoid_derivative };
    constexpr inline layer_activation relu_layer = { &relu, &relu_derivative };

  }  // namespace layer_defns
}  // namespace other

#endif  // OTHERLIB_NEURAL_ACTIVATION_HPP