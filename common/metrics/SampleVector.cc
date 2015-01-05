// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/metrics/SampleVector.h>

#include <claire/common/metrics/BucketRanges.h>
#include <claire/common/logging/Logging.h>

namespace claire {

typedef Histogram::Count Count;
typedef Histogram::Sample Sample;

SampleVector::SampleVector(const BucketRanges* bucket_ranges)
    : counts_(bucket_ranges->bucket_count()),
      bucket_ranges_(bucket_ranges)
{
    CHECK_GE(bucket_ranges_->bucket_count(), 1u);
}

void SampleVector::Accumulate(Sample value, Count count)
{
    size_t bucket_index = GetBucketIndex(value);
    counts_[bucket_index]++; // FIXME: atomic
    IncreaseSum(count*value);
}

Count SampleVector::GetCount(Sample value) const
{
    size_t bucket_index = GetBucketIndex(value);
    return counts_[bucket_index];
}

Count SampleVector::TotalCount() const
{
    Count count = 0;
    for (auto it = counts_.begin(); it != counts_.end(); ++it)
    {
        count += *it;
    }
    return count;
}

Count SampleVector::GetCountAtIndex(size_t bucket_index) const
{
    DCHECK(bucket_index < counts_.size());
    return counts_[bucket_index];
}

std::unique_ptr<SampleCountIterator> SampleVector::Iterator() const
{
    return std::unique_ptr<SampleCountIterator>(new SampleVectorIterator(&counts_, bucket_ranges_));
}

bool SampleVector::AddSubtractImpl(SampleCountIterator* iter,
                                   HistogramSamples::Operator op)
{
    Histogram::Sample min;
    Histogram::Sample max;
    Histogram::Count count;

    // Go through the iterator and add the counts into correct bucket.
    size_t index = 0;
    while (index < counts_.size() && !iter->Done())
    {
        iter->Get(&min, &max, &count);
        if (min == bucket_ranges_->range(index) &&
            max == bucket_ranges_->range(index + 1)) {
            // Sample matches this bucket!
            counts_[index] += (op ==  HistogramSamples::ADD) ? count : -count;
            iter->Next();
        } else if (min > bucket_ranges_->range(index)) {
            // Sample is larger than current bucket range. Try next.
            index++;
        } else {
            // Sample is smaller than current bucket range. We scan buckets from
            // smallest to largest, so the sample value must be invalid.
            return false;
        }
    }

    return iter->Done();
}

// Use simple binary search.  This is very general, but there are better
// approaches if we knew that the buckets were linearly distributed.
size_t SampleVector::GetBucketIndex(Sample value) const
{
    size_t bucket_count = bucket_ranges_->bucket_count();
    CHECK_GE(bucket_count, 1u);
    CHECK_GE(value, bucket_ranges_->range(0));
    CHECK_LT(value, bucket_ranges_->range(bucket_count));

    size_t under = 0;
    size_t over = bucket_count;
    size_t mid;
    do
    {
        DCHECK_GE(over, under);
        mid = under + (over - under)/2;
        if (mid == under)
            break;
        if (bucket_ranges_->range(mid) <= value)
            under = mid;
        else
            over = mid;
    } while (true);

    DCHECK_LE(bucket_ranges_->range(mid), value);
    CHECK_GT(bucket_ranges_->range(mid + 1), value);
    return mid;

}

SampleVectorIterator::SampleVectorIterator(const std::vector<Count>* counts,
                                           const BucketRanges* bucket_ranges)
    : counts_(counts),
      bucket_ranges_(bucket_ranges),
      index_(0)
{
    CHECK_GE(bucket_ranges_->bucket_count(), counts_->size());
    SkipEmptyBuckets();
}

SampleVectorIterator::~SampleVectorIterator() {}

bool SampleVectorIterator::Done() const
{
    return index_ == counts_->size();
}

void SampleVectorIterator::Next()
{
    DCHECK(!Done());
    index_++;
    SkipEmptyBuckets();
}

void SampleVectorIterator::Get(Histogram::Sample* min,
                               Histogram::Sample* max,
                               Histogram::Count * count) const
{
    DCHECK(!Done());
    if (min)
    {
        *min = bucket_ranges_->range(index_);
    }

    if (max)
    {
        *max = bucket_ranges_->range(index_+1);
    }

    if (count)
    {
        *count = (*counts_)[index_];
    }
}

bool SampleVectorIterator::GetBucketIndex(size_t* index) const
{
    DCHECK(!Done());
    if (index != NULL)
    {
        *index = index_;
    }
    return true;
}

void SampleVectorIterator::SkipEmptyBuckets()
{
    if (Done())
    {
        return ;
    }

    while (index_ < counts_->size())
    {
        if ((*counts_)[index_] != 0)
        {
            return ;
        }
        index_++;
    }
}

} // namespace claire
