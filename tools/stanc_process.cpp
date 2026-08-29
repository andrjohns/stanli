#include "stanc_process.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace stanli::tooling {
namespace {

#ifdef _WIN32

class Handle {
 public:
  explicit Handle(HANDLE handle = INVALID_HANDLE_VALUE) : handle_(handle) {}
  Handle(const Handle&) = delete;
  Handle& operator=(const Handle&) = delete;
  Handle(Handle&& other) noexcept : handle_(other.release()) {}
  Handle& operator=(Handle&& other) noexcept {
    reset(other.release());
    return *this;
  }
  ~Handle() { reset(); }

  HANDLE get() const { return handle_; }
  HANDLE release() {
    const HANDLE handle = handle_;
    handle_ = INVALID_HANDLE_VALUE;
    return handle;
  }
  void reset(HANDLE handle = INVALID_HANDLE_VALUE) {
    if (handle_ != INVALID_HANDLE_VALUE && handle_ != nullptr)
      CloseHandle(handle_);
    handle_ = handle;
  }

 private:
  HANDLE handle_;
};

std::runtime_error windows_error(const char* operation) {
  return std::runtime_error(std::string(operation) + " failed (Windows error " +
                            std::to_string(GetLastError()) + ")");
}

std::wstring widen_utf8(const std::string& text) {
  if (text.empty()) return {};
  const int size =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                          static_cast<int>(text.size()), nullptr, 0);
  if (size == 0) throw windows_error("MultiByteToWideChar");
  std::wstring wide(static_cast<size_t>(size), L'\0');
  if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                          static_cast<int>(text.size()), wide.data(),
                          size) == 0)
    throw windows_error("MultiByteToWideChar");
  return wide;
}

// CreateProcess receives one command-line string even when the application is
// named separately. Quote each argv element by the CommandLineToArgvW rules so
// spaces and trailing backslashes survive unchanged in the child.
std::wstring quote_windows_arg(const std::wstring& arg) {
  if (arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) return arg;
  std::wstring quoted(1, L'"');
  size_t backslashes = 0;
  for (const wchar_t ch : arg) {
    if (ch == L'\\') {
      ++backslashes;
      continue;
    }
    if (ch == L'"') {
      quoted.append(backslashes * 2 + 1, L'\\');
      quoted.push_back(ch);
    } else {
      quoted.append(backslashes, L'\\');
      quoted.push_back(ch);
    }
    backslashes = 0;
  }
  quoted.append(backslashes * 2, L'\\');
  quoted.push_back(L'"');
  return quoted;
}

std::string run_stanc_windows(const std::string& stanc,
                              const std::string& model) {
  SECURITY_ATTRIBUTES inheritable{};
  inheritable.nLength = sizeof(inheritable);
  inheritable.bInheritHandle = TRUE;

  HANDLE raw_read = INVALID_HANDLE_VALUE;
  HANDLE raw_write = INVALID_HANDLE_VALUE;
  if (!CreatePipe(&raw_read, &raw_write, &inheritable, 0))
    throw windows_error("CreatePipe");
  Handle read_pipe(raw_read);
  Handle write_pipe(raw_write);
  if (!SetHandleInformation(read_pipe.get(), HANDLE_FLAG_INHERIT, 0))
    throw windows_error("SetHandleInformation");

  Handle null_device(CreateFileW(
      L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
      &inheritable, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
  if (null_device.get() == INVALID_HANDLE_VALUE)
    throw windows_error("CreateFileW(NUL)");

  const std::wstring executable = widen_utf8(stanc);
  const std::array<std::wstring, 4> args = {
      executable, L"--O1", L"--debug-optimized-mir", widen_utf8(model)};
  std::wstring command_line;
  for (const auto& arg : args) {
    if (!command_line.empty()) command_line.push_back(L' ');
    command_line += quote_windows_arg(arg);
  }

  STARTUPINFOW startup{};
  startup.cb = sizeof(startup);
  startup.dwFlags = STARTF_USESTDHANDLES;
  startup.hStdInput = null_device.get();
  startup.hStdOutput = write_pipe.get();
  startup.hStdError = null_device.get();
  PROCESS_INFORMATION process{};
  if (!CreateProcessW(executable.c_str(), command_line.data(), nullptr, nullptr,
                      TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startup,
                      &process))
    throw windows_error("CreateProcessW");
  Handle process_handle(process.hProcess);
  Handle thread_handle(process.hThread);
  write_pipe.reset();
  null_device.reset();

  std::string out;
  std::array<char, 1 << 16> buffer;
  for (;;) {
    DWORD read_size = 0;
    if (!ReadFile(read_pipe.get(), buffer.data(),
                  static_cast<DWORD>(buffer.size()), &read_size, nullptr)) {
      if (GetLastError() == ERROR_BROKEN_PIPE) break;
      throw windows_error("ReadFile");
    }
    if (read_size == 0) break;
    out.append(buffer.data(), read_size);
  }
  if (WaitForSingleObject(process_handle.get(), INFINITE) != WAIT_OBJECT_0)
    throw windows_error("WaitForSingleObject");
  // _popen(..., "r") used to give callers a text-mode stream. Preserve that
  // behavior now that ReadFile sees the child's raw CRLF bytes.
  std::string normalized;
  normalized.reserve(out.size());
  for (size_t i = 0; i < out.size(); ++i) {
    if (out[i] == '\r' && i + 1 < out.size() && out[i + 1] == '\n') continue;
    normalized.push_back(out[i]);
  }
  return normalized;
}

#else

std::string run_stanc_posix(const std::string& stanc,
                            const std::string& model) {
  int descriptors[2];
  if (pipe(descriptors) != 0)
    throw std::runtime_error(std::string("pipe failed: ") +
                             std::strerror(errno));

  const pid_t child = fork();
  if (child == -1) {
    const int error = errno;
    close(descriptors[0]);
    close(descriptors[1]);
    throw std::runtime_error(std::string("fork failed: ") +
                             std::strerror(error));
  }
  if (child == 0) {
    close(descriptors[0]);
    if (dup2(descriptors[1], STDOUT_FILENO) == -1) _exit(127);
    close(descriptors[1]);
    const int null_error = open("/dev/null", O_WRONLY);
    if (null_error == -1 || dup2(null_error, STDERR_FILENO) == -1) _exit(127);
    close(null_error);
    execl(stanc.c_str(), stanc.c_str(), "--O1", "--debug-optimized-mir",
          model.c_str(), static_cast<char*>(nullptr));
    _exit(127);
  }

  close(descriptors[1]);
  std::string out;
  std::array<char, 1 << 16> buffer;
  for (;;) {
    const ssize_t read_size =
        read(descriptors[0], buffer.data(), buffer.size());
    if (read_size > 0) {
      out.append(buffer.data(), static_cast<size_t>(read_size));
      continue;
    }
    if (read_size == -1 && errno == EINTR) continue;
    if (read_size == -1) {
      const int error = errno;
      close(descriptors[0]);
      while (waitpid(child, nullptr, 0) == -1 && errno == EINTR) {
      }
      throw std::runtime_error(std::string("read failed: ") +
                               std::strerror(error));
    }
    break;
  }
  close(descriptors[0]);
  while (waitpid(child, nullptr, 0) == -1) {
    if (errno != EINTR)
      throw std::runtime_error(std::string("waitpid failed: ") +
                               std::strerror(errno));
  }
  return out;
}

#endif

}  // namespace

std::string run_stanc_process(const std::string& stanc,
                              const std::string& model) {
#ifdef _WIN32
  return run_stanc_windows(stanc, model);
#else
  return run_stanc_posix(stanc, model);
#endif
}

}  // namespace stanli::tooling
