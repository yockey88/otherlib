/**
 * \file python/interpreter.hpp
 **/
#ifndef OTHER_SCRIPTING_PYTHON_INTERPRETER_HPP
#define OTHER_SCRIPTING_PYTHON_INTERPRETER_HPP

namespace pybind11 {
  class scoped_interpreter;
}  // namespace pybind11

namespace other {

  class python_interpreter {
   public:
    python_interpreter() = default;
    ~python_interpreter() = default;

    void load_host();
    void unload_host();
    void call_entry_point();

   private:
    pybind11::scoped_interpreter* guard = nullptr;
  };

}  // namespace other

#endif  // OTHER_SCRIPTING_PYTHON_INTERPRETER_HPP