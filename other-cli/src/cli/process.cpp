/**
 * \file cli/process.cpp
 **/
#include "cli/process.hpp"

#include <array>
#include <format>

#ifdef OTHER_ENVIRONMENT_WINDOWS
  #include <windows.h>
#endif

namespace other {
  namespace cli {
    namespace {

      /// standard windows argv quoting: wrap when needed, backslash-escape embedded
      ///  quotes and the backslash runs that precede them
      std::string quote_argument(std::string_view arg) {
        const bool needs_quotes = arg.empty() || arg.find_first_of(" \t\"") != std::string_view::npos;
        if (!needs_quotes) {
          return std::string{ arg };
        }

        std::string quoted = "\"";
        size_t backslashes = 0;
        for (const char c : arg) {
          if (c == '\\') {
            ++backslashes;
            continue;
          }

          if (c == '"') {
            quoted.append(backslashes * 2 + 1, '\\');
            quoted.push_back('"');
          } else {
            quoted.append(backslashes, '\\');
            quoted.push_back(c);
          }
          backslashes = 0;
        }
        quoted.append(backslashes * 2, '\\');
        quoted.push_back('"');
        return quoted;
      }

#ifdef OTHER_ENVIRONMENT_WINDOWS
      std::wstring widen(std::string_view text) {
        if (text.empty()) {
          return L"";
        }

        const int required = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (required <= 0) {
          return L"";
        }

        std::wstring wide(static_cast<size_t>(required), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), required);
        return wide;
      }
#endif

    }  // namespace

    std::string format_command_line(const process_launch& launch) {
      std::string command = quote_argument(launch.executable.string());
      for (const std::string& arg : launch.arguments) {
        command.push_back(' ');
        command.append(quote_argument(arg));
      }
      return command;
    }

#ifdef OTHER_ENVIRONMENT_WINDOWS

    opt<filepath> find_program_on_path(std::string_view name) {
      std::array<wchar_t, 4096> buffer = {};
      const std::wstring wide_name = widen(name);
      const DWORD length = SearchPathW(nullptr, wide_name.c_str(), L".exe", static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
      if (length == 0 || length >= buffer.size()) {
        return std::nullopt;
      }
      return filepath(std::wstring_view(buffer.data(), length));
    }

    process_result launch_process(const process_launch& launch) {
      process_result result;
      if (launch.executable.empty()) {
        result.error = "no executable given";
        return result;
      }

      std::wstring command_line = widen(format_command_line(launch));
      std::wstring working_directory = launch.working_directory.empty() ? L"" : launch.working_directory.wstring();

      STARTUPINFOW startup_info = {};
      startup_info.cb = sizeof(startup_info);
      PROCESS_INFORMATION process_info = {};

      DWORD creation_flags = CREATE_UNICODE_ENVIRONMENT;
      if (!launch.wait_for_exit && launch.new_console) {
        creation_flags |= CREATE_NEW_CONSOLE;
      }

      /// CreateProcessW may scribble on the command line buffer, hence the mutable copy
      const BOOL created = CreateProcessW(
        launch.executable.wstring().c_str(),
        command_line.data(),
        nullptr, nullptr, FALSE,
        creation_flags,
        nullptr,
        working_directory.empty() ? nullptr : working_directory.c_str(),
        &startup_info, &process_info);

      if (created == FALSE) {
        result.error = std::format("CreateProcess failed for '{}' (error {})", launch.executable.string(), GetLastError());
        return result;
      }

      result.started = true;
      if (launch.wait_for_exit) {
        WaitForSingleObject(process_info.hProcess, INFINITE);
        DWORD exit_code = 0;
        if (GetExitCodeProcess(process_info.hProcess, &exit_code) != FALSE) {
          result.exit_code = static_cast<int32_t>(exit_code);
        }
      }

      CloseHandle(process_info.hThread);
      CloseHandle(process_info.hProcess);
      return result;
    }

#else

    opt<filepath> find_program_on_path(std::string_view name) {
      (void)name;
      return std::nullopt;
    }

    process_result launch_process(const process_launch& launch) {
      process_result result;
      result.error = "process launching is not implemented on this platform";
      return result;
    }

#endif

  }  // namespace cli
}  // namespace other
