/* MIT License — Copyright (c) 2025 Gonzalo Rojas
 *
 * GColorspaceCore.h
 *
 * Self-contained color science library.  Zero dependency on the Nuke UI or
 * knob system — safe to include in any plugin, unit-test, or command-line tool.
 *
 * What lives here
 * ───────────────
 *  • Transfer-curve functions   (LinTo* / *ToLin)
 *  • Matrix math helpers        (Mat9, Vec3, mat_mul_*, mat_inverse …)
 *  • Whitepoint / CAT           (calcWhite, calcRGBtoXYZ)
 *  • ColorTransform struct      (plain-data description of one conversion)
 *  • colorspace::transform()    (apply a ColorTransform to one RGBcolor)
 *  • colorspace::toLinear()     (encoded → linear, curve only)
 *  • colorspace::fromLinear()   (linear → encoded, curve only)
 *
 * Usage in a second plugin
 * ────────────────────────
 *   #include "GColorspaceCore.h"
 *   #include "GColorspaceKnobs.h"   // optional — adds the UI helpers
 *
 *   // Inside pixel_engine / engine():
 *   colorspace::ColorTransform xf = knobs.buildTransform();  // from GColorspaceKnobs
 *   RGBcolor out = colorspace::transform(xf, in);
 *
 * All functions are inline to avoid ODR violations when the header is
 * included in multiple translation units.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cmath>

// Nuke SDK — only DDMath (MAX macro) and the bare minimum for Mat/Vec names.
// No Knob, no Op, no Row.
#include <DDImage/DDMath.h>

// ── Internal helpers ─────────────────────────────────────────────────────────
#include "Colorspace/ColorspaceAliases.h"  // RGBcolor, XYZMat, TransformDispatcher, ColorLut
#include "Colorspace/ColorspaceConstants.h"  // enums: Colorspaces, Whitepoint, PrimaryColorspaces, CatMethods
#include "Colorspace/ColorspaceData.h"        // raw float[] primaries, whitepoints, XYZ matrices
#include "Colorspace/ColorspaceDispatcher.h"  // TransformInDispatcher / TransformOutDispatcher
#include "Colorspace/ColorspaceLut.h"         // all transfer-curve functions
#include "Colorspace/ColorspaceUtils.h"       // isInXYZMatrix, removeExp

// ─────────────────────────────────────────────────────────────────────────────
// Matrix math  (row-major 3×3)
// ─────────────────────────────────────────────────────────────────────────────

using Mat9 = std::array<float, 9>;
using Vec3 = std::array<float, 3>;

namespace colorspace
{

  // v = M * v
  inline Vec3 mat_mul_vec(const Mat9& m, const Vec3& v)
  {
    return {m[0] * v[0] + m[1] * v[1] + m[2] * v[2], m[3] * v[0] + m[4] * v[1] + m[5] * v[2],
            m[6] * v[0] + m[7] * v[1] + m[8] * v[2]};
  }

  // C = A * B
  inline Mat9 mat_mul_mat(const Mat9& A, const Mat9& B)
  {
    Mat9 C{};
    for(int r = 0; r < 3; ++r)
      for(int c = 0; c < 3; ++c)
        for(int k = 0; k < 3; ++k) C[r * 3 + c] += A[r * 3 + k] * B[k * 3 + c];
    return C;
  }

  // Invert a 3×3 row-major matrix via cofactors
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

  inline Mat9 mat_identity()
  {
    return {1, 0, 0, 0, 1, 0, 0, 0, 1};
  }
  inline Mat9 mat_diag(float a, float b, float c)
  {
    return {a, 0, 0, 0, b, 0, 0, 0, c};
  }

  // ─────────────────────────────────────────────────────────────────────────
  // Chromaticity / whitepoint helpers
  // ─────────────────────────────────────────────────────────────────────────

  inline Vec3 xyY_to_XYZ(const float* xy)
  {
    float x = xy[0], y = xy[1];
    float safeY = MAX(y, 1e-10f);
    return {x / safeY, 1.0f, (1.0f - x - y) / safeY};
  }

  /// Chromatic adaptation matrix: srcWhite → dstWhite (Bradford or CAT02).
  /// Pass catMat = nullptr to get identity (no adaptation).
  inline Mat9 calcWhite(const float* srcWhite, const float* dstWhite, const float* catMat)
  {
    if(catMat == nullptr) return mat_identity();

    Mat9 M = {catMat[0], catMat[1], catMat[2], catMat[3], catMat[4],
              catMat[5], catMat[6], catMat[7], catMat[8]};

    Vec3 src = mat_mul_vec(M, xyY_to_XYZ(srcWhite));
    Vec3 dst = mat_mul_vec(M, xyY_to_XYZ(dstWhite));

    Mat9 D = mat_diag(dst[0] / MAX(src[0], 1e-10f), dst[1] / MAX(src[1], 1e-10f),
                      dst[2] / MAX(src[2], 1e-10f));

    return mat_mul_mat(mat_inverse(M), mat_mul_mat(D, M));
  }

  /// RGB primaries → RGB↔XYZ matrix (Lindbloom method).
  /// primaries: float[6] = { rx,ry, gx,gy, bx,by }
  /// whitepoint: float[2] = { wx, wy }
  /// Returns RGB→XYZ.  Use mat_inverse() to get XYZ→RGB.
  inline Mat9 calcRGBtoXYZ(const float* primaries, const float* whitepoint)
  {
    Vec3 Xr = xyY_to_XYZ(primaries + 0);
    Vec3 Xg = xyY_to_XYZ(primaries + 2);
    Vec3 Xb = xyY_to_XYZ(primaries + 4);

    Mat9 M = {Xr[0], Xg[0], Xb[0], Xr[1], Xg[1], Xb[1], Xr[2], Xg[2], Xb[2]};
    Vec3 S = mat_mul_vec(mat_inverse(M), xyY_to_XYZ(whitepoint));

    return {S[0] * Xr[0], S[1] * Xg[0], S[2] * Xb[0], S[0] * Xr[1], S[1] * Xg[1],
            S[2] * Xb[1], S[0] * Xr[2], S[1] * Xg[2], S[2] * Xb[2]};
  }

  // ─────────────────────────────────────────────────────────────────────────
  // ColorTransform — plain-data description of one full conversion.
  //
  // Build once per frame (or whenever knobs change) and reuse for every pixel.
  // Designed to be filled either manually or by GColorspaceKnobs::buildTransform().
  // ─────────────────────────────────────────────────────────────────────────

  struct ColorTransform
  {
    // Transfer curves
    int curveIn = Constants::COLOR_LINEAR;  ///< Constants::Colorspaces value
    int curveOut = Constants::COLOR_LINEAR;

    // Whitepoints (Constants::Whitepoint values)
    int whiteIn = Constants::WHITE_D65;
    int whiteOut = Constants::WHITE_D65;

    // Primary colorspaces (Constants::PrimaryColorspaces values)
    int primIn = Constants::PRIM_COLOR_SRGB;
    int primOut = Constants::PRIM_COLOR_SRGB;

    // When true, a Bradford CAT is applied between whiteIn and whiteOut
    // (in addition to the Lindbloom whitepoint embedding).
    // Leave false to match Nuke's native Colorspace node behaviour.
    bool useBradfordCAT = false;

    /// Pre-built combined RGB→RGB matrix (XYZ→RGB ∘ CAT ∘ RGB→XYZ).
    /// Filled automatically by buildMatrix() — do not set manually.
    Mat9 combinedMatrix = {1, 0, 0, 0, 1, 0, 0, 0, 1};

    /// Recomputes combinedMatrix from the current field values.
    /// Call after changing any field, or use GColorspaceKnobs::buildTransform()
    /// which calls this automatically.
    void buildMatrix()
    {
      if(isInXYZMatrix(curveIn) || isInXYZMatrix(curveOut)) {
        combinedMatrix = mat_identity();
        return;
      }

      const float* wIn = WhitepointDispatcher(whiteIn);
      const float* wOut = WhitepointDispatcher(whiteOut);

      Mat9 rgbToXYZ = calcRGBtoXYZ(PrimaryDispatcher(primIn), wIn);
      Mat9 xyzToRGB = mat_inverse(calcRGBtoXYZ(PrimaryDispatcher(primOut), wOut));

      Mat9 cat = mat_identity();
      if(useBradfordCAT && whiteIn != whiteOut) cat = calcWhite(wIn, wOut, bradford);

      combinedMatrix = mat_mul_mat(xyzToRGB, mat_mul_mat(cat, rgbToXYZ));
    }

    /// Returns true when no pixel work needs to be done.
    bool isPassthrough() const
    {
      return curveIn == curveOut && whiteIn == whiteOut && primIn == primOut;
    }
  };

  // ─────────────────────────────────────────────────────────────────────────
  // Core transform functions
  // ─────────────────────────────────────────────────────────────────────────

  /// Encoded → linear using only the input transfer curve.
  inline RGBcolor toLinear(const ColorTransform& xf, const RGBcolor& p)
  {
    return TransformOutDispatcher(xf.curveIn)(p);
  }

  /// Linear → encoded using only the output transfer curve.
  inline RGBcolor fromLinear(const ColorTransform& xf, const RGBcolor& p)
  {
    return TransformInDispatcher(xf.curveOut)(p);
  }

  /// Full pipeline: curve-in → matrix → curve-out.
  ///
  /// @param xf   ColorTransform built (and matrix pre-computed) before the pixel loop.
  /// @param p    Input encoded pixel.
  /// @return     Output encoded pixel.
  inline RGBcolor transform(const ColorTransform& xf, const RGBcolor& p)
  {
    // 1. Encoded → linear
    RGBcolor lin = TransformOutDispatcher(xf.curveIn)(p);

    // 2. CIE spaces use their own fixed matrices — no primaries/whitepoint step
    if(isInXYZMatrix(xf.curveIn) || isInXYZMatrix(xf.curveOut)) {
      RGBcolor out = TransformInDispatcher(xf.curveOut)(lin);
      return removeExp(out);
    }

    // 3. RGB → XYZ → (optional CAT) → XYZ → RGB  via pre-built matrix
    Vec3 v = {lin[0], lin[1], lin[2]};
    Vec3 out = mat_mul_vec(xf.combinedMatrix, v);

    // 4. Linear → encoded
    RGBcolor rgb = {out[0], out[1], out[2]};
    rgb = TransformInDispatcher(xf.curveOut)(rgb);

    return removeExp(rgb);
  }

  /// Convenience: build a ColorTransform and immediately apply it to one pixel.
  /// Useful for one-off conversions.  For pixel loops, build the transform once
  /// outside the loop and call transform(xf, p) per pixel.
  inline RGBcolor transformOnce(int curveIn, int whiteIn, int primIn, int curveOut, int whiteOut,
                                int primOut, bool useBradfordCAT, const RGBcolor& p)
  {
    ColorTransform xf;
    xf.curveIn = curveIn;
    xf.whiteIn = whiteIn;
    xf.primIn = primIn;
    xf.curveOut = curveOut;
    xf.whiteOut = whiteOut;
    xf.primOut = primOut;
    xf.useBradfordCAT = useBradfordCAT;
    xf.buildMatrix();
    return transform(xf, p);
  }

}  // namespace colorspace