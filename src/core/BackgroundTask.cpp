// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "BackgroundTask.h"

const char* BackgroundTask::CancelledException::what() const noexcept {
  return "BackgroundTask cancelled";
}

void BackgroundTask::throwIfCancelled() const {
  if (isCancelled()) {
    throw CancelledException();
  }
}
