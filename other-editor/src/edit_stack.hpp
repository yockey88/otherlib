/**
 * \file edit_stack.hpp
 **/
#ifndef OTHER_EDITOR_EDIT_STACK_HPP
#define OTHER_EDITOR_EDIT_STACK_HPP

namespace other {

  struct edit {
    using action = std::function<void()>;
    action apply;
    action undo;
  };

  class edit_stack {
   public:
    void commit(const edit& e);
    void undo();
    void redo();

    inline bool can_undo() const { return !edit_history.empty(); }
    inline bool can_redo() const { return !edit_future.empty(); }

   private:
    std::vector<edit> edit_history;
    std::vector<edit> edit_future;
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDIT_STACK_HPP