/**
 * \file backends/jolt_api.cpp
 **/
#include "physics/backends/jolt_api.hpp"

#include "math/matrix.hpp"

#include "physics_world/physics_world.hpp"

// clang-format off
#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#define JPH_PROFILE_ENABLED
#define JPH_OBJECT_STREAM
#define JPH_SHARED_LIBRARY 
// #define JPH_DOUBLE_PRECISION 
#define JPH_DEBUG_RENDERER
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
// clang-format on

namespace other {
  namespace Layers {

    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;

  };  // namespace Layers
  namespace BroadPhaseLayers {

    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS(2);

  };  // namespace BroadPhaseLayers

  struct shape_container {
    bool default_empty = false;
    JPH::ShapeRefC jolt_shape = nullptr;
  };

  class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
   public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override;
  };

  // BroadPhaseLayerInterface implementation
  // This defines a mapping between object and broadphase layers.
  class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
   public:
    BPLayerInterfaceImpl();

    virtual JPH::uint GetNumBroadPhaseLayers() const override;

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

   private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
  };

  /// Class that determines if an object layer can collide with a broadphase layer
  class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
   public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
  };

  class MyContactListener : public JPH::ContactListener {
   public:
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;
    virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
    virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;
    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;
  };

  class MyDebugRenderer : public JPH::DebugRendererSimple {
   public:
    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
      debug_lines.push_back(physics_api::line{
        .start = glm::vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ()),
        .end = glm::vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ()),
      });
      // clang-format off
      debug_line_colors.push_back(glm::vec4(
        static_cast<float>(inColor.r) / 255.0f,
        static_cast<float>(inColor.g) / 255.0f,
        static_cast<float>(inColor.b) / 255.0f,
        static_cast<float>(inColor.a) / 255.0f
      ));
      // clang-format on
    }

    virtual void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override {
      debug_triangles.push_back(physics_api::triangle{
        .v0 = glm::vec3(inV1.GetX(), inV1.GetY(), inV1.GetZ()),
        .v1 = glm::vec3(inV2.GetX(), inV2.GetY(), inV2.GetZ()),
        .v2 = glm::vec3(inV3.GetX(), inV3.GetY(), inV3.GetZ()),
      });
      // clang-format off
      debug_triangle_colors.push_back(glm::vec4(
        static_cast<float>(inColor.r) / 255.0f,
        static_cast<float>(inColor.g) / 255.0f,
        static_cast<float>(inColor.b) / 255.0f,
        static_cast<float>(inColor.a) / 255.0f
      ));
      // clang-format on
    }

    virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override {
    }

    std::vector<physics_api::line> debug_lines;
    std::vector<glm::vec4> debug_line_colors;

    std::vector<physics_api::triangle> debug_triangles;
    std::vector<glm::vec4> debug_triangle_colors;
  };

  static void trace_impl(const char* inFMT, ...) {
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    std::ranges::fill(std::span(buffer), 0);
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    CORE_LOG_TRACE("[JOLT] {}", buffer);
  }

  physics_api::physics_render_debug_data jolt_api::get_debug_render_data(natural_t id, const physics_world* world) const {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt debug render data retrieval.");

    auto itr = jolt_scenes.find(id);
    OTHER_ASSERT(itr != jolt_scenes.end(), "Jolt physics scene with id '{}' does not exist during debug render data retrieval.", id);

    MyDebugRenderer debug_renderer;
    JPH::BodyManager::DrawSettings body_draw_settings;
    body_draw_settings.mDrawShape = true;
    itr->second->DrawBodies(body_draw_settings, &debug_renderer);
    itr->second->DrawConstraints(&debug_renderer);

    physics_api::physics_render_debug_data debug_data;
    debug_data.debug_lines = std::move(debug_renderer.debug_lines);
    debug_data.debug_line_colors = std::move(debug_renderer.debug_line_colors);
    debug_data.debug_triangles = std::move(debug_renderer.debug_triangles);
    debug_data.debug_triangle_colors = std::move(debug_renderer.debug_triangle_colors);

    return debug_data;
  }

  void jolt_api::initialize_world(natural_t id, physics_world* world) {
    if (jolt_scenes.find(id) != jolt_scenes.end()) {
      CORE_LOG_WARN("Jolt physics scene with id '{}' already exists.", id);
      return;
    }

    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const JPH::uint cMaxBodies = 1024;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    const JPH::uint cNumBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const JPH::uint cMaxBodyPairs = 1024;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
    const JPH::uint cMaxContactConstraints = 1024;

    JPH::PhysicsSystem* physics_system = new JPH::PhysicsSystem();
    physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *bp_layer_interface, *object_vs_broadphase_layer_filter, *object_layer_pair_filter);
    physics_system->SetContactListener(new MyContactListener());

    jolt_scenes.emplace(id, physics_system);
  }

  void jolt_api::shutdown_world(physics_world* world) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt shutdown.");

    auto itr = jolt_scenes.find(world->world_id);
    if (itr != jolt_scenes.end()) {
      delete itr->second;
      jolt_scenes.erase(itr);
    } else {
      CORE_LOG_ERROR("Jolt physics scene with id '{}' does not exist during shutdown.", world->world_id);
    }
  }

  void jolt_api::on_scene_start(natural_t world_id, physics_world* world) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt scene start.");

    auto itr = jolt_scenes.find(world_id);
    OTHER_ASSERT(itr != jolt_scenes.end(), "Jolt physics scene with id '{}' does not exist during scene start.", world_id);

    itr->second->OptimizeBroadPhase();
  }

  void jolt_api::register_physics_body(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt physics body registration.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Jolt physics body registration.");

    auto itr = jolt_scenes.find(world_id);
    OTHER_ASSERT(itr != jolt_scenes.end(), "Jolt physics scene with id '{}' does not exist during physics body registration.", world_id);

    auto [sitr, success] = jolt_shapes.emplace(body->id, new shape_container());
    OTHER_ASSERT(success && sitr != jolt_shapes.end(), "Failed to create Jolt shape during physics body registration.");
    shape_container* container = static_cast<shape_container*>(sitr->second);

    /// by default attach an empty shape, this will register as no shape in the UI
    JPH::EmptyShapeSettings shape_settings;
    shape_settings.SetEmbedded();

    JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
    container->jolt_shape = shape_result.Get();
    container->default_empty = true;

    JPH::BodyInterface& body_interface = itr->second->GetBodyInterface();

    glm::vec3 position = body->get_current_position();
    glm::quat rotation = body->get_current_rotation();

    JPH::EMotionType type;
    switch (body->body_type) {
      case BODY_TYPE_STATIC: type = JPH::EMotionType::Static; break;
      case BODY_TYPE_DYNAMIC: type = JPH::EMotionType::Dynamic; break;
      case BODY_TYPE_KINEMATIC: type = JPH::EMotionType::Kinematic; break;
      default: type = JPH::EMotionType::Static; break;
    }

    JPH::BodyCreationSettings body_settings(container->jolt_shape, JPH::RVec3(position.x, position.y, position.z), JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w), type, Layers::MOVING);

    JPH::Body* jolt_body = body_interface.CreateBody(body_settings);
    OTHER_ASSERT(jolt_body != nullptr, "Failed to create Jolt body for physics body registration.");

    JPH::BodyID body_id = jolt_body->GetID();
    body_interface.AddBody(jolt_body->GetID(), JPH::EActivation::Activate);

    body->scene_id = body_id.GetIndexAndSequenceNumber();
    auto [bitr, success2] = jolt_body_ids.emplace(body->id, body_id.GetIndexAndSequenceNumber());
    OTHER_ASSERT(success2 && bitr != jolt_body_ids.end(), "Failed to store Jolt body info during physics body registration.");
  }

  void jolt_api::unregister_physics_body(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt physics body unregistration.");

    auto itr = jolt_scenes.find(world_id);
    OTHER_ASSERT(itr != jolt_scenes.end(), "Jolt physics scene with id '{}' does not exist during physics body unregistration.", world_id);

    JPH::BodyInterface& body_interface = itr->second->GetBodyInterface();
    body_interface.RemoveBody(JPH::BodyID(static_cast<uint32_t>(body->scene_id)));

    auto bitr = jolt_body_ids.find(body->id);
    if (bitr != jolt_body_ids.end()) {
      jolt_body_ids.erase(bitr);
    } else {
      CORE_LOG_ERROR("Jolt body info for physics body id '{}' does not exist during physics body unregistration.", body->id);
    }

    auto sitr = jolt_shapes.find(body->id);
    if (sitr != jolt_shapes.end()) {
      static_cast<shape_container*>(sitr->second)->jolt_shape = nullptr;
      delete static_cast<shape_container*>(sitr->second);
      jolt_shapes.erase(sitr);
    } else {
      CORE_LOG_ERROR("Jolt shape with id '{}' does not exist during physics body unregistration.", body->id);
    }
  }

  void jolt_api::attach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) {
    /// no-op for now
  }

  void jolt_api::detach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) {
    /// no-op for now
  }

  void jolt_api::configure_empty_shape(physics_shape* shape) {
    OTHER_ASSERT(shape != nullptr, "Physics shape is null during Jolt empty shape configuration.");

    JPH::EmptyShapeSettings empty_settings;
    empty_settings.SetEmbedded();

    JPH::ShapeSettings::ShapeResult shape_result = empty_settings.Create();
    OTHER_ASSERT(shape_result.IsValid(), "Failed to create Jolt empty shape during empty shape configuration.");

    auto itr = jolt_shapes.find(shape->body_id);
    OTHER_ASSERT(itr != jolt_shapes.end(), "Jolt shape with id '{}' does not exist during empty shape configuration.", shape->body_id);

    shape_container* container = static_cast<shape_container*>(itr->second);
    container->jolt_shape = shape_result.Get();
  }

  void jolt_api::configure_box_shape(physics_shape* shape, const glm::vec3& half_extents) {
    OTHER_ASSERT(shape != nullptr, "Physics shape is null during Jolt box shape configuration.");

    JPH::BoxShapeSettings box_settings(JPH::Vec3(half_extents.x, half_extents.y, half_extents.z));
    box_settings.SetEmbedded();

    JPH::ShapeSettings::ShapeResult shape_result = box_settings.Create();
    OTHER_ASSERT(shape_result.IsValid(), "Failed to create Jolt box shape during box shape configuration.");

    auto itr = jolt_shapes.find(shape->id);
    OTHER_ASSERT(itr != jolt_shapes.end(), "Jolt shape with id '{}' does not exist during box shape configuration.", shape->id);

    shape_container* container = static_cast<shape_container*>(itr->second);
    container->jolt_shape = shape_result.Get();
  }

  void jolt_api::step_simulation(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt step simulation.");

    auto itr = jolt_scenes.find(world_id);
    OTHER_ASSERT(itr != jolt_scenes.end(), "Jolt physics scene with id '{}' does not exist during step simulation.", world_id);

    const JPH::uint cCollisionSteps = 1;
    itr->second->Update(delta_time, cCollisionSteps, temp_allocator, job_system);
  }

  void jolt_api::update_active_transforms(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt update active transforms.");

    auto itr = jolt_scenes.find(world_id);
    OTHER_ASSERT(itr != jolt_scenes.end(), "Jolt physics scene with id '{}' does not exist during update active transforms.", world_id);

    JPH::BodyInterface& body_interface = itr->second->GetBodyInterface();
    for (auto [id, body_id] : jolt_body_ids) {
      auto* b = world->find_if([&](physics_body* p) { return p != nullptr && p->id == id && p->scene_id == body_id; });
      OTHER_ASSERT(b != nullptr, "Failed to find physics body during transform update.");

      JPH::RMat44 t = body_interface.GetWorldTransform(JPH::BodyID(body_id));
      JPH::RVec3 position = t.GetTranslation();
      JPH::Quat rotation = t.GetQuaternion();

      glm::vec3 jolt_position(static_cast<float>(position.GetX()), static_cast<float>(position.GetY()), static_cast<float>(position.GetZ()));
      glm::quat jolt_rotation(static_cast<float>(rotation.GetX()), static_cast<float>(rotation.GetY()), static_cast<float>(rotation.GetZ()), static_cast<float>(rotation.GetW()));
      glm::vec3 scale(1.0f, 1.0f, 1.0f);

      glm::mat4 transform = compose_mat4(jolt_position, jolt_rotation, scale);

      b->previous_transform = b->current_transform;
      b->current_transform = transform;
    }
  }

  void jolt_api::on_initialize(const config_table& configuration) {
    JPH::RegisterDefaultAllocator();

    JPH::Trace = trace_impl;
    JPH::Factory::sInstance = new JPH::Factory();

    JPH::RegisterTypes();

    temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
    job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    // Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
    bp_layer_interface = new BPLayerInterfaceImpl();

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    // Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler interface.
    object_vs_broadphase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    // Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
    object_layer_pair_filter = new ObjectLayerPairFilterImpl();
  }

  void jolt_api::on_shutdown() {
    delete object_layer_pair_filter;
    delete object_vs_broadphase_layer_filter;
    delete bp_layer_interface;

    object_layer_pair_filter = nullptr;
    object_vs_broadphase_layer_filter = nullptr;
    bp_layer_interface = nullptr;

    delete job_system;
    delete temp_allocator;

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
  }

  bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const {
    switch (inObject1) {
      case Layers::NON_MOVING: return inObject2 == Layers::MOVING;  // Non moving only collides with moving
      case Layers::MOVING: return true;                             // Moving collides with everything
      default:
        JPH_ASSERT(false);
        return false;
    }
  }

  BPLayerInterfaceImpl::BPLayerInterfaceImpl() {
    // Create a mapping table from object to broad phase layer
    mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
    mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
  }

  JPH::uint BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const {
    return BroadPhaseLayers::NUM_LAYERS;
  }

  JPH::BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const {
    JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
    return mObjectToBroadPhase[inLayer];
  }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const {
    switch ((JPH::BroadPhaseLayer::Type)inLayer) {
      case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING: return "NON_MOVING";
      case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING: return "MOVING";
      default: JPH_ASSERT(false); return "INVALID";
    }
  }
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

  bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const {
    switch (inLayer1) {
      case Layers::NON_MOVING:
        return inLayer2 == BroadPhaseLayers::MOVING;
      case Layers::MOVING:
        return true;
      default:
        JPH_ASSERT(false);
        return false;
    }
  }

  JPH::ValidateResult MyContactListener::OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) {
    // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
  }

  void MyContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
  }

  void MyContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) {
  }

  void MyContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) {
  }

}  // namespace other