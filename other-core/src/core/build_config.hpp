/**
 * \file core/build_config.hpp
 **/
#ifndef OTHER_CORE_BUILD_CONFIG_HPP
#define OTHER_CORE_BUILD_CONFIG_HPP

#define bit(x) (1ll << x)

#ifdef OTHER_CLIENT
  #define OTHER_DYNAMIC_DRIVER
#elif defined(OTHER_APPLICATION) && !defined(OTHER_TEST_ENVIRONMENT)
  #define OTHER_STATIC_DRIVER
#elif defined(OTHER_PLUGIN_LIBRARY)
  #define OTHER_PLUGIN_DYNAMIC_LIBRARY
#else
  #define OTHER_STATIC_LIBRARY
#endif

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #if defined(OTHER_CLIENT) || defined(OTHER_PLUGIN_LIBRARY)
    #define OTHER_API __declspec(dllexport)
    #define OTHER_CLASS __declspec(dllexport)
    #define OTHER_ALIGN(x) __declspec(align(x))
  #else
    #define OTHER_API
    #define OTHER_CLASS
    #define OTHER_ALIGN(x)
  #endif  // OTHER_CLIENT
#endif    // OTHER_ENVIRONMENT_WINDOWS

#ifdef OTHER_ENVIRONMENT_LINUX
  #ifdef OTHER_CLIENT
    #define OTHER_API __attribute__((visibility("default")))
    #define OTHER_CLASS __attribute__((visibility("default")))
    #define OTHER_ALIGN(x) __attribute__((aligned(x)))
  #else
    #define OTHER_API
    #define OTHER_CLASS
    #define OTHER_ALIGN(x)
  #endif  // OTHER_CLIENT
#endif    // OTHER_ENVIRONMENT_LINUX

#ifdef OTHER_ENVIRONMENT_DEBUG
  #define OTHER_DEBUG_BUILD
#endif  // !OTHER_DEBUG

#ifdef OTHER_ENVIRONMENT_RELEASE
  #define OTHER_RELEASE_BUILD
#endif  // !OTHER_RELEASE

#ifdef OTHER_ENVIRONMENT_PROFILE
  #define OTHER_PROFILE_BUILD
#endif  // !OTHER_PROFILE

#ifdef OTHER_ENVIRONMENT_PROFILED
  #define OTHER_PROFILED_BUILD
#endif  // !OTHER_PROFILED

#ifdef OTHER_ABORT_USE_STD_TERMINATE
  #include <cstdlib>
  #define OTHER_ABORT() std::terminate()
#else
  #define OTHER_ABORT() std::abort()
#endif  // !OTHER_ABORT_USE_STD_TERMINATE

#ifndef OTHER_API
  #error "OTHER_API is not defined. Please define it for your platform."
#endif  // !OTHER_API
#ifndef OTHER_CLASS
  #error "OTHER_CLASS is not defined. Please define it for your platform."
#endif  // !OTHER_CLASS
#ifndef OTHER_ALIGN
  #error "OTHER_ALIGN is not defined. Please define it for your platform."
#endif  // !OTHER_ALIGN

#endif  // OTHER_CORE_BUILD_CONFIG_HPP