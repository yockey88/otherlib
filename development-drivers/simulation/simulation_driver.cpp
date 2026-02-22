/**
 * \file simulation_driver.cpp
 **/
#include "simulation_driver.hpp"

#include <cmath>

#include "gpu_resource/texture.hpp"

#include "object/scene_object.hpp"

#include "imgui.h"
#include "simulation.hpp"

namespace other {

  struct cell_data {
    enum type {
      AIR = 0,
      DIRT,
      GRASS,
      WATER
    };
    type cell_type = AIR;
  };

  struct simple_life_sim_data {
    std::vector<cell_data> cells;
    glm::uvec2 dimensions = { 0, 0 };
  };

  namespace {

    constexpr inline size_t kNumTextureChannels = 4;
    constexpr inline size_t kTextureDimensions = 1024;
    constexpr inline size_t kNumRawPixels = kTextureDimensions * kTextureDimensions;
    constexpr inline size_t kNumTextureFloats = kNumRawPixels * kNumTextureChannels;

    void write_pixel_data(const glm::uvec2& sim_dim, const std::span<const simulation::cell> cells, std::vector<float>& pixel_data);

    struct texture_writer {
      using color_chooser = glm::vec4 (*)(const simulation::cell& cell);
      void operator()(color_chooser chooser, const glm::uvec2& sim_dim, const std::span<const simulation::cell> cells, std::vector<float>& pixel_data);
    };

    static constexpr simulation::evaluation_rules cgol_rules = {
      .birth_condition = [](simulation* sim, const simulation::cell& cell, const simulation::cell_neighbors& neighbors) -> bool {
        return neighbors.num_living_neighbors() == 3;
      },
      .death_condition = [](simulation* sim, const simulation::cell& cell, const simulation::cell_neighbors& neighbors) -> bool {
        size_t living_neighbors = neighbors.num_living_neighbors();
        return living_neighbors >= 4 || living_neighbors <= 1;
      },
      .cell_update = [](simulation* sim, simulation::cell& cell, const simulation::cell_neighbors& neighbors) {
        // No-op for conway's game of life
      }
    };

  }  // namespace

  enum channels {
    CHANNEL_R = 0,
    CHANNEL_G = 1,
    CHANNEL_B = 2,
    CHANNEL_A = 3
  };

  void simulation_driver::on_initialize(const command_line& cmd) {
    config_table config = configuration();

    {
      renderer = get_renderer();
      OTHER_ASSERT(renderer != nullptr, "Renderer backend is not initialized.");

      renderer->set_clear_color(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
      renderer->add_pipeline<empty_pipeline>("Empty Pipeline");
    }

    scene_object& simulation_data = active_scene.create_object("Simulation Data");
    auto& sim_data = active_scene.add_component<simple_life_sim_data>(&simulation_data);

    scene_object& simulation_obj = active_scene.create_object("Simulation");
    simulation_id = simulation_obj.id;

    auto& sim = active_scene.add_component<simulation>(&simulation_obj);
    sim.read_initial_config("development-drivers/simulation/resources/conway-gol-config.osim");

    sim_data.dimensions = { sim.dimensions().x, sim.dimensions().y };
    sim_data.cells.resize(sim.dimensions().x * sim.dimensions().y);
    OTHER_ASSERT(sim_data.cells.size() == sim.get_cell_count(), "Cell count does not match simulation dimensions.");
    for (int32_t i = 0; i < sim.get_cell_data().size(); ++i) {
      auto& cell = sim.get_cell_data()[i];

      sim_data.cells[i].cell_type = cell.alive ?
        cell_data::GRASS :
        cell_data::DIRT;
      cell.user_data = &sim_data.cells[i];
    }

    pixel_colors.resize(kNumTextureFloats);
    // clang-format off
    CORE_LOG_DEBUG("\nNum Cells: {}\nSimulation dimensions: ({}, {})\nCell Pixel Size: ({}, {})\nTexture Dimensions: ({}, {})\nTexture Dimensions (w/ Channels): ({}, {})", 
                    sim.get_cell_count(), sim.dimensions().x, sim.dimensions().y, kTextureDimensions / sim.dimensions().x, kTextureDimensions / sim.dimensions().y, 
                    kTextureDimensions, kTextureDimensions, kTextureDimensions * kNumTextureChannels, kTextureDimensions * kNumTextureChannels);
    // clang-format on
    sim_texture = texture::create("simulation_texture", texture::TEXTURE_2D, texture::RGBA32F, kTextureDimensions, kTextureDimensions);
  }

  void simulation_driver::run() {
    simulation* sim = active_scene.get_component<simulation>(simulation_id);
    OTHER_ASSERT(sim != nullptr, "Simulation component is not found in the scene.");

    write_pixel_data(sim->dimensions(), sim->read_cell_data(), pixel_colors);
    renderer->get_resource<texture>(sim_texture)
      .set_data(pixel_colors.data(), pixel_colors.size() * sizeof(float))
      .finalize_texture();

    sim->start_simulation();

    while (running) {
      MARK_NAMED_FRAME("Main Frame");
      PROFILE_SECTION("simulation-driver::main-loop");

      pump_events();
      if (!running) {
        break;
      }

      if (sim->update_simulation(cgol_rules)) {
      }

      write_pixel_data(sim->dimensions(), sim->read_cell_data(), pixel_colors);
      renderer->get_resource<texture>(sim_texture)
        .set_data(pixel_colors.data(), pixel_colors.size() * sizeof(float))
        .finalize_texture();

      render_data scene_render_data = active_scene.prepare_render_data();
      renderer->begin_frame(&scene_render_data);
      renderer->render();

      renderer->begin_ui_frame();
      if (ImGui::Begin("Simulation Window")) {
        void* texture_gpu_resource = renderer->get_texture_gpu_resource(sim_texture);
        if (texture_gpu_resource) {
          ImGui::Image((ImTextureID)(uintptr_t)texture_gpu_resource, ImVec2(kTextureDimensions, kTextureDimensions), ImVec2(0, 1), ImVec2(1, 0));
        } else {
          ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "No texture for simulation!");
        }
      }
      ImGui::End();

      if (ImGui::Begin("Simulation Control")) {
        ImGui::SeparatorText("Controls");
        int32_t tick_dur = sim->get_tick_duration().count();
        ImGui::DragInt("Tick Duration (ms)", &tick_dur, 1, 100, 5000, "%d ms");
        sim->set_tick_duration(simulation_duration(tick_dur));
      }
      ImGui::End();
      renderer->end_ui_frame();

      renderer->end_frame();
    }
  }

  void simulation_driver::on_shutdown() {
    OTHER_ASSERT(renderer != nullptr, "Renderer is not initialized.");

    renderer->remove_pipeline("Empty Pipeline");
    renderer = nullptr;
  }

  void simulation_driver::on_event(SDL_Event* event) {
    switch (event->type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        running = false;
        break;

      case SDL_EVENT_KEY_DOWN:
        break;

      default:
        break;
    }
  }

  namespace {

    void write_pixel_data(const glm::uvec2& sim_dim, const std::span<const simulation::cell> cells, std::vector<float>& pixel_data) {
      auto writer = [](const simulation::cell& cell) -> glm::vec4 {
        return cell.alive ? glm::vec4(1.f, 1.f, 1.f, 1.f) : glm::vec4(0.f, 0.f, 0.f, 1.f);
      };
      texture_writer{}(writer, sim_dim, cells, pixel_data);
    }

    void texture_writer::operator()(color_chooser chooser, const glm::uvec2& sim_dim, const std::span<const simulation::cell> cells, std::vector<float>& pixel_data) {
      OTHER_ASSERT(chooser != nullptr, "Color chooser function is not set.");
      std::ranges::fill(pixel_data, 0.f);

      size_t pixel_dim = kTextureDimensions / sim_dim.x;
      for (size_t cell_idx = 0; cell_idx < cells.size(); ++cell_idx) {
        const simulation::cell& cell = cells[cell_idx];

        size_t col_num = cell_idx % sim_dim.x;
        size_t row_num = std::floor((float)cell_idx / sim_dim.x);

        glm::vec4 color = chooser(cell);

        glm::uvec2 pixel_xbounds = { col_num * pixel_dim, (col_num + 1) * pixel_dim };
        glm::uvec2 pixel_ybounds = { row_num * pixel_dim, (row_num + 1) * pixel_dim };

        for (size_t y = pixel_ybounds.x; y < pixel_ybounds.y; ++y) {
          for (size_t x = pixel_xbounds.x; x < pixel_xbounds.y; ++x) {
            size_t channel_y = y * kNumTextureChannels;
            size_t channel_x = x * kNumTextureChannels;

            size_t pixel_index = channel_y * kTextureDimensions + channel_x;

            pixel_data[pixel_index + CHANNEL_R] = color.r;
            pixel_data[pixel_index + CHANNEL_G] = color.g;
            pixel_data[pixel_index + CHANNEL_B] = color.b;
            pixel_data[pixel_index + CHANNEL_A] = color.a;
          }
        }
      }
    }

  }  // namespace
}  // namespace other
