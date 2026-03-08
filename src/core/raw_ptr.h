// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <absl/log/check.h>
#include <memory>

namespace ogre {

/// @brief A wrapper around a raw ptr that adds additional unopt mode only
/// checks.
template <typename T>
class raw_ptr {
 public:
  explicit raw_ptr(const std::shared_ptr<T>& ptr)
      : ptr_(ptr.get())
#if !NDEBUG
        ,
        weak_ptr_(ptr)
#endif
  {
  }

  raw_ptr() : ptr_(nullptr) {}

  T* operator->() {
#if !NDEBUG
    CHECK(weak_ptr_.lock());
#endif
    return ptr_;
  }

  const T* operator->() const {
#if !NDEBUG
    CHECK(weak_ptr_.lock());
#endif
    return ptr_;
  }

  T* get() {
#if !NDEBUG
    CHECK(weak_ptr_.lock());
#endif
    return ptr_;
  }

  T& operator*() {
#if !NDEBUG
    CHECK(weak_ptr_.lock());
#endif
    return *ptr_;
  }

  const T& operator*() const {
#if !NDEBUG
    CHECK(weak_ptr_.lock());
#endif
    return *ptr_;
  }

  template <class U>
  inline bool operator==(raw_ptr<U> const& other) const {
    return ptr_ == other.ptr_;
  }

  template <class U>
  inline bool operator!=(raw_ptr<U> const& other) const {
    return !(*this == other);
  }

  explicit operator bool() const { return !!ptr_; }

 private:
  T* ptr_;
#if !NDEBUG
  std::weak_ptr<T> weak_ptr_;
#endif
};

}  // namespace ogre
