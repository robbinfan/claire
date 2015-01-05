// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/metrics/Counter.h>
#include <claire/common/metrics/CounterProvider.h>

namespace claire {

void Counter::Set(int value)
{
    auto loc = GetPtr();
    if (loc)
    {
        *loc = value;
    }
}

void Counter::Add(int value)
{
    int* loc = GetPtr();
    if (loc)
    {
        *loc += value;
    }
}

int* Counter::GetPtr()
{
    if (counter_id_ == -1)
    {
        counter_id_ = CounterProvider::instance()->GetCounterId(name_);
    }

    if (counter_id_ > 0)
    {
        return CounterProvider::instance()->GetLocation(counter_id_);
    }

    return NULL;
}

} // namespace claire
