// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// HistogramRecorder holds all Histograms that are used in the system.
// It provides a general place for Histograms to register, and supports
// a global API for accessing (i.e., dumping, or graphing) the data.

#ifndef _CLAIRE_COMMON_METRICS_HISTOGRAMRECORDER_H_
#define _CLAIRE_COMMON_METRICS_HISTOGRAMRECORDER_H_

#include <map>
#include <vector>
#include <string>

#include <boost/noncopyable.hpp>

#include <claire/common/threading/Mutex.h>
#include <claire/common/threading/Singleton.h>

namespace claire {

class Histogram;

class HistogramRecorder : boost::noncopyable
{
public:
    typedef std::vector<Histogram*> Histograms;

    static HistogramRecorder* instance();

    // Register, or add a new histogram to the collection of statistics. If an
    // identically named histogram is already registered, then the argument
    // |histogram| will deleted.  The returned value is always the registered
    // histogram (either the argument, or the pre-existing registered histogram).
    Histogram* RegisterOrDeleteDuplicate(Histogram* histogram);

    // Methods for printing histograms.  Only histograms which have query as
    // a substring are written to output (an empty string will process all
    // registered histograms).
    void WriteHTMLGraph(const std::string& query, std::string* output);
    void WriteGraph(const std::string& query, std::string* output);

    // Find a histogram by name. It matches the exact name. This method is thread
    // safe.  It returns NULL if a matching histogram is not found.
    Histogram* FindHistogram(const std::string& name);

    // GetSnapshot copies some of the pointers to registered histograms into the
    // caller supplied vector (Histograms).  Only histograms with names matching
    // query are returned. The query must be a substring of histogram name for its
    // pointer to be copied.
    void GetSnapshot(const std::string& query, Histograms* snapshot);

private:
    // We keep all registered histograms in a map, from name to histogram.
    typedef std::map<std::string, Histogram*> HistogramMap;

    HistogramRecorder() {}
    ~HistogramRecorder() {}

    friend class Singleton<HistogramRecorder>;

    Mutex mutex_; // Lock protects access to above maps.
    HistogramMap histograms_;
};

} // namespace claire

#endif // _CLAIRE_COMMON_METRICS_HISTOGRAMRECORDER_H_
