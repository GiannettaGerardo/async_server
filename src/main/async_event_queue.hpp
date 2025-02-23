#pragma once

#include <stdexcept>

namespace async 
{

class EventQueueException : public std::runtime_error {
public:
    EventQueueException(const std::string msg) : std::runtime_error{msg} {}
};

template <typename EventType, typename Options> 
class EventQueue {
public:
    virtual ~EventQueue() {}
    virtual bool addEvent(const EventType event) = 0;
    virtual EventType getEvent(const int eventIdx) = 0;
    virtual void clearQueue() = 0;
    virtual int waitForEvents(const Options options) = 0;
    virtual bool safeSkipErrorEvent(const int eventIdx) = 0;
    virtual bool isEvent(const int eventIdx, const EventType event) = 0;
};

} // namespace async