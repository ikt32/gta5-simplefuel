#include "NativeMemory.hpp"

#include "../Util/Logger.hpp"
#include "../Util/Versions.h"

#include <inc/main.h>

#include <Windows.h>
#include <Psapi.h>

#include <cstdlib>
#include <cstring>
#include <iterator>
#include <sstream>
#include <string>

namespace {
template <typename Out>
void Split(const std::string& value, char delimiter, Out result) {
    std::stringstream stream(value);
    std::string item;
    while (std::getline(stream, item, delimiter)) {
        *(result++) = item;
    }
}

std::vector<std::string> Split(const std::string& value, char delimiter) {
    std::vector<std::string> elements;
    Split(value, delimiter, std::back_inserter(elements));
    return elements;
}

HMODULE scriptHookModule = nullptr;
BYTE* (*scriptHookGetAddress)(int handle) = nullptr;
bool addressFailureLogged = false;
}

namespace mem {
using GetAddressOfEntity = uintptr_t(__fastcall*)(int handle);
GetAddressOfEntity gameGetAddress = nullptr;

void InitScriptHookAddressGetter() {
    scriptHookModule = GetModuleHandleW(L"ScriptHookV.dll");
    if (!scriptHookModule) {
        logger.Write("[Memory] ScriptHookV.dll was not found");
        return;
    }

    scriptHookGetAddress = reinterpret_cast<BYTE* (*)(int)>(
        GetProcAddress(scriptHookModule, "?getScriptHandleBaseAddress@@YAPEAEH@Z"));
    if (!scriptHookGetAddress) {
        logger.Write("[Memory] ScriptHookV getScriptHandleBaseAddress export was not found");
    }
}

void InitGameAddressGetter() {
    uintptr_t address = 0;

    if (!Versions::IsEnhanced()) {
        if (getGameVersion() >= Versions::L_1_0_3788_0) {
            address = FindPattern(
                "\x85\xED\x74\x0F\x8B\xCD\xE8\x00\x00\x00\x00\x48\x8B\xF8\x48\x85\xC0\x74\x2E",
                "xxxxxxx????xxxxxxxx");
            gameGetAddress = address
                ? reinterpret_cast<GetAddressOfEntity>(address + 11 + *reinterpret_cast<int*>(address + 7))
                : nullptr;
        }
        else {
            address = FindPattern(
                "\x83\xF9\xFF\x74\x31\x4C\x8B\x0D\x00\x00\x00\x00\x44\x8B\xC1\x49\x8B\x41\x08",
                "xxxxxxxx????xxxxxxx");
            gameGetAddress = address ? reinterpret_cast<GetAddressOfEntity>(address) : nullptr;
        }
    }
    else if (getGameVersion() >= Versions::E_1_0_1013_33) {
        address = FindPattern("41 8B 4C 1C ? E8");
        gameGetAddress = address
            ? reinterpret_cast<GetAddressOfEntity>(address + 10 + *reinterpret_cast<int*>(address + 6))
            : nullptr;
    }
    else {
        address = FindPattern("83 F9 FF 74 64 41 89 C8");
        gameGetAddress = address ? reinterpret_cast<GetAddressOfEntity>(address) : nullptr;
    }

    if (address) {
        logger.Writef("[Memory] Found GetAddressOfEntity at 0x%llX", static_cast<unsigned long long>(address));
    }
    else {
        logger.Write("[Memory] Could not find GetAddressOfEntity; ScriptHookV fallback will be used");
    }
}

void Init() {
    InitGameAddressGetter();
    InitScriptHookAddressGetter();
}

uintptr_t FindPattern(const char* pattern, const char* mask) {
    MODULEINFO moduleInfo{};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(nullptr), &moduleInfo, sizeof(moduleInfo));

    const auto* start = static_cast<const char*>(moduleInfo.lpBaseOfDll);
    const auto size = static_cast<uintptr_t>(moduleInfo.SizeOfImage);
    intptr_t position = 0;
    const auto searchLength = static_cast<uintptr_t>(strlen(mask) - 1);

    for (const char* candidate = start; candidate < start + size; ++candidate) {
        if (*candidate == pattern[position] || mask[position] == '?') {
            if (mask[position + 1] == '\0') {
                return reinterpret_cast<uintptr_t>(candidate) - searchLength;
            }
            ++position;
        }
        else {
            position = 0;
        }
    }
    return 0;
}

uintptr_t FindPattern(const char* pattern) {
    const auto tokens = Split(pattern, ' ');
    if (tokens.empty()) {
        return 0;
    }

    MODULEINFO moduleInfo{};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(nullptr), &moduleInfo, sizeof(moduleInfo));

    auto* start = static_cast<uint8_t*>(moduleInfo.lpBaseOfDll);
    const auto size = static_cast<uintptr_t>(moduleInfo.SizeOfImage);
    std::vector<uint8_t> bytes;
    bytes.reserve(tokens.size());
    for (const auto& token : tokens) {
        bytes.push_back(token == "?" || token == "??"
            ? 0
            : static_cast<uint8_t>(std::strtoul(token.c_str(), nullptr, 16)));
    }

    uintptr_t position = 0;
    for (auto* candidate = start; candidate < start + size; ++candidate) {
        if (tokens[position] == "?" || tokens[position] == "??" || *candidate == bytes[position]) {
            if (position + 1 == tokens.size()) {
                return reinterpret_cast<uintptr_t>(candidate) - tokens.size() + 1;
            }
            ++position;
        }
        else {
            position = 0;
        }
    }
    return 0;
}

uintptr_t GetEntityAddress(int entity) {
    if (gameGetAddress) {
        return gameGetAddress(entity);
    }
    if (scriptHookGetAddress) {
        return reinterpret_cast<uintptr_t>(scriptHookGetAddress(entity));
    }

    if (!addressFailureLogged) {
        logger.Write("[Memory] No entity address resolver is available");
        addressFailureLogged = true;
    }
    return 0;
}
}
