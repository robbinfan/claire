// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <claire/common/metrics/HistogramRecorder.h>

#include <claire/common/threading/Singleton.h>
#include <claire/common/metrics/Histogram.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/strings/StringPrintf.h>

namespace claire {

// static
HistogramRecorder* HistogramRecorder::instance()
{
    return Singleton<HistogramRecorder>::instance();
}

Histogram* HistogramRecorder::RegisterOrDeleteDuplicate(Histogram* histogram)
{
    Histogram* histogram_to_delete = NULL;
    Histogram* histogram_to_return = NULL;
    {
        MutexLock lock(mutex_);
        const auto& name = histogram->histogram_name();
        auto it = histograms_.find(name);
        if (histograms_.end() == it)
        {
            histograms_[name] = histogram;
            histogram_to_return = histogram;
        }
        else if (histogram == it->second)
        {
            // The histogram was registered before.
            histogram_to_return = histogram;
        }
        else
        {
            // We already have one histogram with this name.
            histogram_to_return = it->second;
            histogram_to_delete = histogram;
        }
    }
    delete histogram_to_delete;
    return histogram_to_return;
}

void HistogramRecorder::WriteHTMLGraph(const std::string& query,
                                       std::string* output)
{
    Histograms snapshot;
    GetSnapshot(query, &snapshot);
    for (auto it = snapshot.begin(); it != snapshot.end(); ++it)
    {
        (*it)->WriteHTMLGraph(output);
        output->append("<br><hr><br>");
    }
}

void HistogramRecorder::WriteGraph(const std::string& query,
                                   std::string* output)
{
    if (query.length())
        StringAppendF(output, "Collections of histograms for %s\n", query.c_str());
    else
        output->append("Collections of all histograms\n");

    Histograms snapshot;
    GetSnapshot(query, &snapshot);
    for (auto it = snapshot.begin(); it != snapshot.end(); ++it)
    {
        (*it)->WriteAscii(output);
        output->append("\n");
    }
}

Histogram* HistogramRecorder::FindHistogram(const std::string& name)
{
    MutexLock lock(mutex_);
    auto it = histograms_.find(name);
    if (histograms_.end() == it)
        return NULL;
    return it->second;
}

void HistogramRecorder::GetSnapshot(const std::string& query, Histograms* snapshot)
{
    MutexLock lock(mutex_);
    for (auto it = histograms_.begin(); histograms_.end() != it; ++it)
    {
        if (it->first.find(query) != std::string::npos)
            snapshot->push_back(it->second);
    }
}

} // namespace claire
