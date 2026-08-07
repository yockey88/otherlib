/**
 * \file edit_stack.hpp
 **/
#ifndef OTHER_EDITOR_EDIT_STACK_HPP
#define OTHER_EDITOR_EDIT_STACK_HPP

#include "data-structures/std_container.hpp"

namespace other {

  struct edit {
    using action = std::function<void()>;
    action apply;
    action undo;
  };

  class edit_stack {
   public:
    /// applies the edit immediately, then records it
    void commit(const edit& e);
    /// records an edit whose mutation ALREADY happened (widget writes, snapshot pairs);
    ///  apply only runs on redo
    void record(const edit& e);
    void undo();
    void redo();
    void clear();

    inline bool can_undo() const { return !edit_history.empty(); }
    inline bool can_redo() const { return !edit_future.empty(); }

   private:
    ostd::vector<edit> edit_history;
    ostd::vector<edit> edit_future;
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDIT_STACK_HPP