/**
 * \file math/vector.hpp
 **/
#ifndef OTHER_CORE_MATH_VECTOR_HPP
#define OTHER_CORE_MATH_VECTOR_HPP

#include "core/defines.hpp"

namespace other {

  template <natural_t N>
  struct vector;

  namespace detail {

    consteval real_t square_root(real_t value) {
      if (value == 0) {
        return 0;
      }

      real_t x = value / 2;
      real_t y = 0;
      while (x != y) {
        y = x;
        x = (value / x + x) / 2;
      }
      return x;
    }

    template <typename T, natural_t N, natural_t... I>
    constexpr std::array<T, N> create_array_helper(std::index_sequence<I...>, T (&values)[N]) {
      return { values[I]... };
    }

    template <typename T, natural_t N>
    constexpr std::array<T, N> create_array(T (&values)[N]) {
      return create_array_helper<T, N>(std::make_index_sequence<N>{}, values);
    }

  }  // namespace detail

  template <natural_t N>
  struct vector {
    constexpr vector() = default;

    constexpr vector(real_t val) {
      static_assert(N > 0, "Vector dimension must be greater than zero.");
      values.fill(val);
    }

    constexpr vector(const std::array<real_t, N>& vals) {
      values = vals;
    }

    template <typename... Vals>
      requires(sizeof...(Vals) == N && (std::is_same_v<std::remove_cvref_t<Vals>, real_t> && ...))
    constexpr vector(Vals&&... vals) {
      static_assert(sizeof...(vals) == N, "Number of arguments must match the dimension of the point.");
      static_assert((std::is_same_v<std::remove_cvref_t<decltype(vals)>, real_t> && ...), "All arguments must be of type real.");

      values = std::array<real_t, sizeof...(vals)>{ vals... };
    }

    constexpr vector(real_t (&vals)[N]) {
      values = detail::create_array<real_t, N>(vals);
    }

    template <typename F>
      requires(std::is_invocable_r_v<real_t, F, const std::array<real_t, N>&>)
    constexpr real_t apply(F&& fn) const noexcept {
      return std::invoke(std::forward<F>(fn), values);
    }

    constexpr auto begin() const noexcept { return values.begin(); }
    constexpr auto end() const noexcept { return values.end(); }
    constexpr auto size() const noexcept { return values.size(); }
    constexpr auto data() const noexcept { return values.data(); }

    constexpr real_t& operator[](natural_t i) { return values[i]; }
    constexpr real_t operator[](natural_t i) const { return values[i]; }

    static std::string write_string(const vector& v) {
      std::string result = "{ ";
      for (natural_t i = 0; i < N; ++i) {
        result += std::to_string(v[i]);
        if (i != N - 1) {
          result += ", ";
        }
      }
      result += " }";
      return result;
    }

    std::array<real_t, N> values{ 0.f };
  };

  template <natural_t N1, natural_t N2>
  inline constexpr bool operator==(const vector<N1>& lhs, const vector<N2>& rhs) {
    static_assert(N1 > 0 && N2 > 0, "Vectors must have the same dimension to be compared.");
    if constexpr (N1 == N2) {
      return std::ranges::equal(lhs, rhs);
    } else {
      return false;
    }
  };

  template <natural_t N1, natural_t N2>
  inline constexpr bool operator!=(const vector<N1>& lhs, const vector<N2>& rhs) {
    return !(lhs == rhs);
  };

  template <natural_t N>
  constexpr inline vector<N> origin;  /// all zeroes

  namespace detail {

    template <natural_t N, typename Fn, natural_t I>
      requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
    constexpr real_t apply_to_vector_elements_at(const vector<N>& p1, const vector<N>& p2, Fn&& fn) {
      return std::invoke(std::forward<Fn>(fn), p1[I], p2[I]);
    }

    template <natural_t N, typename Fn, size_t... I>
      requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
    constexpr vector<N> apply_to_vector_elements_helper(const vector<N>& p1, const vector<N>& p2, Fn&& fn, std::index_sequence<I...>) {
      return vector<N>{ apply_to_vector_elements_at<N, Fn, I>(p1, p2, std::forward<Fn>(fn))... };
    }

    template <natural_t N, typename Fn>
      requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
    constexpr vector<N> apply_to_vector_elements(const vector<N>& p1, const vector<N>& p2, Fn&& fn) {
      return apply_to_vector_elements_helper<N, Fn>(p1, p2, std::forward<Fn>(fn), std::make_index_sequence<N>{});
    }

    template <natural_t N, typename OuterFn, typename InnerFn>
      requires std::is_invocable_r_v<real_t, OuterFn, real_t, real_t> && std::is_invocable_r_v<real_t, InnerFn, real_t, real_t>
    constexpr real_t accumulate_vector_elements(const vector<N>& p1, const vector<N>& p2, real_t acc, OuterFn&& outer_fn, InnerFn&& inner_fn) {
      return fold_left(apply_to_vector_elements(p1, p2, inner_fn).values, 0.f, std::forward<OuterFn>(outer_fn));
    }

    struct vector_sum_fn {
      template <natural_t N>
      constexpr auto operator()(const vector<N>& p1, const vector<N>& p2) const {
        return apply_to_vector_elements(p1, p2, [](const real_t& a, const real_t& b) { return a + b; });
      }
    };

    struct vector_difference_fn {
      template <natural_t N>
      constexpr auto operator()(const vector<N>& p1, const vector<N>& p2) const {
        return apply_to_vector_elements(p1, p2, [](const real_t& a, const real_t& b) { return a - b; });
      }
    };

    struct vector_scalar_product_fn {
      template <natural_t N>
      constexpr auto operator()(const real_t scalar, const vector<N>& p) const {
        return apply_scalar_product<N>(scalar, p);
      }

      template <natural_t N>
      constexpr auto operator()(const vector<N>& p, const real_t scalar) const {
        return (*this)(scalar, p);
      }

     private:
      template <natural_t N>
      constexpr vector<N> apply_scalar_product(const real_t scalar, const vector<N>& p) const {
        return vector_component_product<N>(p, scalar, [](const real_t& a, const real_t& b) { return a * b; });
      }

      template <natural_t N, typename Fn>
        requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
      constexpr vector<N> vector_component_product(const vector<N>& p, const real_t scalar, Fn&& fn) const {
        return vector_component_product_helper<N, Fn>(p, scalar, std::forward<Fn>(fn), std::make_index_sequence<N>{});
      }

      template <natural_t N, typename Fn, size_t... I>
        requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
      constexpr vector<N> vector_component_product_helper(const vector<N>& p, const real_t scalar, Fn&& fn, std::index_sequence<I...>) const {
        return vector<N>{ vector_component_product_at<N, Fn, I>(scalar, p, std::forward<Fn>(fn))... };
      }

      template <natural_t N, typename Fn, size_t I>
        requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
      constexpr real_t vector_component_product_at(const real_t scalar, const vector<N>& p, Fn&& fn) const {
        return std::invoke(std::forward<Fn>(fn), scalar, p[I]);
      }
    };

    struct vector_magnitude_fn {
      template <natural_t N>
      constexpr auto operator()(const vector<N>& p) const {
        return square_root(fold_left(p.values, 0.f, [](const real_t& acc, const real_t& x) {
          return acc + (x * x);
        }));
      }
    };

    struct vector_hadamard_product_fn {
      template <natural_t N>
      constexpr auto operator()(const vector<N>& p1, const vector<N>& p2) const {
        return apply_to_vector_elements(p1, p2, [](const real_t& a, const real_t& b) { return a * b; });
      }
    };

    struct dot_product_fn {
      template <natural_t N>
      constexpr real_t operator()(const vector<N>& p1, const vector<N>& p2) const {
        return accumulate_vector_elements(
          p1, p2, 0.f,
          [](const real_t& acc, const real_t& x) { return acc + x; },
          [](const real_t& x, const real_t& y) { return x * y; }
        );
      }
    };

  }  // namespace detail

  constexpr inline detail::vector_sum_fn vector_sum{};
  constexpr inline detail::vector_difference_fn vector_difference{};
  constexpr inline detail::vector_scalar_product_fn scalar_product{};
  constexpr inline detail::vector_magnitude_fn vector_magnitude{};
  constexpr inline detail::vector_hadamard_product_fn hadamard_product{};
  constexpr inline detail::dot_product_fn dot_product{};

}  // namespace other

#endif  // OTHER_CORE_MATH_VECTOR_HPP