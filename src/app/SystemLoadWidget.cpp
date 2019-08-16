// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "SystemLoadWidget.h"
#include <core/IconProvider.h>
#include <QSettings>
#include <QThread>
#include <QToolTip>

static const char* const key = "settings/batch_processing_threads";

SystemLoadWidget::SystemLoadWidget(QWidget* parent) : QWidget(parent), m_maxThreads(QThread::idealThreadCount()) {
  ui.setupUi(this);
  setupIcons();

  if (sizeof(void*) <= 4) {
    // Restricting num of processors for 32-bit due to
    // address space constraints.
    if (m_maxThreads > 2) {
      m_maxThreads = 2;
    }
  }
  int num_threads = std::min<int>(m_maxThreads, QSettings().value(key, m_maxThreads).toInt());

  ui.slider->setRange(1, m_maxThreads);
  ui.slider->setValue(num_threads);

  connect(ui.slider, SIGNAL(sliderPressed()), SLOT(sliderPressed()));
  connect(ui.slider, SIGNAL(sliderMoved(int)), SLOT(sliderMoved(int)));
  connect(ui.slider, SIGNAL(valueChanged(int)), SLOT(valueChanged(int)));
  connect(ui.minusBtn, SIGNAL(clicked()), SLOT(decreaseLoad()));
  connect(ui.plusBtn, SIGNAL(clicked()), SLOT(increaseLoad()));
}

void SystemLoadWidget::sliderPressed() {
  showHideToolTip(ui.slider->value());
}

void SystemLoadWidget::sliderMoved(int threads) {
  showHideToolTip(threads);
}

void SystemLoadWidget::valueChanged(int threads) {
  QSettings settings;
  if (threads == m_maxThreads) {
    settings.remove(key);
  } else {
    settings.setValue(key, threads);
  }
}

void SystemLoadWidget::decreaseLoad() {
  ui.slider->setValue(ui.slider->value() - 1);
  showHideToolTip(ui.slider->value());
}

void SystemLoadWidget::increaseLoad() {
  ui.slider->setValue(ui.slider->value() + 1);
  showHideToolTip(ui.slider->value());
}

void SystemLoadWidget::showHideToolTip(int threads) {
  // Show the tooltip immediately.
  const QPoint center(ui.slider->rect().center());
  QPoint tooltip_pos(ui.slider->mapFromGlobal(QCursor::pos()));
  if ((tooltip_pos.x() < 0) || (tooltip_pos.x() >= ui.slider->width())) {
    tooltip_pos.setX(center.x());
  }
  if ((tooltip_pos.y() < 0) || (tooltip_pos.y() >= ui.slider->height())) {
    tooltip_pos.setY(center.y());
  }
  tooltip_pos = ui.slider->mapToGlobal(tooltip_pos);
  QToolTip::showText(tooltip_pos, QString("%1/%2").arg(threads).arg(m_maxThreads), ui.slider);
}

void SystemLoadWidget::setupIcons() {
  ui.minusBtn->setIcon(IconProvider::getInstance().getIcon("minus"));
  ui.plusBtn->setIcon(IconProvider::getInstance().getIcon("plus"));
}
