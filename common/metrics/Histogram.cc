// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/metrics/Histogram.h>

#include <math.h>
#include <limits.h>

#include "thirdparty/rapidjson/document.h"
#include "thirdparty/rapidjson/stringbuffer.h"
#include "thirdparty/rapidjson/prettywriter.h"

#include <claire/common/strings/StringPrintf.h>
#include <claire/common/metrics/HistogramSamples.h>
#include <claire/common/metrics/BucketRanges.h>
#include <claire/common/metrics/SampleVector.h>
#include <claire/common/metrics/HistogramRecorder.h>
#include <claire/common/logging/Logging.h>

namespace claire {

const Histogram::Sample Histogram::kSampleTypeMax = INT_MAX;
const size_t Histogram::kBucketCountMax = 16384u;

typedef Histogram::Count Count;
typedef Histogram::Sample Sample;

Histogram* Histogram::FactoryGet(const std::string& name,
                                 Sample minimum,
                                 Sample maximum,
                                 size_t bucket_count)
{
    auto histogram = HistogramRecorder::instance()->FindHistogram(name);
    if (!histogram)
    {
        Histogram* tentative_histogram =
            new Histogram(name, minimum, maximum, bucket_count);
        histogram =
            HistogramRecorder::instance()->RegisterOrDeleteDuplicate(tentative_histogram);
    }

    return histogram;
}

Histogram::Histogram(const std::string& name,
                     Sample minimum,
                     Sample maximum,
                     size_t bucket_count__)
    : histogram_name_(name),
      bucket_ranges_(new BucketRanges(bucket_count__+1)),
      declared_min_(minimum),
      declared_max_(maximum)
{
    InitializeBucketRanges(minimum, maximum, bucket_ranges_.get());
    samples_.reset(new SampleVector(bucket_ranges_.get()));
}

Histogram::~Histogram() {}

void Histogram::InitializeBucketRanges(Sample minimum, Sample maximum, BucketRanges* ranges)
{
    double log_max = log(static_cast<double>(maximum));
    double log_ratio;
    double log_next;
    size_t bucket_index = 1;
    Sample current = minimum;
    ranges->set_range(bucket_index, current);
    size_t bucket_count = ranges->bucket_count();
    while (bucket_count > ++bucket_index)
    {
        double log_current;
        log_current = log(static_cast<double>(current));
        // Calculate the count'th root of the range.
        log_ratio = (log_max - log_current) / static_cast<double>(bucket_count - bucket_index);
        // See where the next bucket would start.
        log_next = log_current + log_ratio;
        Sample next;
        next = static_cast<int>(floor(exp(log_next) + 0.5));
        if (next > current)
            current = next;
        else
            ++current;  // Just do a narrow bucket, and keep trying.
        ranges->set_range(bucket_index, current);
    }
    ranges->set_range(ranges->bucket_count(), kSampleTypeMax);
}

Sample Histogram::ranges(size_t i) const
{
    return bucket_ranges_->range(i);
}

size_t Histogram::bucket_count() const
{
    return bucket_ranges_->bucket_count();
}

// static
bool Histogram::InspectConstructionArguments(const std::string& name,
                                             Sample* minimum,
                                             Sample* maximum,
                                             size_t* bucket_count)
{
    // Defensive code for backward compatibility.
    if (*minimum < 1)
    {
        //DVLOG(1) << "Histogram: " << name << " has bad minimum: " << *minimum;
        *minimum = 1;
    }

    if (*maximum >= kSampleTypeMax)
    {
        //DVLOG(1) << "Histogram: " << name << " has bad maximum: " << *maximum;
        *maximum = kSampleTypeMax - 1;
    }

    if (*bucket_count >= kBucketCountMax)
    {
        //DVLOG(1) << "Histogram: " << name << " has bad bucket_count: "
        //         << *bucket_count;
        *bucket_count = kBucketCountMax - 1;
    }

    if (*minimum >= *maximum)
        return false;
    if (*bucket_count < 3)
        return false;
    if (*bucket_count > static_cast<size_t>(*maximum - *minimum + 2))
        return false;
    return true;
}

std::string Histogram::type() const
{
    return std::string("Histogram");
}

void Histogram::Add(int value)
{
    DCHECK_EQ(0, ranges(0));
    DCHECK_EQ(kSampleTypeMax, ranges(bucket_count()));

    if (value >= kSampleTypeMax)
        value = kSampleTypeMax - 1;
    if (value < 0)
        value = 0;
    samples_->Accumulate(value, 1);
}

std::unique_ptr<HistogramSamples> Histogram::SnapshotSamples() const
{
    return std::unique_ptr<HistogramSamples>(SnapshotSampleVector().release());
}

void Histogram::AddSamples(const HistogramSamples& samples)
{
    samples_->Add(samples);
}

// The following methods provide a graphical histogram display.
void Histogram::WriteHTMLGraph(std::string* output) const
{
    // TBD(jar) Write a nice HTML bar chart, with divs an mouse-overs etc.
    output->append("<PRE>");
    WriteAsciiImpl(true, "<br>", output);
    output->append("</PRE>");
}

void Histogram::WriteAscii(std::string* output) const
{
    WriteAsciiImpl(true, "\n", output);
}

void Histogram::WriteJSON(std::string* output) const
{
    rapidjson::Document doc;
    doc.SetObject();
    doc.AddMember("name", rapidjson::StringRef(histogram_name_.c_str()), doc.GetAllocator());

    auto snapshot = SnapshotSampleVector();
    doc.AddMember("count", snapshot->TotalCount(), doc.GetAllocator());
    doc.AddMember("sum", snapshot->sum(), doc.GetAllocator());

    doc.AddMember("type", rapidjson::StringRef(type().c_str()), doc.GetAllocator());
    doc.AddMember("min", declared_min(), doc.GetAllocator());
    doc.AddMember("max", declared_max(), doc.GetAllocator());
    doc.AddMember("bucket_count", bucket_count(), doc.GetAllocator());

    rapidjson::Value buckets(rapidjson::kArrayType);
    for (size_t i = 0; i < bucket_count(); i++)
    {
        Sample count = snapshot->GetCountAtIndex(i);
        if (count > 0)
        {
            rapidjson::Value bucket_value;
            bucket_value.AddMember("low", ranges(i), doc.GetAllocator());

            if (i != bucket_count() - 1)
            {
                bucket_value.AddMember("high", ranges(i+1), doc.GetAllocator());
                bucket_value.AddMember("count", count, doc.GetAllocator());
            }
            buckets.PushBack(bucket_value, doc.GetAllocator());
        }
    }
    doc.AddMember("buckets", buckets, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    output->assign(buffer.GetString());
    return ;
}

bool Histogram::PrintEmptyBucket(size_t index) const
{
    return true;
}

// Use the actual bucket widths (like a linear histogram) until the widths get
// over some transition value, and then use that transition width.  Exponentials
// get so big so fast (and we don't expect to see a lot of entries in the large
// buckets), so we need this to make it possible to see what is going on and
// not have 0-graphical-height buckets.
double Histogram::GetBucketSize(Count current, size_t i) const
{
    DCHECK_GT(ranges(i + 1), ranges(i));
    static const double kTransitionWidth = 5;
    double denominator = ranges(i + 1) - ranges(i);
    if (denominator > kTransitionWidth)
        denominator = kTransitionWidth;  // Stop trying to normalize.
    return current/denominator;
}

const std::string Histogram::GetAsciiBucketRange(size_t i) const
{
    return GetSimpleAsciiBucketRange(ranges(i));
}

std::unique_ptr<SampleVector> Histogram::SnapshotSampleVector() const
{
    std::unique_ptr<SampleVector> samples(new SampleVector(bucket_ranges()));
    samples->Add(*samples_);
    return samples;
}

const std::string Histogram::GetSimpleAsciiBucketRange(Sample sample) const
{
    std::string result;
    StringAppendF(&result, "%d", sample);
    return result;
}

void Histogram::WriteAsciiBucketGraph(double current_size,
                                      double max_size,
                                      std::string* output) const
{
    const int k_line_length = 72;  // Maximal horizontal width of graph.
    int x_count = static_cast<int>(k_line_length * (current_size / max_size) + 0.5);
    int x_remainder = k_line_length - x_count;

    while (0 < x_count--)
        output->append("-");
    output->append("O");
    while (0 < x_remainder--)
        output->append(" ");
}

void Histogram::WriteAsciiBucketValue(Count current,
                                      double scaled_sum,
                                      std::string* output) const
{
    StringAppendF(output, " (%d = %3.1f%%)", current, current/scaled_sum);
}

void Histogram::WriteAsciiImpl(bool graph_it,
                               const std::string& newline,
                               std::string* output) const
{
    // Get local (stack) copies of all effectively volatile class data so that we
    // are consistent across our output activities.
    auto snapshot = SnapshotSampleVector();
    Count sample_count = snapshot->TotalCount();

    WriteAsciiHeader(*snapshot, sample_count, output);
    output->append(newline);

    // Prepare to normalize graphical rendering of bucket contents.
    double max_size = 0;
    if (graph_it)
        max_size = GetPeakBucketSize(*snapshot);

    // Calculate space needed to print bucket range numbers.  Leave room to print
    // nearly the largest bucket range without sliding over the histogram.
    size_t largest_non_empty_bucket = bucket_count() - 1;
    while (0 == snapshot->GetCountAtIndex(largest_non_empty_bucket))
    {
        if (0 == largest_non_empty_bucket)
            break;  // All buckets are empty.
        --largest_non_empty_bucket;
    }

    // Calculate largest print width needed for any of our bucket range displays.
    size_t print_width = 1;
    for (size_t i = 0; i < bucket_count(); ++i)
    {
        if (snapshot->GetCountAtIndex(i))
        {
            size_t width = GetAsciiBucketRange(i).size() + 1;
            if (width > print_width)
                print_width = width;
        }
    }

    int64_t remaining = sample_count;
    int64_t past = 0;
    // Output the actual histogram graph.
    for (size_t i = 0; i < bucket_count(); ++i)
    {
        Count current = snapshot->GetCountAtIndex(i);
        if (!current && !PrintEmptyBucket(i))
            continue;
        remaining -= current;
        auto range = GetAsciiBucketRange(i);
        output->append(range);
        for (size_t j = 0; range.size() + j < print_width + 1; ++j)
            output->push_back(' ');
        if (0 == current && i < bucket_count() - 1 &&
            0 == snapshot->GetCountAtIndex(i + 1))
        {
            while (i < bucket_count() - 1 &&
                   0 == snapshot->GetCountAtIndex(i + 1))
            {
                ++i;
            }
            output->append("... ");
            output->append(newline);
            continue;  // No reason to plot emptiness.
        }
        double current_size = GetBucketSize(current, i);
        if (graph_it)
            WriteAsciiBucketGraph(current_size, max_size, output);
        WriteAsciiBucketContext(past, current, remaining, i, output);
        output->append(newline);
        past += current;
    }
    DCHECK_EQ(sample_count, past);
}

double Histogram::GetPeakBucketSize(const SampleVector& samples) const
{
    double max = 0;
    for (size_t i = 0; i < bucket_count() ; ++i)
    {
        double current_size = GetBucketSize(samples.GetCountAtIndex(i), i);
        if (current_size > max)
            max = current_size;
    }
    return max;
}

void Histogram::WriteAsciiHeader(const SampleVector& samples,
                                 Count sample_count,
                                 std::string* output) const
{
    StringAppendF(output,
                  "Histogram: %s recorded %d samples",
                  histogram_name().c_str(),
                  sample_count);
    if (0 == sample_count)
    {
        DCHECK_EQ(samples.sum(), 0);
    }
    else
    {
        double average = static_cast<double>(samples.sum()) / sample_count;
        StringAppendF(output, ", average = %.1f", average);
    }
}

void Histogram::WriteAsciiBucketContext(const int64_t past,
                                        const Count current,
                                        const int64_t remaining,
                                        const size_t i,
                                        std::string* output) const
{
    double scaled_sum = static_cast<double>(past + current + remaining) / 100.0;
    WriteAsciiBucketValue(current, scaled_sum, output);
    if (0 < i)
    {
        double percentage = static_cast<double>(past) / scaled_sum;
        StringAppendF(output, " {%3.1f%%}", percentage);
    }
}

} // namespace claire
