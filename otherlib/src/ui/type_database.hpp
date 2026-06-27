/**
 * \file ui/type_database.hpp
 **/
#ifndef OTHERLIB_UI_TYPE_DATABASE_HPP
#define OTHERLIB_UI_TYPE_DATABASE_HPP

#include "ui/ui_window.hpp"

namespace other {
  namespace ui {

    class type_database : public ui_window {
     public:
      type_database(event_system& event);
      virtual ~type_database() = default;
    };

  }  // namespace ui
}  // namespace other

#endif  // OTHERLIB_UI_TYPE_DATABASE_HPP