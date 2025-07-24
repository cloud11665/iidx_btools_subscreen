#pragma once

#include <utility>
#include <type_traits>

class _Defer {
public:
    // Just a tag type for macro
    constexpr _Defer() noexcept = default;

    template <typename Fn>
    struct Scope {
        Fn fn;
        bool active = true;
        Scope(Fn&& f) : fn(std::move(f)) {}
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&& other) noexcept
            : fn(std::move(other.fn)), active(other.active) {
            other.active = false;
        }
        ~Scope() { if (active) fn(); }
        void dismiss() noexcept { active = false; }
    };

    // Overload: _Defer{} && lambda
    template <typename Fn>
    friend auto operator&&(_Defer, Fn&& fn) {
        return Scope<std::decay_t<Fn>>(std::forward<Fn>(fn));
    }
};

// Macro for unique variable name
#define _DEFER_CONCAT(a, b) a##b
#define _DEFER_MAKE_NAME(a, b) _DEFER_CONCAT(a, b)
#define defer \
    auto _DEFER_MAKE_NAME(_defer_, __COUNTER__) = _Defer{} && [&]()
