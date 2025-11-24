/**
 * \file runtime-dev/runtime.cpp
 **/
#include "runtime.hpp"

#include <cstdint>
#include <memory>

#include <imgui/ImReflect.hpp>
#include <imgui/imgui.h>

#include "core/timer.hpp"

#include "renderer/camera.hpp"
#include "renderer/renderer.hpp"
#include "script/scripting_environment.hpp"

#include "object/animation_controller.hpp"
#include "object/object_serialization.hpp"
#include "object/render_component.hpp"
#include "object/scene_object.hpp"
#include "object/script_component.hpp"

#include "rendering-pipelines/default_instancing_pipeline.hpp"
#include "scripting/execution_nodes/transform_nodes.hpp"
#include "ui/console.hpp"
#include "ui/console_history_node.hpp"
#include "ui/node_editor_canvas_node.hpp"

#include "behavior_tree.hpp"

namespace other {

  void runtime_state_machine::on_enter_state(runtime_state new_state) {
    CORE_LOG_DEBUG("Server state changed to {}", new_state);
    if (new_state == runtime_state::RUNTIME_STATE_RUNNING) {
    }
  }

  void runtime::on_initialize(const command_line& cmd) {
    CORE_LOG_DEBUG("Runtime...");
    state_machine.handle_event(runtime_event::RUNTIME_EVENT_START, this);

    renderer = get_renderer();
    if (!renderer) {
      CORE_LOG_ERROR("Renderer backend is not initialized.");
      return;
    }
    renderer->set_clear_color(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
    renderer->add_pipeline<default_instancing_pipeline>("Default Instancing Pipeline");

    asset_mgr = make_scope<asset_handler>(net_context->io_context);

    get_event_system()->add_listener("shutdown-requested", [this](const value& data) {
      CORE_LOG_DEBUG("Shutdown requested event received in runtime.");
      state_machine.handle_event(runtime_event::RUNTIME_EVENT_STOP, this);
    });

    get_event_system()->register_event("project-loaded");
    get_event_system()->add_listener("project-loaded", [this](const value& data) {
      CORE_LOG_DEBUG("Project loaded event received in runtime.");
      /// load first scene
      /// for now just harcode scene set up here
      current_scene_id = create_new_scene("Main Scene");
      auto* scene_ptr = get_scene(current_scene_id);

      /// light
      {
        scene_object& light_obj = scene_ptr->create_object("Light", glm::vec3(0.f, 2.f, 0.f));
        scene_ptr->add_object_tag(light_obj.id, "scene-ambient-light");

        transform& light_transform = scene_ptr->get_transform(&light_obj);
        light_transform.local_scale = glm::vec3(0.1f, 0.1f, 0.1f);

        gpu::point_light& light_plight = scene_ptr->add_component<gpu::point_light>(&light_obj);
        light_plight.light_position = glm::vec3(0.f, 5.f, 0.f);
        light_plight.color = glm::vec3(1.f, 1.f, 1.f);

        gpu::directional_light& light_dlight = scene_ptr->add_component<gpu::directional_light>(&light_obj);
        light_dlight.direction = glm::vec3(0.f, -1.f, 0.f);
        light_dlight.color = glm::vec3(1.f, 1.f, 1.f);
      }

      /// camera
      {
        scene_object& cam_obj = scene_ptr->create_object("Main Camera", glm::vec3(0.f, 0.f, 0.f));
        scene_ptr->add_object_tag(cam_obj.id, "main-camera");

        other::camera& cam = scene_ptr->add_component<camera>(&cam_obj);
        cam.sensitivity = 0.35f;
        cam.look({ 0.f, 1.f, 4.5f }, { 0.f, 0.f, 0.f });
        CORE_LOG_INFO("Camera data loaded from file: \n{}", type_data_handler<camera>::as_string("cam", cam));
      }

      /// donut
      {
        scene_object& donut_obj = scene_ptr->create_object("Donut", glm::vec3(0.f, -0.5f, 0.f));
        donut_id = donut_obj.id;

        script_component* donut_script = scene_ptr->get_component<script_component>(&donut_obj);
        OTHER_ASSERT(donut_script != nullptr, "Failed to get script component for donut object in runtime");

        const auto& config = configuration();
        std::string model_path = config.get_value<std::string>("assets.test-model", "resources/models/suzanne3.fbx");
        donut_model_id = asset_mgr->load_asset(model_path);

        behavior_tree& bt = scene_ptr->add_component<behavior_tree>(&donut_obj);
        bt.add_node<constant_angular_velocity_node>("Spin", glm::vec3(0.f, 1.f, 0.f));
        bt.connect_nodes("InputNode", 0, "OutputNode", 0);

        bt.connect_nodes("InputNode", 1, "Spin", 0);
        bt.connect_nodes("Spin", 0, "OutputNode", 1);

        bt.connect_nodes("InputNode", 2, "OutputNode", 2);
      }

      CORE_LOG_INFO("Scene '{}' created and set as current scene in runtime.", scene_ptr->name);

      scene_object& donut_obj = scene_ptr->get_object(donut_id);
      behavior_tree* bt = scene_ptr->get_component<behavior_tree>(&donut_obj);
      OTHER_ASSERT(bt != nullptr, "Behavior tree component is null in runtime draw loop for donut object");

      auto topo_sort = bt->topological_sort();
      for (natural_t node_id : topo_sort) {
        const auto& node = bt->get_node_by_id(node_id);
        node_editor->add_editor_node(node.name, node.exec_node->get_num_inputs(), node.exec_node->get_num_outputs());
      }
      for (const auto& link : bt->links) {
        const auto& from_node = bt->get_node_by_id(link.from.node_id);
        const auto& to_node = bt->get_node_by_id(link.to.node_id);
        node_editor->connect_node_pins(from_node.name, link.from.pin_index, to_node.name, link.to.pin_index);
      }
      node_editor->reorganize_nodes();
    });

    get_event_system()->register_timed_event("application-fixed-update", duration_cast<microseconds>(seconds(1)), true);
    get_event_system()->add_listener("application-fixed-update", [this](const value& data) {
      // driver_step_device();
      if (auto* current_scene = get_scene(current_scene_id); current_scene != nullptr) {
        current_scene->fixed_update(1.f / 60.f);
      }
    });

    runtime_ui = make_scope<runtime_control_window>(*get_event_system());
    // runtime_ui->add_node(make_scope<ui::device_display>(core_device, runtime_ui.get(), "Runtime"));

    integer_t session_id = cmd.session_id.value_or(-1);
    uint16_t port = cmd.port.value_or(49222);

    net_thread = make_scope<network_thread>(net_thread_message_bus);
    net_thread->launch();
    net_thread_message_bus.register_thread();

    if (session_id == -1) {
      CORE_LOG_WARN("No session ID provided to runtime");
    } else {
      message msg;
      msg.header = {
        .category = COMMAND,
        .id = SESSION_CHECK_IN,
      };

      const uint8_t* id_bytes = reinterpret_cast<const uint8_t*>(&session_id);
      const uint8_t* port_bytes = reinterpret_cast<const uint8_t*>(&port);
      msg.data.append_range(std::span(id_bytes, sizeof(integer_t)));
      msg.data.append_range(std::span(port_bytes, sizeof(uint16_t)));
      net_thread_message_bus.send_message(std::move(msg));
    }

    node_editor = make_scope<ui::node_editor>(*get_event_system());
    console_window = make_scope<ui::console_window>(*get_event_system(), std::bind_front(&runtime::handle_console_command, this));
    console_lua_script = subsystem<scripting_environment>::get()->load_lua_file("resources/lua/console_commands.lua");
    if (console_lua_script == nullptr || !console_lua_script->is_valid()) {
      CORE_LOG_ERROR("Failed to load console Lua script in runtime.");
    } else {
      CORE_LOG_INFO("Console Lua script loaded successfully in runtime.");
      auto& state = console_lua_script->get_state();
      auto native_table = state["__other_native"];
      /// add current driver table
    }

    last_frame_time = std::chrono::steady_clock::now();

    running = true;
  }

  void runtime::run() {
    last_frame_time = std::chrono::steady_clock::now();

    do {
      pump_events();
      core_update();

      switch (state_machine.get_current_state()) {
        case runtime_state::RUNTIME_STATE_LOADING_PROJECT: update_loading_project(); break;
        case runtime_state::RUNTIME_STATE_WAITING_FOR_START_SCENE_LOAD: update_waiting_for_start_scene_load(); break;
        case runtime_state::RUNTIME_STATE_RUNNING: update_running(); break;
        case runtime_state::RUNTIME_STATE_SHUTTING_DOWN: update_shutting_down(); break;
        default:
          CORE_LOG_ERROR("Server in unknown state {}", state_machine.get_current_state());
          running = false;
          break;
      }
      draw();
    } while (running);
  }

  void runtime::on_shutdown() {
    CORE_LOG_DEBUG("Shutting down runtime...");
    auto* env = subsystem<scripting_environment>::get();
    if (env && builder_obj_id != -1) {
      env->destroy_object(builder_obj_id);
      builder_obj_id = -1;
    }

    running = false;

    net_thread->shutdown();
    net_thread = nullptr;

    asset_mgr = nullptr;
    CORE_LOG_DEBUG("Runtime shut down complete.");
  }

  void runtime::catch_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
      CORE_LOG_INFO("Received signal {}, shutting down runtime...", signal);
      running = false;
    } else {
      CORE_LOG_WARN("Received unhandled signal {}", signal);
    }
  }

  void runtime::core_update() {
    std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
    curr_frame_delta_time = std::chrono::duration<float>(current_time - last_frame_time).count();
    last_frame_time = current_time;

    net_context->io_context.poll();
    if (net_context->io_context.stopped()) {
      net_context->io_context.restart();
    }

    asset_mgr->update_pipelines();
  }

  void runtime::update_loading_project() {
    get_event_system()->trigger_event("project-loaded");
    state_machine.handle_event(runtime_event::RUNTIME_EVENT_READY, this);
  }

  void runtime::update_waiting_for_start_scene_load() {
    if (asset_mgr->get_asset_state(donut_model_id) == asset_state::LOADED) {
      CORE_LOG_DEBUG("Donut model asset loaded successfully in runtime.");

      auto* current_scene = get_scene(current_scene_id);
      OTHER_ASSERT(current_scene != nullptr, "Current scene is null in runtime while loading donut model");

      uint64_t hash = asset_mgr->get_asset_hash(donut_model_id);
      ref<model_source> donut_source = subsystem<renderer_backend>::get()->get_model_source(hash);
      if (donut_source != nullptr) {
        CORE_LOG_DEBUG("Donut model source loaded successfully in runtime.");
        scene_object& donut_obj = current_scene->get_object(donut_id);

        render_component& donut_render = current_scene->add_component<render_component>(&donut_obj);
        donut_render.material.diffuse_color = glm::vec3(0.4f, 0.6f, 0.8f);
        donut_render.material.diffuse_reflectivity = 0.5f;
        donut_render.material.specular_color = glm::vec3(0.8f, 0.8f, 0.8f);
        donut_render.material.specular_reflectivity = 0.5f;
        donut_render.material.emissivity = 0.1f;
        donut_render.material.shininess = 16.f;
        donut_render.material.transparency = 0.f;

        donut_model = donut_source->produce_model("Donut");
        donut_render.model = &donut_model;

        // if (const auto& animations = donut_source->get_animations(); !animations.empty()) {
        //   animation_controller& anim_ctrl = current_scene->add_component<animation_controller>(&donut_obj);
        //   anim_ctrl.anim_ptr = donut_source->get_animation(0);
        //   anim_ctrl.model_ptr = donut_render.model;
        // }
      }

      CORE_LOG_DEBUG("Writing scene ID {} to device and emitting load scene opcode.", current_scene_id);
      set_scene_to_active(current_scene_id);
      state_machine.handle_event(runtime_event::RUNTIME_EVENT_READY, this);
    }
  }

  void runtime::update_running() {
    scene* current_scene = get_scene(current_scene_id);
    OTHER_ASSERT(current_scene != nullptr, "Current scene is null in runtime update loop");

    scene_object& donut_obj = current_scene->get_object(donut_id);
    behavior_tree* bt = current_scene->get_component<behavior_tree>(&donut_obj);
    OTHER_ASSERT(bt != nullptr, "Behavior tree component is null in runtime update loop for donut object");

    transform& donut_transform = current_scene->get_transform(&donut_obj);
    bt->set_input_transform(donut_transform);
    donut_transform = bt->execute_tree();
  }

  void runtime::update_shutting_down() {
    running = false;
    CORE_LOG_DEBUG("Runtime shut down complete");

    state_machine.handle_event(runtime_event::RUNTIME_EVENT_STOP, this);
  }

  void runtime::draw() {
    render_data data = {};
    if (auto* active_scene = get_active_scene(); active_scene != nullptr) {
      PROFILE_SECTION("rendering-dev--render-frame");
      data = active_scene->prepare_render_data();
      renderer->begin_frame(&data);
    } else {
      renderer->begin_frame(nullptr);
    }

    renderer->render();
    renderer->begin_ui_frame();

    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Exit")) {
          get_event_system()->trigger_event("shutdown-requested");
        }
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Tools")) {
        if (ImGui::MenuItem("Toggle Node Editor")) {
          show_node_editor = !show_node_editor;
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    if (ImGui::Begin("Runtime Debug")) {
      ImGui::Text("Frame Time: %.3f ms", curr_frame_delta_time * 1000.f);
      ImGui::Text("FPS: %.1f", 1.0f / curr_frame_delta_time);
    }
    ImGui::End();

    if (show_node_editor) {
      node_editor->render();
    }
    if (show_console_window) {
      console_window->render();
    }

    // runtime_ui->render();

    renderer->end_ui_frame();
    renderer->end_frame();
  }

  bool runtime::handle_console_command(const std::string_view command, system_timepoint timestamp) {
    if (console_lua_script != nullptr) {
      return console_lua_script->call_function<bool>("handle_console_command", std::string(command));
    } else {
      return false;
    }
  }

  void runtime::on_event(SDL_Event* event) {
    OTHER_ASSERT(event != nullptr, "Event is null");
    switch (event->type) {
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED: get_event_system()->trigger_event("shutdown-requested"); break;
      default: break;
    }
  }

}  // namespace other