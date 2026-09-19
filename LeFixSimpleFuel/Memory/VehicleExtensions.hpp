#pragma once

#include <inc/types.h>

#include <cstdint>
#include <vector>

class VehicleExtensions {
public:
    VehicleExtensions();

    BYTE* GetAddress(Vehicle handle);
    float GetCurrentRPM(Vehicle handle);
    float GetFuelLevel(Vehicle handle);
    void SetFuelLevel(Vehicle handle, float value);
    uint64_t GetHandlingPtr(Vehicle handle);
    float GetPetrolTankVolume(Vehicle handle);
    uint16_t GetGearCurr(Vehicle handle);
    float GetThrottle(Vehicle handle);
    uint8_t GetTopGear(Vehicle handle);
    bool GetHandbrake(Vehicle handle);
    std::vector<uint32_t> GetVehicleFlags(Vehicle handle);

    void GetOffsets();
};
