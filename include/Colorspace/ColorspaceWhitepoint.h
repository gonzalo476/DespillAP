#ifndef COLORSPACE_WHITEPOINT_H
#define COLORSPACE_WHITEPOINT_H

#include <DDImage/DDMath.h>
#include <DDImage/Matrix3.h>
#include <DDImage/Vector3.h>

#include <algorithm>
#include <array>
#include <cmath>

#include "Colorspace/ColorspaceAliases.h"
#include "Colorspace/ColorspaceData.h"

// -----------------------------------------------------------------------------
// All matrix math uses float[9] in ROW-MAJOR order:
//   m[0] m[1] m[2]   <- row 0
//   m[3] m[4] m[5]   <- row 1
//   m[6] m[7] m[8]   <- row 2
//
// No dependency on Nuke Matrix3 internals for computation.
// Matrix3 is only used as a final wrapper when Nuke APIs require it.
// -----------------------------------------------------------------------------

using Mat9 = std::array<float, 9>;
using Vec3 = std::array<float, 3>;

// v = M * v  (row-major 3x3 * column vector)
inline Vec3 mat_mul_vec(const Mat9& m, const Vec3& v)
{
  return {m[0] * v[0] + m[1] * v[1] + m[2] * v[2], m[3] * v[0] + m[4] * v[1] + m[5] * v[2],
          m[6] * v[0] + m[7] * v[1] + m[8] * v[2]};
}

// C = A * B  (both row-major)
inline Mat9 mat_mul_mat(const Mat9& A, const Mat9& B)
{
  Mat9 C{};
  for(int r = 0; r < 3; ++r)
    for(int c = 0; c < 3; ++c)
      for(int k = 0; k < 3; ++k) C[r * 3 + c] += A[r * 3 + k] * B[k * 3 + c];
  return C;
}

// Invert a 3x3 row-major matrix via cofactors
inline Mat9 mat_inverse(const Mat9& m)
{
  float det = m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6]) +
              m[2] * (m[3] * m[7] - m[4] * m[6]);
  float inv = 1.0f / det;
  return {(m[4] * m[8] - m[5] * m[7]) * inv, -(m[1] * m[8] - m[2] * m[7]) * inv,
          (m[1] * m[5] - m[2] * m[4]) * inv, -(m[3] * m[8] - m[5] * m[6]) * inv,
          (m[0] * m[8] - m[2] * m[6]) * inv, -(m[0] * m[5] - m[2] * m[3]) * inv,
          (m[3] * m[7] - m[4] * m[6]) * inv, -(m[0] * m[7] - m[1] * m[6]) * inv,
          (m[0] * m[4] - m[1] * m[3]) * inv};
}

inline Mat9 mat_diag(float a, float b, float c)
{
  return {a, 0, 0, 0, b, 0, 0, 0, c};
}
inline Mat9 mat_identity()
{
  return {1, 0, 0, 0, 1, 0, 0, 0, 1};
}

// Wrap row-major Mat9 into Nuke Matrix3 (column-major).
// Nuke Matrix3(a,b,c, d,e,f, g,h,i) stores col0={a,b,c}, col1={d,e,f}, col2={g,h,i}
// so we transpose on the way in.
inline DD::Image::Matrix3 toNukeMatrix(const Mat9& m)
{
  return DD::Image::Matrix3(m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8]);
}

// -----------------------------------------------------------------------------
// Chromaticity helpers
// -----------------------------------------------------------------------------

inline Vec3 xyY_to_XYZ(const float* xy)
{
  float x = xy[0], y = xy[1];
  float safeY = MAX(y, 1e-10f);
  return {x / safeY, 1.0f, (1.0f - x - y) / safeY};
}

// -----------------------------------------------------------------------------
// Chromatic adaptation  (Von Kries / Bradford / CAT02)
// Returns row-major CAT matrix: srcWhite -> dstWhite
// Formula: CAT = M^-1 * diag(dstCone/srcCone) * M
// -----------------------------------------------------------------------------

inline Mat9 calcWhite(const float* srcWhite, const float* dstWhite, const float* catMat)
{
  if(catMat == nullptr) return mat_identity();

  Mat9 M = {catMat[0], catMat[1], catMat[2], catMat[3], catMat[4],
            catMat[5], catMat[6], catMat[7], catMat[8]};

  Vec3 srcCone = mat_mul_vec(M, xyY_to_XYZ(srcWhite));
  Vec3 dstCone = mat_mul_vec(M, xyY_to_XYZ(dstWhite));

  Mat9 D = mat_diag(dstCone[0] / MAX(srcCone[0], 1e-10f), dstCone[1] / MAX(srcCone[1], 1e-10f),
                    dstCone[2] / MAX(srcCone[2], 1e-10f));

  return mat_mul_mat(mat_inverse(M), mat_mul_mat(D, M));
}

// -----------------------------------------------------------------------------
// RGB primaries -> RGB<->XYZ matrix  (Lindbloom method)
// primaries: float[6] = { rx,ry, gx,gy, bx,by }
// whitepoint: float[2] = { wx, wy }
// Returns row-major RGB->XYZ matrix. Use mat_inverse() to get XYZ->RGB.
// -----------------------------------------------------------------------------

inline Mat9 calcRGBtoXYZ(const float* primaries, const float* whitepoint)
{
  Vec3 Xr = xyY_to_XYZ(primaries + 0);
  Vec3 Xg = xyY_to_XYZ(primaries + 2);
  Vec3 Xb = xyY_to_XYZ(primaries + 4);

  // Primaries as columns (row-major):
  Mat9 M = {Xr[0], Xg[0], Xb[0], Xr[1], Xg[1], Xb[1], Xr[2], Xg[2], Xb[2]};

  Vec3 S = mat_mul_vec(mat_inverse(M), xyY_to_XYZ(whitepoint));

  return {S[0] * Xr[0], S[1] * Xg[0], S[2] * Xb[0], S[0] * Xr[1], S[1] * Xg[1],
          S[2] * Xb[1], S[0] * Xr[2], S[1] * Xg[2], S[2] * Xb[2]};
}

#endif  // COLORSPACE_WHITEPOINT_H