// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>

namespace fml {

// Returns a UTF-8 encoded equivalent of a UTF-16 encoded input wide string.
std::string WideStringToUtf8(const std::wstring_view str);

// Returns a UTF-16 encoded wide string equivalent of a UTF-8 encoded input
// string.
std::wstring Utf8ToWideString(const std::string_view str);

// Returns a UTF-16 encoded equivalent of a UTF-16 encoded wide string.
std::u16string WideStringToUtf16(const std::wstring_view str);

// Returns a UTF-16 encoded wide string equivalent of a UTF-16 string.
std::wstring Utf16ToWideString(const std::u16string_view str);

}  // namespace fml
