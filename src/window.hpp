#pragma once

#include <span>
#include <string_view>

#include "imgui.h"

namespace window
{
    /**
     * @brief Non-owning handle for accessing binary resources embedded via RC_DATA.
     *
     * The Resource class provides convenient, type-safe access to static binary resources
     * embedded in the executable (via Windows .rc files and RT_RCDATA). Each resource is
     * identified by an integer tag (resource ID) as specified in resource_def.h and assets.rc.
     *
     * This class does not own or manage resource memory. The loaded data is backed by the
     * program image and remains valid for the entire process lifetime; unloading or manual
     * freeing is not required or supported.
     *
     * Usage:
     *   Resource res(RESOURCE_MY_DATA);
     *   if (res.load()) {
     *       auto bytes = res.data();
     *       // Use bytes as needed
     *   }
     */
    class Resource
    {
    private:
        int32_t m_tag = -1;
        void* m_data = nullptr;
        int32_t m_size = -1;

    public:
        explicit Resource(int32_t tag) : m_tag(tag) {}
        Resource(const Resource&) = delete;
        Resource& operator=(const Resource&) = delete;
        Resource(Resource&&) noexcept = default;
        Resource& operator=(Resource&&) noexcept = default;

        /**
         * @brief Loads the specified resource from the executable.
         * @return true if the resource was found and loaded successfully; false otherwise.
         *
         * @note Safe to call once per Resource instance. Multiple calls without resetting are undefined.
         */
        auto load() -> void;

        /**
         * @brief Returns a span over the loaded resource data.
         * @return std::span to the immutable resource data bytes.
         * @throws std::logic_error if called before successful load().
         */
        auto data() const -> std::span<uint8_t>;
    };


    /**
     * @brief Simple RAII wrapper for a D3D11 shader resource view (texture).
     *
     * Provides static factory methods for loading a texture from a memory buffer or a file.
     * The texture is represented as an ID3D11ShaderResourceView*, along with its width and height.
     *
     * Instances of this class can only be created via the static factory methods, enforcing
     * proper initialization. The class manages the lifetime of the shader resource view,
     * releasing it when the Texture is destroyed.
     *
     * Example usage:
     *   auto tex = Texture::from_file("foo.png");
     *   ImVec2 dims = tex.size();
     */
    class Texture
    {
    private:
        int m_width = 0;
        int m_height = 0;
        ID3D11ShaderResourceView* m_srv = nullptr;
        
        Texture() = default;
        Texture(int w, int h, ID3D11ShaderResourceView* d) : m_width(w), m_height(h), m_srv(d) {}
    public:
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;
        ~Texture();

        static auto from_buffer(std::span<uint8_t> buffer) -> Texture;
        static auto from_file(std::string_view path) -> Texture;

        ID3D11ShaderResourceView* srv() const noexcept { return m_srv; }
        auto width() const -> float { return float(m_width); }
        auto height() const -> float { return float(m_height); }
        auto size() const -> ImVec2 { return { float(m_width), float(m_height) }; }
    };

    struct TouchAnimation {
        ImVec2 pos;
        int start_frame;
    };

    auto find_iidx() -> void;
    auto create_device() -> void;
    auto init_imgui() -> void;
    auto init_touch() -> void;
    auto init_resources() -> void;
    auto cleanup() -> void;

    auto process_touch() -> void;
    auto render_touch_animations() -> void;
}