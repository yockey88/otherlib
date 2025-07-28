/**
 * \file simulation.hpp
 **/
#ifndef OTHER_SIMULATION_HPP
#define OTHER_SIMULATION_HPP

#include <chrono>
#include <cstdint>

#include "math/definitions.hpp"

#include "glm/fwd.hpp"

namespace other {

  using simulation_duration = std::chrono::milliseconds;

  struct timer {
    std::chrono::high_resolution_clock::time_point start_time = now();
    std::chrono::high_resolution_clock::time_point end_time = start_time;

    bool started = false;

    void tick();
    void start();
    void stop();
    void reset();

    simulation_duration elapsed() const;
    std::chrono::milliseconds milliseconds_elapsed() const;
    std::chrono::seconds seconds_elapsed() const;
    std::chrono::high_resolution_clock::time_point now() const;

    template <typename T>
    T duration() const {
      return std::chrono::duration_cast<T>(end_time - start_time);
    }
  };

  struct simulation_coordinate_frame {
    glm::uvec3 dimensions = { 0, 0, 0.f };

    size_t get_cell_index(int32_t x, int32_t y, int32_t z) const;
    bool valid_position(int32_t x, int32_t y, int32_t z) const;

   private:
    bool index_in_bounds(int32_t x, int32_t y, int32_t z) const {
      return x >= 0 && x < static_cast<int32_t>(dimensions.x) &&
        y >= 0 && y < static_cast<int32_t>(dimensions.y) &&
        z >= 0 && z < static_cast<int32_t>(dimensions.z);
    }

    bool index_in_bounds(int32_t x, int32_t y) const {
      return index_in_bounds(x, y, 0);
    }
  };

  class simulation {
   public:
    struct cell;

    struct cell_neighbors {
      enum direction {
        NORTH = 0,
        WEST,
        SOUTH,
        EAST,

        NORTH_WEST,
        NORTH_EAST,
        SOUTH_WEST,
        SOUTH_EAST
      };

      constexpr static inline size_t kNumNeighbors = 8;
      cell* neighbors[kNumNeighbors] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

      inline bool has_neighbor(direction dir) const {
        return neighbors[dir] != nullptr;
      }
      inline cell* get_neighbor(direction dir) const {
        return neighbors[dir];
      }
      inline size_t num_living_neighbors() const {
        return std::ranges::count_if(neighbors, [](const cell* neighbor) { return neighbor != nullptr && neighbor->alive; });
      }
    };

    struct cell {
      int32_t index = 0;
      glm::uvec3 position = { 0, 0, 0 };

      bool alive = true;
      bool death_pending = false;
      bool birth_pending = false;

      template <typename T>
      T& user_data_as() {
        OTHER_ASSERT(user_data != nullptr, "User data is not set.");
        return *static_cast<T*>(user_data);
      }
      template <typename T>
      const T& user_data_as() const {
        OTHER_ASSERT(user_data != nullptr, "User data is not set.");
        return *static_cast<T*>(user_data);
      }
      void* user_data = nullptr;
    };

    using evaluation_rule = bool (*)(simulation* sim, const cell& cell, const cell_neighbors& neighbors);
    using cell_operator = void (*)(simulation* sim, cell& cell, const cell_neighbors& neighbors);
    struct evaluation_rules {
      evaluation_rule birth_condition = nullptr;
      evaluation_rule death_condition = nullptr;

      cell_operator cell_update = nullptr;
    };

    simulation() = default;
    ~simulation() = default;

    inline int32_t get_cell_index(int32_t x, int32_t y) const {
      return get_cell_index(x, y, 0);
    }
    inline int32_t get_cell_index(int32_t x, int32_t y, int32_t z) const {
      return coordinate_frame.get_cell_index(x, y, z);
    }

    simulation_duration get_tick_duration() const { return tick_duration; }
    void set_tick_duration(simulation_duration duration) { tick_duration = duration; }

    std::span<cell> get_cell_data() { return std::span<cell>(cells.data(), cells.size()); }
    const std::span<const cell> read_cell_data() const { return std::span<const cell>(cells.data(), cells.size()); }
    const glm::uvec3& dimensions() const { return coordinate_frame.dimensions; }

    size_t get_living_cells_count() const { return living_cells; }
    size_t get_starting_cells_count() const { return starting_cells; }
    size_t get_cell_count() const { return cells.size(); }

    bool all_cells_alive() const { return std::ranges::all_of(read_cell_data(), &cell::alive); }
    bool any_cells_alive() const { return std::ranges::any_of(read_cell_data(), &cell::alive); }
    bool all_cells_dead() const { return std::ranges::none_of(read_cell_data(), &cell::alive); }
    void invalidate();

    void read_initial_config(const std::string_view config_path);

    void start_simulation();

    /// true if tick occurred
    bool update_simulation(const evaluation_rules& rules);
    /// returns true if sim running (at least one cell is alive)
    bool evaluate_position(const evaluation_rules& rules);

    void set_dimensions(const glm::uvec2& dimensions);
    cell& get_cell(int32_t x, int32_t y);
    inline void set_cell_user_data(int32_t x, int32_t y, void* user_data) {
      get_cell(x, y).user_data = user_data;
    }

    void set_dimensions(const glm::uvec3& dimensions);
    cell& get_cell(int32_t x, int32_t y, int32_t z);
    inline void set_cell_user_data(int32_t x, int32_t y, int32_t z, void* user_data) {
      get_cell(x, y, z).user_data = user_data;
    }

    void set_cell_user_data(const std::vector<void*>& user_datas);

   private:
    timer tick_controller;

    size_t starting_cells = 0;
    size_t living_cells = 0;

    simulation_duration tick_duration = simulation_duration(100);
    simulation_coordinate_frame coordinate_frame;
    std::vector<cell> cells;

    void update_cell(cell& cell);
    cell_neighbors get_cell_neighbors(int32_t x, int32_t y);
  };

}  // namespace other

OTHER_REFLECT(
  other::simulation_coordinate_frame,
  field(dimensions, other::attr::serializable())
)

OTHER_REFLECT(
  other::simulation::cell,
  field(index, other::attr::serializable()),
  field(position, other::attr::serializable()),
  field(alive, other::attr::serializable()),
  field(death_pending, other::attr::serializable()),
  field(birth_pending, other::attr::serializable())
);

#endif  // OTHER_SIMULATION_HPP