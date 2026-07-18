/**
 * \file /neural/ann.cpp
 **/
#include "neural/ann.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <print>
#include <sstream>
#include <string>

#include "core/logger.hpp"
#include "math/dynamic_matrix.hpp"
#include "math/dynamic_vector.hpp"

#include "neural/activation.hpp"

namespace other {

  ann::ann(const std::span<const natural_t> topology)
      : L(topology.size()) {
    OTHER_ASSERT(L > 0, "Topology must have at least one layer.");
    initialize(topology);
  }

  ann::ann(const std::span<const natural_t> topology, const std::vector<nn_matrix>& W, const std::vector<nn_vector>& b)
      : L(topology.size()), W(W), b(b) {
    OTHER_ASSERT(L > 0, "Topology must have at least one layer.");
    initialize(topology, false);
  }

  ann::ann(const std::span<const natural_t> topology, const std::vector<nn_matrix>& W, const std::vector<nn_vector>& b, const std::vector<layer_activation>& activation_functions)
      : L(topology.size()), W(W), b(b), activation_functions(activation_functions) {
    OTHER_ASSERT(L > 0, "Topology must have at least one layer.");
    initialize(topology, false, false);
  }

  std::string ann::write_string(const ann& model, size_t indent) {
    std::stringstream ss;
    if (indent > 0) {
      ss << std::string(indent, ' ');
    }
    ss << model.print_topology();
    return ss.str();
  }

  void ann::train_simple_model(ann& model, nn_matrix& input_data, nn_matrix& output_data, const natural_t iterations, const real_t learning_rate) {
    /// we have to compute an initial cost as a sort of 'warmup' for the system (this needs to be fixed later)
    ///   this is a hack because the model is 'lazy' in an extremely loose sense
    //    (pls I know lazy doesnt apply here, but like it does even if it doesnt, you know? like it is but it isnt?
    //     like i swear it is i promise like actually tho)
    [[maybe_unused]] real_t initial_cost = compute_cost(input_data, output_data, model);
    for (size_t i = 0; i < iterations; ++i) {
      ann gradient = backpropogate(model, input_data, output_data);
      learn(model, gradient, learning_rate);
    }
  }

  void ann::write_model(const ann& model, const std::filesystem::path& directory) {
    if (!std::filesystem::exists(directory)) {
      CORE_LOG_ERROR("Directory {} does not exist, cannot write model.", directory.string());
      return;
    }

    std::ofstream ofs(directory / "model.onn", std::ios::binary);
    if (!ofs) {
      CORE_LOG_ERROR("Failed to open file {} for writing.", (directory / "model.tnn").string());
      return;
    }

    // ofs.close();
  }

  natural_t ann::input_size() const {
    return l_i[0];
  }

  natural_t ann::output_size() const {
    return l_i[L - 1];
  }

  natural_t ann::num_layers() const {
    return L;
  }

  natural_t ann::num_hidden_layers() const {
    return L - 1;
  }

  nn_matrix& ann::weight(natural_t layer_index) {
    OTHER_ASSERT(layer_index < L, "Layer index out of bounds.");
    return W[layer_index];
  }

  nn_vector& ann::bias(natural_t layer_index) {
    OTHER_ASSERT(layer_index < L, "Layer index out of bounds.");
    return b[layer_index];
  }

  nn_vector& ann::output(natural_t layer_index) {
    OTHER_ASSERT(layer_index < L, "Layer index out of bounds.");
    return layer_outputs[layer_index];
  }

  nn_vector& ann::activated_output(natural_t layer_index) {
    OTHER_ASSERT(layer_index < L, "Layer index out of bounds.");
    return activated_outputs[layer_index];
  }

  layer_activation& ann::activation(natural_t layer_index) {
    return activation_functions[layer_index];
  }

  nn_vector ann::forward(const nn_vector& input) {
    OTHER_ASSERT(input.size > 0, "Input vector is empty.");
    OTHER_ASSERT(input.size == this->l_i[0], "Input size does not match ANN input size.");

    layer_outputs.clear();
    layer_outputs.resize(L);
    for (size_t i = 0; i < L; ++i) {
      layer_outputs[i] = nn_vector(l_i[i]);
    }

    activated_outputs.clear();
    activated_outputs.resize(L);
    for (size_t i = 0; i < L; ++i) {
      activated_outputs[i] = nn_vector(l_i[i]);
    }

    layer_outputs[0] = input;
    layer_bound[0] = YesNo::Yes;
    on_validate();

    activated_outputs[0] = layer_outputs[0];
    for (size_t i = 0; i < L - 1; ++i) {
      OTHER_ASSERT(activation_functions[i].activation_function != nullptr, "Activation function is not set.");
      OTHER_ASSERT(W[i].rows == l_i[i + 1] && W[i].cols == l_i[i], "Matrix dimensions do not match.");
      OTHER_ASSERT(b[i].size == l_i[i + 1], "Bias vector size does not match layer size.");

      nn_vector product = dynamic_matrix_vector_product(W[i], activated_outputs[i]);
      nn_vector sum = dynamic_vector_sum(product, b[i]);
      layer_outputs[i + 1] = sum;
      activated_outputs[i + 1] = activation_functions[i].activation_function(sum);
    }

    return activated_outputs.back();
  }

  void ann::zero() {
    for (natural_t l = 0; l < L; ++l) {
      for (natural_t i = 0; i < layer_outputs[l].size; ++i) {
        layer_outputs[l][i] = 0.f;
      }
    }

    for (natural_t l = 0; l < L - 1; ++l) {
      for (natural_t i = 0; i < W[l].data.size(); ++i) {
        W[l].data[i] = 0.f;
      }
      for (natural_t i = 0; i < b[l].data.size(); ++i) {
        b[l].data[i] = 0.f;
      }
    }
  }

  natural_t ann::get_layer_size(natural_t layer_index) const {
    OTHER_ASSERT(layer_index < L, "Layer index out of bounds.");
    return l_i[layer_index];
  }

  void ann::bind_affine_layer(natural_t layer_index, const ann& other) {
    bind_affine_layer(layer_index, other.W[layer_index], other.b[layer_index], other.activation_functions[layer_index]);
  }

  void ann::bind_affine_layer(natural_t layer_index, layer_activation activation_function) {
    OTHER_ASSERT(layer_index < L, "Layer index out of bounds.");
    activation_functions[layer_index] = activation_function;

    /// could probably do this better
    try {
      [[maybe_unused]] auto& _ = W.at(layer_index);
      [[maybe_unused]] auto& _b = b.at(layer_index);
      layer_bound[layer_index] = YesNo::Yes;
    } catch (...) {
      layer_bound[layer_index] = YesNo::No;
    }
  }

  void ann::bind_affine_layer(natural_t layer_index, const nn_matrix& W, const nn_vector& b, layer_activation activation_function) {
    OTHER_ASSERT(layer_index < L, "Layer index out of bounds.");
    this->W[layer_index] = W;
    this->b[layer_index] = b;
    this->activation_functions[layer_index] = activation_function;

    layer_bound[layer_index] = YesNo::Yes;
  }

  std::string ann::print_topology(size_t indent) const {
    std::stringstream ss;
    ss << std::string(indent, ' ');
    ss << "ANN Topology: ";
    ss << " -- " << l_i.size() << " layers\n";

    ss << std::string(indent, ' ');
    ss << " -- Input size: " << l_i.front() << "\n";

    ss << std::string(indent, ' ');
    ss << " -- Output size: " << l_i.back() << "\n";

    /// -2 bc input and output :D 1 + 1 = 2 :D
    ss << std::string(indent, ' ');
    ss << " -- Hidden layers: " << L - 2 << "\n";

    ss << std::string(indent, ' ');
    ss << " -- Total Layers = " << L + 1 << "\n";

    ss << std::string(indent, ' ');
    ss << " -- layers --\n";

    for (size_t i = 0; i < L; ++i) {
      ss << std::string(indent, ' ');
      ss << "[layer [" << i << "], dim = " << l_i[i] << "";
      if (i < L - 1) {
        ss << std::string(indent + 2, ' ');
        ss << "- parameters:\n";
        ss << "> W :"
           << std::format("{}{}", std::string(indent + 4, ' '), W[i]);
        ss << "> b :"
           << std::format("{}{}", std::string(indent + 4, ' '), b[i]);
        ss << "]\n";
      }
    }
    return ss.str();
  }

  void ann::initialize(const std::span<const natural_t> topology, bool randomize_parameters, bool default_activation) {
    l_i.resize(topology.size());
    std::ranges::copy(topology, l_i.begin());

    layer_outputs.resize(L);
    activated_outputs.resize(L);

    layer_bound.resize(L);
    std::ranges::fill(layer_bound, YesNo::No);

    activation_functions.resize(L - 1);
    if (randomize_parameters) {
      auto [rand_weights, rand_biases] = random_parameters(topology, -1.f, 1.f);
      W = rand_weights;
      b = rand_biases;
    }

    if (default_activation) {
      for (size_t i = 0; i < L - 1; ++i) {
        activation_functions[i] = layer_defns::relu_layer;
      }
    }
  }

  void ann::on_validate() const {
    for (size_t i = 0; i < layer_bound.size() - 1; ++i) {
      if (layer_bound[i] == YesNo::No) {
        std::print("Layer [{}] is not bound to a parameter set.\n", i);
      }
    }
  }

  std::pair<std::vector<nn_matrix>, std::vector<nn_vector>> random_parameters(const std::span<const natural_t> topology, real_t min, real_t max) {
    std::vector<nn_matrix> W;
    std::vector<nn_vector> b;

    for (size_t i = 0; i < topology.size() - 1; ++i) {
      W.push_back(rand_matrix<real_t>(topology[i + 1], topology[i], min, max));
      b.push_back(rand_vector(topology[i + 1], min, max));
    }

    return { W, b };
  }

  real_t compute_cost(const nn_matrix& input_data, const nn_matrix& output_data, ann& model) {
    natural_t n = input_data.rows;

    real_t cost = 0.f;
    for (size_t i = 0; i < n; ++i) {
      nn_vector in_i = input_data.get_row(i);
      nn_vector o_i = output_data.get_row(i);

      nn_vector result = model.forward(in_i);

      for (size_t j = 0; j < result.size; ++j) {
        nn_vector diffv = dynamic_vector_difference(result, o_i);
        cost += dynamic_vector_dot_product(diffv, diffv);
      }
    }

    return cost / n;
  }

  void finite_difference(real_t cost, ann& model, ann& gradient, const nn_matrix& input_data, const nn_matrix& output_data, real_t eps) {
    real_t saved = 0.f;

    for (natural_t l = 0; l < model.num_hidden_layers(); ++l) {
      nn_matrix& W = model.weight(l);
      nn_vector& b = model.bias(l);

      nn_matrix& gW = gradient.weight(l);
      nn_vector& gb = gradient.bias(l);

      for (natural_t i = 0; i < W.data.size(); ++i) {
        saved = W.data[i];
        W.data[i] += eps;
        gW.data[i] = (compute_cost(input_data, output_data, model) - cost) / eps;
        W.data[i] = saved;
      }

      for (natural_t i = 0; i < b.data.size(); ++i) {
        saved = b.data[i];
        b.data[i] += eps;
        gb.data[i] = (compute_cost(input_data, output_data, model) - cost) / eps;
        b.data[i] = saved;
      }
    }
  }

  void learn(ann& model, ann& gradient, real_t learning_rate) {
    for (natural_t l = 0; l < model.num_hidden_layers(); ++l) {
      for (natural_t i = 0; i < model.weight(l).rows; ++i) {
        for (natural_t j = 0; j < model.weight(l).cols; ++j) {
          model.weight(l)(i, j) -= learning_rate * gradient.weight(l)(i, j);
        }
      }
      for (natural_t i = 0; i < model.bias(l).data.size(); ++i) {
        model.bias(l).data[i] -= learning_rate * gradient.bias(l).data[i];
      }
    }
  }

  ann backpropogate(ann& model, const nn_matrix& input_data, const nn_matrix& output_data) {
    OTHER_ASSERT(input_data.rows == output_data.rows, "Input and output data must have the same number of rows.");
    OTHER_ASSERT(input_data.cols == model.input_size(), "Input data size does not match ANN input size.");
    OTHER_ASSERT(output_data.cols == model.output_size(), "Output data size does not match ANN output size.");

    ann gradient = model;
    gradient.zero();

    /// backpropogation works over all sets of input->output pairs
    ///     we are computing approximate minimizers of cost function C(N, x) = sum_1,M(|N(x) - y|^2) * (1/M)
    ///     where N is the neural network, x is the input data, and y is the output data
    /// so we find the gradient of the cost function (embedded in some high-dimensional space) and move against it
    ///     to find a valley in the cost landscape so that we can 'be as close as possible' to the desired output
    for (size_t i = 0; i < input_data.rows; ++i) {
      nn_vector in_i = input_data.get_row(i);
      nn_vector o_i = output_data.get_row(i);
      nn_vector result = model.forward(in_i);

      ///   if C(N, x) = sum_1,M(|N(x) - y|^2) * (1/M)
      ///   then, for each idx
      //          dC_i/d<var> = 2 * (N(x) - y) * dN_i/d<var>
      //     and for each var
      //          dN_i/d<var> = f'(N_i) * dN_i/d<var>
      //    so, we have dc = 2 * (N(x) - y) * f'(N_i)
      ///   then dC_i/d<var> = 2 * (N(x) - y) * f'(N_i) * dN_i/d<var>
      ///        and dC_i / d<var> = dc * dN_i/d<var>
      ///   where dc = 2 * (N(x) - y)

      /// 2 * (N(x) - y)
      nn_vector dc = dynamic_vector_scalar_product(dynamic_vector_difference(result, o_i), 2.f);
      gradient.output(model.num_hidden_layers()) = dc;

      for (natural_t layer = model.num_hidden_layers(); layer > 0; --layer) {
        nn_vector activation_derivative = model.activation(layer - 1).derivative_function(model.output(layer));

        /// element-wise product to get the gradient at this layer
        gradient.output(layer) = dynamic_vector_hadamard_product(gradient.output(layer), activation_derivative);
        gradient.bias(layer - 1) = dynamic_vector_sum(gradient.bias(layer - 1), gradient.output(layer));

        /// model.output(layer).size == gradient.output(layer).size so this is safe
        for (natural_t r = 0; r < model.output(layer).size; ++r) {
          /// very important that we use activated_output here otherwise we will be computing the gradient using
          ///   the non-activated layer outputs which is incorrect
          for (size_t c = 0; c < model.activated_output(layer - 1).size; ++c) {
            gradient.weight(layer - 1)(r, c) += gradient.output(layer)[r] * model.activated_output(layer - 1)[c];
          }
        }

        /// if we are not at the input layer, propagate the gradient backwards
        if (layer > 1) {
          for (size_t c = 0; c < model.output(layer - 1).size; ++c) {
            real_t sum = 0.f;
            for (natural_t r = 0; r < model.output(layer).size; ++r) {
              sum += model.weight(layer - 1)(r, c) * gradient.output(layer)[r];
            }

            gradient.output(layer - 1)[c] += sum;
          }
        }
      }
    }

    for (natural_t layer = 0; layer < model.num_hidden_layers(); ++layer) {
      for (natural_t i = 0; i < gradient.weight(layer).data.size(); ++i) {
        gradient.weight(layer)[i] /= input_data.rows;
      }
      for (natural_t i = 0; i < gradient.bias(layer).data.size(); ++i) {
        gradient.bias(layer)[i] /= input_data.rows;
      }
    }

    return gradient;
  }

}  // namespace other