// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_BASE_LIKELY_H_
#define _CLAIRE_COMMON_BASE_LIKELY_H_

#undef LIKELY
#undef UNLIKELY

#define LIKELY(x)   (__builtin_expect(!!(x), 0))
#define UNLIKELY(x) (__builtin_expect((x), 0))

#endif // _CLAIRE_COMMON_BASE_LIKELY_H_
