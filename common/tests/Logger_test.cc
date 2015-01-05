// Copyright (c) 2013 The Claire Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <stdio.h>
#include <sys/resource.h>

#include <gflags/gflags.h>

#include <claire/common/time/Timestamp.h>
#include <claire/common/logging/Logging.h>
#include <claire/common/threading/ThreadPool.h>
#include <claire/common/threading/ThisThread.h>

void bench()
{
    std::string str(150, 'X');
    str += " ";
    int cnt = 0;

    for (int i = 0;i < 30; i++)
    {
        auto start = claire::Timestamp::Now();
        for (int j = 0;j < 1000;j++)
        {
            LOG(INFO) << "Hello, World" << cnt << str;
            cnt++;
        }
        auto end = claire::Timestamp::Now();

        printf("tid: %d, %f\n", claire::ThisThread::tid(), static_cast<double>(claire::TimeDifference(end, start)) / 1000) ;
        struct timespec ts = {0, 500 * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
}

int main(int argc, char* argv[])
{
    ::google::ParseCommandLineFlags(&argc, &argv, true);
    claire::InitClaireLogging(argv[0]);

    sleep(30);
    claire::ThreadPool pool("logger_test");
    pool.Start(5);
    pool.Run(bench);
    pool.Run(bench);
    pool.Run(bench);
    pool.Run(bench);
    pool.Run(bench);

    sleep(6);
    return 0;
}

