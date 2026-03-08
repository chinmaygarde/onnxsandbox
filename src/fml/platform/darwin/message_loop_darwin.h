// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <CoreFoundation/CoreFoundation.h>

#include <atomic>

#include "fml/macros.h"
#include "fml/message_loop_impl.h"
#include "fml/platform/darwin/cf_utils.h"

namespace fml {

class MessageLoopDarwin : public MessageLoopImpl {
 public:
  // A custom CFRunLoop mode used when processing flutter messages,
  // so that the CFRunLoop can be run without being interrupted by UIKit,
  // while still being able to receive and be interrupted by framework messages.
  static CFStringRef kMessageLoopCFRunLoopMode;

 private:
  std::atomic_bool running_;
  CFRef<CFRunLoopTimerRef> delayed_wake_timer_;
  CFRef<CFRunLoopRef> loop_;

  MessageLoopDarwin();

  ~MessageLoopDarwin() override;

  // |fml::MessageLoopImpl|
  void Run() override;

  // |fml::MessageLoopImpl|
  void Terminate() override;

  // |fml::MessageLoopImpl|
  void WakeUp(fml::TimePoint time_point) override;

  static void OnTimerFire(CFRunLoopTimerRef timer, MessageLoopDarwin* loop);

  FML_FRIEND_MAKE_REF_COUNTED(MessageLoopDarwin);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MessageLoopDarwin);
  FML_DISALLOW_COPY_AND_ASSIGN(MessageLoopDarwin);
};

}  // namespace fml
