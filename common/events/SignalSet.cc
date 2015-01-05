// Copyright (c) 2013 The claire-common Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <claire/common/events/SignalSet.h>

#include <signal.h>
#include <sys/signalfd.h>

#include <boost/bind.hpp>

#include <claire/common/events/Channel.h>
#include <claire/common/events/EventLoop.h>
#include <claire/common/logging/Logging.h>

namespace claire {

namespace {

int CreateSignalfd(int sfd, const std::set<int>& signals)
{
    sigset_t mask;
    sigemptyset(&mask);
    for (auto it = signals.cbegin(); it != signals.cend(); ++it)
    {
        sigaddset(&mask, *it);
    }

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    {
        LOG(ERROR) << "sigprocmask failed " ;
        return -1;
    }

    int signalfd = ::signalfd(sfd, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (signalfd < 0)
    {
        LOG(FATAL) << "Failed in signalfd_create";
    }

    return signalfd;
}

int ReadSignalFd(int sfd, struct signalfd_siginfo* siginfo)
{
    ssize_t s = read(sfd, siginfo, sizeof(struct signalfd_siginfo));
    if (s != sizeof(struct signalfd_siginfo))
    {
        LOG(ERROR) << "read signalfd fail: " << strerror_tl(errno);
        return -1;
    }

    return siginfo->ssi_signo;
}

} // namespace


SignalSet::SignalSet(EventLoop* loop)
    : loop_(loop),
      fd_(-1)
{}

SignalSet::SignalSet(EventLoop* loop, int signal_number_1)
    : loop_(loop),
      signals_({signal_number_1}),
      fd_(CreateSignalfd(-1, signals_)),
      channel_(new Channel(loop, fd_))
{
    channel_->set_priority(Channel::Priority::kLow);
    channel_->set_read_callback(
        boost::bind(&SignalSet::OnSignal, this));
    channel_->EnableReading();
}

SignalSet::SignalSet(EventLoop* loop,
                     int signal_number_1,
                     int signal_number_2)
    : loop_(loop),
      signals_({signal_number_1, signal_number_2}),
      fd_(CreateSignalfd(-1, signals_)),
      channel_(new Channel(loop, fd_))
{
    channel_->set_priority(Channel::Priority::kLow);
    channel_->set_read_callback(
        boost::bind(&SignalSet::OnSignal, this));
    channel_->EnableReading();
}

SignalSet::SignalSet(EventLoop* loop,
                     int signal_number_1,
                     int signal_number_2,
                     int signal_number_3)
    : loop_(loop),
      signals_({signal_number_1, signal_number_2, signal_number_3}),
      fd_(CreateSignalfd(-1, signals_)),
      channel_(new Channel(loop, fd_))
{
    channel_->set_priority(Channel::Priority::kLow);
    channel_->set_read_callback(
        boost::bind(&SignalSet::OnSignal, this));
    channel_->EnableReading();
}

SignalSet::~SignalSet()
{
    if (fd_ != -1)
    {
        ::close(fd_);
    }
}

void SignalSet::Add(int signal_number)
{
    loop_->AssertInLoopThread();

    if (signals_.insert(signal_number).second)
    {
        fd_ = CreateSignalfd(fd_, signals_);
    }

    if (fd_ != -1 && signals_.size() == 1)
    {
        channel_.reset(new Channel(loop_, fd_));
        channel_->set_priority(Channel::Priority::kLow);
        channel_->set_read_callback(
            boost::bind(&SignalSet::OnSignal, this));
        channel_->EnableReading();
    }
}

void SignalSet::Wait(const SignalHandler& handler)
{
    loop_->AssertInLoopThread();
    handlers_.push_back(handler);
}

void SignalSet::Cancel()
{
    loop_->AssertInLoopThread();
    std::vector<SignalHandler> v;
    handlers_.swap(v);
}

void SignalSet::Clear()
{
    signals_.clear();
    //FIXME: when signals_ empty, close signalfd_?
}

void SignalSet::Remove(int signal_number)
{
    loop_->AssertInLoopThread();

    if (signals_.erase(signal_number))
    {
        fd_ = CreateSignalfd(fd_, signals_);
    }

    //FIXME: when signals_ empty, close signalfd_?
}

void SignalSet::OnSignal()
{
    struct signalfd_siginfo siginfo;
    int signal_number = ReadSignalFd(fd_, &siginfo);
    if (signal_number != -1)
    {
        for (auto it = handlers_.cbegin(); it != handlers_.cend(); ++it)
        {
            (*it)(signal_number);
        }
    }
}

} // namespace claire
