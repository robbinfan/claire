// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// SampleVector implements HistogramSamples interface. It is used by all
// Histogram based classes to store samples.

#ifndef _CLAIRE_COMMON_METRICS_SAMPLEVECTOR_H_
#define _CLAIRE_COMMON_METRICS_SAMPLEVECTOR_H_

#include <memory>
#include <vector>

#include <boost/noncopyable.hpp>

#include <claire/common/metrics/Histogram.h>
#include <claire/common/metrics/HistogramSamples.h>

namespace claire {

class BucketRanges;

class SampleVector : boost::noncopyable,
                     public HistogramSamples
{
public:
    explicit SampleVector(const BucketRanges* bucket_ranges);
    virtual ~SampleVector() {}

    // HistogramSamples implementation:
    virtual void Accumulate(Histogram::Sample value,
                            Histogram::Count count);
    virtual Histogram::Count GetCount(Histogram::Sample value) const;
    virtual Histogram::Count TotalCount() const;
    virtual std::unique_ptr<SampleCountIterator> Iterator() const;

    // Get count of a specific bucket.
    Histogram::Count GetCountAtIndex(size_t index) const;

protected:
    virtual bool AddSubtractImpl(
        SampleCountIterator* iter,
        HistogramSamples::Operator op); // |op| is ADD or SUBTRACT.

    virtual size_t GetBucketIndex(Histogram::Sample value) const;

private:
    std::vector<Histogram::Count> counts_;
    // Shares the same BucketRanges with Histogram object.
    const BucketRanges* const bucket_ranges_;
};

class SampleVectorIterator : public SampleCountIterator
{
public:
    SampleVectorIterator(const std::vector<Histogram::Count>* counts,
                         const BucketRanges* bucket_ranges);
    virtual ~SampleVectorIterator();

    // SampleCountIterator implementation:
    virtual bool Done() const;
    virtual void Next();
    virtual void Get(Histogram::Sample* min,
                     Histogram::Sample* max,
                     Histogram::Count* count) const;

    // SampleVector uses predefined buckets, so iterator can return bucket index.
    virtual bool GetBucketIndex(size_t* index) const;

private:
    void SkipEmptyBuckets();

    const std::vector<Histogram::Count>* counts_;
    const BucketRanges* bucket_ranges_;

    size_t index_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_METRICS_SAMPLEVECTOR_H_
