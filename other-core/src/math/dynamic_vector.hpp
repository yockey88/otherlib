/**
 * \file math/dynamic_vector.hpp
 **/
#ifndef OTHER_CORE_MATH_DYNAMIC_VECTOR_HPP
#define OTHER_CORE_MATH_DYNAMIC_VECTOR_HPP

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "data-structures/std_container.hpp"
#include "math/vector.hpp"

namespace other {

  struct dynamic_vector {
    dynamic_vector() = default;
    dynamic_vector(natural_t s);
    dynamic_vector(natural_t s, real_t vals);

    dynamic_vector(dynamic_vector&& other) noexcept;
    dynamic_vector(const dynamic_vector& other);
    dynamic_vector& operator=(dynamic_vector&& other) noexcept;
    dynamic_vector& operator=(const dynamic_vector& other);

    template <natural_t N>
    dynamic_vector(const std::array<real_t, N>& vals)
        : size(N) {
      OTHER_ASSERT(size > 0, "Vector size must be greater than zero.");

      data.reserve(size);
      for (natural_t i = 0; i < N; ++i) {
        data[i] = vals[i];
      }
    }

    template <natural_t N>
    dynamic_vector(const vector<N>& vals)
        : size(N) {
      OTHER_ASSERT(size > 0, "Vector size must be greater than zero.");

      data.reserve(size);
      for (natural_t i = 0; i < N; ++i) {
        data[i] = vals[i];
      }
    }

    real_t& operator[](natural_t i) { return data[i]; }
    real_t operator[](natural_t i) const { return data[i]; }

    real_t& operator()(natural_t i) { return data[i]; }
    real_t operator()(natural_t i) const { return data[i]; }

    static std::string write_string(const dynamic_vector& v);

   public:
    natural_t size = 0;
    ostd::vector<real_t> data = {};
  };

  using vectorn = dynamic_vector;

  // static inline vectorn make_vector(natural_t size) {
  //   return vectorn::create(size);
  // }

  namespace detail {

    struct dynamic_vector_sum_fn {
      dynamic_vector operator()(const dynamic_vector& v1, const dynamic_vector& v2) const;
    };

    struct dynamic_vector_difference_fn {
      dynamic_vector operator()(const dynamic_vector& v1, const dynamic_vector& v2) const;
    };

    struct dynamic_vector_hadamard_product_fn {
      dynamic_vector operator()(const dynamic_vector& v1, const dynamic_vector& v2) const;
    };

    struct dynamic_vector_scalar_product_fn {
      dynamic_vector operator()(const dynamic_vector& v, const real_t scalar) const;
    };

    struct dynamic_vector_dot_product_fn {
      real_t operator()(const dynamic_vector& v1, const dynamic_vector& v2) const;
    };

    struct dynamic_vector_magnitude_fn {
      real_t operator()(const dynamic_vector& v) const;
    };

    struct dynamic_vector_rep_fn {
      dynamic_vector operator()(const real_t value, const natural_t size) const;
    };

  }  // namespace detail

  constexpr inline detail::dynamic_vector_sum_fn dynamic_vector_sum{};
  constexpr inline detail::dynamic_vector_difference_fn dynamic_vector_difference{};
  constexpr inline detail::dynamic_vector_hadamard_product_fn dynamic_vector_hadamard_product{};
  constexpr inline detail::dynamic_vector_scalar_product_fn dynamic_vector_scalar_product{};
  constexpr inline detail::dynamic_vector_dot_product_fn dynamic_vector_dot_product{};
  constexpr inline detail::dynamic_vector_magnitude_fn dynamic_vector_magnitude{};
  constexpr inline detail::dynamic_vector_rep_fn dynamic_vector_rep{};

  dynamic_vector rand_vector(natural_t size, real_t min = -1.f, real_t max = 1.f);

}  // namespace other

namespace std {

  template <>
  struct formatter<other::dynamic_vector> : public std::formatter<std::string> {
    auto format(const other::dynamic_vector& v, std::format_context& ctx) const {
      std::string s = other::dynamic_vector::write_string(v);
      return std::format_to(ctx.out(), "{}", s);
    }
  };

}  // namespace std

#endif  // OTHER_CORE_MATH_DYNAMIC_VECTOR_HPP