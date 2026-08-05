/**
 * \file backends/jolt_api.cpp
 **/
#include "physics/backends/jolt_api.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

/// JPH_* ABI defines come from other-physics/CMakeLists.txt (target_compile_definitions)
///  and must match how the prebuilt extern/jolt binaries were built
// clang-format off
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

#include "data-structures/std_container.hpp"
#include "math/matrix.hpp"

#include "physics_world/physics_world.hpp"

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

  /// per-world backend state; owns the jolt system, its listener, and this world's body map
  struct jolt_world {
    JPH::PhysicsSystem* system = nullptr;
    MyContactListener* listener = nullptr;
    ostd::map<integer_t, uint32_t> bodies;  ///< physics_body::id -> jolt BodyID bits
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

  jolt_world& jolt_api::world_state(natural_t world_id) {
    auto itr = jolt_worlds.find(world_id);
    OTHER_ASSERT(itr != jolt_worlds.end(), "Jolt physics world with id '{}' does not exist.", world_id);
    return *itr->second;
  }

  const jolt_world& jolt_api::world_state(natural_t world_id) const {
    auto itr = jolt_worlds.find(world_id);
    OTHER_ASSERT(itr != jolt_worlds.end(), "Jolt physics world with id '{}' does not exist.", world_id);
    return *itr->second;
  }

  physics_api::physics_render_debug_data jolt_api::get_debug_render_data(natural_t id, const physics_world* world) const {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt debug render data retrieval.");

    const jolt_world& jw = world_state(id);

    MyDebugRenderer debug_renderer;
    JPH::BodyManager::DrawSettings body_draw_settings;
    body_draw_settings.mDrawShape = true;
    jw.system->DrawBodies(body_draw_settings, &debug_renderer);
    jw.system->DrawConstraints(&debug_renderer);

    physics_api::physics_render_debug_data debug_data;
    debug_data.debug_lines = ostd::vector<line>(debug_renderer.debug_lines.begin(), debug_renderer.debug_lines.end());
    debug_data.debug_line_colors = ostd::vector<glm::vec4>(debug_renderer.debug_line_colors.begin(), debug_renderer.debug_line_colors.end());
    debug_data.debug_triangles = ostd::vector<triangle>(debug_renderer.debug_triangles.begin(), debug_renderer.debug_triangles.end());
    debug_data.debug_triangle_colors = ostd::vector<glm::vec4>(debug_renderer.debug_triangle_colors.begin(), debug_renderer.debug_triangle_colors.end());

    return debug_data;
  }

  void jolt_api::initialize_world(natural_t id, physics_world* world, const physics_world_config& config) {
    if (jolt_worlds.find(id) != jolt_worlds.end()) {
      CORE_LOG_WARN("Jolt physics world with id '{}' already exists.", id);
      return;
    }

    JPH::PhysicsSystem* physics_system = new JPH::PhysicsSystem();
    physics_system->Init(config.max_bodies, /*num body mutexes (0 = default)*/ 0, config.max_body_pairs, config.max_contact_constraints,
                         *bp_layer_interface, *object_vs_broadphase_layer_filter, *object_layer_pair_filter);
    physics_system->SetGravity(JPH::Vec3(config.gravity.x, config.gravity.y, config.gravity.z));

    jolt_world* jw = new jolt_world();
    jw->system = physics_system;
    jw->listener = new MyContactListener();
    physics_system->SetContactListener(jw->listener);

    jolt_worlds.emplace(id, jw);
  }

  void jolt_api::shutdown_world(physics_world* world) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt shutdown.");

    auto itr = jolt_worlds.find(world->world_id);
    if (itr == jolt_worlds.end()) {
      CORE_LOG_ERROR("Jolt physics world with id '{}' does not exist during shutdown.", world->world_id);
      return;
    }

    jolt_world* jw = itr->second;
    if (!jw->bodies.empty()) {
      /// scene teardown drains bodies through the entt destroy hooks; anything left here leaked
      CORE_LOG_WARN("Jolt world '{}' shutting down with {} live bodies, force-destroying them.", world->world_id, jw->bodies.size());
      JPH::BodyInterface& body_interface = jw->system->GetBodyInterface();
      for (auto& [body_id, jolt_id] : jw->bodies) {
        body_interface.RemoveBody(JPH::BodyID(jolt_id));
        body_interface.DestroyBody(JPH::BodyID(jolt_id));
      }
      jw->bodies.clear();
    }

    delete jw->listener;
    delete jw->system;
    delete jw;
    jolt_worlds.erase(itr);
  }

  void jolt_api::on_scene_start(natural_t world_id, physics_world* world) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt scene start.");
    world_state(world_id).system->OptimizeBroadPhase();
  }

  void jolt_api::register_physics_body(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt physics body registration.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Jolt physics body registration.");

    jolt_world& jw = world_state(world_id);

    auto [sitr, success] = jolt_shapes.emplace(body->id, new shape_container());
    OTHER_ASSERT(success && sitr != jolt_shapes.end(), "Failed to create Jolt shape during physics body registration.");
    shape_container* container = static_cast<shape_container*>(sitr->second);

    /// by default attach an empty shape, this will register as no shape in the UI
    JPH::EmptyShapeSettings shape_settings;
    shape_settings.SetEmbedded();

    JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
    container->jolt_shape = shape_result.Get();
    container->default_empty = true;

    JPH::BodyInterface& body_interface = jw.system->GetBodyInterface();

    glm::vec3 position = body->get_current_position();
    glm::quat rotation = body->get_current_rotation();

    JPH::EMotionType type;
    switch (body->body_type) {
      case physics_body::STATIC: type = JPH::EMotionType::Static; break;
      case physics_body::KINEMATIC: type = JPH::EMotionType::Kinematic; break;
      case physics_body::DYNAMIC: type = JPH::EMotionType::Dynamic; break;
      default: type = JPH::EMotionType::Static; break;
    }

    JPH::BodyCreationSettings body_settings(container->jolt_shape,
                                            JPH::RVec3(position.x, position.y, position.z),
                                            JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                            type,
                                            body->body_type == physics_body::STATIC ? Layers::NON_MOVING : Layers::MOVING);
    body_settings.mUserData = static_cast<JPH::uint64>(body->id);
    if (body->body_type == physics_body::DYNAMIC) {
      body_settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
      body_settings.mMassPropertiesOverride.mMass = body->mass;
    }

    JPH::Body* jolt_body = body_interface.CreateBody(body_settings);
    OTHER_ASSERT(jolt_body != nullptr, "Failed to create Jolt body for physics body registration.");

    JPH::BodyID body_id = jolt_body->GetID();
    body_interface.AddBody(body_id, body->body_type == physics_body::STATIC ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

    body->backend_id = body_id.GetIndexAndSequenceNumber();
    auto [bitr, success2] = jw.bodies.emplace(body->id, body_id.GetIndexAndSequenceNumber());
    OTHER_ASSERT(success2 && bitr != jw.bodies.end(), "Failed to store Jolt body info during physics body registration.");
  }

  void jolt_api::unregister_physics_body(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt physics body unregistration.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Jolt physics body unregistration.");

    jolt_world& jw = world_state(world_id);

    JPH::BodyInterface& body_interface = jw.system->GetBodyInterface();
    JPH::BodyID jolt_id(static_cast<uint32_t>(body->backend_id));
    body_interface.RemoveBody(jolt_id);
    body_interface.DestroyBody(jolt_id);

    auto bitr = jw.bodies.find(body->id);
    if (bitr != jw.bodies.end()) {
      jw.bodies.erase(bitr);
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

  void jolt_api::teleport_body(natural_t world_id, physics_world* world, physics_body* body, const glm::mat4& world_transform) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt body teleport.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Jolt body teleport.");

    jolt_world& jw = world_state(world_id);

    glm::vec3 position, scale;
    glm::quat rotation;
    decompose_mat4(world_transform, position, rotation, scale);

    JPH::BodyInterface& body_interface = jw.system->GetBodyInterface();
    JPH::BodyID jolt_id(static_cast<uint32_t>(body->backend_id));
    body_interface.SetPositionAndRotation(jolt_id,
                                          JPH::RVec3(position.x, position.y, position.z),
                                          JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                          JPH::EActivation::DontActivate);
    if (body->body_type != physics_body::STATIC) {
      body_interface.SetLinearAndAngularVelocity(jolt_id, JPH::Vec3::sZero(), JPH::Vec3::sZero());
    }

    /// keep the presentation buffer coherent so nothing blends from the stale pose
    body->previous_transform = world_transform;
    body->current_transform = world_transform;
    body->interpolated_transform = world_transform;
  }

  void jolt_api::move_kinematic(natural_t world_id, physics_world* world, physics_body* body, const glm::mat4& world_transform, double step) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt kinematic move.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Jolt kinematic move.");

    jolt_world& jw = world_state(world_id);

    glm::vec3 position, scale;
    glm::quat rotation;
    decompose_mat4(world_transform, position, rotation, scale);

    jw.system->GetBodyInterface().MoveKinematic(JPH::BodyID(static_cast<uint32_t>(body->backend_id)),
                                                JPH::RVec3(position.x, position.y, position.z),
                                                JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                                static_cast<float>(step));
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

    /// the map is keyed by the owning BODY's id (see register_physics_body)
    auto itr = jolt_shapes.find(shape->body_id);
    OTHER_ASSERT(itr != jolt_shapes.end(), "Jolt shape for body id '{}' does not exist during box shape configuration.", shape->body_id);

    shape_container* container = static_cast<shape_container*>(itr->second);
    container->jolt_shape = shape_result.Get();
  }

  void jolt_api::step_simulation(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt step simulation.");

    const JPH::uint cCollisionSteps = 1;
    world_state(world_id).system->Update(delta_time, cCollisionSteps, temp_allocator, job_system);
  }

  void jolt_api::update_active_transforms(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt update active transforms.");

    jolt_world& jw = world_state(world_id);

    JPH::BodyInterface& body_interface = jw.system->GetBodyInterface();
    for (auto [id, body_id] : jw.bodies) {
      auto* b = world->find_if([&](physics_body* p) { return p != nullptr && p->id == id; });
      OTHER_ASSERT(b != nullptr, "Physics body {} in jolt world {} has no engine-side body.", id, world_id);

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

    /// dedicated jolt worker pool; capped by default because the engine's asio job pool
    ///  already claims most cores (physics.worker-threads overrides, 0 = default)
    uint32_t worker_threads = configuration.get_value<uint32_t>("physics.worker-threads", 0);
    if (worker_threads == 0) {
      worker_threads = std::min(4u, std::max(1u, std::thread::hardware_concurrency() - 1));
    }
    job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, static_cast<int>(worker_threads));

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
    if (!jolt_worlds.empty()) {
      CORE_LOG_WARN("Jolt backend shutting down with {} live worlds.", jolt_worlds.size());
      while (!jolt_worlds.empty()) {
        jolt_world* jw = jolt_worlds.begin()->second;
        delete jw->listener;
        delete jw->system;
        delete jw;
        jolt_worlds.erase(jolt_worlds.begin());
      }
    }

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
