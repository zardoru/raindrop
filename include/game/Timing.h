#pragma once

template <class T, class U>
struct TimedEvent
{
    U Time;

    inline bool operator< (const T &rhs)
    {
        return Time < rhs.Time;
    }

    inline bool operator>(const T &rhs)
    {
        return Time > rhs.Time;
    }

    inline bool operator<(const U &rhs)
    {
        return Time < rhs;
    }

    inline bool operator>(const U &rhs)
    {
        return Time > rhs;
    }

    inline bool compareSegment (const U &lhs, const T &rhs)
    {
        return lhs < rhs.Time;
    }

    inline bool compareTime(const T &lhs, const U &rhs)
    {
        return lhs.Time < rhs;
    }

    operator U() const
    {
        return Time;
    }

    TimedEvent(U val) : Time(val) {};
    TimedEvent() = default;
};

struct TimingSegment : public TimedEvent < TimingSegment, double >
{
    double Value; // in bpm
    TimingSegment(double T, double V) : TimedEvent(T), Value(V) {};
    TimingSegment() : TimingSegment(0, 0) {};
};

constexpr unsigned char EnabledFlag = 1 << 0;
constexpr unsigned char WasHitFlag = 1 << 1;
constexpr unsigned char HeadEnabledFlag = 1 << 2;
constexpr unsigned char FailedHitFlag = 1 << 3;
constexpr unsigned char InvisibleFlag = 1 << 4;



