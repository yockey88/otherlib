/**
 * \file data-structures/matrix.hpp
 **/
#ifndef OTHER_CORE_DATA_STRUCTURES_MATRIX_HPP
#define OTHER_CORE_DATA_STRUCTURES_MATRIX_HPP

#include "core/defines.hpp"
#include "math/vector.hpp"

namespace other {

  template <natural_t Rows, natural_t Cols>
  struct matrix;

  namespace detail {

    template <natural_t R, natural_t C>
    constexpr inline natural_t matrix_dim = R * C;

    constexpr index_t matrix_index(natural_t row, natural_t col, natural_t cols) {
      return row * cols + col;
    }

    template <natural_t R, natural_t C, natural_t I>
    constexpr real_t get_element_of_row_in_column(const matrix<R, C>& m, natural_t row) {
      return m(row, I);
    }

    template <natural_t R, natural_t C, natural_t I>
    constexpr real_t get_element_of_col_in_row(const matrix<R, C>& m, natural_t col) {
      return m(I, col);
    }

    template <natural_t R, natural_t C, size_t... I>
    consteval vector<C> get_all_elements_of_row_helper(const matrix<R, C>& m, natural_t row, std::index_sequence<I...>) {
      return vector<C>{ get_element_of_row_in_column<R, C, I>(m, row)... };
    }

    template <natural_t R, natural_t C, size_t... I>
    consteval vector<R> get_all_elements_of_col_helper(const matrix<R, C>& m, natural_t col, std::index_sequence<I...>) {
      return vector<R>{ get_element_of_col_in_row<R, C, I>(m, col)... };
    };

    template <size_t... C>
    constexpr auto build_flattened_array(const std::array<real_t, C>... arrays) {
      constexpr std::size_t total_size = (C + ...);

      auto all_elements = std::tuple_cat(arrays...);

      return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::array<real_t, total_size>{ { std::get<Is>(all_elements)... } };
      }(std::make_index_sequence<total_size>{});
    }

    constexpr auto build_array_from_series(auto&&... values) {
      static_assert(sizeof...(values) > 0, "At least one value is required.");

      constexpr std::size_t size = sizeof...(values);
      return std::array<real_t, size>{ values... };
    }

    template <natural_t R, natural_t C, size_t... I>
    constexpr std::array<real_t, matrix_dim<R, C>> flatten_impl(const std::array<std::array<real_t, C>, R>& arr, std::index_sequence<I...>) {
      return build_flattened_array(arr[I]...);
    }

    template <natural_t R, natural_t C>
    constexpr std::array<real_t, matrix_dim<R, C>> flatten(const std::array<std::array<real_t, C>, R>& arr) {
      return flatten_impl<R, C>(arr, std::make_index_sequence<R>{});
    }

  }  // namespace detail

  template <natural_t R, natural_t C>
  constexpr vector<C> get_matrix_row(const matrix<R, C>& m, natural_t row) {
    return detail::get_all_elements_of_row_helper<R, C>(m, row, std::make_index_sequence<C>{});
  }

  template <natural_t R, natural_t C>
  constexpr vector<R> get_matrix_col(const matrix<R, C>& m, natural_t col) {
    return detail::get_all_elements_of_col_helper<R, C>(m, col, std::make_index_sequence<R>{});
  }

  template <natural_t Rows, natural_t Cols>
  struct matrix {
    constexpr static natural_t rows = Rows;
    constexpr static natural_t cols = Cols;
    constexpr static natural_t data_size = detail::matrix_dim<Rows, Cols>;

    constexpr matrix() = default;

    template <typename... Vals>
      requires(sizeof...(Vals) == data_size && (std::is_same_v<std::remove_cvref_t<Vals>, real_t> && ...))
    constexpr matrix(Vals&&... vals) {
      static_assert(sizeof...(vals) == data_size, "Number of arguments must match the dimension of the point.");
      static_assert((std::is_same_v<std::remove_cvref_t<decltype(vals)>, real_t> && ...), "All arguments must be of type real.");

      values = { std::forward<decltype(vals)>(vals)... };
    }

    constexpr matrix(const std::array<real_t, data_size>& vals) {
      values = vals;
    }

    constexpr matrix(const std::array<std::array<real_t, Cols>, Rows>& rows) {
      values = detail::flatten<Rows, Cols>(rows);
    }

    constexpr real_t& operator()(natural_t row, natural_t col) {
      return values[detail::matrix_index(row, col, cols)];
    }

    constexpr const real_t& operator()(natural_t row, natural_t col) const {
      return values[detail::matrix_index(row, col, cols)];
    }

    constexpr real_t operator[](natural_t i) { return values[i]; }
    constexpr real_t operator[](natural_t i) const { return values[i]; }

    constexpr auto begin() const noexcept { return values.begin(); }
    constexpr auto end() const noexcept { return values.end(); }
    constexpr auto size() const noexcept { return values.size(); }
    constexpr auto data() const noexcept { return values.data(); }

    static std::string write_string(const matrix& m) {
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

   private:
    std::array<real_t, data_size> values = { 0 };
  };

  template <natural_t R1, natural_t C1, natural_t R2, natural_t C2>
  constexpr bool operator==(const matrix<R1, C1>& lhs, const matrix<R2, C2>& rhs) {
    static_assert(R1 > 0 && R2 > 0 && C1 > 0 && C2 > 0, "Matrix dimensions must be greater than zero.");
    if constexpr (R1 == R2 && C1 == C2) {
      return std::ranges::equal(lhs, rhs);
    } else {
      return false;
    }
  }

  template <natural_t R1, natural_t C1, natural_t R2, natural_t C2>
  constexpr bool operator!=(const matrix<R1, C1>& lhs, const matrix<R2, C2>& rhs) {
    return !(lhs == rhs);
  }

  namespace detail {

    template <natural_t R, natural_t C, natural_t I, size_t... J>
    constexpr std::array<real_t, C> build_identity_matrix_row(std::index_sequence<J...>) {
      return std::array<real_t, C>{ ((J == I) ? 1.f : 0.f)... };
    }

    template <natural_t R, natural_t C, size_t... I>
    constexpr std::array<real_t, matrix_dim<R, C>> build_identity_matrix_helper(std::index_sequence<I...>) {
      return build_flattened_array(build_identity_matrix_row<R, C, I>(std::make_index_sequence<C>{})...);
    }

    template <natural_t R, natural_t C>
    constexpr matrix<R, C> build_identity_matrix() {
      return matrix<R, C>{ build_identity_matrix_helper<R, C>(std::make_index_sequence<R>{}) };
    }

    template <natural_t R, natural_t C>
    constexpr matrix<R, C> identity_builder() {
      return build_identity_matrix<R, C>();
    }

  }  // namespace detail

  template <natural_t R, natural_t C>
  constexpr inline matrix<R, C> zero_matrix{};

  template <natural_t N>
  constexpr inline matrix<N, N> identity = detail::identity_builder<N, N>();

  namespace detail {

    template <natural_t R, natural_t C, typename Fn, natural_t I, natural_t J>
      requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
    constexpr real_t apply_to_matrix_elements_at(const matrix<R, C>& m1, const matrix<R, C>& m2, Fn&& fn) {
      return std::invoke(std::forward<Fn>(fn), m1(I, J), m2(I, J));
    }

    template <natural_t R, natural_t C, typename Fn, natural_t I, size_t... J>
      requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
    constexpr std::array<real_t, C> apply_to_matrix_row_elements(const matrix<R, C>& m1, const matrix<R, C>& m2, Fn&& fn, std::index_sequence<J...>) {
      return std::array<real_t, C>{ apply_to_matrix_elements_at<R, C, Fn, I, J>(m1, m2, std::forward<Fn>(fn))... };
    }

    template <natural_t R, natural_t C, typename Fn, natural_t I, size_t... J>
      requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
    constexpr std::array<real_t, C> apply_to_matrix_row(const matrix<R, C>& m1, const matrix<R, C>& m2, Fn&& fn, std::index_sequence<J...>) {
      return std::array<real_t, C>{ std::invoke(std::forward<Fn>(fn), m1(I, J), m2(I, J))... };
    }

    template <natural_t R, natural_t C, typename Fn, size_t... I>
      requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
    constexpr std::array<real_t, matrix_dim<R, C>> apply_to_matrix_elements_helper(const matrix<R, C>& m1, const matrix<R, C>& m2, Fn&& fn, std::index_sequence<I...>) {
      return build_flattened_array(apply_to_matrix_row<R, C, Fn, I>(m1, m2, std::forward<Fn>(fn), std::make_index_sequence<R>{})...);
    }

    template <natural_t R, natural_t C, typename Fn>
      requires std::is_invocable_r_v<real_t, Fn, real_t, real_t>
    constexpr matrix<R, C> apply_to_matrix_elements(const matrix<R, C>& m1, const matrix<R, C>& m2, Fn&& fn) {
      return matrix<R, C>{ apply_to_matrix_elements_helper<R, C, Fn>(m1, m2, std::forward<Fn>(fn), std::make_index_sequence<R>{}) };
    }

    template <natural_t R1, natural_t C1, natural_t R2, natural_t C2, size_t I, size_t... J>
    constexpr std::array<real_t, C2> get_matrix_product_row(const matrix<R1, C1>& m1, const matrix<R2, C2>& m2, std::index_sequence<J...>) {
      return std::array<real_t, C2>{ dot_product(get_matrix_row(m1, I), get_matrix_col(m2, J))... };
    }

    template <natural_t R1, natural_t C1, natural_t R2, natural_t C2, size_t... I>
    constexpr std::array<real_t, matrix_dim<R1, C2>> apply_matrix_product_helper(const matrix<R1, C1>& m1, const matrix<R2, C2>& m2, std::index_sequence<I...>) {
      return build_flattened_array(get_matrix_product_row<R1, C1, R2, C2, I>(m1, m2, std::make_index_sequence<C2>{})...);
    }

    template <natural_t R1, natural_t C1, natural_t R2, natural_t C2>
    constexpr matrix<R1, C2> apply_matrix_product(const matrix<R1, C1>& m1, const matrix<R2, C2>& m2) {
      return matrix<R1, C2>{ apply_matrix_product_helper<R1, C1, R2, C2>(m1, m2, std::make_index_sequence<R1>{}) };
    }

    template <natural_t R, natural_t C, size_t I>
    constexpr real_t get_matrix_vector_product_row(const matrix<R, C>& m, const vector<C>& v) {
      return dot_product(get_matrix_row(m, I), v);
    }

    template <natural_t R, natural_t C, size_t... I>
    constexpr std::array<real_t, R> apply_matrix_vector_product_helper(const matrix<R, C>& m, const vector<C>& v, std::index_sequence<I...>) {
      return build_array_from_series(get_matrix_vector_product_row<R, C, I>(m, v)...);
    }

    /////// DISGUSTING HACK ////////
    /// TODO: code should work at compile-time and runtime
    template <natural_t R, natural_t C>
    vector<C> runtime_get_matrix_row(const matrix<R, C>& m, natural_t row) {
      vector<C> result{ 0.f };
      for (natural_t i = 0; i < C; ++i) {
        result[i] = m(row, i);
      }
      return result;
    }

    template <natural_t R, natural_t C>
    vector<R> runtime_matrix_vector_product(const matrix<R, C>& m, const vector<C>& v) {
      vector<R> result{ 0.f };
      for (natural_t i = 0; i < R; ++i) {
        result[i] = dot_product(runtime_get_matrix_row<R, C>(m, i), v);
      }
      return result;
    }
    /////// DISGUSTING HACK ////////

    template <natural_t R, natural_t C>
    constexpr vector<R> apply_matrix_vector_product(const matrix<R, C>& m, const vector<C>& v) {
      return vector<R>{ apply_matrix_vector_product_helper<R, C>(m, v, std::make_index_sequence<R>{}) };
    }

    struct matrix_sum_fn {
      template <natural_t R1, natural_t C1, natural_t R2, natural_t C2>
      constexpr auto operator()(const matrix<R1, C1>& m1, const matrix<R2, C2>& m2) const {
        static_assert(R1 > 0 && R2 > 0 && C1 > 0 && C2 > 0, "Matrix dimensions must be greater than zero.");
        static_assert(R1 == R2 && C1 == C2, "Matrix dimensions must be equal.");
        return matrix<R1, C1>{ apply_to_matrix_elements<R1, C1>(m1, m2, [](const real_t& a, const real_t& b) { return a + b; }) };
      }
    };

    struct matrix_difference_fn {
      template <natural_t R1, natural_t C1, natural_t R2, natural_t C2>
      constexpr auto operator()(const matrix<R1, C1>& m1, const matrix<R2, C2>& m2) const {
        static_assert(R1 > 0 && R2 > 0 && C1 > 0 && C2 > 0, "Matrix dimensions must be greater than zero.");
        static_assert(R1 == R2 && C1 == C2, "Matrix dimensions must be equal.");
        return matrix<R1, C1>{ apply_to_matrix_elements<R1, C1>(m1, m2, [](const real_t& a, const real_t& b) { return a - b; }) };
      }
    };

    struct matrix_product_fn {
      template <natural_t R1, natural_t C1, natural_t R2, natural_t C2>
      constexpr matrix<R1, C2> operator()(const matrix<R1, C1>& m1, const matrix<R2, C2>& m2) const {
        static_assert(R1 > 0 && C1 > 0 && R2 > 0 && C2 > 0, "Matrix dimensions must be greater than zero.");
        static_assert(R1 == C2, "Matrix dimensions must be equal.");
        static_assert(C1 == R2, "Matrix dimensions must be equal.");

        return apply_matrix_product<R1, C1, R2, C2>(m1, m2);
      }
    };

    struct matrix_hadamard_product_fn {
      template <natural_t R1, natural_t C1, natural_t R2, natural_t C2>
      constexpr auto operator()(const matrix<R1, C1>& m1, const matrix<R2, C2>& m2) const {
        static_assert(R1 > 0 && R2 > 0 && C1 > 0 && C2 > 0, "Matrix dimensions must be greater than zero.");
        static_assert(R1 == R2 && C1 == C2, "Matrix dimensions must be equal.");

        return apply_to_matrix_elements<R1, C1>(m1, m2, [](const real_t& a, const real_t& b) { return a * b; });
      }
    };

    struct matrix_vector_product_fn {
      template <natural_t R, natural_t C>
      constexpr vector<R> operator()(const matrix<R, C>& m, const vector<C>& v) const {
        /// TODO: FIX this hack and make the same code work in compile-time contexts and runtime contexts.
        static_assert(R > 0 && C > 0, "Matrix dimensions must be greater than zero.");

        if consteval {
          return apply_matrix_vector_product<R, C>(m, v);
        } else {
          return runtime_matrix_vector_product<R, C>(m, v);
        }
      }
    };

  }  // namespace detail

  constexpr inline detail::matrix_sum_fn matrix_sum{};
  constexpr inline detail::matrix_difference_fn matrix_difference{};
  constexpr inline detail::matrix_product_fn matrix_product{};
  constexpr inline detail::matrix_hadamard_product_fn matrix_hadamard_product{};
  constexpr inline detail::matrix_vector_product_fn matrix_vector_product{};

  void decompose_mat4(const glm::mat4& mat, glm::vec3& out_translation, glm::quat& out_rotation, glm::vec3& out_scale);

}  // namespace other

#endif  // OTHER_CORE_DATA_STRUCTURES_MATRIX_HPP