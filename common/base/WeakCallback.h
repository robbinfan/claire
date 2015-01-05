// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _CLAIRE_COMMON_BASE_WEAKCALLBACK_H_
#define _CLAIRE_COMMON_BASE_WEAKCALLBACK_H_

#include <boost/function.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace claire {

template<typename CLASS, typename... ARGS>
class WeakCallback
{
public:
    WeakCallback(const boost::function<void (CLASS*, ARGS...)>& function,
                 const boost::weak_ptr<CLASS>& object)
        : function_(function),
          object_(object)
    {}

    void operator()(ARGS&&... args) const
    {
        auto ptr(object_.lock());
        if (ptr)
        {
            function_(get_pointer(ptr), std::forward<ARGS>(args)...);
        }
    }

private:
    boost::function<void (CLASS*, ARGS...)> function_;
    boost::weak_ptr<CLASS> object_;
};

template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> MakeWeakCallback(void (CLASS::*function)(ARGS...),
                                              const boost::shared_ptr<CLASS>& object)
{
    return WeakCallback<CLASS, ARGS...>(function, object);
}

template<typename CLASS, typename... ARGS>
WeakCallback<CLASS, ARGS...> MakeWeakCallback(void (CLASS::*function)(ARGS...) const,
                                              const boost::shared_ptr<CLASS>& object)
{
    return WeakCallback<CLASS, ARGS...>(function, object);
}

} // namespace claire

#endif // _CLAIRE_COMMON_BASE_WEAKCALLBACK_H_
