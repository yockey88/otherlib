/**
 * \file editor-dev/editor.hpp
 **/
#ifndef OTHER_EDITOR_HPP
#define OTHER_EDITOR_HPP

#include "driver/driver.hpp"

namespace other {

  class OTHER_CLASS editor : public driver {
   public:
    editor(const config_table& config)
        : driver(config) {}
    virtual ~editor() = default;

    void on_initialize() override;
    void run() override;
    void on_shutdown() override;
  };

}  // namespace other

OTHER_DRIVER(other::editor)

#endif  // OTHER_EDITOR_HPP