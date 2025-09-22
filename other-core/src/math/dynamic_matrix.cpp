/**
 * \file math/dynamic_matrix.cpp
 **/
#include "math/dynamic_matrix.hpp"

#include "math/epsilon_math.hpp"
#include "math/random.hpp"

namespace other {

  dynamic_matrix::dynamic_matrix(natural_t r, natural_t c)
      : rows(r), cols(c) {
    OTHER_ASSERT(rows > 0 && cols > 0, "Matrix dimensions must be greater than zero.");
    data.reserve(rows * cols);
    for (size_t i = 0; i < rows * cols; ++i) {
      data[i] = 0;
    }
  }

  dynamic_matrix::dynamic_matrix(natural_t r, natural_t c, real_t vals)
      : rows(r), cols(c) {
    OTHER_ASSERT(rows > 0 && cols > 0, "Matrix dimensions must be greater than zero.");
    data.reserve(rows * cols);
    for (size_t i = 0; i < rows * cols; ++i) {
      data[i] = vals;
    }
  }

  dynamic_matrix::dynamic_matrix(dynamic_matrix&& other) noexcept
      : rows(other.rows), cols(other.cols), data(std::move(other.data)) {
    other.rows = 0;
    other.cols = 0;
  }

  dynamic_matrix& dynamic_matrix::operator=(dynamic_matrix&& other) noexcept {
    if (this != &other) {
      rows = other.rows;
      cols = other.cols;
      data = std::move(other.data);
      other.rows = 0;
      other.cols = 0;
    }
    return *this;
  }

  real_t& dynamic_matrix::operator()(natural_t row, natural_t col) {
    OTHER_ASSERT(row < rows && col < cols, "Index out of bounds.");
    return data[row * cols + col];
  }

  real_t dynamic_matrix::operator()(natural_t row, natural_t col) const {
    OTHER_ASSERT(row < rows && col < cols, "Index out of bounds.");
    return data[row * cols + col];
  }

  dynamic_vector dynamic_matrix::get_row(natural_t row) const {
    dynamic_vector res{ cols };
    for (natural_t i = 0; i < cols; ++i) {
      res(i) = (*this)(row, i);
    }
    return res;
  }

  dynamic_vector dynamic_matrix::get_col(natural_t col) const {
    dynamic_vector res{ rows };
    for (natural_t i = 0; i < rows; ++i) {
      res(i) = (*this)(i, col);
    }
    return res;
  }

  std::string dynamic_matrix::write_string(const dynamic_matrix& m) {
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

  namespace detail {

    dynamic_matrix dynamic_matrix_sum_fn::operator()(const dynamic_matrix& m1, const dynamic_matrix& m2) const {
      if (m1.rows != m2.rows || m1.cols != m2.cols) {
        throw std::invalid_argument("Matrix dimensions must match for addition.");
      }

      dynamic_matrix res{ m1.rows, m1.cols };
      for (natural_t i = 0; i < m1.rows; ++i) {
        for (natural_t j = 0; j < m1.cols; ++j) {
          res(i, j) = detail::epsilon_sum(m1(i, j), m2(i, j));
        }
      }
      return res;
    }

    dynamic_matrix dynamic_matrix_difference_fn::operator()(const dynamic_matrix& m1, const dynamic_matrix& m2) const {
      if (m1.rows != m2.rows || m1.cols != m2.cols) {
        throw std::invalid_argument("Matrix dimensions must match for subtraction.");
      }

      dynamic_matrix res{ m1.rows, m1.cols };
      for (natural_t i = 0; i < m1.rows; ++i) {
        for (natural_t j = 0; j < m1.cols; ++j) {
          res(i, j) = detail::epsilon_difference(m1(i, j), m2(i, j));
        }
      }
      return res;
    }

    dynamic_matrix dynamic_matrix_product_fn::operator()(const dynamic_matrix& m1, const dynamic_matrix& m2) const {
      if (m1.cols != m2.rows) {
        throw std::invalid_argument("Matrix dimensions do not match for multiplication.");
      }

      dynamic_matrix res{ m1.rows, m1.cols };
      for (natural_t j = 0; j < m2.cols; ++j) {
        for (natural_t i = 0; i < m1.rows; ++i) {
          res(i, j) = dynamic_vector_dot_product(m1.get_row(i), m2.get_col(j));
        }
      }

      return res;
    }

    dynamic_matrix dynamic_matrix_hadamard_product_fn::operator()(const dynamic_matrix& m1, const dynamic_matrix& m2) const {
      if (m1.rows != m2.rows || m1.cols != m2.cols) {
        throw std::invalid_argument("Matrix dimensions must match for Hadamard product.");
      }

      dynamic_matrix res{ m1.rows, m1.cols };
      for (natural_t i = 0; i < m1.rows; ++i) {
        for (natural_t j = 0; j < m1.cols; ++j) {
          res(i, j) = detail::epsilon_product(m1(i, j), m2(i, j));
        }
      }

      return res;
    }

    dynamic_vector dynamic_matrix_vector_product_fn::operator()(const dynamic_matrix& m, const dynamic_vector& vector) const {
      if (m.cols != vector.size) {
        throw std::invalid_argument("Matrix and vector dimensions do not match for multiplication.");
      }

      dynamic_vector result{ m.rows };
      for (natural_t i = 0; i < m.rows; ++i) {
        result(i) = dynamic_vector_dot_product(m.get_row(i), vector);
      }
      return result;
    }

    dynamic_vector dynamic_matrix_vector_product_fn::operator()(const dynamic_vector& v, const dynamic_matrix& m) const {
      if (v.size != m.rows) {
        throw std::invalid_argument("Vector and matrix dimensions do not match for multiplication.");
      }

      dynamic_vector result{ m.cols };
      for (natural_t j = 0; j < m.cols; ++j) {
        result(j) = dynamic_vector_dot_product(v, m.get_col(j));
      }
      return result;
    }

    dynamic_matrix dynamic_matrix_diagonalize_vector_fn::operator()(const dynamic_vector& vector) const {
      dynamic_matrix res{ vector.size, vector.size };
      for (natural_t i = 0; i < vector.size; ++i) {
        res(i, i) = vector(i);
      }
      return res;
    }

  }  // namespace detail

  dynamic_matrix rand_matrix(natural_t rows, natural_t cols, const real_t min, const real_t max) {
    dynamic_matrix m{ rows, cols };
    for (natural_t i = 0; i < rows; ++i) {
      for (natural_t j = 0; j < cols; ++j) {
        m(i, j) = rand_float() * (max - min) + min;
      }
    }
    return m;
  }

}  // namespace other