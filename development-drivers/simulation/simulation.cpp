/**
 * \file simulation.cpp
 **/
#include "simulation.hpp"

#include <cstdint>

#include "sim-config-spec_generated.h"

namespace other {

  void timer::tick() {
    if (!started) {
      start();
    }
    end_time = now();
  }

  void timer::start() {
    start_time = now();
    started = true;
  }

  void timer::stop() {
    end_time = now();
    started = false;
  }

  void timer::reset() {
    start_time = now();
    end_time = start_time;
    started = false;
  }

  simulation_duration timer::elapsed() const {
    return duration<simulation_duration>();
  }

  std::chrono::milliseconds timer::milliseconds_elapsed() const {
    return duration<std::chrono::milliseconds>();
  }

  std::chrono::seconds timer::seconds_elapsed() const {
    return duration<std::chrono::seconds>();
  }

  std::chrono::high_resolution_clock::time_point timer::now() const {
    return std::chrono::high_resolution_clock::now();
  }

  size_t simulation_coordinate_frame::get_cell_index(int32_t x, int32_t y, int32_t z) const {
    return static_cast<size_t>(z) * dimensions.x * dimensions.y + static_cast<size_t>(y) * dimensions.x + static_cast<size_t>(x);
  }

  bool simulation_coordinate_frame::valid_position(int32_t x, int32_t y, int32_t z) const {
    return index_in_bounds(x, y, z);
  }

  namespace detail {

    std::vector<uint8_t> read_to_bytes(const std::string& filename) {
      std::ifstream file(filename, std::ios::binary | std::ios::in);
      if (!file.is_open()) {
        CORE_LOG_ERROR("Failed to open file: {}", filename);
        return {};
      }

      file.seekg(0, std::ios::end);
      size_t size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::vector<uint8_t> buffer(size);
      file.read(reinterpret_cast<char*>(buffer.data()), size);
      file.close();
      return buffer;
    }

  }  // namespace detail

  void simulation::invalidate() {
    coordinate_frame.dimensions = { 0, 0, 0 };
    cells.clear();
    starting_cells = 0;
    living_cells = 0;
  }

  void simulation::read_initial_config(const std::string_view config_path) {
    auto path = std::filesystem::path(config_path);
    if (!std::filesystem::exists(path)) {
      invalidate();
      CORE_LOG_ERROR("Simulation config file does not exist: {}", path.string());
      return;
    }

    auto buffer = detail::read_to_bytes(path.string());
    if (buffer.empty()) {
      invalidate();
      CORE_LOG_ERROR("Failed to read simulation config file: {}", path.string());
      return;
    }

    auto sim = GetSimulation(buffer.data());
    if (sim == nullptr) {
      invalidate();
      CORE_LOG_ERROR("Failed to parse simulation config file: {}", path.string());
      return;
    }

    if (sim->cells() == nullptr || sim->cells()->size() == 0) {
      invalidate();
      CORE_LOG_ERROR("Simulation config file has no cells: {}", path.string());
      return;
    }

    CORE_LOG_INFO("Successfully loaded simulation config file: {}", path.string());
    set_dimensions({ sim->dimensions()->x(), sim->dimensions()->y() });

    for (int32_t i = 0; i < sim->cells()->size(); ++i) {
      const auto& cell = *sim->cells()->Get(i);
      if (cell.position() == nullptr) {
        CORE_LOG_ERROR("Cell at index {} has no position defined.", i);
        continue;
      }

      int32_t x = cell.position()->x();
      int32_t y = cell.position()->y();
      int32_t z = cell.position()->z();
      if (!coordinate_frame.valid_position(x, y, z)) {
        CORE_LOG_ERROR("Cell at index {} has invalid position ({}, {}, {}).", i, x, y, z);
        continue;
      }

      auto& sim_cell = get_cell(x, y, z);
      sim_cell.alive = cell.alive();
      CORE_LOG_DEBUG("Loading cell [{}]:\n{}", sim_cell.index, type_data_handler<simulation::cell>::as_string(sim_cell));
    }

    tick_duration = simulation_duration(sim->tick());
    CORE_LOG_INFO("Loaded simulation tick duration: {}", tick_duration.count());
  }

  void simulation::start_simulation() {
    tick_controller.start();
  }

  bool simulation::update_simulation(const evaluation_rules& rules) {
    OTHER_ASSERT(rules.birth_condition != nullptr, "Birth condition is not set.");
    OTHER_ASSERT(rules.death_condition != nullptr, "Death condition is not set.");
    OTHER_ASSERT(rules.cell_update != nullptr, "Cell update function is not set.");

    bool tick_occurred = false;
    tick_controller.tick();
    if (tick_controller.elapsed() >= tick_duration) {
      tick_occurred = true;
      tick_controller.reset();

      bool _ = evaluate_position(rules);
      /// do something with all cells being dead
    }

    return tick_occurred;
  }

  bool simulation::evaluate_position(const evaluation_rules& rules) {
    OTHER_ASSERT(rules.birth_condition != nullptr, "Birth condition is not set.");
    OTHER_ASSERT(rules.death_condition != nullptr, "Death condition is not set.");
    OTHER_ASSERT(rules.cell_update != nullptr, "Cell update function is not set.");

    for (auto& cell : cells) {
      cell_neighbors neighbors = get_cell_neighbors(cell.position.x, cell.position.y);

      if (cell.alive) {
        if (rules.death_condition(this, cell, neighbors)) {
          cell.death_pending = true;
        }
      } else if (!cell.alive && rules.birth_condition(this, cell, neighbors)) {
        cell.birth_pending = true;
      }

      rules.cell_update(this, cell, neighbors);
    }

    for (auto& cell : cells) {
      update_cell(cell);
    }

    return any_cells_alive();
  }

  void simulation::set_dimensions(const glm::uvec2& dimensions) {
    set_dimensions(glm::uvec3(dimensions, 1));
  }

  simulation::cell& simulation::get_cell(int32_t x, int32_t y) {
    return get_cell(x, y, 0);
  }

  void simulation::set_dimensions(const glm::uvec3& dimensions) {
    coordinate_frame.dimensions = dimensions;
    cells.resize(dimensions.x * dimensions.y * dimensions.z);
    starting_cells = dimensions.x * dimensions.y * dimensions.z;
    living_cells = starting_cells;

    CORE_LOG_DEBUG("simulation::set_dimensions({}, {}, {}) ({} cells)", dimensions.x, dimensions.y, dimensions.z, cells.size());
    for (int32_t z = 0; z < dimensions.z; ++z) {
      for (int32_t y = 0; y < dimensions.y; ++y) {
        for (int32_t x = 0; x < dimensions.x; ++x) {
          int32_t idx = coordinate_frame.get_cell_index(x, y, z);
          OTHER_ASSERT(idx < cells.size(), "Index out of bounds: ({}, {}, {}) for dimensions ({}, {}, {})", x, y, z, dimensions.x, dimensions.y, dimensions.z);

          cells[idx] = {
            .index = idx,
            .position = { x, y, z },
            .alive = false,
            .death_pending = false,
            .birth_pending = false,
            .user_data = nullptr,
          };
        }
      }
    }
  }

  simulation::cell& simulation::get_cell(int32_t x, int32_t y, int32_t z) {
    size_t idx = coordinate_frame.get_cell_index(x, y, z);

    OTHER_ASSERT(idx < cells.size(), "Index out of bounds: ({}, {}, {}) for dimensions ({}, {}, {})", x, y, z, coordinate_frame.dimensions.x, coordinate_frame.dimensions.y, coordinate_frame.dimensions.z);
    return cells[idx];
  }

  void simulation::set_cell_user_data(const std::vector<void*>& user_datas) {
    OTHER_ASSERT(user_datas.size() == cells.size(), "User data size does not match cell count.");

    for (size_t i = 0; i < cells.size(); ++i) {
      cells[i].user_data = user_datas[i];
    }
  }

  void simulation::update_cell(cell& cell) {
    if (cell.death_pending) {
      cell.alive = false;
      cell.death_pending = false;
      --living_cells;
    } else if (cell.birth_pending) {
      cell.alive = true;
      cell.birth_pending = false;
      ++living_cells;
    }
  }

  simulation::cell_neighbors simulation::get_cell_neighbors(int32_t x, int32_t y) {
    cell_neighbors neighbors;

    for (int i = 0; i < cell_neighbors::kNumNeighbors; ++i) {
      int32_t dx = 0, dy = 0;
      switch (i) {
        case cell_neighbors::NORTH: dy = 1; break;
        case cell_neighbors::WEST: dx = -1; break;
        case cell_neighbors::SOUTH: dy = -1; break;
        case cell_neighbors::EAST: dx = 1; break;

        case cell_neighbors::NORTH_WEST:
          dx = -1;
          dy = 1;
          break;
        case cell_neighbors::NORTH_EAST:
          dx = 1;
          dy = 1;
          break;

        case cell_neighbors::SOUTH_WEST:
          dx = -1;
          dy = -1;
          break;
        case cell_neighbors::SOUTH_EAST:
          dx = 1;
          dy = -1;
          break;
      }

      if (!coordinate_frame.valid_position(x + dx, y + dy, 0)) {
        neighbors.neighbors[i] = nullptr;
      } else {
        size_t idx = coordinate_frame.get_cell_index(x + dx, y + dy, 0);
        neighbors.neighbors[i] = &cells[idx];
      }
    }

    return neighbors;
  }

}  // namespace other