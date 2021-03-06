// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_SPFIT_SQDISTAPPROXIMANT_H_
#define SCANTAILOR_SPFIT_SQDISTAPPROXIMANT_H_

#include <QLineF>

#include "MatMNT.h"
#include "VecNT.h"

namespace spfit {
class FrenetFrame;

/**
 * A quadratic function of the form:\n
 * F(x) = x^T * A * x + b^T * x + c\n
 * where:
 * \li x: a column vector representing a point in 2D space.
 * \li A: a 2x2 positive semidefinite matrix.
 * \li b: a 2 element column vector.
 * \li c: some constant.
 *
 * The function is meant to approximate the squared distance
 * to the target model.  It's only accurate in a neighbourhood
 * of some unspecified point.
 * The iso-curves of the function are rotated ellipses in a general case.
 *
 * \see Eq 8 in [1], Fig 4, 5 in [2].
 */
struct SqDistApproximant {
  Mat22d A;
  Vec2d b;
  double c;

  /**
   * Constructs a distance function that always evaluates to zero.
   * Passing it to Optimizer::addSample() will have no effect.
   */
  SqDistApproximant() : c(0) {}

  /**
   * \brief The general case constructor.
   *
   * We have a coordinate system at \p origin with orthonormal basis formed
   * by vectors \p u and \p v.  Given a point p in the global coordinate system,
   * the appoximant will evaluate to:
   * \code
   * sqdist = m * i^2 + n * j^2;
   * // Where i and j are projections onto u and v respectively.
   * // More precisely:
   * i = (p - origin) . u;
   * j = (p - origin) . v;
   * \endcode
   */
  SqDistApproximant(const Vec2d& origin, const Vec2d& u, const Vec2d& v, double m, double n);

  static SqDistApproximant pointDistance(const Vec2d& pt);

  static SqDistApproximant weightedPointDistance(const Vec2d& pt, double weight);

  static SqDistApproximant lineDistance(const QLineF& line);

  static SqDistApproximant weightedLineDistance(const QLineF& line, double weight);

  static SqDistApproximant curveDistance(const Vec2d& referencePoint,
                                         const FrenetFrame& frenetFrame,
                                         double signedCurvature);

  static SqDistApproximant weightedCurveDistance(const Vec2d& referencePoint,
                                                 const FrenetFrame& frenetFrame,
                                                 double signedCurvature,
                                                 double weight);

  double evaluate(const Vec2d& pt) const;
};
}  // namespace spfit
#endif  // ifndef SCANTAILOR_SPFIT_SQDISTAPPROXIMANT_H_
