// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/metrics/BucketRanges.h>

#include <string.h>

#include <claire/common/logging/Logging.h>

namespace claire {

void BucketRanges::set_range(size_t i, Histogram::Sample value)
{
    DCHECK_LT(i, ranges_.size());
    CHECK_GT(value, 0);
    ranges_[i] = value;
}

} // namspace claire
