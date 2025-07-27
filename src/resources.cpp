#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <ranges>
#include <algorithm>
#include <span>
#include <string_view>
#include <fstream>

#include <d3d11.h>
#include "imgui.h"

#include "resources.hpp"
#include "globals.hpp"
#include "defer.hpp"
#include "exceptions.hpp"

using namespace resources;

auto Resource::load() -> void
{
    if (this->m_data)
        throw error_fmt("Resource '{}' is already loaded!", this->m_name);
    if (!g_hInstance)
        throw error_fmt("g_hInstance is uninitialized!");

    HRSRC hRsrc = FindResource(g_hInstance, MAKEINTRESOURCE(this->m_tag), RT_RCDATA);
    if (!hRsrc)
        throw error_fmt("Failed to find resource '{}'!", this->m_name);

    HGLOBAL hGlob = ::LoadResource(g_hInstance, hRsrc);
    if (!hGlob)
        throw error_fmt("Failed to load resource '{}'!", this->m_name);

    this->m_data = reinterpret_cast<uint8_t*>(::LockResource(hGlob));
    this->m_size = ::SizeofResource(g_hInstance, hRsrc);
}

auto Resource::data() const -> std::span<uint8_t>
{
    if (this->m_data == nullptr)
        throw error_fmt("Resource::data() called before resource was loaded ('{}')", this->m_name);
    return { reinterpret_cast<uint8_t*>(this->m_data), static_cast<size_t>(this->m_size) };
}

auto Resource::bind_texture() -> void
{
    if (this->m_data == nullptr)
        throw error_fmt("Resource::bind_texture() called before resource was loaded ('{}')", this->m_name);
    if (this->m_tex)
        throw error_fmt("Resource::bind_texture() called more than once ('{}')", this->m_name);
    this->m_tex = Texture::from_buffer(this->data());
}

[[nodiscard]] auto Resource::tex() const -> Texture*
{
    return this->m_tex.get();
}

auto Resource::name() const -> std::string_view
{
    return this->m_name;
}

auto Resource::tag() const -> int32_t
{
    return this->m_tag;
}

Texture::~Texture()
{
    if (m_srv) m_srv->Release();
}

Texture::Texture(Texture&& other) noexcept
    : m_width(other.m_width), m_height(other.m_height), m_srv(other.m_srv)
{
    other.m_width = 0;
    other.m_height = 0;
    other.m_srv = nullptr;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        if (m_srv)
            m_srv->Release();

        m_width = other.m_width;
        m_height = other.m_height;
        m_srv = other.m_srv;

        other.m_width = 0;
        other.m_height = 0;
        other.m_srv = nullptr;
    }
    return *this;
}

[[nodiscard]] auto Texture::from_buffer(std::span<uint8_t> buffer) -> std::unique_ptr<Texture>
{
    ID3D11ShaderResourceView* srv{ nullptr };
    int width, height;

    uint8_t* image_data = stbi_load_from_memory(buffer.data(), buffer.size(), &width, &height, nullptr, 4);
    if (image_data == nullptr)
        throw std::runtime_error("stbi_load_from_memory failed!");
    defer{ stbi_image_free(image_data); };

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;

    HRESULT hr;
    ID3D11Texture2D* pTexture = nullptr;
    hr = g_dx11_dev->CreateTexture2D(&desc, &subResource, &pTexture);
    if (FAILED(hr))
        throw std::runtime_error("CreateTexture2D failed!");
    defer{ pTexture->Release(); };

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    hr = g_dx11_dev->CreateShaderResourceView(pTexture, &srvDesc, &srv);
    if (FAILED(hr))
        throw std::runtime_error("CreateShaderResourceView failed! HRESULT = " + std::to_string(hr));

    return std::make_unique<Texture>(width, height, srv);
}

[[nodiscard]] auto Texture::from_file(std::string_view path) -> std::unique_ptr<Texture>
{
    std::ifstream file(path.data(), std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Could not open file");

    std::streamsize size = file.tellg();
    if (size < 0)
        throw std::runtime_error("File size error");
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        throw std::runtime_error("File read error");

    return Texture::from_buffer(buffer);
}

auto resources::load_all() -> void
{
    for (auto& resource : assets::all_resources)
    {
        resource->load();
        if (RESOURCE_IS_TEXTURE(resource->tag()))
        {
            resource->bind_texture();
        }
    }
}
