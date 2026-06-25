/**
 * \file edit_stack.cpp
 **/
#include "edit_stack.hpp"

namespace other {

  void edit_stack::commit(const edit& e) {
    OTHER_ASSERT(e.apply != nullptr, "Cannot commit edit: no apply action defined.");
    OTHER_ASSERT(e.undo != nullptr, "Cannot commit edit: no undo action defined.");
    e.apply();
    edit_future.clear();
    edit_history.push_back(e);
  }

  void edit_stack::undo() {
    if (can_undo()) {
      edit e = edit_history.back();
      OTHER_ASSERT(e.undo != nullptr, "Cannot undo edit: no undo action defined.");
      e.undo();

      edit_history.pop_back();
      edit_future.push_back(e);
    }
  }

  void edit_stack::redo() {
    if (can_redo()) {
      edit e = edit_future.back();
      OTHER_ASSERT(e.apply != nullptr, "Cannot redo edit: no apply action defined.");
      e.apply();

      edit_future.pop_back();
      edit_history.push_back(e);
    }
  }

}  // namespace other