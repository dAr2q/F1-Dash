// File: PacketCarTelemetryData2.h
#ifndef PACKETCARTELEMETRYDATA2_H
#define PACKETCARTELEMETRYDATA2_H

#include "PHeader.h"
#include <inttypes.h>

#pragma pack(push, 1)

struct CarTelemetryData2
{
    uint8_t m_activeAeroMode; // Active aero mode, 0 = Corner Mode, 1 = Straight Mode
    uint8_t m_activeAeroAvailable; // Active aero availability, 0 = Unavailable, 1 = Available
    uint16_t m_activeAeroActivationDistance; // 0 = Active aero not available, >0 = Distance in metres until active aero will be available.
    uint8_t m_overtakeAvailable; // Overtake availability, 0 = Unavailable, 1 = Available
    uint8_t m_overtakeActive; // Overtake activation, 0 = Not activated
    uint16_t m_overtakeActivationDistance; // 0 = Overtake not available, >0 = Distance in metres until overtake will be available
    uint8_t m_2026Regulations; // 0 = pre-2026 regulations, 1 = 2026 regulations
    uint8_t m_drivingWrongWay; // 0 = Driving the correct way, 1 = Driving the wrong way
}; 

class PacketCarTelemetryData2 : public PHeader
{
public:
    PacketCarTelemetryData2();
    virtual ~PacketCarTelemetryData2();
    CarTelemetryData2 m_carTelemetryData2(int index);
    // 0 if no gear suggested
    void push(char *receiveBuffer);

private:
    CarTelemetryData2 m_carTelemetryData2_[24];
};
#pragma pack(pop)

#endif 

