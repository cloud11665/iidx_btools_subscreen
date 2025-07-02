#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <d3d9.h>

#include <algorithm>

//------------------------------------------------------------------------------
// Loads an image from memory (PNG/JPEG/etc.) and creates a D3D9 texture.
//   device      - your IDirect3DDevice9*
//   data        - pointer to the image file in memory
//   data_size   - size of that block in bytes
//   out_texture - receives the new IDirect3DTexture9*
//   out_width   - receives image width
//   out_height  - receives image height
//------------------------------------------------------------------------------
bool LoadTextureFromMemory(
    IDirect3DDevice9* device,
    const void* data,
    size_t                 data_size,
    IDirect3DTexture9** out_texture,
    int* out_width,
    int* out_height)
{
    if (!device || !data || !out_texture || !out_width || !out_height)
        return false;

    // Decode with stb_image (force 4 channels: RGBA)
    int w = 0, h = 0;
    unsigned char* pixels = stbi_load_from_memory(
        (const unsigned char*)data,
        (int)data_size,
        &w,
        &h,
        nullptr,
        4
    );
    if (!pixels)
        return false;

    for (int i = 0, n = w * h; i < n; ++i)
    {
        unsigned char* p = pixels + i * 4;
        // p[0]=R, p[1]=G, p[2]=B, p[3]=A
        std::swap(p[0], p[2]);  // now p is B G R A
    }

    // Create a managed texture in A8R8G8B8 format, no mipmaps:
    IDirect3DTexture9* tex = nullptr;
    HRESULT hr = device->CreateTexture(
        w,
        h,
        1,                  // levels (no mips)
        0,                  // usage
        D3DFMT_A8R8G8B8,    // format
        //D3DFMT_R8G8B8,    // format
        D3DPOOL_MANAGED,    // pool
        &tex,
        nullptr
    );
    if (FAILED(hr) || !tex) {
        stbi_image_free(pixels);
        return false;
    }

    // Lock the top level and copy scanlines
    D3DLOCKED_RECT rect;
    hr = tex->LockRect(0, &rect, nullptr, 0);
    if (FAILED(hr)) {
        tex->Release();
        stbi_image_free(pixels);
        return false;
    }

    // rect.Pitch may be >= width*4; copy row by row
    for (int y = 0; y < h; ++y) {
        memcpy(
            (unsigned char*)rect.pBits + y * rect.Pitch,
            pixels + y * w * 4,
            w * 4
        );
    }

    tex->UnlockRect(0);
    stbi_image_free(pixels);

    // Success—return values
    *out_texture = tex;
    *out_width = w;
    *out_height = h;
    return true;
}

//------------------------------------------------------------------------------
// Loads a file into memory and then calls the above.
//------------------------------------------------------------------------------
bool LoadTextureFromFile(
    IDirect3DDevice9* device,
    const char* filename,
    IDirect3DTexture9** out_texture,
    int* out_width,
    int* out_height)
{
    if (!device || !filename || !out_texture || !out_width || !out_height)
        return false;

    FILE* f = fopen(filename, "rb");
    if (!f)
        return false;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz <= 0) {
        fclose(f);
        return false;
    }
    fseek(f, 0, SEEK_SET);

    unsigned char* file_data = (unsigned char*)malloc(sz);
    if (!file_data) {
        fclose(f);
        return false;
    }

    size_t read = fread(file_data, 1, sz, f);
    fclose(f);
    if (read != (size_t)sz) {
        free(file_data);
        return false;
    }

    bool ok = LoadTextureFromMemory(
        device,
        file_data,
        sz,
        out_texture,
        out_width,
        out_height
    );
    free(file_data);
    return ok;
}
