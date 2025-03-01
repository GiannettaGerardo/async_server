#pragma once

#include <any>
#include <cstdlib>
#include <deque>
#include <functional>

namespace async 
{

class Promise {
private:
    #define _ERROR_ 0b00000001
    #define _FINALLY_ 0b00000010
    using future_fn = std::function<void(std::any)>;
    std::deque<future_fn> nexts;
    std::any result;
    future_fn errorCallback;
    future_fn finallyCallback;
    __uint8_t mask;

    inline void addToMask(__uint8_t new_mask) {
        mask |= new_mask;
    }

    inline void removeToMask(__uint8_t new_mask) {
        mask ^= new_mask;
    }
    
public:
    Promise()
    :   errorCallback{nullptr}, 
        finallyCallback{nullptr},
        mask{0}
    {}

    ~Promise() {}

    Promise& then(future_fn&& callback) {
        nexts.push_back(std::move(callback));
        return *this;
    }

    Promise& error(future_fn&& callback) {
        errorCallback = std::move(callback);
        return *this;
    }

    Promise& finally(future_fn&& callback) {
        finallyCallback = std::move(callback);
        return *this;
    }

    inline void resolve(std::any&& _result) {
        result = std::move(_result);
    }

    inline void reject(std::any&& _result) {
        addToMask(_ERROR_);
        result = std::move(_result);
    }

    bool poll() {
        if (mask & _FINALLY_) {
            bool res = false;
            if (finallyCallback != nullptr) {
                finallyCallback(std::move(result));
                res = true;
            }
            removeToMask(_FINALLY_);
            return res;
        }

        if (mask & _ERROR_) {
            if (nullptr != errorCallback) {
                errorCallback(std::move(result));
                errorCallback = nullptr;
                addToMask(_FINALLY_);
                return true;
            } else {
                return mask & _FINALLY_;
            }
        }

        if (nexts.empty())
            return false;

        // deque is not empty here
        future_fn& callback = nexts.front();
        if (callback == nullptr) {
            nexts.pop_front();

            // maybe deque is empty now
            while (!nexts.empty()) {
                // deque is not empty here
                callback = nexts.front();
                if (callback != nullptr)
                    break;
                nexts.pop_front();
            }

            // now if callback is null than deque is empty
            if (callback == nullptr)
                return false;
        }

        callback(std::move(result));
        nexts.pop_front();
        if (nexts.empty())
            addToMask(_FINALLY_);
        return true;
    }

    #undef _ERROR_
    #undef _FINALLY_
};

} // namespace async
