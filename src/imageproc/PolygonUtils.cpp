// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "PolygonUtils.h"

#include <QLineF>
#include <QPolygonF>
#include <cassert>
#include <cmath>

namespace imageproc {
const double PolygonUtils::ROUNDING_MULTIPLIER = 1 << 12;
const double PolygonUtils::ROUNDING_RECIP_MULTIPLIER = 1.0 / ROUNDING_MULTIPLIER;

class PolygonUtils::Before {
 public:
  bool operator()(const QPointF& lhs, const QPointF& rhs) const { return compare(lhs, rhs) < 0; }

  bool operator()(const QLineF& lhs, const QLineF& rhs) {
    int comp = compare(lhs.p1(), rhs.p1());
    if (comp != 0) {
      return comp < 0;
    }
    return compare(lhs.p2(), rhs.p2()) < 0;
  }

 private:
  static int compare(const QPointF& lhs, const QPointF& rhs) {
    const double dx = lhs.x() - rhs.x();
    const double dy = lhs.y() - rhs.y();
    if (std::fabs(dx) > std::fabs(dy)) {
      if (dx < 0.0) {
        return -1;
      } else if (dx > 0.0) {
        return 1;
      }
    } else {
      if (dy < 0.0) {
        return -1;
      } else if (dy > 0.0) {
        return 1;
      }
    }
    return 0;
  }
};


QPolygonF PolygonUtils::round(const QPolygonF& poly) {
  QPolygonF rounded;
  rounded.reserve(poly.size());

  for (const QPointF& p : poly) {
    rounded.push_back(roundPoint(p));
  }
  return rounded;
}

bool PolygonUtils::fuzzyCompare(const QPolygonF& poly1, const QPolygonF& poly2) {
  if ((poly1.size() < 2) && (poly2.size() < 2)) {
    return true;
  } else if ((poly1.size() < 2) || (poly2.size() < 2)) {
    return false;
  }

  assert(poly1.size() >= 2 && poly2.size() >= 2);

  QPolygonF closed1(poly1);
  QPolygonF closed2(poly2);
  // Close if necessary.
  if (closed1.back() != closed1.front()) {
    closed1.push_back(closed1.front());
  }
  if (closed2.back() != closed2.front()) {
    closed2.push_back(closed2.front());
  }

  std::vector<QLineF> edges1(extractAndNormalizeEdges(closed1));
  std::vector<QLineF> edges2(extractAndNormalizeEdges(closed2));

  if (edges1.size() != edges2.size()) {
    return false;
  }

  std::sort(edges1.begin(), edges1.end(), Before());
  std::sort(edges2.begin(), edges2.end(), Before());
  return fuzzyCompareImpl(edges1, edges2);
}  // PolygonUtils::fuzzyCompare

QPointF PolygonUtils::roundPoint(const QPointF& p) {
  return QPointF(roundValue(p.x()), roundValue(p.y()));
}

double PolygonUtils::roundValue(const double val) {
  return std::floor(val * ROUNDING_MULTIPLIER + 0.5) * ROUNDING_RECIP_MULTIPLIER;
}

std::vector<QLineF> PolygonUtils::extractAndNormalizeEdges(const QPolygonF& poly) {
  std::vector<QLineF> edges;

  const int numEdges = poly.size();
  if (numEdges > 1) {
    for (int i = 1; i < numEdges; ++i) {
      maybeAddNormalizedEdge(edges, poly[i - 1], poly[i]);
    }
    maybeAddNormalizedEdge(edges, poly[numEdges - 1], poly[0]);
  }
  return edges;
}

void PolygonUtils::maybeAddNormalizedEdge(std::vector<QLineF>& edges, const QPointF& p1, const QPointF& p2) {
  if (fuzzyCompareImpl(p1, p2)) {
    return;
  }

  if (Before()(p2, p1)) {
    edges.emplace_back(p2, p1);
  } else {
    edges.emplace_back(p1, p2);
  }
}

bool PolygonUtils::fuzzyCompareImpl(const std::vector<QLineF>& lines1, const std::vector<QLineF>& lines2) {
  assert(lines1.size() == lines2.size());
  const size_t size = lines1.size();
  for (size_t i = 0; i < size; ++i) {
    if (!fuzzyCompareImpl(lines1[i], lines2[i])) {
      return false;
    }
  }
  return true;
}

bool PolygonUtils::fuzzyCompareImpl(const QLineF& line1, const QLineF& line2) {
  return fuzzyCompareImpl(line1.p1(), line2.p1()) && fuzzyCompareImpl(line1.p2(), line2.p2());
}

bool PolygonUtils::fuzzyCompareImpl(const QPointF& p1, const QPointF& p2) {
  const double dx = std::fabs(p1.x() - p2.x());
  const double dy = std::fabs(p1.y() - p2.y());
  return dx <= ROUNDING_RECIP_MULTIPLIER && dy <= ROUNDING_RECIP_MULTIPLIER;
}

namespace {
struct LexicographicPointComparator {
  bool operator()(const QPointF& p1, const QPointF& p2) const {
    if (p1.x() != p2.x()) {
      return p1.x() < p2.x();
    } else {
      return p1.y() < p2.y();
    }
  }
};

// 2D cross product of OA and OB vectors, i.e. z-component of their 3D cross product.
// Returns a positive value, if OAB makes a counter-clockwise turn,
// negative for clockwise turn, and zero if the points are collinear.
double cross(const QPointF& O, const QPointF& A, const QPointF& B) {
  return (A.x() - O.x()) * (B.y() - O.y()) - (A.y() - O.y()) * (B.x() - O.x());
}
}  // anonymous namespace

QPolygonF PolygonUtils::convexHull(std::vector<QPointF> pointCloud) {
  // "Monotone chain" algorithm.
  // http://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain

  const auto n = static_cast<int>(pointCloud.size());
  int k = 0;
  std::vector<QPointF> hull(n * 2);

  // Sort points by x, then y.
  std::sort(pointCloud.begin(), pointCloud.end(), LexicographicPointComparator());

  // Build lower hull.
  for (int i = 0; i < n; ++i) {
    while (k >= 2 && cross(hull[k - 2], hull[k - 1], pointCloud[i]) <= 0) {
      k--;
    }
    hull[k++] = pointCloud[i];
  }
  // Build upper hull.
  for (int i = n - 2, t = k + 1; i >= 0; --i) {
    while (k >= t && cross(hull[k - 2], hull[k - 1], pointCloud[i]) <= 0) {
      k--;
    }
    hull[k++] = pointCloud[i];
  }

  hull.resize(k);

  QPolygonF poly(k);
  for (const QPointF& pt : hull) {
    poly << pt;
  }
  return poly;
}
}  // namespace imageproc