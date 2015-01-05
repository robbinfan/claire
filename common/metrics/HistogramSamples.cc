// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/metrics/HistogramSamples.h>

#include <claire/common/logging/Logging.h>

namespace claire {

void HistogramSamples::Add(const HistogramSamples& other)
{
    sum_ += other.sum();
    bool success = AddSubtractImpl(other.Iterator().get(), ADD);
    DCHECK(success);
}

void HistogramSamples::Subtract(const HistogramSamples& other)
{
    sum_ -= other.sum();
    bool success = AddSubtractImpl(other.Iterator().get(), SUBTRACT);
    DCHECK(success);
}

void HistogramSamples::IncreaseSum(int64_t diff)
{
    sum_ += diff;
}

SampleCountIterator::~SampleCountIterator() {}

bool SampleCountIterator::GetBucketIndex(size_t* index) const
{
    DCHECK(!Done());
    return false;
}

} // namespace claire
