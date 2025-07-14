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

#pragma pack(push, 1)
  struct ms_dos_header {
    uint8_t expected_bytes[128] = {};
    uint32_t get_lfanew() { return *reinterpret_cast<uint32_t*>(&expected_bytes[0x3c]); }
  };
  static_assert(sizeof(ms_dos_header) == 128, "incorrect ms-dos header");

  struct pe_file_header {
    uint16_t machine = 0x00;
    uint16_t num_sections = 0;
    uint32_t time_stamp = 0;
    uint32_t pointer_to_sym = 0;
    uint32_t num_symbols = 0;
    uint16_t optional_header_size;
    uint16_t characteristics;

    // 0 2 Machine Always 0x14c.
    // 2 2 Number of Sections Number of sections; indicates size of the Section Table, which immediately follows the headers.
    // 4 4 Time/Date Stamp Time and date the file was created in seconds since January 1st 1970 00:00:00 or 0.
    // 8 4 Pointer to Symbol Table Always 0 (§II.24.1).
    // 12 4 Number of Symbols Always 0 (§II.24.1).
    // 16 2 Optional Header Size Size of the optional header, the format is described below.
    // 18 2 Characteristics Flags indicating attributes of the file, see §II.25.2.2.1.
  };

  struct pe_optional_header {
    uint8_t standard_fields[28];
    uint8_t mt_specific_fields[68];
    uint8_t data_directories[128];

    // 0 28 Standard fields These define general properties of the PE file, see §II.25.2.3.1.
    // 28 68 NT-specific fields These include additional fields to support specific features of Windows, see II.25.2.3.2.
    // 96 128 Data directories These fields are address/size pairs for special tables, found in the image file (for example, Import Table and Export Table).
  };

  struct cli_header {
    uint8_t signature_magic[16];
    uint8_t version[16];
    uint32_t user_entry_point;
    uint32_t count_of_methods;
    uint32_t count_of_scopes;
    uint32_t count_of_vars;
    uint32_t count_of_using;
    uint32_t count_of_constants;
    uint32_t count_of_documents;
    uint32_t count_of_sequence_points;
    uint32_t count_of_misc_bytes;
    uint32_t count_of_string_bytes;
  };
#pragma pack(pop)

  static_assert(sizeof(cli_header) == 72, "Header size must be 72 bytes for common language interface assemblies.");

  void scripting_driver::on_initialize() {
    CORE_LOG_DEBUG("Initializing scripting driver...");

    dotnet.load_host();
    dotnet.call_entry_point();
  }

  void scripting_driver::run() {
    CORE_LOG_DEBUG("Running scripting driver...");

    std::ifstream file{ "build/development-drivers/script-testing/csharp/Debug/DotnetTesting.dll" };
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
      uint8_t* cursor = assembly_data.data();
      ms_dos_header msdos = *reinterpret_cast<ms_dos_header*>(cursor);

      uint32_t lfanew = msdos.get_lfanew();
      CORE_LOG_DEBUG("lfanew = {}", lfanew);
      cursor = assembly_data.data() + lfanew + 4;  // skips PE/0/0

      pe_file_header peheader = *reinterpret_cast<pe_file_header*>(cursor);

      CORE_LOG_DEBUG(
        "pe-header :\nMachine = {:#06x}\nNumber of Sections = {} \nTimeStamp = {}\nPointer to Symbol Table = {}\nNumber of Symbols = {}\nOptional Header Size = {}\nCharacteristics = {}",
        peheader.machine,
        peheader.num_sections,
        peheader.time_stamp,
        peheader.pointer_to_sym,
        peheader.num_symbols,
        peheader.optional_header_size,
        peheader.characteristics
      );
    }

    /// hex dump of the assembly data
    {
      std::stringstream ss;
      ss << "Loaded assembly data size: " << assembly_data.size() << " bytes\n";

      /// print out assembly in classic hexdump format
      ss << "Assembly Data:\n";

      ss << std::format("|{:->92}|\n|          | ", "");
      for (size_t i = 0; i < 16; ++i) {
        ss << std::format("{:>#04x} ", i);
      }
      ss << "|\n";

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

}  // namespace other