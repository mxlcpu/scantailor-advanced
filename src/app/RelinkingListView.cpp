// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkingListView.h"
#include <QPainter>
#include <QStyledItemDelegate>
#include "RelinkingModel.h"

class RelinkingListView::Delegate : public QStyledItemDelegate {
 public:
  explicit Delegate(RelinkingListView* owner) : QStyledItemDelegate(owner), m_owner(owner) {}

  void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    m_owner->maybeDrawStatusLayer(painter, index, option.rect);
    QStyledItemDelegate::paint(painter, option, index);
  }

 private:
  RelinkingListView* m_owner;
};


class RelinkingListView::IndicationGroup {
 public:
  QRect rect;
  int status;

  IndicationGroup(const QRect& r, int st) : rect(r), status(st) {}
};


class RelinkingListView::GroupAggregator {
 public:
  void process(const QRect& rect, int status);

  const std::vector<IndicationGroup>& groups() const { return m_groups; }

 private:
  std::vector<IndicationGroup> m_groups;
};


RelinkingListView::RelinkingListView(QWidget* parent) : QListView(parent), m_statusLayerDrawn(false) {
  setItemDelegate(new Delegate(this));
}

void RelinkingListView::paintEvent(QPaintEvent* e) {
  m_statusLayerDrawn = false;
  QListView::paintEvent(e);
}

void RelinkingListView::maybeDrawStatusLayer(QPainter* painter,
                                             const QModelIndex& item_index,
                                             const QRect& item_paint_rect) {
  if (m_statusLayerDrawn) {
    return;
  }

  painter->save();
  // Now, the painter is configured for drawing an ItemView cell.
  // We can't be sure about its origin (widget top-left, viewport top-left?)
  // and its clipping region.  The origin is not hard to figure out,
  // while the clipping region we are just going to reset.
  painter->translate(item_paint_rect.topLeft() - visualRect(item_index).topLeft());
  painter->setClipRect(viewport()->rect());
  drawStatusLayer(painter);
  painter->restore();

  m_statusLayerDrawn = true;
}

void RelinkingListView::drawStatusLayer(QPainter* painter) {
  const QRect drawing_rect(viewport()->rect());
  QModelIndex top_index(this->indexAt(drawing_rect.topLeft()));
  if (!top_index.isValid()) {
    // No [visible] elements at all?
    return;
  }

  if (top_index.row() > 0) {
    // The appearence of any element actually depends on its neighbours,
    // so we start with the element above our topmost visible one.
    top_index = top_index.sibling(top_index.row() - 1, 0);
  }

  GroupAggregator group_aggregator;
  const int rows = top_index.model()->rowCount(top_index.parent());

  for (int row = top_index.row(); row < rows; ++row) {
    const QModelIndex index(top_index.sibling(row, 0));
    const QRect item_rect(visualRect(index));

    QRect rect(drawing_rect);
    rect.setTop(item_rect.top());
    rect.setBottom(item_rect.bottom());
    rect.setWidth(item_rect.height());
    rect.moveRight(drawing_rect.right());

    const int status = index.data(RelinkingModel::UncommittedStatusRole).toInt();
    group_aggregator.process(rect, status);

    if ((row != top_index.row()) && !item_rect.intersects(drawing_rect)) {
      // Note that we intentionally break *after* processing
      // the first invisible item. That's because the appearence
      // of its immediate predecessor depends on it. The topmost item
      // is allowed to be invisible, as it's processed for the same reason,
      // that is to make its immediate neighbour to display correctly.
      break;
    }
  }

  painter->setRenderHint(QPainter::Antialiasing);

  // Prepare for drawing existing items.
  QPen pen(QColor(0x3a5827));
  pen.setWidthF(1.5);
  QBrush brush(QColor(0x89e74a));

  // Draw existing, then missing items.
  for (int status = RelinkingModel::Exists, i = 0; i < 2; ++i) {
    painter->setPen(pen);
    painter->setBrush(brush);

    for (const IndicationGroup& group : group_aggregator.groups()) {
      if (group.status == status) {
        const qreal radius = 0.5 * group.rect.width();
        QRectF rect(group.rect);
        rect.adjust(pen.widthF(), pen.widthF(), -pen.widthF(), -pen.widthF());
        painter->drawRoundedRect(rect, radius, radius);
      }
    }
    // Prepare for drawing missing items.
    status = RelinkingModel::Missing;
    pen.setColor(QColor(0x6f2719));
    brush.setColor(QColor(0xff674b));
  }
}  // RelinkingListView::drawStatusLayer

void RelinkingListView::GroupAggregator::process(const QRect& rect, int status) {
  if (m_groups.empty() || (m_groups.back().status != status)) {
    m_groups.emplace_back(rect, status);
  } else {
    m_groups.back().rect |= rect;
  }
}
