/**
 * \file physics/backends/physx_api.cpp
 **/
#include "physics/backends/physx_api.hpp"

#include <format>
#include <string_view>

#include <physx/OmniPvdFileWriteStream.h>
#include <physx/OmniPvdWriter.h>
#include <physx/PxPhysicsAPI.h>
#include <physx/PxRigidDynamic.h>
#include <physx/PxRigidStatic.h>
#include <physx/PxScene.h>
#include <physx/extensions/PxExtensionsAPI.h>
#include <physx/omnipvd/PxOmniPvd.h>

#include "core/defines.hpp"
#include "core/logger.hpp"
#include "memory/arena.hpp"

#include "physics_world/physics_world.hpp"

#include "physx/OmniPvdDefines.h"
#include "physx/foundation/PxErrors.h"


namespace other {
  namespace {

    glm::mat4 physx_transform_to_mat4(const physx::PxTransform& transform) {
      glm::mat4 mat = glm::mat4(1.0f);
      mat[3][0] = transform.p.x;
      mat[3][1] = transform.p.y;
      mat[3][2] = transform.p.z;

      glm::mat4 rot_mat = glm::mat4(1.0f);
      rot_mat[0][0] = transform.q.w * transform.q.w + transform.q.x * transform.q.x - transform.q.y * transform.q.y - transform.q.z * transform.q.z;
      rot_mat[1][0] = 2.0f * (transform.q.x * transform.q.y - transform.q.w * transform.q.z);
      rot_mat[2][0] = 2.0f * (transform.q.x * transform.q.z + transform.q.w * transform.q.y);

      rot_mat[0][1] = 2.0f * (transform.q.x * transform.q.y + transform.q.w * transform.q.z);
      rot_mat[1][1] = transform.q.w * transform.q.w - transform.q.x * transform.q.x + transform.q.y * transform.q.y - transform.q.z * transform.q.z;
      rot_mat[2][1] = 2.0f * (transform.q.y * transform.q.z - transform.q.w * transform.q.x);

      rot_mat[0][2] = 2.0f * (transform.q.x * transform.q.z - transform.q.w * transform.q.y);
      rot_mat[1][2] = 2.0f * (transform.q.y * transform.q.z + transform.q.w * transform.q.x);
      rot_mat[2][2] = transform.q.w * transform.q.w - transform.q.x * transform.q.x - transform.q.y * transform.q.y + transform.q.z * transform.q.z;

      return mat * rot_mat;
    }

  }  // namespace

  class OtherPxAllocatorCallback : public physx::PxAllocatorCallback {
   public:
    virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override {
      // return arena::allocate(size);
      return malloc(size);
    }

    virtual void deallocate(void* ptr) override {
      // arena::free(ptr);
      free(ptr);
    }
  };

  class OtherPxErrorCallback : public physx::PxErrorCallback {
   public:
    virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override {
      if (code == physx::PxErrorCode::eABORT) {
        OTHER_ASSERT(false, "PhysX Abort Error: {} ({}:{})", message, file, line);
      } else {
        CORE_LOG_ERROR("PhysX Error (code: {}): {} ({}:{})", code, message, file, line);
      }
    }
  };

  class OtherPxSimulationEventCallback : public physx::PxSimulationEventCallback {
   public:
    virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override {
      // Handle contact events here
    }

    virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override {
      // Handle trigger events here
    }

    virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override {
      // Handle constraint break events here
    }

    virtual void onWake(physx::PxActor** actors, physx::PxU32 count) override {
      // Handle wake events here
      CORE_LOG_DEBUG("PhysX onWake called for {} actors.", count);
      for (physx::PxU32 i = 0; i < count; ++i) {
        physx::PxActor* actor = actors[i];
        CORE_LOG_DEBUG(" - actor: {}", actor->getName());
      }
    }

    virtual void onSleep(physx::PxActor** actors, physx::PxU32 count) override {
      CORE_LOG_DEBUG("PhysX onSleep called for {} actors.", count);
      for (physx::PxU32 i = 0; i < count; ++i) {
        physx::PxActor* actor = actors[i];
        CORE_LOG_DEBUG(" - actor: {}", actor->getName());
      }
    }
  };

  void omni_log_fn(char* message) {
    // CORE_LOG_DEBUG(" [OmniPVD]: {}", message);
  }

  physics_api::physics_render_debug_data physx_api::get_debug_render_data(natural_t id, const physics_world* world) const {
    return {};
  }

  void physx_api::on_initialize(const config_table& configuration) {
    PROFILE_SECTION("physx_api::on_initialize");

    static OtherPxAllocatorCallback allocator_callback;
    static OtherPxErrorCallback error_callback;

    foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator_callback, error_callback);
    OTHER_ASSERT(foundation != nullptr, "Failed to create PhysX Foundation.");

/// \todo turn this on for PhysX Visual Debugger
// #define OTHER_PHYSX_VISUAL_DEBUGGER
#ifdef OTHER_PHYSX_VISUAL_DEBUGGER
    pvd = physx::PxCreatePvd(*foundation);
    physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
    pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

    omni_pvd = PxCreateOmniPvd(*foundation);

    OmniPvdFileWriteStream* omni_file_stream = nullptr;
    if (omni_pvd != nullptr) {
      OmniPvdWriter* omni_writer = omni_pvd->getWriter();
      if (omni_writer != nullptr) {
        omni_writer->setLogFunction(&omni_log_fn);
      }

      omni_file_stream = omni_pvd->getFileWriteStream();
      if (omni_writer && omni_file_stream) {
        omni_writer->setWriteStream(*omni_file_stream);
      }
    }

#endif

    constexpr static bool kTrackOutstandingAllocations = false;
    physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), kTrackOutstandingAllocations, pvd, omni_pvd);
    OTHER_ASSERT(physics != nullptr, "Failed to create PhysX Physics instance.");
    OTHER_ASSERT(PxInitExtensions(*physics, pvd) == true, "Failed to initialize PhysX extensions.");

#ifdef OTHER_PHYSX_VISUAL_DEBUGGER
    if (omni_file_stream) {
      omni_file_stream->setFileName("logs/omnipvd_log.ovd");
    }
#endif

    if (omni_pvd != nullptr && !omni_pvd->startSampling()) {
      CORE_LOG_ERROR("Failed to start OmniPVD sampling.");
    }

    CORE_LOG_INFO("PhysX Foundation created successfully.");
  }

  void physx_api::on_shutdown() {
    PROFILE_SECTION("physx_api::on_shutdown");
    OTHER_ASSERT(foundation != nullptr, "PhysX Foundation is null during shutdown.");
    OTHER_ASSERT(physics != nullptr, "PhysX Physics instance is null during shutdown.");

    PxCloseExtensions();

    physics->release();

    if (omni_pvd != nullptr) {
      omni_pvd->release();
    }
    if (pvd != nullptr) {
      pvd->release();
    }

    foundation->release();

    physics = nullptr;
    foundation = nullptr;

    CORE_LOG_INFO("PhysX API shutdown successfully.");
  }

  void physx_api::initialize_world(natural_t id, physics_world* world) {
    auto itr = physx_scenes.find(id);
    if (itr != physx_scenes.end()) {
      CORE_LOG_WARN("PhysX scene with id '{}' already exists.", id);
      return;
    }

    auto scene_desc = physx::PxSceneDesc(physx::PxTolerancesScale());
    scene_desc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
    scene_desc.cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
    scene_desc.filterShader = physx::PxDefaultSimulationFilterShader;

    physx::PxScene* px_scene = physics->createScene(scene_desc);
    OTHER_ASSERT(px_scene != nullptr, "Failed to create PhysX scene '{}'.", id);
    physx_scenes.emplace(id, px_scene);
  }

  void physx_api::shutdown_world(physics_world* world) {
    auto itr = physx_scenes.find(world->world_id);
    if (itr == physx_scenes.end()) {
      CORE_LOG_ERROR("PhysX scene with id '{}' does not exist.", world->world_id);
      return;
    }

    itr->second->release();
    physx_scenes.erase(itr);
  }

  void physx_api::register_physics_body(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(world != nullptr, "Physics world is null. Cannot register physics body.");
    OTHER_ASSERT(body != nullptr, "Physics body is null. Cannot register physics body. ");

    /// \todo implement body description and creation, for now hardcode a static body
    auto itr = physx_scenes.find(world_id);
    OTHER_ASSERT(itr != physx_scenes.end(), "PhysX scene with id '{}' does not exist.", world_id);

    physx::PxShapeFlags shape_flags = physx::PxShapeFlag::eSIMULATION_SHAPE | physx::PxShapeFlag::eSCENE_QUERY_SHAPE;
    physx::PxMaterial* default_material = physics->createMaterial(0.5f, 0.5f, 0.6f);

    physx::PxRigidDynamic* rigid_dynamic = physics->createRigidDynamic(physx::PxTransform(physx::PxVec3(0.f, 2.5f, 0.f)));
    {
      physx::PxShape* shape = physics->createShape(physx::PxBoxGeometry(0.5f, 0.5f, 0.5f), &default_material, 1, true, shape_flags);
      rigid_dynamic->attachShape(*shape);
      shape->release();
    }

    if (!itr->second->addActor(*rigid_dynamic)) {
      CORE_LOG_ERROR("Failed to add physics body to PhysX scene with Physx-Environment-ID id '{}'.", world_id);
    } else {
      CORE_LOG_DEBUG(" Added physics body to PhysX scene with Physx-Environment-ID '{}'.", world_id);
      body->scene_id = rigid_dynamic->getEnvironmentID();
    }

    rigid_dynamic->wakeUp();
    rigid_dynamic->release();
  }

  void physx_api::unregister_physics_body(natural_t world_id, physics_world* world, physics_body* body) {
    OTHER_ASSERT(world != nullptr, "Physics world is null. Cannot unregister physics body.");
    OTHER_ASSERT(body != nullptr, "Physics body is null. Cannot unregister physics body.");

    auto itr = physx_scenes.find(world_id);
    OTHER_ASSERT(itr != physx_scenes.end(), "PhysX scene with id '{}' does not exist.", world_id);

    // Find the actor by environment ID
    physx::PxU32 num_actors = itr->second->getNbActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC | physx::PxActorTypeFlag::eRIGID_STATIC);
    if (num_actors == 0) {
      CORE_LOG_ERROR("No actors found in PhysX scene with id '{}'. Cannot unregister physics body.", world_id);
      return;
    }

    std::vector<physx::PxActor*> actors(num_actors);
    itr->second->getActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC | physx::PxActorTypeFlag::eRIGID_STATIC, actors.data(), num_actors, 0);
    for (physx::PxActor* actor : actors) {
      OTHER_ASSERT(actor != nullptr, "PhysX actor is null during unregister.");
      if (actor->getEnvironmentID() == body->scene_id) {
        itr->second->removeActor(*actor);
        CORE_LOG_DEBUG(" Unregistered physics body from PhysX scene with id '{}'.", world_id);
        return;
      }
    }
  }

  void physx_api::attach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) {}
  void physx_api::detach_shape(natural_t world_id, physics_world* world, physics_body* body, physics_shape* shape) {}

  void physx_api::configure_empty_shape(physics_shape* shape) {}

  void physx_api::configure_box_shape(physics_shape* shape, const glm::vec3& half_extents) {}

  void physx_api::step_simulation(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null. Cannot step simulation.");
    PROFILE_SECTION("physx_api::step_simulation");

    auto itr = physx_scenes.find(world_id);
    OTHER_ASSERT(itr != physx_scenes.end(), "PhysX scene with id '{}' does not exist.", world_id);

    accumulator += static_cast<float>(delta_time);
    if (accumulator < kFixedTimeStep) {
      return;
    }
    accumulator -= kFixedTimeStep;
    alpha = accumulator / kFixedTimeStep;

    {
      PROFILE_SECTION("physx_api::step_simulation");
      itr->second->simulate(static_cast<physx::PxReal>(kFixedTimeStep));

      physx::PxU32 error_code;
      itr->second->fetchResults(true, &error_code);
      if (error_code != physx::PxErrorCode::eNO_ERROR) {
        CORE_LOG_ERROR("PhysX simulation step fetchResults returned error code: {}", error_code);
      }
    }
  }

  void physx_api::update_active_transforms(natural_t world_id, physics_world* world, double delta_time) {
    OTHER_ASSERT(world != nullptr, "Physics world is null. Cannot update interpolated transforms.");
    PROFILE_SECTION("physx_api::update_interpolated_transforms");

    auto itr = physx_scenes.find(world_id);
    OTHER_ASSERT(itr != physx_scenes.end(), "PhysX scene with id '{}' does not exist.", world_id);

    physx::PxU32 num_actors = itr->second->getNbActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC | physx::PxActorTypeFlag::eRIGID_STATIC);
    if (num_actors == 0) {
      return;
    }

    std::vector<physx::PxActor*> actors(num_actors);
    itr->second->getActors(physx::PxActorTypeFlag::eRIGID_DYNAMIC | physx::PxActorTypeFlag::eRIGID_STATIC, actors.data(), num_actors, 0);

    for (physx::PxActor* actor : actors) {
      OTHER_ASSERT(actor != nullptr, "PhysX actor is null during transform update.");
      physx::PxU32 env_id = actor->getEnvironmentID();

      /// \note this transform is global so there needs to a translation back to local space to be stored in a scene object's
      ///         transform component

      auto get_transform = []<typename AT>(const physx::PxActor* a) -> glm::mat4 {
        return physx_transform_to_mat4(static_cast<const AT*>(a)->getGlobalPose());
      };

      glm::mat4 transform = {};
      switch (actor->getType()) {
        case physx::PxActorType::eRIGID_DYNAMIC: transform = get_transform.operator()<physx::PxRigidDynamic>(actor); break;
        case physx::PxActorType::eRIGID_STATIC: transform = get_transform.operator()<physx::PxRigidStatic>(actor); break;
        default:
          OTHER_ASSERT(false, "Unsupported PhysX actor type during transform update.");
      }

      auto* b = world->find_if([env_id](physics_body* live_obj) { return live_obj != nullptr && live_obj->scene_id == env_id; });
      OTHER_ASSERT(b != nullptr, "Failed to find physics body during transform update.");

      b->previous_transform = b->current_transform;
      b->current_transform = transform;
    }
  }

}  // namespace other