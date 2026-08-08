/**
 * \file backends/jolt_api.cpp
 **/
#include "physics/backends/jolt_api.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

/// JPH_* ABI defines come from other-physics/CMakeLists.txt; must match the prebuilt extern/jolt binaries
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
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
// clang-format on

#include "core/profiler.hpp"
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

  /// queues contact transitions from jolt's worker threads for the main-thread drain. kBegin/
  ///  kTriggerBegin carry engine body ids; kEnd only has backend BodyIDs, resolved by the drain
  class contact_listener final : public JPH::ContactListener {
   public:
    void OnContactAdded(const JPH::Body& body_a, const JPH::Body& body_b, const JPH::ContactManifold& manifold, JPH::ContactSettings&) override {
      contact_event ev;
      ev.type = (body_a.IsSensor() || body_b.IsSensor()) ? contact_event::kTriggerBegin : contact_event::kBegin;
      ev.body_a = static_cast<integer_t>(body_a.GetUserData());
      ev.body_b = static_cast<integer_t>(body_b.GetUserData());
      JPH::RVec3 p = manifold.GetWorldSpaceContactPointOn1(0);
      ev.point = { static_cast<float>(p.GetX()), static_cast<float>(p.GetY()), static_cast<float>(p.GetZ()) };
      ev.normal = { manifold.mWorldSpaceNormal.GetX(), manifold.mWorldSpaceNormal.GetY(), manifold.mWorldSpaceNormal.GetZ() };
      std::lock_guard lock(mutex);
      events.push_back(ev);
    }

    void OnContactRemoved(const JPH::SubShapeIDPair& pair) override {
      contact_event ev;
      ev.type = contact_event::kEnd;
      ev.body_a = static_cast<integer_t>(pair.GetBody1ID().GetIndexAndSequenceNumber());
      ev.body_b = static_cast<integer_t>(pair.GetBody2ID().GetIndexAndSequenceNumber());
      std::lock_guard lock(mutex);
      events.push_back(ev);
    }

    void drain_into(ostd::vector<contact_event>& out) {
      std::lock_guard lock(mutex);
      out.insert(out.end(), events.begin(), events.end());
      events.clear();
    }

   private:
    std::mutex mutex;
    std::vector<contact_event> events;  /// std:: — appended from jolt worker threads, the arena-backed
  };                                    ///   ostd containers are main-thread machinery

  /// per-world backend state; owns the jolt system, its listener, and this world's body/shape/joint maps
  struct jolt_world {
    JPH::PhysicsSystem* system = nullptr;
    contact_listener* listener = nullptr;
    ostd::map<integer_t, uint32_t> bodies;         ///< physics_body::id -> jolt BodyID bits
    ostd::map<integer_t, shape_container> shapes;  ///< physics_body::id -> that body's jolt shape
    ostd::map<integer_t, JPH::Ref<JPH::TwoBodyConstraint>> joints;
    integer_t next_joint_id = 0;
  };

  namespace {

    constexpr float kMinShapeExtent = 0.001f;

    float max_axis(const glm::vec3& v) {
      return std::max({ std::abs(v.x), std::abs(v.y), std::abs(v.z) });
    }

  }  // namespace

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

    ostd::vector<physics_api::line> debug_lines;
    ostd::vector<glm::vec4> debug_line_colors;

    ostd::vector<physics_api::triangle> debug_triangles;
    ostd::vector<glm::vec4> debug_triangle_colors;
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
    PROFILE_SECTION("jolt_api::get_debug_render_data");

    const jolt_world& jw = world_state(id);

    MyDebugRenderer debug_renderer;
    JPH::BodyManager::DrawSettings body_draw_settings;
    body_draw_settings.mDrawShape = true;
    body_draw_settings.mDrawShapeWireframe = true;  /// overlay stays readable over scene geometry
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
    PROFILE_SECTION("jolt_api::initialize_world");
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
    jw->listener = new contact_listener();
    physics_system->SetContactListener(jw->listener);

    jolt_worlds.emplace(id, jw);
  }

  void jolt_api::shutdown_world(physics_world* world) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt shutdown.");
    PROFILE_SECTION("jolt_api::shutdown_world");

    auto itr = jolt_worlds.find(world->world_id);
    if (itr == jolt_worlds.end()) {
      CORE_LOG_ERROR("Jolt physics world with id '{}' does not exist during shutdown.", world->world_id);
      return;
    }

    jolt_world* jw = itr->second;
    for (auto& [joint_id, constraint] : jw->joints) {
      jw->system->RemoveConstraint(constraint);
    }
    jw->joints.clear();
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
    PROFILE_SECTION("jolt_api::on_scene_start");
    world_state(world_id).system->OptimizeBroadPhase();
  }

  void jolt_api::register_physics_body(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt physics body registration.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Jolt physics body registration.");
    PROFILE_SECTION("jolt_api::register_physics_body");

    jolt_world& jw = world_state(world_id);

    shape_container& container = jw.shapes[body->id];

    /// by default attach an empty shape, this will register as no shape in the UI
    JPH::EmptyShapeSettings shape_settings;
    shape_settings.SetEmbedded();

    JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
    container.jolt_shape = shape_result.Get();
    container.default_empty = true;

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

    const physics_body::settings& authored = body->applied_settings;
    JPH::BodyCreationSettings body_settings(container.jolt_shape,
                                            JPH::RVec3(position.x, position.y, position.z),
                                            JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                            type,
                                            body->body_type == physics_body::STATIC ? Layers::NON_MOVING : Layers::MOVING);
    body_settings.mUserData = static_cast<JPH::uint64>(body->id);
    body_settings.mFriction = authored.friction;
    body_settings.mRestitution = authored.restitution;
    body_settings.mLinearDamping = authored.linear_damping;
    body_settings.mAngularDamping = authored.angular_damping;
    body_settings.mGravityFactor = authored.gravity_factor;
    body_settings.mIsSensor = authored.is_trigger;
    body_settings.mMotionQuality = authored.continuous_cd ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
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
    PROFILE_SECTION("jolt_api::unregister_physics_body");

    jolt_world& jw = world_state(world_id);

    /// welds referencing a dying body must go first or jolt is left holding a dangling constraint
    for (auto itr = jw.joints.begin(); itr != jw.joints.end();) {
      JPH::TwoBodyConstraint* constraint = itr->second;
      const uint32_t bits = static_cast<uint32_t>(body->backend_id);
      if (constraint->GetBody1()->GetID().GetIndexAndSequenceNumber() == bits ||
          constraint->GetBody2()->GetID().GetIndexAndSequenceNumber() == bits) {
        jw.system->RemoveConstraint(constraint);
        itr = jw.joints.erase(itr);
      } else {
        ++itr;
      }
    }

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

    jw.shapes.erase(body->id);
  }

  void jolt_api::teleport_body(natural_t world_id, physics_world* world, physics_body* body, const glm::mat4& world_transform) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt body teleport.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Jolt body teleport.");
    PROFILE_SECTION("jolt_api::teleport_body");

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

  bool jolt_api::set_body_shape(natural_t world_id, physics_world* world, physics_body* body,
                                const physics_shape_desc& desc, const glm::vec3& world_scale,
                                const shape_geometry* geometry) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt shape build.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Jolt shape build.");
    PROFILE_SECTION("jolt_api::set_body_shape");

    jolt_world& jw = world_state(world_id);

    JPH::ShapeSettings::ShapeResult result;
    switch (desc.shape_kind) {
      case PHYSICS_SHAPE_NONE: {
        JPH::EmptyShapeSettings s;
        s.SetEmbedded();
        result = s.Create();
      } break;
      case PHYSICS_SHAPE_BOX: {
        glm::vec3 he = glm::max(desc.half_extents * glm::abs(world_scale), glm::vec3(kMinShapeExtent));
        JPH::BoxShapeSettings s(JPH::Vec3(he.x, he.y, he.z));
        s.SetEmbedded();
        result = s.Create();
      } break;
      case PHYSICS_SHAPE_SPHERE: {
        /// non-uniform scale cannot apply to a sphere; the largest axis wins
        JPH::SphereShapeSettings s(std::max(desc.radius * max_axis(world_scale), kMinShapeExtent));
        s.SetEmbedded();
        result = s.Create();
      } break;
      case PHYSICS_SHAPE_CAPSULE: {
        float r = std::max(desc.radius * std::max(std::abs(world_scale.x), std::abs(world_scale.z)), kMinShapeExtent);
        float hh = std::max(desc.half_height * std::abs(world_scale.y), kMinShapeExtent);
        JPH::CapsuleShapeSettings s(hh, r);
        s.SetEmbedded();
        result = s.Create();
      } break;
      case PHYSICS_SHAPE_CONVEX_HULL: {
        PROFILE_SECTION("jolt_api::set_body_shape--convex_hull");
        if (geometry == nullptr || geometry->positions.empty()) {
          CORE_LOG_ERROR("Convex hull for body {} requires geometry.", body->id);
          return false;
        }
        JPH::Array<JPH::Vec3> points;
        points.reserve(geometry->positions.size());
        for (const glm::vec3& p : geometry->positions) {
          glm::vec3 sp = p * world_scale;
          points.push_back(JPH::Vec3(sp.x, sp.y, sp.z));
        }
        JPH::ConvexHullShapeSettings s(points);
        s.SetEmbedded();
        result = s.Create();
      } break;
      case PHYSICS_SHAPE_TRIANGLE_MESH: {
        PROFILE_SECTION("jolt_api::set_body_shape--triangle_mesh");
        if (geometry == nullptr || geometry->positions.empty() || geometry->indices.size() < 3) {
          CORE_LOG_ERROR("Triangle mesh for body {} requires indexed geometry.", body->id);
          return false;
        }
        JPH::VertexList vertices;
        vertices.reserve(geometry->positions.size());
        for (const glm::vec3& p : geometry->positions) {
          glm::vec3 sp = p * world_scale;
          vertices.push_back(JPH::Float3(sp.x, sp.y, sp.z));
        }
        JPH::IndexedTriangleList triangles;
        triangles.reserve(geometry->indices.size() / 3);
        for (size_t i = 0; i + 2 < geometry->indices.size(); i += 3) {
          triangles.push_back(JPH::IndexedTriangle(geometry->indices[i], geometry->indices[i + 1], geometry->indices[i + 2], 0));
        }
        JPH::MeshShapeSettings s(vertices, triangles);
        s.SetEmbedded();
        result = s.Create();
      } break;
      default:
        CORE_LOG_ERROR("Unknown physics shape kind {} for body {}.", desc.shape_kind, body->id);
        return false;
    }

    if (!result.IsValid()) {
      /// authored data failed to build (degenerate hull, ...) — data error, never an assert
      CORE_LOG_ERROR("Shape build failed for body {}: {}", body->id, result.GetError().c_str());
      return false;
    }

    shape_container& container = jw.shapes[body->id];
    container.jolt_shape = result.Get();
    container.default_empty = (desc.shape_kind == PHYSICS_SHAPE_NONE);

    JPH::BodyID jolt_id(static_cast<uint32_t>(body->backend_id));
    jw.system->GetBodyInterface().SetShape(jolt_id, container.jolt_shape,
                                           /*inUpdateMassProperties=*/ body->body_type == physics_body::DYNAMIC,
                                           body->body_type == physics_body::STATIC ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);

    if (body->body_type == physics_body::DYNAMIC && body->mass > 0.f) {
      /// SetShape recomputed mass from the shape's density; rescale to the AUTHORED mass
      ///  (keeps the shape-derived inertia tensor's proportions)
      JPH::BodyLockWrite lock(jw.system->GetBodyLockInterface(), jolt_id);
      if (lock.Succeeded()) {
        lock.GetBody().GetMotionProperties()->ScaleToMass(body->mass);
      }
    }
    return true;
  }

  void jolt_api::step_simulation(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt step simulation.");
    PROFILE_SECTION("jolt_api::step_simulation");

    const JPH::uint cCollisionSteps = 1;
    world_state(world_id).system->Update(delta_time, cCollisionSteps, temp_allocator, job_system);
  }

  void jolt_api::update_active_transforms(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Jolt update active transforms.");
    PROFILE_SECTION("jolt_api::update_active_transforms");

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

  void jolt_api::drain_contacts(natural_t world_id, physics_world* world, ostd::vector<contact_event>& out) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during contact drain.");
    PROFILE_SECTION("jolt_api::drain_contacts");
    jolt_world& jw = world_state(world_id);

    scratch_events.clear();
    jw.listener->drain_into(scratch_events);
    for (contact_event& ev : scratch_events) {
      if (ev.type == contact_event::kEnd) {
        /// end events carry backend BodyID bits; resolve them against this world's body map
        auto resolve = [&jw](integer_t backend_bits) -> integer_t {
          for (const auto& [engine_id, bits] : jw.bodies) {
            if (bits == static_cast<uint32_t>(backend_bits)) {
              return engine_id;
            }
          }
          return -1;
        };
        ev.body_a = resolve(ev.body_a);
        ev.body_b = resolve(ev.body_b);
        if (ev.body_a < 0 || ev.body_b < 0) {
          continue;  /// a side died the same step — dropped by contract
        }
      }
      out.push_back(ev);
    }
  }

  namespace {

    /// sensors are triggers, not surfaces — rays pass through them
    class ignore_sensors_filter final : public JPH::BodyFilter {
     public:
      bool ShouldCollideLocked(const JPH::Body& body) const override {
        return !body.IsSensor();
      }
    };

  }  // namespace

  raycast_hit jolt_api::cast_ray(natural_t world_id, physics_world* world, const glm::vec3& origin,
                                 const glm::vec3& direction, float max_distance) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during raycast.");
    PROFILE_SECTION("jolt_api::cast_ray");
    jolt_world& jw = world_state(world_id);

    raycast_hit out;
    glm::vec3 d = glm::normalize(direction) * max_distance;
    JPH::RRayCast ray{ JPH::RVec3(origin.x, origin.y, origin.z), JPH::Vec3(d.x, d.y, d.z) };
    JPH::RayCastResult result;

    static const ignore_sensors_filter kIgnoreSensors;
    if (!jw.system->GetNarrowPhaseQuery().CastRay(ray, result, JPH::BroadPhaseLayerFilter{}, JPH::ObjectLayerFilter{}, kIgnoreSensors)) {
      return out;
    }

    JPH::BodyLockRead lock(jw.system->GetBodyLockInterface(), result.mBodyID);
    if (!lock.Succeeded()) {
      return out;
    }

    out.hit = true;
    out.body_id = static_cast<integer_t>(lock.GetBody().GetUserData());
    out.distance = result.mFraction * max_distance;
    JPH::RVec3 p = ray.GetPointOnRay(result.mFraction);
    out.point = { static_cast<float>(p.GetX()), static_cast<float>(p.GetY()), static_cast<float>(p.GetZ()) };
    JPH::Vec3 n = lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, p);
    out.normal = { n.GetX(), n.GetY(), n.GetZ() };
    return out;
  }

  integer_t jolt_api::create_fixed_joint(natural_t world_id, physics_world* world, physics_body* body_a, physics_body* body_b) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during weld creation.");
    OTHER_ASSERT(body_a != nullptr && body_b != nullptr, "Cannot weld a null physics body.");
    PROFILE_SECTION("jolt_api::create_fixed_joint");
    jolt_world& jw = world_state(world_id);

    JPH::BodyID ids[2] = { JPH::BodyID(static_cast<uint32_t>(body_a->backend_id)),
                           JPH::BodyID(static_cast<uint32_t>(body_b->backend_id)) };
    JPH::BodyLockMultiWrite lock(jw.system->GetBodyLockInterface(), ids, 2);
    JPH::Body* a = lock.GetBody(0);
    JPH::Body* b = lock.GetBody(1);
    if (a == nullptr || b == nullptr) {
      CORE_LOG_ERROR("Cannot weld bodies {} and {}: a jolt body is missing.", body_a->id, body_b->id);
      return -1;
    }

    JPH::FixedConstraintSettings settings;
    settings.SetEmbedded();
    settings.mAutoDetectPoint = true;
    JPH::TwoBodyConstraint* constraint = settings.Create(*a, *b);
    jw.system->AddConstraint(constraint);

    integer_t joint_id = jw.next_joint_id++;
    jw.joints.emplace(joint_id, constraint);
    return joint_id;
  }

  void jolt_api::destroy_joint(natural_t world_id, physics_world* world, integer_t joint_id) {
    PROFILE_SECTION("jolt_api::destroy_joint");
    jolt_world& jw = world_state(world_id);
    auto itr = jw.joints.find(joint_id);
    if (itr == jw.joints.end()) {
      CORE_LOG_ERROR("Joint {} does not exist in jolt world {}.", joint_id, world_id);
      return;
    }
    jw.system->RemoveConstraint(itr->second);
    jw.joints.erase(itr);
  }

  float jolt_api::joint_reaction_force(natural_t world_id, physics_world* world, integer_t joint_id, double step) {
    OTHER_ASSERT(step > 0.0, "Joint reaction force requires a positive step.");
    jolt_world& jw = world_state(world_id);
    auto itr = jw.joints.find(joint_id);
    if (itr == jw.joints.end()) {
      return 0.f;
    }
    JPH::FixedConstraint* fixed = static_cast<JPH::FixedConstraint*>(itr->second.GetPtr());
    /// position impulse accumulated over the step / dt = force sustained through the weld
    return fixed->GetTotalLambdaPosition().Length() / static_cast<float>(step);
  }

  void jolt_api::set_linear_velocity(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& velocity) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during velocity write.");
    if (body->body_type == physics_body::STATIC) {
      CORE_LOG_WARN("Ignoring velocity write on static physics body {}.", body->id);
      return;
    }
    world_state(world_id).system->GetBodyInterface().SetLinearVelocity(JPH::BodyID(static_cast<uint32_t>(body->backend_id)),
                                                                      JPH::Vec3(velocity.x, velocity.y, velocity.z));
  }

  glm::vec3 jolt_api::get_linear_velocity(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during velocity read.");
    if (body->body_type == physics_body::STATIC) {
      return glm::vec3(0.f);
    }
    JPH::Vec3 v = world_state(world_id).system->GetBodyInterface().GetLinearVelocity(JPH::BodyID(static_cast<uint32_t>(body->backend_id)));
    return { v.GetX(), v.GetY(), v.GetZ() };
  }

  void jolt_api::set_angular_velocity(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& velocity) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during velocity write.");
    if (body->body_type == physics_body::STATIC) {
      CORE_LOG_WARN("Ignoring velocity write on static physics body {}.", body->id);
      return;
    }
    world_state(world_id).system->GetBodyInterface().SetAngularVelocity(JPH::BodyID(static_cast<uint32_t>(body->backend_id)),
                                                                       JPH::Vec3(velocity.x, velocity.y, velocity.z));
  }

  glm::vec3 jolt_api::get_angular_velocity(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during velocity read.");
    if (body->body_type == physics_body::STATIC) {
      return glm::vec3(0.f);
    }
    JPH::Vec3 v = world_state(world_id).system->GetBodyInterface().GetAngularVelocity(JPH::BodyID(static_cast<uint32_t>(body->backend_id)));
    return { v.GetX(), v.GetY(), v.GetZ() };
  }

  void jolt_api::add_force(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& force) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during force application.");
    if (body->body_type != physics_body::DYNAMIC) {
      CORE_LOG_WARN("Ignoring force on non-dynamic physics body {}.", body->id);
      return;
    }
    world_state(world_id).system->GetBodyInterface().AddForce(JPH::BodyID(static_cast<uint32_t>(body->backend_id)),
                                                              JPH::Vec3(force.x, force.y, force.z));
  }

  void jolt_api::add_impulse(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& impulse) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during impulse application.");
    if (body->body_type != physics_body::DYNAMIC) {
      CORE_LOG_WARN("Ignoring impulse on non-dynamic physics body {}.", body->id);
      return;
    }
    world_state(world_id).system->GetBodyInterface().AddImpulse(JPH::BodyID(static_cast<uint32_t>(body->backend_id)),
                                                                JPH::Vec3(impulse.x, impulse.y, impulse.z));
  }

  void jolt_api::add_torque(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& torque) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during torque application.");
    if (body->body_type != physics_body::DYNAMIC) {
      CORE_LOG_WARN("Ignoring torque on non-dynamic physics body {}.", body->id);
      return;
    }
    world_state(world_id).system->GetBodyInterface().AddTorque(JPH::BodyID(static_cast<uint32_t>(body->backend_id)),
                                                               JPH::Vec3(torque.x, torque.y, torque.z));
  }

  void jolt_api::on_initialize(const config_table& configuration) {
    PROFILE_SECTION("jolt_api::on_initialize");
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

    // maps object layers to broadphase layers; PhysicsSystem keeps a reference, so this
    //  instance must stay alive (see BroadPhaseLayerInterfaceTable/Mask for alternatives)
    bp_layer_interface = new BPLayerInterfaceImpl();

    // filters object vs broadphase layers; PhysicsSystem keeps a reference, so this instance
    //  must stay alive (see ObjectVsBroadPhaseLayerFilterTable/Mask for alternatives)
    object_vs_broadphase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();

    // filters object vs object layers; PhysicsSystem keeps a reference, so this instance
    //  must stay alive (see ObjectLayerPairFilterTable/Mask for alternatives)
    object_layer_pair_filter = new ObjectLayerPairFilterImpl();
  }

  void jolt_api::on_shutdown() {
    PROFILE_SECTION("jolt_api::on_shutdown");
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

}  // namespace other
