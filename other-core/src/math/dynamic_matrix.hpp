/**
 * \file math/dynamic_matrix.hpp
 **/
#ifndef OTHER_CORE_MATH_DYNAMIC_MATRIX_HPP
#define OTHER_CORE_MATH_DYNAMIC_MATRIX_HPP

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/ref.hpp"
#include "math/dynamic_vector.hpp"
#include "math/matrix.hpp"

#include "data-structures/arena_vector.hpp"

namespace other {

  struct dynamic_matrix : public ref_counted {
    natural_t rows = 0;
    natural_t cols = 0;

    dynamic_matrix() = default;
    dynamic_matrix(natural_t r, natural_t c);
    dynamic_matrix(natural_t r, natural_t c, real_t vals);

    dynamic_matrix(dynamic_matrix&& other) noexcept;
    dynamic_matrix& operator=(dynamic_matrix&& other) noexcept;

    dynamic_matrix(const dynamic_matrix& other) = delete;
    dynamic_matrix& operator=(const dynamic_matrix& other) = delete;

    static inline dynamic_matrix create_matrix(natural_t rows, natural_t cols) {
      return dynamic_matrix(rows, cols);
    }

    template <natural_t R, natural_t C>
    static inline dynamic_matrix create_matrix(const matrix<R, C>& vals) {
      return dynamic_matrix(vals);
    }

    template <size_t R, size_t C, size_t N>
    dynamic_matrix(const std::array<real_t, N>& vals)
        : rows(R), cols(C) {
      static_assert(N == R * C, "Array size must match matrix dimensions.");
      OTHER_ASSERT(rows > 0 && cols > 0, "Matrix dimensions must be greater than zero.");

      data.reserve(rows * cols);
      for (size_t i = 0; i < N; ++i) {
        data[i] = vals[i];
      }
    }

    template <size_t R, size_t C>
    dynamic_matrix(const std::array<std::array<real_t, C>, R>& vals)
        : rows(R), cols(C) {
      OTHER_ASSERT(rows > 0 && cols > 0, "Matrix dimensions must be greater than zero.");
      data.reserve(rows * cols);
      for (size_t i = 0; i < R; ++i) {
        for (size_t j = 0; j < C; ++j) {
          (*this)(i, j) = vals[i][j];
        }
      }
    }

    template <natural_t R, natural_t C>
    dynamic_matrix(const matrix<R, C>& vals)
        : rows(R), cols(C) {
      OTHER_ASSERT(rows > 0 && cols > 0, "Matrix dimensions must be greater than zero.");
      data.reserve(rows * cols);
      for (natural_t i = 0; i < R; ++i) {
        for (natural_t j = 0; j < C; ++j) {
          (*this)(i, j) = vals(i, j);
        }
      }
    }

    real_t& operator()(natural_t row, natural_t col);
    real_t operator()(natural_t row, natural_t col) const;

    real_t& operator[](natural_t i) { return data[i]; }
    real_t operator[](natural_t i) const { return data[i]; }

    dynamic_vector get_row(natural_t row) const;
    dynamic_vector get_col(natural_t col) const;

    static std::string write_string(const dynamic_matrix& m);

    arena_vector<real_t> data;
  };

  using matrix_nxm = dynamic_matrix;

  namespace detail {

    struct dynamic_matrix_sum_fn {
      dynamic_matrix operator()(const dynamic_matrix& m1, const dynamic_matrix& m2) const;
    };

    struct dynamic_matrix_difference_fn {
      dynamic_matrix operator()(const dynamic_matrix& m1, const dynamic_matrix& m2) const;
    };

    struct dynamic_matrix_product_fn {
      dynamic_matrix operator()(const dynamic_matrix& m1, const dynamic_matrix& m2) const;
    };

    struct dynamic_matrix_hadamard_product_fn {
      dynamic_matrix operator()(const dynamic_matrix& m1, const dynamic_matrix& m2) const;
    };

    struct dynamic_matrix_vector_product_fn {
      dynamic_vector operator()(const dynamic_matrix& m, const dynamic_vector& v) const;
      dynamic_vector operator()(const dynamic_vector& v, const dynamic_matrix& m) const;
    };

    struct dynamic_matrix_diagonalize_vector_fn {
      dynamic_matrix operator()(const dynamic_vector& v) const;
    };

  }  // namespace detail

  constexpr inline detail::dynamic_matrix_sum_fn dynamic_matrix_sum{};
  constexpr inline detail::dynamic_matrix_difference_fn dynamic_matrix_difference{};
  constexpr inline detail::dynamic_matrix_product_fn dynamic_matrix_product{};
  constexpr inline detail::dynamic_matrix_hadamard_product_fn dynamic_matrix_hadamard_product{};
  constexpr inline detail::dynamic_matrix_vector_product_fn dynamic_matrix_vector_product{};
  constexpr inline detail::dynamic_matrix_diagonalize_vector_fn dynamic_matrix_diagonalize_vector{};

  dynamic_matrix rand_matrix(natural_t rows, natural_t cols, real_t min = -1.f, real_t max = 1.f);

}  // namespace other

#endif  // OTHER_CORE_MATH_DYNAMIC_MATRIX_HPP