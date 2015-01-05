// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Histogram is an object that aggregates statistics, and can summarize them in
// various forms, including ASCII graphical, HTML, and numerically (as a
// vector of numbers corresponding to each of the aggregating buckets).

// It supports calls to accumulate either time intervals (which are processed
// as integral number of milliseconds), or arbitrary integral units.

// For Histogram(exponential histogram),
// the minimum for a declared range is 1 (instead of 0), while the maximum is
// (HistogramBase::kSampleType_MAX - 1). Currently you can declare histograms
// with ranges exceeding those limits (e.g. 0 as minimal or
// Histogram::kSampleTypeMax as maximal), but those excesses will be
// silently clamped to those limits (for backwards compatibility with existing
// code). Best practice is to not exceed the limits.

// Each use of a histogram with the same name will reference the same underlying
// data, so it is safe to record to the same histogram from multiple locations
// in the code. It is a runtime error if all uses of the same histogram do not
// agree exactly in type, bucket size and range.

// For Histogram, the maximum for a declared range should
// always be larger (not equal) than minimal range. Zero and
// HistogramBase::kSampleTypeMax are implicitly added as first and last ranges,
// so the smallest legal bucket_count is 3.

// The buckets layout of class Histogram is exponential. For example, buckets
// might contain (sequentially) the count of values in the following intervals:
// [0,1), [1,2), [2,4), [4,8), [8,16), [16,32), [32,64), [64,infinity)
// That bucket allocation would actually result from construction of a histogram
// for values between 1 and 64, with 8 buckets, such as:
// Histogram count("some name", 1, 64, 8);
// Note that the underflow bucket [0,1) and the overflow bucket [64,infinity)
// are also counted by the constructor in the user supplied "bucket_count"
// argument.
// The above example has an exponential ratio of 2 (doubling the bucket width
// in each consecutive bucket.  The Histogram class automatically calculates
// the smallest ratio that it can use to construct the number of buckets
// selected in the constructor.  An another example, if you had 50 buckets,
// and millisecond time values from 1 to 10000, then the ratio between
// consecutive bucket widths will be approximately somewhere around the 50th
// root of 10000.  This approach provides very fine grain (narrow) buckets
// at the low end of the histogram scale, but allows the histogram to cover a
// gigantic range with the addition of very few buckets.

// Usually we use macros to define and use a histogram. These macros use a
// pattern involving a function static variable, that is a pointer to a
// histogram.  This static is explicitly initialized on any thread
// that detects a uninitialized (NULL) pointer.  The potentially racy
// initialization is not a problem as it is always set to point to the same
// value (i.e., the FactoryGet always returns the same value).

#ifndef _CLAIRE_COMMON_METRICS_HISTOGRAM_H_
#define _CLAIRE_COMMON_METRICS_HISTOGRAM_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace claire {

// Histograms are often put in areas where they are called many many times, and
// performance is critical.  As a result, they are designed to have a very low
// recurring cost of executing (adding additional samples).  Toward that end,
// the macros declare a static pointer to the histogram in question, and only
// take a "slow path" to construct (or find) the histogram on the first run
// through the macro.  We leak the histograms at shutdown time so that we don't
// have to validate using the pointers at any time during the running of the
// process.

// The following code is generally what a thread-safe static pointer
// initializaion looks like for a histogram (after a macro is expanded).

#define STATIC_HISTOGRAM_POINTER_BLOCK(constant_histogram_name, \
                                       histogram_add_method_invocation, \
                                       histogram_factory_get_invocation) \
do { \
    static __thread ::claire::Histogram* histogram_pointer = NULL; \
    if (!histogram_pointer) { \
      histogram_pointer = histogram_factory_get_invocation; \
    } \
    DCHECK_EQ(histogram_pointer->histogram_name(), \
              std::string(constant_histogram_name)); \
    histogram_pointer->histogram_add_method_invocation; \
} while (0)

// For folks that need real specific times, use this to select a precise range
// of times you want plotted, and the number of buckets you want used.
#define HISTOGRAM_CUSTOM_TIMES(name, sample, min, max, bucket_count) \
    STATIC_HISTOGRAM_POINTER_BLOCK(name, Add(sample), \
        claire::Histogram::FactoryGet(name, min, max, bucket_count))

#define HISTOGRAM_COUNTS(name, sample) \
    HISTOGRAM_CUSTOM_COUNTS(name, sample, 1, 1000000, 50)

#define HISTOGRAM_COUNTS_100(name, sample) \
    HISTOGRAM_CUSTOM_COUNTS(name, sample, 1, 100, 50)

#define HISTOGRAM_COUNTS_10000(name, sample) \
    HISTOGRAM_CUSTOM_COUNTS(name, sample, 1, 10000, 50)

#define HISTOGRAM_CUSTOM_COUNTS(name, sample, min, max, bucket_count) \
    STATIC_HISTOGRAM_POINTER_BLOCK(name, Add(sample), \
        claire::Histogram::FactoryGet(name, min, max, bucket_count))

#define HISTOGRAM_MEMORY_KB(name, sample) \
    HISTOGRAM_CUSTOM_COUNTS(name, sample, 1000, 500000, 50)

class BucketRanges;
class SampleVector;
class HistogramSamples;

class Histogram : boost::noncopyable
{
public:
    typedef int Sample;
    typedef int Count; // FIXME: atomic
    typedef std::vector<Count> Counts;

    static const Sample kSampleTypeMax; // INT_MAX
    static const size_t kBucketCountMax; // 16384

    //----------------------------------------------------------------------------
    // For a valid histogram, input should follow these restrictions:
    // minimum > 0 (if a minimum below 1 is specified, it will implicitly be
    //              normalized up to 1)
    // maximum > minimum
    // buckets > 2 [minimum buckets needed: underflow, overflow and the range]
    // Additionally,
    // buckets <= (maximum - minimum + 2) - this is to ensure that we don't have
    // more buckets than the range of numbers; having more buckets than 1 per
    // value in the range would be nonsensical.
    static Histogram* FactoryGet(const std::string& name,
                                 Sample minimum,
                                 Sample maximum,
                                 size_t bucket_count);

    static void InitializeBucketRanges(Sample minimum,
                                       Sample maximum,
                                       BucketRanges* ranges);

    std::string histogram_name() const { return histogram_name_;}
    virtual std::string type() const;

    virtual void Add(Sample value);
    virtual void AddSamples(const HistogramSamples& samples);

    virtual std::unique_ptr<HistogramSamples> SnapshotSamples() const;

    // The following methods provide graphical histogram displays.
    virtual void WriteHTMLGraph(std::string* output) const;
    virtual void WriteAscii(std::string* output) const;

    // Produce a JSON representation of the histogram. This is implemented with
    // the help of GetParameters and GetCountAndBucketData; overwrite them to
    // customize the output.
    void WriteJSON(std::string* output) const;

    //----------------------------------------------------------------------------
    // Accessors for factory constuction, serialization and testing.
    //----------------------------------------------------------------------------
    Sample declared_min() const { return declared_min_; }
    Sample declared_max() const { return declared_max_; }
    virtual Sample ranges(size_t i) const;
    virtual size_t bucket_count() const;
    const BucketRanges* bucket_ranges() const { return bucket_ranges_.get(); }


    // This function validates histogram construction arguments. It returns false
    // if some of the arguments are totally bad.
    // Note. Currently it allow some bad input, e.g. 0 as minimum, but silently
    // converts it to good input: 1.
    // TODO(kaiwang): Be more restrict and return false for any bad input, and
    // make this a readonly validating function.
    static bool InspectConstructionArguments(const std::string& name,
                                             Sample* minimum,
                                             Sample* maximum,
                                             size_t* bucket_count);

protected:
    Histogram(const std::string& name,
              Sample minimum,
              Sample maximum,
              size_t bucket_count);
    virtual ~Histogram();

    // Method to override to skip the display of the i'th bucket if it's empty.
    virtual bool PrintEmptyBucket(size_t index) const;

    // Get normalized size, relative to the ranges(i).
    virtual double GetBucketSize(Count current, size_t i) const;

    // Return a string description of what goes in a given bucket.
    // Most commonly this is the numeric value, but in derived classes it may
    // be a name (or string description) given to the bucket.
    virtual const std::string GetAsciiBucketRange(size_t it) const;

private:
    friend class HistogramRecorder;

    //// Produce actual graph (set of blank vs non blank char's) for a bucket.
    void WriteAsciiBucketGraph(double current_size,
                               double max_size,
                               std::string* output) const;

    // Return a string description of what goes in a given bucket.
    const std::string GetSimpleAsciiBucketRange(Sample sample) const;

    // Write textual description of the bucket contents (relative to histogram).
    // Output is the count in the buckets, as well as the percentage.
    void WriteAsciiBucketValue(Count current,
                               double scaled_sum,
                               std::string* output) const;

    // Implementation of SnapshotSamples function.
    std::unique_ptr<SampleVector> SnapshotSampleVector() const;

    //----------------------------------------------------------------------------
    // Helpers for emitting Ascii graphic.  Each method appends data to output.

    void WriteAsciiImpl(bool graph_it,
                        const std::string& newline,
                        std::string* output) const;

    // Find out how large (graphically) the largest bucket will appear to be.
    double GetPeakBucketSize(const SampleVector& samples) const;

    // Write a common header message describing this histogram.
    void WriteAsciiHeader(const SampleVector& samples,
                          Count sample_count,
                          std::string* output) const;

    // Write information about previous, current, and next buckets.
    // Information such as cumulative percentage, etc.
    void WriteAsciiBucketContext(const int64_t past,
                                 const Count current,
                                 const int64_t remaining,
                                 const size_t i,
                                 std::string* output) const;

    std::string histogram_name_;

    boost::scoped_ptr<BucketRanges> bucket_ranges_;
    Sample declared_min_; // Less than this goes into the first bucket.
    Sample declared_max_; // Over this goes into the last bucket.

    // Finally, provide the state that changes with the addition of each new
    // sample.
    boost::scoped_ptr<SampleVector> samples_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_METRICS_HISTOGRAMIMPL_H_
