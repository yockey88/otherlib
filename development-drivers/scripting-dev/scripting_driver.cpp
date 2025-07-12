/**
 * \file scripting-dev/scripting_driver.cpp
 **/
#include "scripting-dev/scripting_driver.hpp"

#include <dotnet/coreclr_delegates.h>
#include <dotnet/hostfxr.h>
#include <dotnet/nethost.h>

// #ifdef DOTOTHER_WINDOWS
#include <ShlObj_core.h>
#include <Windows.h>
#define DOTOTHER_CALLTYPE __cdecl
#define DOTOTHER_HOSTFXR_NAME "hostfxr.dll"

#ifdef _WCHAR_T_DEFINED
  #define DOTOTHER_WIDE_CHARS
  #define DNET_STR(s) L##s
#else
  #define DNET_STR(s) s
#endif  // _WCHAR_T_DEFINED
// #endif

#define DOTOTHER_DOTNET_TARGET_VERSION_MAJOR 9
#define DOTOTHER_DOTNET_TARGET_VERSION_MAJOR_STR '9'
#define DOTOTHER_UNMANAGED_FUNCTION UNMANAGEDCALLERSONLY_METHOD

namespace other {
  using dnet_char = char_t;

  // enum class managed_type {
  //   UNKNOWN,

  //   SBYTE,
  //   BYTE,
  //   SHORT,
  //   USHORT,
  //   INT,
  //   UINT,
  //   LONG,
  //   ULONG,

  //   FLOAT,
  //   DOUBLE,

  //   BOOL,

  //   POINTER,
  // };

  // enum class assembly_load_status {
  //   SUCCESS,
  //   FILE_NOT_FOUND,
  //   FILE_LOAD_FAILED,
  //   INVALID_FILE_PATH,
  //   INVALID_ASSEMBLY,
  //   CORRUPT_ASSEMBLY,
  //   UNKNOWN_ERROR,
  // };

  // enum class type_accessibility {
  //   PUBLIC,
  //   PRIVATE,
  //   PROTECTED,
  //   INTERNAL,
  //   PROTECTED_PUBLIC,
  //   PRIVATE_PROTECTED
  // };

  // namespace {

  //   std::string wstr_to_str(const dnet_char* str) {
  //     if constexpr (std::is_same_v<dnet_char, wchar_t>) {
  //       std::wstring wstr(str);
  //       return std::string(wstr.begin(), wstr.end());
  //     } else {
  //       return std::string((const char*)str);
  //     }
  //   }

  //   enum class MessageLevel {
  //     TRACE = 0,
  //     DEBUG = 1,
  //     INFO = 2,
  //     WARNING = 3,
  //     ERR = 4,
  //     CRITICAL = 5,

  //     MESSAGE = 6,
  //   };

  //   class NString {
  //    public:
  //     static NString New(const char* str) {
  //       NString result;
  //       result.Assign(str);
  //       return result;
  //     }

  //     static NString New(std::string_view str) {
  //       NString result;
  //       result.Assign(str);
  //       return result;
  //     }

  //     static void Free(NString& str) {
  //       if (str.string == nullptr)
  //         return;

  //       // Memory::FreeCoTaskMem(str.string);
  //       str.string = nullptr;
  //     }

  //     static void Assign(std::string_view str) {
  //       // if (string != nullptr)
  //       //   Memory::FreeCoTaskMem(string);

  //       // string = Memory::NStringToCoTaskMemAuto(util::CharToWide(str));
  //     }

  //     operator std::string() const {
  //       if (string == nullptr) {
  //         return "";
  //       }

  //       //         dostring_view str(string);
  //       //         return
  //       // #ifdef _WIN32
  //       //           util::WideToChar(str);
  //       // #else
  //       //           std::string(str);
  //       // #endif  // _WIN32
  //     }

  //     bool operator==(const NString& InOther) const {
  //       if (string == InOther.string)
  //         return true;

  //       if (string == nullptr || InOther.string == nullptr)
  //         return false;

  //       return wcscmp(string, InOther.string) == 0;
  //     }

  //     bool operator==(std::string_view InOther) const {
  //       // auto str = NStringHelper::ConvertUtf8ToWide(InOther);
  //       // return wcscmp(m_NString, str.data()) == 0;
  //       return false;
  //     }

  //     wchar_t* Data() {
  //       return string;
  //     }

  //     const wchar_t* Data() const {
  //       return string;
  //     }

  //    private:
  //     wchar_t* string = nullptr;
  //     uint32_t disposed = false;
  //   };

  // }  // namespace

  void scripting_driver::on_initialize() {
    CORE_LOG_DEBUG("Initializing scripting driver...");

    dotnet.load_host();
    dotnet.call_entry_point();
  }

  void scripting_driver::run() {
    CORE_LOG_DEBUG("Running scripting driver...");

    std::ifstream file{ "build/development-drivers/script-testing/csharp/Debug/DotnetTesting.dll", std::ios::binary };
    if (!file.is_open()) {
      CORE_LOG_ERROR("Failed to open DotnetTesting.dll");
      return;
    }

    std::vector<uint8_t> assembly_data{};
    {
      size_t size = 0;
      file.seekg(0, std::ios::end);
      size = file.tellg();
      file.seekg(0, std::ios::beg);

      assembly_data.resize(size);
      file.read(reinterpret_cast<char*>(assembly_data.data()), size);
      file.close();
    }
    {
      std::stringstream ss;
      ss << "Loaded assembly data size: " << assembly_data.size() << " bytes\n";

      /// print out assembly in classic hexdump format
      ss << "Assembly Data:\n";
      ss << std::format("|{:->92}|\n|", "");
      for (size_t i = 0; i < assembly_data.size(); ++i) {
        if (i % 16 == 0 && i != 0) {
          ss << "|\n|";
        }
        if (i % 16 == 0) {
          ss << std::format(" {:0>08x} | ", i);
        }

        ss << std::format("{:>#04x} ", assembly_data[i]);
      }

      ss << "|\n";
      ss << std::format("|{:-<92}|\n", "");
      ss << "End of assembly data.\n";

      CORE_LOG_DEBUG("{}", ss.str());
    }
  }

  void scripting_driver::on_shutdown() {
    dotnet.unload_host();

    CORE_LOG_DEBUG("Scripting driver shut down.");
  }

  void scripting_driver::on_event(SDL_Event* event) {
    // Event handling code here
  }

}  // namespace other