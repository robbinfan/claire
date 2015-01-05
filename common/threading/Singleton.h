// Copyright (c) 2013 The claire-comomn Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_THREADING_SINGLETON_H_
#define _CLAIRE_COMMON_THREADING_SINGLETON_H_

#include <stdlib.h>
#include <pthread.h>

#include <boost/noncopyable.hpp>

namespace claire {

template <typename T>
class Singleton : boost::noncopyable
{
public:
    static T* instance()
    {
        pthread_once(&ponce_, &Singleton::Init);
        return value_;
    }

private:
    Singleton();
    ~Singleton();

    static void Init()
    {
        static_assert(sizeof(T) > 0, "T must be complete type");
        value_ = new T();
        ::atexit(Destroy);
    }

    static void Destroy()
    {
        delete value_;
    }

private:
    static pthread_once_t ponce_;
    static T* value_;
};

template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::value_ = NULL;

} // namespace claire

#endif // _CLAIRE_COMMON_THREADING_SINGLETON_H_