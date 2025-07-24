#pragma once

#include <windows.h>
#include <stdexcept>
#include <format>
#include <string>
#include <sstream>

#include <d3d11.h>


#define FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

/**
 * Generic runtime error wrapper for formatted messages.
 */
class error_fmt final : public std::runtime_error
{
public:
    template <typename... Args>
    explicit error_fmt(std::format_string<Args...> fmt, Args&&... args) :
        std::runtime_error{ std::format(fmt, FWD(args)...) } {}
};

/**
 * Generic runtime error wrapper for win32 errors.
 */
class win32_error : public std::runtime_error {
public:
    win32_error(const std::string& user_message, DWORD err = ::GetLastError())
        : std::runtime_error(compose_message(user_message, err)), code_(err) {}

    DWORD code() const noexcept { return code_; }

private:
    DWORD code_;

    static std::string compose_message(const std::string& user, DWORD err) {
        std::ostringstream oss;
        oss << user;

        if (err != 0) {
            oss << " (Win32 error " << err << ": ";

            char* buf = nullptr;
            DWORD len = ::FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr, err, 0, reinterpret_cast<LPSTR>(&buf), 0, nullptr);

            if (len && buf) {
                // Remove trailing newlines (FormatMessage always adds \r\n)
                std::string msg(buf, len);
                while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) msg.pop_back();
                oss << msg;
                ::LocalFree(buf);
            }
            else {
                oss << "Unknown error";
            }
            oss << ")";
        }

        return oss.str();
    }
};

class d3d_error : public std::runtime_error {
public:
    d3d_error(const std::string& user, HRESULT hr)
        : std::runtime_error(compose_message(user, hr)), hr_(hr) {}

    HRESULT hr() const noexcept { return hr_; }

private:
    HRESULT hr_;

    static std::string compose_message(const std::string& user, HRESULT hr) {
        std::ostringstream oss;
        oss << user << " (HRESULT=0x" << std::hex << hr << ")";
        return oss.str();
    }
};