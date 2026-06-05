// File: PacketLapPositions.h
#ifndef PACKETLAPPOSITIONS_H
#define PACKETLAPPOSITIONS_H

#include "PHeader.h"
#include <inttypes.h>

#pragma pack(push, 1)

static const uint8_t cs_maxNumLapsInLapPositionsHistoryPacket = 50;

class PacketLapPositionsData : public PHeader
{
public:
    PacketLapPositionsData();
    virtual ~PacketLapPositionsData();
    uint8_t m_numLaps(void);
    uint8_t m_lapStart(void);
    uint8_t m_lapPositionsData(int lapIndex, int carIdx);
    void push(char *receiveBuffer);

private:
    uint8_t m_numLaps_;
    uint8_t m_lapStart_;
    uint8_t m_lapPositionsData_[cs_maxNumLapsInLapPositionsHistoryPacket][cs_maxNumCarsInUDPData]; //Players car only
};
#pragma pack(pop)

#endif