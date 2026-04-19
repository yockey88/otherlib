/**
 * \file math/dynamic_matrix.hpp
 **/
#ifndef OTHER_CORE_MATH_DYNAMIC_MATRIX_HPP
#define OTHER_CORE_MATH_DYNAMIC_MATRIX_HPP

#include <type_traits>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "core/ref_counted.hpp"
#include "math/dynamic_vector.hpp"
#include "math/epsilon_math.hpp"
#include "math/matrix.hpp"
#include "math/random.hpp"

#include "data-structures/arena_vector.hpp"

namespace other {

  template <typename T>
    requires std::is_floating_point_v<T> || std::is_integral_v<T>
  struct dynamic_matrix : public ref_counted {
    natural_t rows = 0;
    natural_t cols = 0;

    dynamic_matrix(natural_t r, natural_t c)
        : rows(r), cols(c) {
      data.reserve(rows * cols);
      for (size_t i = 0; i < rows * cols; ++i) {
        data[i] = 0;
      }
    }

    dynamic_matrix(natural_t r, natural_t c, real_t vals)
        : rows(r), cols(c) {
      data.reserve(rows * cols);
      for (size_t i = 0; i < rows * cols; ++i) {
        data[i] = vals;
      }
    }

    dynamic_matrix(dynamic_matrix&& other) noexcept
        : rows(other.rows), cols(other.cols), data(std::move(other.data)) {
      other.rows = 0;
      other.cols = 0;
    }

    dynamic_matrix& operator=(dynamic_matrix&& other) noexcept {
      if (this != &other) {
        rows = other.rows;
        cols = other.cols;
        data = std::move(other.data);
        other.rows = 0;
        other.cols = 0;
      }
      return *this;
    }

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
      data.reserve(rows * cols);
      for (size_t i = 0; i < N; ++i) {
        data[i] = vals[i];
      }
    }

    template <size_t R, size_t C>
    dynamic_matrix(const std::array<std::array<real_t, C>, R>& vals)
        : rows(R), cols(C) {
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
      data.reserve(rows * cols);
      for (natural_t i = 0; i < R; ++i) {
        for (natural_t j = 0; j < C; ++j) {
          (*this)(i, j) = vals(i, j);
        }
      }
    }

    real_t& operator()(natural_t row, natural_t col) {
      OTHER_ASSERT(row < rows && col < cols, "Index out of bounds.");
      return data[row * cols + col];
    }

    real_t operator()(natural_t row, natural_t col) const {
      OTHER_ASSERT(row < rows && col < cols, "Index out of bounds.");
      return data[row * cols + col];
    }

    dynamic_vector get_row(natural_t row) const {
      dynamic_vector res{ cols };
      for (natural_t i = 0; i < cols; ++i) {
        res(i) = (*this)(row, i);
      }
      return res;
    }

    dynamic_vector get_col(natural_t col) const {
      dynamic_vector res{ rows };
      for (natural_t i = 0; i < rows; ++i) {
        res(i) = (*this)(i, col);
      }
      return res;
    }

    static std::string write_string(const dynamic_matrix& m) {
      std::stringstream ss;
      /// TODO: fix spacing

      ss << "\nMatrix [" << m.rows << " x " << m.cols << "]\n";
      for (natural_t i = 0; i < m.rows; ++i) {
        ss << "Row [" << i << "]: ";
        for (natural_t j = 0; j < m.cols; ++j) {
          ss << std::format("{:>.3f}", m(i, j));
          if (j != m.cols - 1) {
            ss << " | ";
          }
        }
        ss << "\n";
      }
      return ss.str();
    }

    arena_vector<T> data;
  };

  template <typename T>
  using matrix_nxm = dynamic_matrix<T>;

  namespace detail {

    struct dynamic_matrix_sum_fn {
      template <typename T, typename U>
        requires(std::is_floating_point_v<T> || std::is_integral_v<T>) && (std::is_floating_point_v<U> || std::is_integral_v<U>) && std::is_convertible_v<T, U>
      dynamic_matrix<T> operator()(const dynamic_matrix<T>& m1, const dynamic_matrix<U>& m2) const {
        OTHER_ASSERT(m1.rows == m2.rows && m1.cols == m2.cols, "Matrix dimensions must match for addition.");

        dynamic_matrix res{ m1.rows, m1.cols };
        for (natural_t i = 0; i < m1.rows; ++i) {
          for (natural_t j = 0; j < m1.cols; ++j) {
            res(i, j) = detail::epsilon_sum(m1(i, j), m2(i, j));
          }
        }
        return res;
      }
    };

    struct dynamic_matrix_difference_fn {
      template <typename T, typename U>
        requires(std::is_floating_point_v<T> || std::is_integral_v<T>) && (std::is_floating_point_v<U> || std::is_integral_v<U>) && std::is_convertible_v<T, U>
      dynamic_matrix<T> operator()(const dynamic_matrix<T>& m1, const dynamic_matrix<U>& m2) const {
        OTHER_ASSERT(m1.rows == m2.rows && m1.cols == m2.cols, "Matrix dimensions must match for subtraction.");

        dynamic_matrix res{ m1.rows, m1.cols };
        for (natural_t i = 0; i < m1.rows; ++i) {
          for (natural_t j = 0; j < m1.cols; ++j) {
            res(i, j) = detail::epsilon_difference(m1(i, j), m2(i, j));
          }
        }
        return res;
      }
    };

    struct dynamic_matrix_product_fn {
      template <typename T, typename U>
        requires(std::is_floating_point_v<T> || std::is_integral_v<T>) && (std::is_floating_point_v<U> || std::is_integral_v<U>) && std::is_convertible_v<T, U>
      dynamic_matrix<T> operator()(const dynamic_matrix<T>& m1, const dynamic_matrix<U>& m2) const {
        OTHER_ASSERT(m1.rows > 0 && m1.cols > 0 && m2.rows > 0 && m2.cols > 0, "Matrix dimensions must be greater than zero.");

        dynamic_matrix res{ m1.rows, m1.cols };
        for (natural_t j = 0; j < m2.cols; ++j) {
          for (natural_t i = 0; i < m1.rows; ++i) {
            res(i, j) = dynamic_vector_dot_product(m1.get_row(i), m2.get_col(j));
          }
        }

        return res;
      }
    };

    struct dynamic_matrix_hadamard_product_fn {
      template <typename T, typename U>
        requires(std::is_floating_point_v<T> || std::is_integral_v<T>) && (std::is_floating_point_v<U> || std::is_integral_v<U>) && std::is_convertible_v<T, U>
      dynamic_matrix<T> operator()(const dynamic_matrix<T>& m1, const dynamic_matrix<U>& m2) const {
        OTHER_ASSERT(m1.rows > 0 && m1.cols > 0 && m2.rows > 0 && m2.cols > 0, "Matrix dimensions must be greater than zero.");

        dynamic_matrix res{ m1.rows, m1.cols };
        for (natural_t i = 0; i < m1.rows; ++i) {
          for (natural_t j = 0; j < m1.cols; ++j) {
            res(i, j) = detail::epsilon_product(m1(i, j), m2(i, j));
          }
        }

        return res;
      }
    };

    struct dynamic_matrix_vector_product_fn {
      template <typename T>
        requires std::is_floating_point_v<T> || std::is_integral_v<T>
      dynamic_vector operator()(const dynamic_matrix<T>& m, const dynamic_vector& v) const {
        OTHER_ASSERT(m.rows > 0 && m.cols > 0, "Matrix dimensions must be greater than zero.");

        dynamic_vector result{ m.rows };
        for (natural_t i = 0; i < m.rows; ++i) {
          result(i) = dynamic_vector_dot_product(m.get_row(i), v);
        }
        return result;
      }
      template <typename T>
        requires std::is_floating_point_v<T> || std::is_integral_v<T>
      dynamic_vector operator()(const dynamic_vector& v, const dynamic_matrix<T>& m) const {
        if (v.size != m.rows) {
          throw std::invalid_argument("Vector and matrix dimensions do not match for multiplication.");
        }

        dynamic_vector result{ m.cols };
        for (natural_t j = 0; j < m.cols; ++j) {
          result(j) = dynamic_vector_dot_product(v, m.get_col(j));
        }
        return result;
      }
    };

    struct dynamic_matrix_diagonalize_vector_fn {
      template <typename T>
        requires std::is_floating_point_v<T> || std::is_integral_v<T>
      dynamic_matrix<T> operator()(const dynamic_vector& v) const {
        dynamic_matrix<T> res{ v.size, v.size };
        for (natural_t i = 0; i < v.size; ++i) {
          res(i, i) = v(i);
        }
        return res;
      }
    };

  }  // namespace detail

  constexpr inline detail::dynamic_matrix_sum_fn dynamic_matrix_sum{};
  constexpr inline detail::dynamic_matrix_difference_fn dynamic_matrix_difference{};
  constexpr inline detail::dynamic_matrix_product_fn dynamic_matrix_product{};
  constexpr inline detail::dynamic_matrix_hadamard_product_fn dynamic_matrix_hadamard_product{};
  constexpr inline detail::dynamic_matrix_vector_product_fn dynamic_matrix_vector_product{};
  constexpr inline detail::dynamic_matrix_diagonalize_vector_fn dynamic_matrix_diagonalize_vector{};

  template <typename T>
    requires std::is_floating_point_v<T> || std::is_integral_v<T>
  dynamic_matrix<T> rand_matrix(natural_t rows, natural_t cols, real_t min = -1.f, real_t max = 1.f) {
    dynamic_matrix<T> m{ rows, cols };
    for (natural_t i = 0; i < rows; ++i) {
      for (natural_t j = 0; j < cols; ++j) {
        m(i, j) = rand_float() * (max - min) + min;
      }
    }
    return m;
  }

}  // namespace other

#endif  // OTHER_CORE_MATH_DYNAMIC_MATRIX_HPP