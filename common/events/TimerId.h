// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_EVENTS_TIMERID_H_
#define _CLAIRE_COMMON_EVENTS_TIMERID_H_

#include <stdint.h>

#include <algorithm>

namespace claire {

class TimerId
{
public:
    TimerId() : id_(0) {}
    TimerId(int64_t id) : id_(id) {}

    void Reset() { id_ = 0; }
    bool Valid() const { return id_ > 0; }
    int64_t get() const { return id_; }

    void swap(TimerId& other)
    {
        std::swap(id_, other.id_);
    }

private:
    int64_t id_;
};

inline bool operator<(TimerId lhs, TimerId rhs)
{
    return lhs.get() < rhs.get();
}

inline bool operator==(TimerId lhs, TimerId rhs)
{
    return lhs.get() == rhs.get();
}

} // namespace claire

#endif // _CLAIRE_COMMON_EVENTS_TIMERID_H_
