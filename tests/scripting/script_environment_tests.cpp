/**
 * \file tests/scripting/script_environment_tests.cpp
 **/
#include "script_environment_tests.hpp"

#include "gtest/gtest.h"

namespace other {

  TEST_F(script_environment_tests, load_and_unload_dotnet_module) {
    auto* env = subsystem<scripting_environment>::get();
    ASSERT_NE(env, nullptr);

    EXPECT_NO_FATAL_FAILURE(env->initialize_script_environment(environment->config));

    ref<assembly> dotnet_asm = nullptr;
#ifdef OTHER_ENVIRONMENT_DEBUG
    EXPECT_NO_FATAL_FAILURE(dotnet_asm = env->load_dotnet_module(main_other_dll_debug.string()));
#elif defined(OTHER_ENVIRONMENT_RELEASE)
    EXPECT_NO_FATAL_FAILURE(dotnet_asm = env->load_dotnet_module(main_other_dll_release.string()));
#else
  #error "Unknown build configuration!"
#endif
    ASSERT_NE(dotnet_asm, nullptr);

    integer_t obj_id = 0;
    EXPECT_NO_FATAL_FAILURE(obj_id = env->create_object("MyObject"));
    ASSERT_GE(obj_id, 0);

    {
      script_object* script_obj = nullptr;
      EXPECT_NO_FATAL_FAILURE(script_obj = env->get_object(obj_id));
      ASSERT_NE(script_obj, nullptr);
    }

    {
      ref<assembly> testing_asm = nullptr;
#ifdef OTHER_ENVIRONMENT_DEBUG
      EXPECT_NO_FATAL_FAILURE(testing_asm = env->load_dotnet_module(testing_dll_debug.string()));
#elif defined(OTHER_ENVIRONMENT_RELEASE)
      EXPECT_NO_FATAL_FAILURE(testing_asm = env->load_dotnet_module(testing_dll_release.string()));
#else
  #error "Unknown build configuration!"
#endif
      ASSERT_NE(testing_asm, nullptr);

      EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_object(obj_id, "TestObject"));

      script_object* script_obj = nullptr;
      EXPECT_NO_FATAL_FAILURE(script_obj = env->get_object(obj_id));
      ASSERT_NE(script_obj, nullptr);
      ASSERT_NE(script_obj->dotnet_object, nullptr);

      int field_value = 0;
      EXPECT_NO_FATAL_FAILURE(field_value = script_obj->dotnet_object->get_field<int>("field_value"));
      EXPECT_EQ(field_value, 10);

      EXPECT_NO_FATAL_FAILURE(script_obj->dotnet_object->set_field("field_value", 42));
      EXPECT_NO_FATAL_FAILURE(field_value = script_obj->dotnet_object->get_field<int>("field_value"));
      EXPECT_EQ(field_value, 42);

      EXPECT_NO_FATAL_FAILURE(env->detach_dotnet_object(obj_id));
      EXPECT_NO_FATAL_FAILURE(env->unload_dotnet_module(testing_asm));
    }

    {
      script_object* script_obj = nullptr;
      EXPECT_NO_FATAL_FAILURE(script_obj = env->get_object(obj_id));
      ASSERT_NE(script_obj, nullptr);
      EXPECT_EQ(script_obj->dotnet_object, nullptr);
    }

    {
      ref<assembly> testing_asm = nullptr;
#ifdef OTHER_ENVIRONMENT_DEBUG
      EXPECT_NO_FATAL_FAILURE(testing_asm = env->load_dotnet_module(testing_dll_debug.string()));
#elif defined(OTHER_ENVIRONMENT_RELEASE)
      EXPECT_NO_FATAL_FAILURE(testing_asm = env->load_dotnet_module(testing_dll_release.string()));
#else
  #error "Unknown build configuration!"
#endif
      ASSERT_NE(testing_asm, nullptr);

      EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_object(obj_id, "TestObject"));

      script_object* script_obj = nullptr;
      EXPECT_NO_FATAL_FAILURE(script_obj = env->get_object(obj_id));
      ASSERT_NE(script_obj, nullptr);
      ASSERT_NE(script_obj->dotnet_object, nullptr);

      int field_value = 0;
      EXPECT_NO_FATAL_FAILURE(field_value = script_obj->dotnet_object->get_field<int>("field_value"));
      EXPECT_EQ(field_value, 10);

      EXPECT_NO_FATAL_FAILURE(env->detach_dotnet_object(obj_id));
      EXPECT_NO_FATAL_FAILURE(env->unload_dotnet_module(testing_asm));
    }

    EXPECT_NO_FATAL_FAILURE(env->destroy_object(obj_id));

    EXPECT_NO_FATAL_FAILURE(env->unload_dotnet_module(dotnet_asm));
    EXPECT_NO_FATAL_FAILURE(env->shutdown_script_environment());
    EXPECT_NO_FATAL_FAILURE(env->initialize_script_environment(environment->config));

    {
      ref<assembly> dotnet_asm = nullptr;
      ref<assembly> testing_asm = nullptr;

#ifdef OTHER_ENVIRONMENT_DEBUG
      EXPECT_NO_FATAL_FAILURE(dotnet_asm = env->load_dotnet_module(main_other_dll_debug.string()));
#elif defined(OTHER_ENVIRONMENT_RELEASE)
      EXPECT_NO_FATAL_FAILURE(dotnet_asm = env->load_dotnet_module(main_other_dll_release.string()));
#else
  #error "Unknown build configuration!"
#endif
      ASSERT_NE(dotnet_asm, nullptr);

#ifdef OTHER_ENVIRONMENT_DEBUG
      EXPECT_NO_FATAL_FAILURE(testing_asm = env->load_dotnet_module(testing_dll_debug.string()));
#elif defined(OTHER_ENVIRONMENT_RELEASE)
      EXPECT_NO_FATAL_FAILURE(testing_asm = env->load_dotnet_module(testing_dll_release.string()));
#else
  #error "Unknown build configuration!"
#endif
      ASSERT_NE(testing_asm, nullptr);

      integer_t obj_id = 0;
      EXPECT_NO_FATAL_FAILURE(obj_id = env->create_object("MyObject"));
      ASSERT_GE(obj_id, 0);

      script_object* script_obj = nullptr;
      EXPECT_NO_FATAL_FAILURE(script_obj = env->get_object(obj_id));
      ASSERT_NE(script_obj, nullptr);
      EXPECT_EQ(script_obj->dotnet_object, nullptr);

      EXPECT_NO_FATAL_FAILURE(env->attach_dotnet_object(obj_id, "TestObject"));
      EXPECT_NO_FATAL_FAILURE(script_obj = env->get_object(obj_id));
      ASSERT_NE(script_obj, nullptr);
      ASSERT_NE(script_obj->dotnet_object, nullptr);

      int field_value = 0;
      EXPECT_NO_FATAL_FAILURE(field_value = script_obj->dotnet_object->get_field<int>("field_value"));
      EXPECT_EQ(field_value, 10);
      EXPECT_NO_FATAL_FAILURE(env->detach_dotnet_object(obj_id));

      EXPECT_NO_FATAL_FAILURE(script_obj = env->get_object(obj_id));
      ASSERT_NE(script_obj, nullptr);
      EXPECT_EQ(script_obj->dotnet_object, nullptr);

      EXPECT_NO_FATAL_FAILURE(env->unload_dotnet_module(testing_asm));
      EXPECT_NO_FATAL_FAILURE(env->unload_dotnet_module(dotnet_asm));
    }

    EXPECT_NO_FATAL_FAILURE(env->shutdown_script_environment());
  }

}  // namespace other