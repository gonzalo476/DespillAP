/* MIT License — Copyright (c) 2025 Gonzalo Rojas
 *
 * ColorspaceKnobs.h  (v2)
 *
 * Drop-in Nuke UI component. Registra knobs de colorspace como funciones libres.
 *
 *   colorspace::ColorspaceIn_knob (f, &ci, &wi, &pi);
 *   colorspace::ColorspaceOut_knob(f, &co, &wo, &po);
 *   colorspace::ColorspaceSwap_knob(f);
 *   colorspace::ColorspaceBradford_knob(f, &brad);
 *
 *   auto xf = colorspace::buildTransform(ci, wi, pi, co, wo, po, brad);
 *   RGBcolor out = colorspace::transform(xf, in);
 */

#pragma once

#include <DDImage/Knobs.h>
#include <DDImage/Op.h>

#include <cstdio>

#include "Colorspace/ColorspaceConstants.h"
#include "Colorspace/ColorspaceCore.h"

namespace colorspace
{

  namespace detail
  {
    inline const char* kname(const char* prefix, const char* suffix)
    {
      static char buf[64];
      std::snprintf(buf, sizeof(buf), "%s_%s", prefix, suffix);
      return buf;
    }
  }  // namespace detail

  // ─────────────────────────────────────────────────────────────────────────────
  // Slot genérico — úsalo directamente si necesitas un prefijo arbitrario.
  // Genera: {prefix}_colorspace  /  {prefix}_illuminant  /  {prefix}_primaries
  // ─────────────────────────────────────────────────────────────────────────────

  inline void ColorspaceSlot_knob(DD::Image::Knob_Callback f, int* curveIdx, int* whiteIdx,
                                  int* primIdx, const char* prefix, const char* label,
                                  bool visible = true)
  {
    using namespace DD::Image;

    Enumeration_knob(f, curveIdx, Constants::COLOR_CURVE, detail::kname(prefix, "colorspace"),
                     label);
    Tooltip(f, "Transfer curve / encoding");
    if(!visible) SetFlags(f, Knob::HIDDEN);

    Enumeration_knob(f, whiteIdx, Constants::WHITEPOINT, detail::kname(prefix, "illuminant"), "");
    Tooltip(f, "Whitepoint (illuminant)");
    ClearFlags(f, Knob::STARTLINE);
    if(!visible) SetFlags(f, Knob::HIDDEN);

    Enumeration_knob(f, primIdx, Constants::PRIMARY_RGB, detail::kname(prefix, "primaries"), "");
    Tooltip(f, "RGB primaries");
    ClearFlags(f, Knob::STARTLINE);
    if(!visible) SetFlags(f, Knob::HIDDEN);
  }

  // Slot de entrada — prefijo default "in"
  inline void ColorspaceIn_knob(DD::Image::Knob_Callback f, int* curveIdx, int* whiteIdx,
                                int* primIdx, const char* prefix = "in", const char* label = "in",
                                bool visible = true)
  {
    ColorspaceSlot_knob(f, curveIdx, whiteIdx, primIdx, prefix, label, visible);
  }

  // Slot de salida — prefijo default "out"
  inline void ColorspaceOut_knob(DD::Image::Knob_Callback f, int* curveIdx, int* whiteIdx,
                                 int* primIdx, const char* prefix = "out",
                                 const char* label = "out", bool visible = true)
  {
    ColorspaceSlot_knob(f, curveIdx, whiteIdx, primIdx, prefix, label, visible);
  }

  // Botón swap — nombre interno: "swap"
  inline void ColorspaceSwap_knob(DD::Image::Knob_Callback f, bool visible = true)
  {
    using namespace DD::Image;
    Button(f, "swap", "swap in/out");
    SetFlags(f, Knob::STARTLINE);
    if(!visible) SetFlags(f, Knob::HIDDEN);
  }

  // Toggle Bradford — nombre interno: "bradford_matrix"
  inline void ColorspaceBradford_knob(DD::Image::Knob_Callback f, int* bradfordFlag)
  {
    using namespace DD::Image;
    Bool_knob(f, (bool*)bradfordFlag, "bradford_matrix", "Bradford matrix");
    Tooltip(f,
            "Applies Bradford chromatic adaptation between whitepoints. "
            "Leave off to match Nuke's native Colorspace node behaviour.");
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // Llamar desde knob_changed(). Devuelve true si el knob pertenece a este sistema.
  // inPrefix / outPrefix deben coincidir con los usados al registrar los slots.
  // ─────────────────────────────────────────────────────────────────────────────

  inline bool colorspace_knob_changed(DD::Image::Knob* k, const char* inPrefix = "in",
                                      const char* outPrefix = "out")
  {
    return k->is(detail::kname(inPrefix, "colorspace")) ||
           k->is(detail::kname(inPrefix, "illuminant")) ||
           k->is(detail::kname(inPrefix, "primaries")) ||
           k->is(detail::kname(outPrefix, "colorspace")) ||
           k->is(detail::kname(outPrefix, "illuminant")) ||
           k->is(detail::kname(outPrefix, "primaries")) || k->is("bradford_matrix") ||
           k->is("swap");
  }

  // Intercambia los valores de los slots in↔out. Llamar cuando k->is("swap").
  inline void swap_colorspace_knobs(DD::Image::Op* op, const char* inPrefix = "in",
                                    const char* outPrefix = "out")
  {
    using namespace DD::Image;

    auto get = [&](const char* prefix, const char* suffix) -> int {
      Knob* k = op->knob(detail::kname(prefix, suffix));
      return k ? static_cast<int>(k->get_value()) : 0;
    };
    auto set = [&](const char* prefix, const char* suffix, int val) {
      Knob* k = op->knob(detail::kname(prefix, suffix));
      if(k) k->set_value(val);
    };

    int ci = get(inPrefix, "colorspace"), wi = get(inPrefix, "illuminant"),
        pi = get(inPrefix, "primaries");
    int co = get(outPrefix, "colorspace"), wo = get(outPrefix, "illuminant"),
        po = get(outPrefix, "primaries");

    set(inPrefix, "colorspace", co);
    set(outPrefix, "colorspace", ci);
    set(inPrefix, "illuminant", wo);
    set(outPrefix, "illuminant", wi);
    set(inPrefix, "primaries", po);
    set(outPrefix, "primaries", pi);
  }

  // ─────────────────────────────────────────────────────────────────────────────
  // Construye el ColorTransform. Llamar una vez antes del pixel loop.
  // ─────────────────────────────────────────────────────────────────────────────

  inline ColorTransform buildTransform(int curveIn, int whiteIn, int primIn, int curveOut,
                                       int whiteOut, int primOut, int bradfordFlag = 0)
  {
    ColorTransform xf;
    xf.curveIn = curveIn;
    xf.whiteIn = whiteIn;
    xf.primIn = primIn;
    xf.curveOut = curveOut;
    xf.whiteOut = whiteOut;
    xf.primOut = primOut;
    xf.useBradfordCAT = (bradfordFlag != 0);
    xf.buildMatrix();
    return xf;
  }

  // Igual que buildTransform pero con in↔out invertidos.
  inline ColorTransform buildTransformInverse(int curveIn, int whiteIn, int primIn, int curveOut,
                                              int whiteOut, int primOut, int bradfordFlag = 0)
  {
    return buildTransform(curveOut, whiteOut, primOut, curveIn, whiteIn, primIn, bradfordFlag);
  }

}  // namespace colorspace