/**
 * \file neural/ann.hpp
 **/
#ifndef TENSORLIB_NEURAL_ANN_HPP
#define TENSORLIB_NEURAL_ANN_HPP

#include <span>
#include <string>

#include "core/defines.hpp"
#include "math/dynamic_matrix.hpp"
#include "math/dynamic_vector.hpp"

#include "neural/activation.hpp"

namespace other {

  class ann {
   public:
    ann(const std::span<const natural_t> topology);
    ann(const std::span<const natural_t> topology, const std::vector<nn_matrix>& W, const std::vector<nn_vector>& b);
    ann(const std::span<const natural_t> topology, const std::vector<nn_matrix>& W, const std::vector<nn_vector>& b, const std::vector<layer_activation>& activation_functions);

    static std::string write_string(const ann& model, size_t indent = 0);
    static void train_simple_model(ann& model, nn_matrix& input_data, nn_matrix& output_data, const natural_t iterations, const real_t learning_rate);
    static void write_model(const ann& model, const std::filesystem::path& directory);

    natural_t input_size() const;
    natural_t output_size() const;

    natural_t num_layers() const;
    natural_t num_hidden_layers() const;

    const std::vector<natural_t>& get_topology() const {
      return l_i;
    }

    nn_matrix& weight(natural_t layer_index);
    nn_vector& bias(natural_t layer_index);
    nn_vector& output(natural_t layer_index);
    nn_vector& activated_output(natural_t layer_index);
    layer_activation& activation(natural_t layer_index);

    nn_vector forward(const nn_vector& input);
    void zero();

    natural_t get_layer_size(natural_t layer_index) const;
    void bind_affine_layer(natural_t layer_index, const ann& other);
    void bind_affine_layer(natural_t layer_index, layer_activation activation_function);
    void bind_affine_layer(natural_t layer_index, const nn_matrix& W, const nn_vector& b, layer_activation activation_function);

    std::string print_topology(size_t indent = 0) const;

    const std::vector<nn_vector>& get_layer_outputs() const { return layer_outputs; }

   private:
    enum YesNo {
      No = 0,
      Yes = 1
    };
    std::vector<YesNo> layer_bound = { No };

    natural_t L = 0;
    std::vector<natural_t> l_i;

    std::vector<nn_matrix> W;
    std::vector<nn_vector> b;

    std::vector<layer_activation> activation_functions;

    std::vector<nn_vector> layer_outputs;
    std::vector<nn_vector> activated_outputs;

    void initialize(const std::span<const natural_t> topology, bool randomize_parameters = true, bool default_activation = true);
    void on_validate() const;
  };

  std::pair<std::vector<nn_matrix>, std::vector<nn_vector>> random_parameters(const std::span<const natural_t> topology, real_t min = -1.f, real_t max = 1.f);

  real_t compute_cost(const nn_matrix& input_data, const nn_matrix& output_data, ann& model);
  void finite_difference(real_t cost, ann& model, ann& gradient, const nn_matrix& input_data, const nn_matrix& output_data, real_t eps = 1e-1f);

  ann backpropogate(ann& model, const nn_matrix& input_data, const nn_matrix& output_data);

  void learn(ann& model, ann& gradient, real_t learning_rate);

}  // namespace other

#endif  // TENSORLIB_NEURAL_ANN_HPP