/**
 * \file object/component.hpp
 **/
#ifndef OTHER_SCENE_OBJECT_COMPONENT_HPP
#define OTHER_SCENE_OBJECT_COMPONENT_HPP

namespace other {

  class component {
   public:
    enum id {
      TRANSFORM = 0,
      RENDERER,
      PHYSICS,
      SCRIPT,
      AUDIO,
      POINT_LIGHT,
      DIRECTION_LIGHT,
      CAMERA,
      ANIMATION,

      REGISTRY,
      CUSTOM,
      NUM_COMPONENT_TYPES,
      INVALID_COMPONENT_TYPE = NUM_COMPONENT_TYPES
    };

    component(id comp_id)
        : comp_id(comp_id) {}
    virtual ~component() = default;

    id get_id() const { return comp_id; }

   private:
    id comp_id = CUSTOM;
  };

}  // namespace other

#endif  // OTHER_SCENE_OBJECT_COMPONENT_HPP