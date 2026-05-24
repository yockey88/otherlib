/**
 * \file renderer/pass_executor_interop.hpp
 **/
#ifndef OTHER_RENDERER_RENDERER_PASS_EXECUTOR_INTEROP_HPP
#define OTHER_RENDERER_RENDERER_PASS_EXECUTOR_INTEROP_HPP

#include "renderer/pass_context.hpp"

namespace other {

  struct pass_invocation_interop {
    pass_context* context;
  };

}  // namespace other

#endif  // OTHER_RENDERER_RENDERER_PASS_EXECUTOR_INTEROP_HPP