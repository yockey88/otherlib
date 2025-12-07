/**
 * \file server-dev/server_tasks.hpp
 **/
#ifndef OTHER_SERVER_SERVER_TASKS_HPP
#define OTHER_SERVER_SERVER_TASKS_HPP

#include <nlohmann/json.hpp>

#include "core/coroutine.hpp"
#include "event/event_system.hpp"

#include "server-ui/project-creator.hpp"

namespace json = nlohmann;

namespace other {

  task build_project(const project_creator::project_context& context, json::json& project_cache_path, event_system& events);

}  // namespace other

#endif  // OTHER_SERVER_SERVER_TASKS_HPP