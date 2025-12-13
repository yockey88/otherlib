/**
 * \file driver/application_list.hpp
 **/
#ifndef OTHERLIB_DRIVER_APPLICATION_LIST_HPP
#define OTHERLIB_DRIVER_APPLICATION_LIST_HPP

#include <deque>
#include <map>

#include "core/defines.hpp"

namespace other {

  struct application_list {
    struct other_application {
      integer_t id = 0;

      natural_t session_info_request_resp_id = 0;

      opt<filepath> working_directory;

      opt<filepath> executable;
      opt<std::string> name;

      std::vector<std::string> args;
      bool connected = false;

      inline std::string get_name() const {
        return name.has_value() ? *name : (executable.has_value() ? executable->filename().stem().string() : "<unnamed>");
      }
    };
    std::deque<other_application> pending_apps;
    std::map<integer_t, other_application> other_apps;
  };

}  // namespace other

#endif  // OTHERLIB_DRIVER_APPLICATION_LIST_HPP