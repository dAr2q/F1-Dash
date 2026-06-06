
// File: PacketCarTelemetryData2.cpp
#include "PacketCarTelemetryData2.h"
#include <inttypes.h>
#include <cstring>

const int CARTELEMETRY2_BUFFER_SIZE = 269;

PacketCarTelemetryData2::PacketCarTelemetryData2()
: PHeader()
{}

PacketCarTelemetryData2::~PacketCarTelemetryData2()
{}

void PacketCarTelemetryData2::push(char *receiveBuffer)
{
    std::memcpy(PHeader::firstElementPointer(), receiveBuffer, CARTELEMETRY2_BUFFER_SIZE);
}


CarTelemetryData2 PacketCarTelemetryData2::m_carTelemetryData2(int index)
{
    if (index >= 0 && index < 24)
        return m_carTelemetryData2_[index];
    else return CarTelemetryData2{};
}