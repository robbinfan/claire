// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// BucketRanges stores the vector of ranges that delimit what samples are
// tallied in the corresponding buckets of a histogram. Histograms that have
// same ranges for all their corresponding buckets should share the same
// BucketRanges object.
//
// E.g. A 5 buckets LinearHistogram with 1 as minimal value and 4 as maximal
// value will need a BucketRanges with 6 ranges:
// 0, 1, 2, 3, 4, INT_MAX
//
// TODO(kaiwang): Currently we keep all negative values in 0~1 bucket. Consider
// changing 0 to INT_MIN.

#ifndef _CLAIRE_COMMON_METRICS_BUCKETRANGES_H_
#define _CLAIRE_COMMON_METRICS_BUCKETRANGES_H_

#include <vector>

#include <boost/noncopyable.hpp>

#include <claire/common/metrics/Histogram.h>

namespace claire {

class BucketRanges : boost::noncopyable
{
public:
    typedef std::vector<Histogram::Sample> Ranges;

    explicit BucketRanges(size_t num_ranges)
        : ranges_(num_ranges, 0) {}

    size_t size() const { return ranges_.size(); }
    Histogram::Sample range(size_t i) const { return ranges_[i]; }
    void set_range(size_t i, Histogram::Sample value);

    // A bucket is defined by a consecutive pair of entries in |ranges|, so there
    // is one fewer bucket than there are ranges.  For example, if |ranges| is
    // [0, 1, 3, 7, INT_MAX], then the buckets in this histogram are
    // [0, 1), [1, 3), [3, 7), and [7, INT_MAX).
    size_t bucket_count() const { return ranges_.size() - 1; }

private:
    // A monotonically increasing list of values which determine which bucket to
    // put a sample into.  For each index, show the smallest sample that can be
    // added to the corresponding bucket.
    Ranges ranges_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_METRICS_BUCKETRANGES_H_
