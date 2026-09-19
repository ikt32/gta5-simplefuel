#pragma once

#include <cstdint>
#include <vector>

namespace mem {
void Init();
uintptr_t FindPattern(const char* pattern, const char* mask);
uintptr_t FindPattern(const char* pattern);
uintptr_t GetEntityAddress(int entity);
}
