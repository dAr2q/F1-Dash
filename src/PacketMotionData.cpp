// File: PacketMotionData.cpp
#include "PacketMotionData.h"
#include <cstring>

const int MOTION_BUFFER_SIZE = 1325;

PacketMotionData::PacketMotionData()
: PHeader()
{}

PacketMotionData::~PacketMotionData()
{}

void PacketMotionData::push(char *receiveBuffer)
{
    std::memcpy(PHeader::firstElementPointer(), receiveBuffer, MOTION_BUFFER_SIZE);
}

CarMotionData PacketMotionData::m_carMotionData(int index)
{
    if (index < 24 && index >= 0)
        return m_carMotionData_[index];
    else return CarMotionData{};
}
