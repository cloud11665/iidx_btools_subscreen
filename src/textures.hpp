#pragma once

bool LoadTextureFromFile(
    IDirect3DDevice9* device,
    const char* filename,
    IDirect3DTexture9** out_texture,
    int* out_width,
    int* out_height
);