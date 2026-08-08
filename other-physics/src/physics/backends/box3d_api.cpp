/**
 * \file backends/box3d_api.cpp
 **/
#include "physics/backends/box3d_api.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <box3d/box3d.h>

#include "core/profiler.hpp"
#include "data-structures/std_container.hpp"
#include "math/matrix.hpp"

#include "physics_world/physics_world.hpp"

namespace other {

  namespace {

    b3Vec3 to_b3(const glm::vec3& v) {
      return b3Vec3{ v.x, v.y, v.z };
    }

    glm::vec3 from_b3(const b3Vec3& v) {
      return { v.x, v.y, v.z };
    }

    b3Quat to_b3(const glm::quat& q) {
      return b3Quat{ .v = { q.x, q.y, q.z }, .s = q.w };
    }

    glm::quat from_b3(const b3Quat& q) {
      return glm::quat(q.s, q.v.x, q.v.y, q.v.z);
    }

    b3BodyId body_id_of(const physics_body* body) {
      return b3LoadBodyId(body->backend_id);
    }

    /// engine body id stored on every box3d body/shape as user data
    integer_t engine_id_of(b3BodyId body_id) {
      return static_cast<integer_t>(reinterpret_cast<intptr_t>(b3Body_GetUserData(body_id)));
    }

    constexpr float kMinShapeExtent = 0.001f;
    constexpr int kSubStepCount = 4;
    constexpr int kMaxHullVertices = 64;

  }  // namespace

  /// per-world backend state; box3d polls events after stepping, so `pending` is filled on
  ///   the main thread inside step_simulation and handed out by drain_contacts
  struct box3d_world {
    b3WorldId world = {};
    ostd::map<integer_t, uint64_t> bodies;      ///< physics_body::id -> b3StoreBodyId bits
    ostd::map<integer_t, b3MeshData*> meshes;   ///< owned mesh data (mesh shapes REFERENCE it)
    ostd::map<integer_t, uint64_t> joints;      ///< engine joint id -> b3StoreJointId bits
    integer_t next_joint_id = 0;
    ostd::vector<contact_event> pending;
  };

  box3d_world& box3d_api::world_state(natural_t world_id) {
    auto itr = box3d_worlds.find(world_id);
    OTHER_ASSERT(itr != box3d_worlds.end(), "Box3D physics world with id '{}' does not exist.", world_id);
    return *itr->second;
  }

  const box3d_world& box3d_api::world_state(natural_t world_id) const {
    auto itr = box3d_worlds.find(world_id);
    OTHER_ASSERT(itr != box3d_worlds.end(), "Box3D physics world with id '{}' does not exist.", world_id);
    return *itr->second;
  }

  namespace {

    /// segment/primitive collectors for b3World_Draw; box3d tessellates hull/mesh shapes through
    ///   an opaque user-shape callback, so those fall back to their bounds via drawBounds
    struct debug_collector {
      physics_api::physics_render_debug_data* data = nullptr;

      static glm::vec4 to_color(b3HexColor color) {
        uint32_t c = static_cast<uint32_t>(color);
        return { static_cast<float>((c >> 16) & 0xFF) / 255.f,
                 static_cast<float>((c >> 8) & 0xFF) / 255.f,
                 static_cast<float>(c & 0xFF) / 255.f, 1.f };
      }

      void line(const glm::vec3& a, const glm::vec3& b, const glm::vec4& color) {
        data->debug_lines.push_back({ a, b });
        data->debug_line_colors.push_back(color);
      }

      void aabb_edges(const glm::vec3& mn, const glm::vec3& mx, const glm::vec4& color) {
        const glm::vec3 c[8] = {
          { mn.x, mn.y, mn.z }, { mx.x, mn.y, mn.z }, { mx.x, mx.y, mn.z }, { mn.x, mx.y, mn.z },
          { mn.x, mn.y, mx.z }, { mx.x, mn.y, mx.z }, { mx.x, mx.y, mx.z }, { mn.x, mx.y, mx.z },
        };
        constexpr int e[12][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
                                   { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
        for (const auto& edge : e) {
          line(c[edge[0]], c[edge[1]], color);
        }
      }

      void circle(const glm::vec3& center, const glm::vec3& axis_u, const glm::vec3& axis_v, float radius, const glm::vec4& color) {
        constexpr int kSegments = 16;
        glm::vec3 prev = center + radius * axis_u;
        for (int i = 1; i <= kSegments; ++i) {
          float angle = (2.f * 3.14159265f * i) / kSegments;
          glm::vec3 next = center + radius * (std::cos(angle) * axis_u + std::sin(angle) * axis_v);
          line(prev, next, color);
          prev = next;
        }
      }

      static void draw_segment(b3Pos p1, b3Pos p2, b3HexColor color, void* context) {
        auto* self = static_cast<debug_collector*>(context);
        self->line(from_b3(p1), from_b3(p2), to_color(color));
      }

      static void draw_bounds(b3AABB aabb, b3HexColor color, void* context) {
        auto* self = static_cast<debug_collector*>(context);
        self->aabb_edges(from_b3(aabb.lowerBound), from_b3(aabb.upperBound), to_color(color));
      }

      static void draw_box(b3Vec3 extents, b3WorldTransform transform, b3HexColor color, void* context) {
        auto* self = static_cast<debug_collector*>(context);
        glm::vec3 he = from_b3(extents);
        glm::quat rot = from_b3(transform.q);
        glm::vec3 pos = from_b3(transform.p);
        const glm::vec3 corners[8] = {
          { -he.x, -he.y, -he.z }, { he.x, -he.y, -he.z }, { he.x, he.y, -he.z }, { -he.x, he.y, -he.z },
          { -he.x, -he.y, he.z }, { he.x, -he.y, he.z }, { he.x, he.y, he.z }, { -he.x, he.y, he.z },
        };
        glm::vec3 world[8];
        for (int i = 0; i < 8; ++i) {
          world[i] = pos + rot * corners[i];
        }
        constexpr int e[12][2] = { { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 4, 5 }, { 5, 6 },
                                   { 6, 7 }, { 7, 4 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
        glm::vec4 c = to_color(color);
        for (const auto& edge : e) {
          self->line(world[edge[0]], world[edge[1]], c);
        }
      }

      static void draw_sphere(b3Pos p, float radius, b3HexColor color, float alpha, void* context) {
        auto* self = static_cast<debug_collector*>(context);
        glm::vec3 center = from_b3(p);
        glm::vec4 c = to_color(color);
        self->circle(center, { 1, 0, 0 }, { 0, 1, 0 }, radius, c);
        self->circle(center, { 1, 0, 0 }, { 0, 0, 1 }, radius, c);
        self->circle(center, { 0, 1, 0 }, { 0, 0, 1 }, radius, c);
      }

      static void draw_capsule(b3Pos p1, b3Pos p2, float radius, b3HexColor color, float alpha, void* context) {
        auto* self = static_cast<debug_collector*>(context);
        glm::vec3 a = from_b3(p1);
        glm::vec3 b = from_b3(p2);
        glm::vec4 c = to_color(color);
        draw_sphere(p1, radius, color, alpha, context);
        draw_sphere(p2, radius, color, alpha, context);
        glm::vec3 axis = b - a;
        glm::vec3 side = glm::abs(axis.y) < 0.9f * glm::length(axis) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        glm::vec3 u = glm::normalize(glm::cross(axis, side)) * radius;
        glm::vec3 v = glm::normalize(glm::cross(axis, u)) * radius;
        self->line(a + u, b + u, c);
        self->line(a - u, b - u, c);
        self->line(a + v, b + v, c);
        self->line(a - v, b - v, c);
      }

      static void draw_point(b3Pos p, float size, b3HexColor color, void* context) {}
      static void draw_string(b3Pos p, const char* s, b3HexColor color, void* context) {}
      static void draw_transform(b3WorldTransform transform, void* context) {}
      static void draw_shape(void* userShape, b3WorldTransform transform, b3HexColor color, void* context) {}
    };

  }  // namespace

  physics_api::physics_render_debug_data box3d_api::get_debug_render_data(natural_t id, const physics_world* world) const {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Box3D debug render data retrieval.");
    PROFILE_SECTION("box3d_api::get_debug_render_data");
    const box3d_world& bw = world_state(id);

    physics_api::physics_render_debug_data data;
    debug_collector collector{ .data = &data };

    b3DebugDraw draw = {};
    draw.DrawSegmentFcn = debug_collector::draw_segment;
    draw.DrawBoundsFcn = debug_collector::draw_bounds;
    draw.DrawBoxFcn = debug_collector::draw_box;
    draw.DrawSphereFcn = debug_collector::draw_sphere;
    draw.DrawCapsuleFcn = debug_collector::draw_capsule;
    draw.DrawPointFcn = debug_collector::draw_point;
    draw.DrawStringFcn = debug_collector::draw_string;
    draw.DrawTransformFcn = debug_collector::draw_transform;
    draw.DrawShapeFcn = debug_collector::draw_shape;
    draw.drawShapes = true;
    draw.drawJoints = true;
    draw.drawBounds = true;  /// hull/mesh shapes tessellate through an opaque callback; bounds cover them
    draw.context = &collector;

    b3World_Draw(bw.world, &draw, UINT64_MAX);
    return data;
  }

  void box3d_api::initialize_world(natural_t id, physics_world* world, const physics_world_config& config) {
    PROFILE_SECTION("box3d_api::initialize_world");
    if (box3d_worlds.find(id) != box3d_worlds.end()) {
      CORE_LOG_WARN("Box3D physics world with id '{}' already exists.", id);
      return;
    }

    b3WorldDef def = b3DefaultWorldDef();
    def.gravity = to_b3(config.gravity);

    box3d_world* bw = new box3d_world();
    bw->world = b3CreateWorld(&def);
    box3d_worlds.emplace(id, bw);
  }

  void box3d_api::shutdown_world(physics_world* world) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Box3D shutdown.");
    PROFILE_SECTION("box3d_api::shutdown_world");

    auto itr = box3d_worlds.find(world->world_id);
    if (itr == box3d_worlds.end()) {
      CORE_LOG_ERROR("Box3D physics world with id '{}' does not exist during shutdown.", world->world_id);
      return;
    }

    box3d_world* bw = itr->second;
    if (!bw->bodies.empty()) {
      CORE_LOG_WARN("Box3D world '{}' shutting down with {} live bodies.", world->world_id, bw->bodies.size());
    }
    b3DestroyWorld(bw->world);  /// destroys all bodies/shapes/joints it still holds
    for (auto& [body_id, mesh] : bw->meshes) {
      b3DestroyMesh(mesh);
    }
    delete bw;
    box3d_worlds.erase(itr);
  }

  void box3d_api::register_physics_body(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Box3D physics body registration.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Box3D physics body registration.");
    PROFILE_SECTION("box3d_api::register_physics_body");

    box3d_world& bw = world_state(world_id);

    const physics_body::settings& authored = body->applied_settings;

    b3BodyDef def = b3DefaultBodyDef();
    switch (body->body_type) {
      case physics_body::STATIC: def.type = b3_staticBody; break;
      case physics_body::KINEMATIC: def.type = b3_kinematicBody; break;
      case physics_body::DYNAMIC: def.type = b3_dynamicBody; break;
      default: def.type = b3_staticBody; break;
    }
    def.position = to_b3(body->get_current_position());
    def.rotation = to_b3(body->get_current_rotation());
    def.linearDamping = authored.linear_damping;
    def.angularDamping = authored.angular_damping;
    def.gravityScale = authored.gravity_factor;
    def.isBullet = authored.continuous_cd;
    def.userData = reinterpret_cast<void*>(static_cast<intptr_t>(body->id));

    b3BodyId body_id = b3CreateBody(bw.world, &def);
    body->backend_id = b3StoreBodyId(body_id);
    bw.bodies.emplace(body->id, body->backend_id);
    /// bodies start shapeless (a point mass); set_body_shape attaches the collider
  }

  void box3d_api::unregister_physics_body(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Box3D physics body unregistration.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Box3D physics body unregistration.");
    PROFILE_SECTION("box3d_api::unregister_physics_body");

    box3d_world& bw = world_state(world_id);

    b3DestroyBody(body_id_of(body));  /// attached shapes and joints die with it
    bw.bodies.erase(body->id);

    if (auto mitr = bw.meshes.find(body->id); mitr != bw.meshes.end()) {
      b3DestroyMesh(mitr->second);
      bw.meshes.erase(mitr);
    }

    /// joints referencing the body were destroyed by box3d; drop the stale entries
    for (auto itr = bw.joints.begin(); itr != bw.joints.end();) {
      if (!b3Joint_IsValid(b3LoadJointId(itr->second))) {
        itr = bw.joints.erase(itr);
      } else {
        ++itr;
      }
    }
  }

  void box3d_api::teleport_body(natural_t world_id, physics_world* world, physics_body* body, const glm::mat4& world_transform) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Box3D body teleport.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Box3D body teleport.");
    PROFILE_SECTION("box3d_api::teleport_body");

    glm::vec3 position, scale;
    glm::quat rotation;
    decompose_mat4(world_transform, position, rotation, scale);

    b3BodyId body_id = body_id_of(body);
    b3Body_SetTransform(body_id, to_b3(position), to_b3(rotation));
    if (body->body_type != physics_body::STATIC) {
      b3Body_SetLinearVelocity(body_id, b3Vec3{ 0.f, 0.f, 0.f });
      b3Body_SetAngularVelocity(body_id, b3Vec3{ 0.f, 0.f, 0.f });
    }

    body->previous_transform = world_transform;
    body->current_transform = world_transform;
    body->interpolated_transform = world_transform;
  }

  void box3d_api::move_kinematic(natural_t world_id, physics_world* world, physics_body* body, const glm::mat4& world_transform, double step) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Box3D kinematic move.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Box3D kinematic move.");

    glm::vec3 position, scale;
    glm::quat rotation;
    decompose_mat4(world_transform, position, rotation, scale);

    b3WorldTransform target;
    target.p = to_b3(position);
    target.q = to_b3(rotation);
    b3Body_SetTargetTransform(body_id_of(body), target, static_cast<float>(step), true);
  }

  bool box3d_api::set_body_shape(natural_t world_id, physics_world* world, physics_body* body,
                                 const physics_shape_desc& desc, const glm::vec3& world_scale,
                                 const shape_geometry* geometry) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Box3D shape build.");
    OTHER_ASSERT(body != nullptr, "Physics body is null during Box3D shape build.");
    PROFILE_SECTION("box3d_api::set_body_shape");

    box3d_world& bw = world_state(world_id);
    b3BodyId body_id = body_id_of(body);
    const physics_body::settings& authored = body->applied_settings;

    /// replace semantics: drop whatever was attached before building the new collider
    b3ShapeId existing[8];
    int shape_count = b3Body_GetShapes(body_id, existing, 8);
    for (int i = 0; i < shape_count; ++i) {
      b3DestroyShape(existing[i], /*updateBodyMass=*/ false);
    }
    if (auto mitr = bw.meshes.find(body->id); mitr != bw.meshes.end()) {
      b3DestroyMesh(mitr->second);
      bw.meshes.erase(mitr);
    }

    b3ShapeDef shape_def = b3DefaultShapeDef();
    shape_def.baseMaterial.friction = authored.friction;
    shape_def.baseMaterial.restitution = authored.restitution;
    shape_def.density = 1000.f;  /// placeholder; authored mass is applied below
    shape_def.isSensor = authored.is_trigger;
    shape_def.enableSensorEvents = true;
    shape_def.enableContactEvents = true;
    shape_def.userData = reinterpret_cast<void*>(static_cast<intptr_t>(body->id));

    switch (desc.shape_kind) {
      case PHYSICS_SHAPE_NONE:
        break;  /// shapeless body = point mass
      case PHYSICS_SHAPE_BOX: {
        glm::vec3 he = glm::max(desc.half_extents * glm::abs(world_scale), glm::vec3(kMinShapeExtent));
        b3BoxHull box = b3MakeBoxHull(he.x, he.y, he.z);
        b3CreateHullShape(body_id, &shape_def, &box.base);
      } break;
      case PHYSICS_SHAPE_SPHERE: {
        float max_axis = std::max({ std::abs(world_scale.x), std::abs(world_scale.y), std::abs(world_scale.z) });
        b3Sphere sphere{ .center = { 0.f, 0.f, 0.f }, .radius = std::max(desc.radius * max_axis, kMinShapeExtent) };
        b3CreateSphereShape(body_id, &shape_def, &sphere);
      } break;
      case PHYSICS_SHAPE_CAPSULE: {
        float radius = std::max(desc.radius * std::max(std::abs(world_scale.x), std::abs(world_scale.z)), kMinShapeExtent);
        float half_height = std::max(desc.half_height * std::abs(world_scale.y), kMinShapeExtent);
        b3Capsule capsule{ .center1 = { 0.f, -half_height, 0.f }, .center2 = { 0.f, half_height, 0.f }, .radius = radius };
        b3CreateCapsuleShape(body_id, &shape_def, &capsule);
      } break;
      case PHYSICS_SHAPE_CONVEX_HULL: {
        PROFILE_SECTION("box3d_api::set_body_shape--convex_hull");
        if (geometry == nullptr || geometry->positions.empty()) {
          CORE_LOG_ERROR("Convex hull for body {} requires geometry.", body->id);
          return false;
        }
        ostd::vector<b3Vec3> points;
        points.reserve(geometry->positions.size());
        for (const glm::vec3& p : geometry->positions) {
          points.push_back(to_b3(p * world_scale));
        }
        b3HullData* hull = b3CreateHull(points.data(), static_cast<int>(points.size()), kMaxHullVertices);
        if (hull == nullptr) {
          CORE_LOG_ERROR("Hull build failed for body {} (degenerate geometry).", body->id);
          return false;
        }
        b3CreateHullShape(body_id, &shape_def, hull);  /// hulls are cloned into the shape
        b3DestroyHull(hull);
      } break;
      case PHYSICS_SHAPE_TRIANGLE_MESH: {
        PROFILE_SECTION("box3d_api::set_body_shape--triangle_mesh");
        if (geometry == nullptr || geometry->positions.empty() || geometry->indices.size() < 3) {
          CORE_LOG_ERROR("Triangle mesh for body {} requires indexed geometry.", body->id);
          return false;
        }
        ostd::vector<b3Vec3> vertices;
        vertices.reserve(geometry->positions.size());
        for (const glm::vec3& p : geometry->positions) {
          vertices.push_back(to_b3(p));  /// scale applies at shape creation below
        }
        ostd::vector<int32_t> indices(geometry->indices.begin(), geometry->indices.end());

        b3MeshDef mesh_def = {};
        mesh_def.vertices = vertices.data();
        mesh_def.indices = indices.data();
        mesh_def.vertexCount = static_cast<int>(vertices.size());
        mesh_def.triangleCount = static_cast<int>(indices.size() / 3);

        b3MeshData* mesh = b3CreateMesh(&mesh_def, nullptr, 0);
        if (mesh == nullptr) {
          CORE_LOG_ERROR("Mesh build failed for body {}.", body->id);
          return false;
        }
        /// mesh shapes REFERENCE the data; the world owns it for the shape's lifetime
        b3CreateMeshShape(body_id, &shape_def, mesh, to_b3(glm::abs(world_scale)));
        bw.meshes.emplace(body->id, mesh);
      } break;
      default:
        CORE_LOG_ERROR("Unknown physics shape kind {} for body {}.", desc.shape_kind, body->id);
        return false;
    }

    if (body->body_type == physics_body::DYNAMIC && body->mass > 0.f) {
      b3Body_ApplyMassFromShapes(body_id);
      b3MassData mass_data = b3Body_GetMassData(body_id);
      if (mass_data.mass > 0.f) {
        /// rescale the shape-derived inertia to the AUTHORED mass (density was a placeholder)
        float k = body->mass / mass_data.mass;
        mass_data.mass = body->mass;
        mass_data.inertia.cx = b3Vec3{ mass_data.inertia.cx.x * k, mass_data.inertia.cx.y * k, mass_data.inertia.cx.z * k };
        mass_data.inertia.cy = b3Vec3{ mass_data.inertia.cy.x * k, mass_data.inertia.cy.y * k, mass_data.inertia.cy.z * k };
        mass_data.inertia.cz = b3Vec3{ mass_data.inertia.cz.x * k, mass_data.inertia.cz.y * k, mass_data.inertia.cz.z * k };
      } else {
        /// shapeless point mass: authored mass with unit rotational inertia
        mass_data.mass = body->mass;
        mass_data.center = b3Vec3{ 0.f, 0.f, 0.f };
        mass_data.inertia = b3Matrix3{ .cx = { body->mass, 0.f, 0.f }, .cy = { 0.f, body->mass, 0.f }, .cz = { 0.f, 0.f, body->mass } };
      }
      b3Body_SetMassData(body_id, mass_data);
    }
    return true;
  }

  void box3d_api::step_simulation(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Box3D step simulation.");
    PROFILE_SECTION("box3d_api::step_simulation");
    box3d_world& bw = world_state(world_id);

    {
      PROFILE_SECTION("box3d_api::step_simulation--world_step");
      b3World_Step(bw.world, static_cast<float>(delta_time), kSubStepCount);
    }

    {
      PROFILE_SECTION("box3d_api::step_simulation--poll_events");
      /// box3d is a polling backend: collect this step's transitions right here, main thread
      b3ContactEvents contacts = b3World_GetContactEvents(bw.world);
      for (int i = 0; i < contacts.beginCount; ++i) {
        const b3ContactBeginTouchEvent& ev = contacts.beginEvents[i];
        contact_event out;
        out.type = contact_event::kBegin;
        out.body_a = engine_id_of(b3Shape_GetBody(ev.shapeIdA));
        out.body_b = engine_id_of(b3Shape_GetBody(ev.shapeIdB));
        if (b3Contact_IsValid(ev.contactId)) {
          b3ContactData data = b3Contact_GetData(ev.contactId);
          if (data.manifoldCount > 0 && data.manifolds[0].pointCount > 0) {
            out.normal = from_b3(data.manifolds[0].normal);
            b3Pos center_a = b3Body_GetWorldCenter(b3Shape_GetBody(ev.shapeIdA));
            out.point = from_b3(center_a) + from_b3(data.manifolds[0].points[0].anchorA);
          }
        }
        bw.pending.push_back(out);
      }
      for (int i = 0; i < contacts.endCount; ++i) {
        const b3ContactEndTouchEvent& ev = contacts.endEvents[i];
        if (!b3Shape_IsValid(ev.shapeIdA) || !b3Shape_IsValid(ev.shapeIdB)) {
          continue;  /// a side died this step — dropped by contract
        }
        contact_event out;
        out.type = contact_event::kEnd;
        out.body_a = engine_id_of(b3Shape_GetBody(ev.shapeIdA));
        out.body_b = engine_id_of(b3Shape_GetBody(ev.shapeIdB));
        bw.pending.push_back(out);
      }

      b3SensorEvents sensors = b3World_GetSensorEvents(bw.world);
      for (int i = 0; i < sensors.beginCount; ++i) {
        const b3SensorBeginTouchEvent& ev = sensors.beginEvents[i];
        contact_event out;
        out.type = contact_event::kTriggerBegin;
        out.body_a = engine_id_of(b3Shape_GetBody(ev.sensorShapeId));
        out.body_b = engine_id_of(b3Shape_GetBody(ev.visitorShapeId));
        bw.pending.push_back(out);
      }
      for (int i = 0; i < sensors.endCount; ++i) {
        const b3SensorEndTouchEvent& ev = sensors.endEvents[i];
        if (!b3Shape_IsValid(ev.sensorShapeId) || !b3Shape_IsValid(ev.visitorShapeId)) {
          continue;
        }
        contact_event out;
        out.type = contact_event::kTriggerEnd;
        out.body_a = engine_id_of(b3Shape_GetBody(ev.sensorShapeId));
        out.body_b = engine_id_of(b3Shape_GetBody(ev.visitorShapeId));
        bw.pending.push_back(out);
      }
    }
  }

  void box3d_api::update_active_transforms(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during Box3D update active transforms.");
    PROFILE_SECTION("box3d_api::update_active_transforms");
    box3d_world& bw = world_state(world_id);

    for (auto& [engine_id, backend_bits] : bw.bodies) {
      auto* body = world->find_if([&](physics_body* p) { return p != nullptr && p->id == engine_id; });
      OTHER_ASSERT(body != nullptr, "Physics body {} in box3d world {} has no engine-side body.", engine_id, world_id);

      b3BodyId body_id = b3LoadBodyId(backend_bits);
      glm::vec3 position = from_b3(b3Body_GetPosition(body_id));
      glm::quat rotation = from_b3(b3Body_GetRotation(body_id));

      glm::mat4 transform = compose_mat4(position, rotation, glm::vec3(1.f));
      body->previous_transform = body->current_transform;
      body->current_transform = transform;
    }
  }

  void box3d_api::drain_contacts(natural_t world_id, physics_world* world, ostd::vector<contact_event>& out) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during contact drain.");
    box3d_world& bw = world_state(world_id);
    out.insert(out.end(), bw.pending.begin(), bw.pending.end());
    bw.pending.clear();
  }

  namespace {

    struct closest_nonsensor_hit {
      b3ShapeId shape = {};
      glm::vec3 point = {};
      glm::vec3 normal = {};
      float fraction = 1.f;
      bool hit = false;
    };

    float raycast_filter_callback(b3ShapeId shapeId, b3Pos point, b3Vec3 normal, float fraction,
                                  uint64_t user_material_id, int triangle_index, int child_index, void* context) {
      if (b3Shape_IsSensor(shapeId)) {
        return -1.f;  /// sensors are triggers, not surfaces — skip and keep casting
      }
      auto* closest = static_cast<closest_nonsensor_hit*>(context);
      closest->shape = shapeId;
      closest->point = from_b3(point);
      closest->normal = from_b3(normal);
      closest->fraction = fraction;
      closest->hit = true;
      return fraction;  /// clip the ray to this hit and continue for anything closer
    }

  }  // namespace

  raycast_hit box3d_api::cast_ray(natural_t world_id, physics_world* world, const glm::vec3& origin,
                                  const glm::vec3& direction, float max_distance) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during raycast.");
    PROFILE_SECTION("box3d_api::cast_ray");
    box3d_world& bw = world_state(world_id);

    glm::vec3 translation = glm::normalize(direction) * max_distance;
    closest_nonsensor_hit closest;
    b3World_CastRay(bw.world, to_b3(origin), to_b3(translation), b3DefaultQueryFilter(), raycast_filter_callback, &closest);

    raycast_hit out;
    if (!closest.hit) {
      return out;
    }
    out.hit = true;
    out.body_id = engine_id_of(b3Shape_GetBody(closest.shape));
    out.point = closest.point;
    out.normal = closest.normal;
    out.distance = closest.fraction * max_distance;
    return out;
  }

  integer_t box3d_api::create_fixed_joint(natural_t world_id, physics_world* world, physics_body* body_a, physics_body* body_b) {
    OTHER_ASSERT(world != nullptr, "Physics world is null during weld creation.");
    OTHER_ASSERT(body_a != nullptr && body_b != nullptr, "Cannot weld a null physics body.");
    PROFILE_SECTION("box3d_api::create_fixed_joint");
    box3d_world& bw = world_state(world_id);

    /// box3d requires the weld's bodyB in the awake set: a static side must be bodyA, and the
    ///  moving side is woken before creation. the weld is symmetric for our purposes
    if (body_a->body_type != physics_body::STATIC && body_b->body_type == physics_body::STATIC) {
      std::swap(body_a, body_b);
    }

    b3BodyId id_a = body_id_of(body_a);
    b3BodyId id_b = body_id_of(body_b);
    if (body_b->body_type != physics_body::STATIC) {
      b3Body_SetAwake(id_b, true);
    }
    if (body_a->body_type != physics_body::STATIC) {
      b3Body_SetAwake(id_a, true);
    }

    /// weld frames coincide at body A's origin: frameA = identity, frameB = xfB^-1 * xfA
    glm::vec3 pos_a = from_b3(b3Body_GetPosition(id_a));
    glm::quat rot_a = from_b3(b3Body_GetRotation(id_a));
    glm::vec3 pos_b = from_b3(b3Body_GetPosition(id_b));
    glm::quat rot_b = from_b3(b3Body_GetRotation(id_b));

    b3Transform xf_a{ .p = to_b3(pos_a), .q = to_b3(rot_a) };
    b3Transform xf_b{ .p = to_b3(pos_b), .q = to_b3(rot_b) };

    b3WeldJointDef def = b3DefaultWeldJointDef();
    def.base.bodyIdA = id_a;
    def.base.bodyIdB = id_b;
    def.base.localFrameA = b3Transform{ .p = { 0.f, 0.f, 0.f }, .q = { .v = { 0.f, 0.f, 0.f }, .s = 1.f } };
    def.base.localFrameB = b3InvMulTransforms(xf_b, xf_a);
    def.linearHertz = 0.f;   /// rigid
    def.angularHertz = 0.f;

    b3JointId joint = b3CreateWeldJoint(bw.world, &def);
    integer_t joint_id = bw.next_joint_id++;
    bw.joints.emplace(joint_id, b3StoreJointId(joint));
    return joint_id;
  }

  void box3d_api::destroy_joint(natural_t world_id, physics_world* world, integer_t joint_id) {
    PROFILE_SECTION("box3d_api::destroy_joint");
    box3d_world& bw = world_state(world_id);
    auto itr = bw.joints.find(joint_id);
    if (itr == bw.joints.end()) {
      CORE_LOG_ERROR("Joint {} does not exist in box3d world {}.", joint_id, world_id);
      return;
    }
    b3JointId joint = b3LoadJointId(itr->second);
    if (b3Joint_IsValid(joint)) {
      b3DestroyJoint(joint, /*wakeAttached=*/ true);
    }
    bw.joints.erase(itr);
  }

  float box3d_api::joint_reaction_force(natural_t world_id, physics_world* world, integer_t joint_id, double step) {
    box3d_world& bw = world_state(world_id);
    auto itr = bw.joints.find(joint_id);
    if (itr == bw.joints.end()) {
      return 0.f;
    }
    b3JointId joint = b3LoadJointId(itr->second);
    if (!b3Joint_IsValid(joint)) {
      return 0.f;
    }
    /// box3d reports constraint FORCE directly (no impulse/dt conversion like jolt)
    return b3Length(b3Joint_GetConstraintForce(joint));
  }

  void box3d_api::set_linear_velocity(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& velocity) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during velocity write.");
    if (body->body_type == physics_body::STATIC) {
      CORE_LOG_WARN("Ignoring velocity write on static physics body {}.", body->id);
      return;
    }
    b3Body_SetLinearVelocity(body_id_of(body), to_b3(velocity));
  }

  glm::vec3 box3d_api::get_linear_velocity(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during velocity read.");
    if (body->body_type == physics_body::STATIC) {
      return glm::vec3(0.f);
    }
    return from_b3(b3Body_GetLinearVelocity(body_id_of(body)));
  }

  void box3d_api::set_angular_velocity(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& velocity) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during velocity write.");
    if (body->body_type == physics_body::STATIC) {
      CORE_LOG_WARN("Ignoring velocity write on static physics body {}.", body->id);
      return;
    }
    b3Body_SetAngularVelocity(body_id_of(body), to_b3(velocity));
  }

  glm::vec3 box3d_api::get_angular_velocity(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during velocity read.");
    if (body->body_type == physics_body::STATIC) {
      return glm::vec3(0.f);
    }
    return from_b3(b3Body_GetAngularVelocity(body_id_of(body)));
  }

  void box3d_api::add_force(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& force) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during force application.");
    if (body->body_type != physics_body::DYNAMIC) {
      CORE_LOG_WARN("Ignoring force on non-dynamic physics body {}.", body->id);
      return;
    }
    b3Body_ApplyForceToCenter(body_id_of(body), to_b3(force), true);
  }

  void box3d_api::add_impulse(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& impulse) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during impulse application.");
    if (body->body_type != physics_body::DYNAMIC) {
      CORE_LOG_WARN("Ignoring impulse on non-dynamic physics body {}.", body->id);
      return;
    }
    b3Body_ApplyLinearImpulseToCenter(body_id_of(body), to_b3(impulse), true);
  }

  void box3d_api::add_torque(natural_t world_id, physics_world* world, physics_body* body, const glm::vec3& torque) {
    OTHER_ASSERT(body != nullptr, "Physics body is null during torque application.");
    if (body->body_type != physics_body::DYNAMIC) {
      CORE_LOG_WARN("Ignoring torque on non-dynamic physics body {}.", body->id);
      return;
    }
    b3Body_ApplyTorque(body_id_of(body), to_b3(torque), true);
  }

  namespace {

    int box3d_assert_handler(const char* condition, const char* file_name, int line_number) {
      CORE_LOG_ERROR("[BOX3D ASSERT] {} ({}:{})", condition, file_name, line_number);
      return 1;  /// still break into the debugger after logging
    }

  }  // namespace

  void box3d_api::on_initialize(const config_table& configuration) {
    /// box3d needs no global bring-up; worlds are self-contained. worker threads stay at the
    ///  library default until a task-system adapter over the engine job system exists
    b3SetAssertFcn(box3d_assert_handler);
    CORE_LOG_INFO("Box3D physics backend initialized.");
  }

  void box3d_api::on_shutdown() {
    PROFILE_SECTION("box3d_api::on_shutdown");
    if (!box3d_worlds.empty()) {
      CORE_LOG_WARN("Box3D backend shutting down with {} live worlds.", box3d_worlds.size());
      while (!box3d_worlds.empty()) {
        box3d_world* bw = box3d_worlds.begin()->second;
        b3DestroyWorld(bw->world);
        for (auto& [body_id, mesh] : bw->meshes) {
          b3DestroyMesh(mesh);
        }
        delete bw;
        box3d_worlds.erase(box3d_worlds.begin());
      }
    }
  }

}  // namespace other
