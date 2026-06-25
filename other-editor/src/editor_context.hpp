/**
 * \file editor_context.hpp
 **/
#ifndef OTHER_EDITOR_EDITOR_CONTEXT_HPP
#define OTHER_EDITOR_EDITOR_CONTEXT_HPP

namespace other {

  class editor_driver;

  struct editor_context {
    struct selection {
      natural_t scene = 0;
      std::vector<natural_t> objects = {};
    };

    editor_driver* driver;
    selection current_selection;
  };

}  // namespace other

#endif  // OTHER_EDITOR_EDITOR_CONTEXT_HPP