// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_UNITSLISTENER_H
#define SCANTAILOR_UNITSLISTENER_H


#include "Units.h"

class Dpi;

class UnitsListener {
 protected:
  UnitsListener();

 public:
  virtual ~UnitsListener();

  virtual void onUnitsChanged(Units units) = 0;
};

#endif  //SCANTAILOR_UNITSLISTENER_H
