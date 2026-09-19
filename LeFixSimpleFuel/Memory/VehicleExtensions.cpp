#include "VehicleExtensions.hpp"

#include "NativeMemory.hpp"
#include "Offsets.hpp"
#include "../Util/Logger.hpp"
#include "../Util/Versions.h"

#include <inc/main.h>

namespace {
int vehicleModelInfoFlagsOffset = 0;
int handlingOffset = 0;
int fuelLevelOffset = 0;
int currentGearOffset = 0;
int topGearOffset = 0;
int currentRpmOffset = 0;
int throttleOffset = 0;
int handbrakeOffset = 0;
}

VehicleExtensions::VehicleExtensions() = default;

BYTE* VehicleExtensions::GetAddress(Vehicle handle) {
    return reinterpret_cast<BYTE*>(mem::GetEntityAddress(handle));
}

void VehicleExtensions::GetOffsets() {
    mem::Init();

    uintptr_t address = 0;
    const bool enhanced = Versions::IsEnhanced();

    if (!enhanced) {
        address = mem::FindPattern(
            "\x48\x85\xC0\x74\x3C\x8B\x80\x00\x00\x00\x00\xC1\xE8\x0F",
            "xxxxxxx????xxx");
        vehicleModelInfoFlagsOffset = address ? *reinterpret_cast<int*>(address + 7) : 0;
    }
    else {
        address = mem::FindPattern("48 8B 46 ? F6 80 ? ? 00 00 02 75 ?");
        vehicleModelInfoFlagsOffset = address ? *reinterpret_cast<int*>(address + 6) - 2 : 0;
    }
    logger.Writef("Vehicle Model Info Flags Offset: 0x%X", vehicleModelInfoFlagsOffset);

    if (!enhanced) {
        address = mem::FindPattern("\x74\x26\x0F\x57\xC9", "xxxxx");
        fuelLevelOffset = address ? *reinterpret_cast<int*>(address + 8) : 0;
    }
    else {
        address = mem::FindPattern(
            "83 E0 07 "
            "66 83 F8 03 "
            "74 ? "
            "F3 0F 10 86 ? ? 00 00");
        fuelLevelOffset = address ? *reinterpret_cast<int*>(address + 13) : 0;
    }
    logger.Writef("Fuel Level Offset: 0x%X", fuelLevelOffset);

    int nextGearOffset = 0;
    int gearRatiosOffset = 0;
    if (!enhanced) {
        address = mem::FindPattern(
            "\x48\x8D\x8F\x00\x00\x00\x00\x4C\x8B\xC3\xF3\x0F\x11\x7C\x24",
            "xxx????xxxxxxxx");
        nextGearOffset = address ? *reinterpret_cast<int*>(address + 3) : 0;
        currentGearOffset = nextGearOffset ? nextGearOffset + 2 : 0;
        topGearOffset = nextGearOffset ? nextGearOffset + 6 : 0;
        gearRatiosOffset = nextGearOffset ? nextGearOffset + 8 : 0;
        if (gearRatiosOffset && getGameVersion() >= Versions::L_1_0_3095_0) {
            gearRatiosOffset += 4;
        }
    }
    else {
        address = mem::FindPattern(
            "0F B7 8E ? ? 00 00 "
            "66 3B 8E ? ? 00 00 "
            "74 ?");
        nextGearOffset = address ? *reinterpret_cast<int*>(address + 3) : 0;
        currentGearOffset = nextGearOffset ? nextGearOffset + 2 : 0;
        topGearOffset = nextGearOffset ? nextGearOffset + 6 : 0;
        gearRatiosOffset = nextGearOffset ? nextGearOffset + 12 : 0;
    }
    logger.Writef("Current Gear Offset: 0x%X", currentGearOffset);
    logger.Writef("Top Gear Offset: 0x%X", topGearOffset);

    if (!enhanced) {
        address = mem::FindPattern(
            "\x76\x03\x0F\x28\xF0\xF3\x44\x0F\x10\x93",
            "xxxxxxxxxx");
        currentRpmOffset = address ? *reinterpret_cast<int*>(address + 10) : 0;
        throttleOffset = currentRpmOffset ? currentRpmOffset + 0x10 : 0;
    }
    else {
        const int driveForceOffset = gearRatiosOffset
            ? gearRatiosOffset + 11 * static_cast<int>(sizeof(float))
            : 0;
        const int driveMaxFlatVelocityOffset = driveForceOffset ? driveForceOffset + 0x08 : 0;
        currentRpmOffset = driveMaxFlatVelocityOffset
            ? driveMaxFlatVelocityOffset + 2 * static_cast<int>(sizeof(float))
            : 0;
        throttleOffset = driveMaxFlatVelocityOffset
            ? driveMaxFlatVelocityOffset + 6 * static_cast<int>(sizeof(float))
            : 0;
    }
    logger.Writef("RPM Offset: 0x%X", currentRpmOffset);
    logger.Writef("Throttle Offset: 0x%X", throttleOffset);

    if (!enhanced) {
        address = mem::FindPattern(
            "\x3C\x03\x0F\x85\x00\x00\x00\x00\x48\x8B\x41\x20\x48\x8B\x88",
            "xxxx????xxxxxxx");
        handlingOffset = address ? *reinterpret_cast<int*>(address + 0x16) : 0;
    }
    else {
        address = mem::FindPattern("88 90 ? ? ? 00 0F B7 90 ? 00 00 00");
        handlingOffset = address ? *reinterpret_cast<int*>(address + 2) - 9 : 0;
    }
    logger.Writef("Handling Offset: 0x%X", handlingOffset);

    if (!enhanced) {
        if (getGameVersion() >= Versions::L_1_0_2060_0_STEAM) {
            address = mem::FindPattern("8A C2 24 01 C0 E0 04 08 81");
            handbrakeOffset = address ? *reinterpret_cast<int*>(address + 19) : 0;
        }
        else {
            address = mem::FindPattern(
                "\x44\x88\xA3\x00\x00\x00\x00\x45\x8A\xF4",
                "xxx????xxx");
            handbrakeOffset = address ? *reinterpret_cast<int*>(address + 3) : 0;
        }
    }
    else {
        address = mem::FindPattern(
            "F3 0F 11 BE ? ? 00 00 "
            "48 8B 86 ? ? 00 00 "
            "F3 0F 59 B8 ? 00 00 00 "
            "48 89 DA");
        const int steeringInputOffset = address ? *reinterpret_cast<int*>(address + 4) : 0;
        const int steeringAngleOffset = steeringInputOffset
            ? steeringInputOffset + 2 * static_cast<int>(sizeof(float))
            : 0;
        handbrakeOffset = steeringAngleOffset
            ? steeringAngleOffset + 4 * static_cast<int>(sizeof(float))
            : 0;
    }
    logger.Writef("Handbrake Offset: 0x%X", handbrakeOffset);
}

float VehicleExtensions::GetCurrentRPM(Vehicle handle) {
    const auto address = currentRpmOffset ? GetAddress(handle) : nullptr;
    return address ? *reinterpret_cast<const float*>(address + currentRpmOffset) : 0.0f;
}

float VehicleExtensions::GetFuelLevel(Vehicle handle) {
    const auto address = fuelLevelOffset ? GetAddress(handle) : nullptr;
    return address ? *reinterpret_cast<const float*>(address + fuelLevelOffset) : 0.0f;
}

void VehicleExtensions::SetFuelLevel(Vehicle handle, float value) {
    const auto address = fuelLevelOffset ? GetAddress(handle) : nullptr;
    if (address) {
        *reinterpret_cast<float*>(address + fuelLevelOffset) = value;
    }
}

uint64_t VehicleExtensions::GetHandlingPtr(Vehicle handle) {
    const auto address = handlingOffset ? GetAddress(handle) : nullptr;
    return address ? *reinterpret_cast<const uint64_t*>(address + handlingOffset) : 0;
}

float VehicleExtensions::GetPetrolTankVolume(Vehicle handle) {
    const auto address = GetHandlingPtr(handle);
    return address ? *reinterpret_cast<const float*>(address + hOffsets.fPetrolTankVolume) : 0.0f;
}

uint16_t VehicleExtensions::GetGearCurr(Vehicle handle) {
    const auto address = currentGearOffset ? GetAddress(handle) : nullptr;
    return address ? *reinterpret_cast<const uint16_t*>(address + currentGearOffset) : 0;
}

float VehicleExtensions::GetThrottle(Vehicle handle) {
    const auto address = throttleOffset ? GetAddress(handle) : nullptr;
    return address ? *reinterpret_cast<const float*>(address + throttleOffset) : 0.0f;
}

uint8_t VehicleExtensions::GetTopGear(Vehicle handle) {
    const auto address = topGearOffset ? GetAddress(handle) : nullptr;
    return address ? *reinterpret_cast<const uint8_t*>(address + topGearOffset) : 0;
}

bool VehicleExtensions::GetHandbrake(Vehicle handle) {
    const auto address = handbrakeOffset ? GetAddress(handle) : nullptr;
    return address ? *reinterpret_cast<const bool*>(address + handbrakeOffset) : false;
}

std::vector<uint32_t> VehicleExtensions::GetVehicleFlags(Vehicle handle) {
    std::vector<uint32_t> flags(6);
    if (!vehicleModelInfoFlagsOffset) {
        return flags;
    }

    const auto address = GetAddress(handle);
    if (!address) {
        return {};
    }

    const auto modelInfo = *reinterpret_cast<const uint64_t*>(address + 0x20);
    if (!modelInfo) {
        return {};
    }

    for (size_t index = 0; index < flags.size(); ++index) {
        flags[index] = *reinterpret_cast<const uint32_t*>(
            modelInfo + vehicleModelInfoFlagsOffset + sizeof(uint32_t) * index);
    }
    return flags;
}
