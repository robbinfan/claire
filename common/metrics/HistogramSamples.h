// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _CLAIRE_COMMON_METRICS_HISTOGRAMSAMPLES_H_
#define _CLAIRE_COMMON_METRICS_HISTOGRAMSAMPLES_H_

#include <memory>

#include <claire/common/metrics/Histogram.h>

namespace claire {

class SampleCountIterator;

class HistogramSamples
{
public:
    HistogramSamples()
        : sum_(0) {}
    virtual ~HistogramSamples() {}

    virtual void Accumulate(Histogram::Sample value,
                            Histogram::Count count) = 0;
    virtual Histogram::Count GetCount(Histogram::Sample value) const = 0;
    virtual Histogram::Count TotalCount() const = 0;

    virtual void Add(const HistogramSamples& other);
    virtual void Subtract(const HistogramSamples& other);

    virtual std::unique_ptr<SampleCountIterator> Iterator() const = 0;

    // Accessor fuctions.
    int64_t sum() const { return sum_; }

protected:
    // Based on |op| type, add or subtract sample counts data from the iterator.
    enum Operator { ADD, SUBTRACT };
    virtual bool AddSubtractImpl(SampleCountIterator* iter, Operator op) = 0;

    void IncreaseSum(int64_t diff);

private:
    int64_t sum_;
};

class SampleCountIterator
{
public:
    virtual ~SampleCountIterator();

    virtual bool Done() const = 0;
    virtual void Next() = 0;

    // Get the sample and count at current position.
    // |min| |max| and |count| can be NULL if the value is not of interest.
    // Requires: !Done();
    virtual void Get(Histogram::Sample* min,
                     Histogram::Sample* max,
                     Histogram::Count* count) const = 0;

    // Get the index of current histogram bucket.
    // For histograms that don't use predefined buckets, it returns false.
    // Requires: !Done();
    virtual bool GetBucketIndex(size_t* index) const;
};

} // namespace claire

#endif // _CLAIRE_COMMON_METRICS_HISTOGRAMSAMPLES_H_
