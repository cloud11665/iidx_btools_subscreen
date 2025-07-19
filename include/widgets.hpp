#pragma once

#include <cstdint>

void EffectorFader(const char* text_top, const char* text_bottom, int* value, int main_tick);

void Effector(int effector_vals[5]);

int32_t Keypad(int side);

void Ticker16seg(const char* text);

void DrawAll();
