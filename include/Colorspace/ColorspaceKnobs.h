/* MIT License — Copyright (c) 2025 Gonzalo Rojas
 *
 * ColorspaceKnobs.h
 *
 * Drop-in Nuke UI component. Embeds colorspace conversion knobs into any plugin.
 *
 * Usage:
 *   colorspace::KnobPair _cs;                        // member
 *   _cs.hideInputSlot();                             // optional flags before addKnobs()
 *   _cs.hideSwap();
 *   _cs.hideBradford();
 *   _cs.addKnobs(f);                                 // in knobs()
 *   if (_cs.knobChanged(k, this)) return 1;          // in knob_changed()
 *   auto xf    = _cs.buildTransform();               // before pixel loop
 *   auto xfInv = _cs.buildTransformInverse();        // optional inverse
 *   RGBcolor out = colorspace::transform(xf, in);    // per pixel
 */

#pragma once

#include <DDImage/Knobs.h>
#include <DDImage/Op.h>

#include "Colorspace/ColorspaceConstants.h"
#include "Colorspace/ColorspaceCore.h"

namespace colorspace
{

  // ─────────────────────────────────────────────────────────────────────────
  // KnobSet — one slot (in OR out)
  // ─────────────────────────────────────────────────────────────────────────

  class KnobSet
  {
   public:
    explicit KnobSet(const char* prefix = "", const char* label = "in")
        : prefix_(prefix), label_(label)
    {
    }

    KnobSet(const KnobSet&) = delete;
    KnobSet& operator=(const KnobSet&) = delete;

    /// Call before addKnobs() to hide all three knobs of this slot.
    void hide() { hidden_ = true; }

    void addKnobs(DD::Image::Knob_Callback f)
    {
      using namespace DD::Image;

      Enumeration_knob(f, &curveIdx_, Constants::COLOR_CURVE, kname("colorspace"), label_);
      Tooltip(f, "Transfer curve / encoding");
      if(hidden_) SetFlags(f, Knob::INVISIBLE);

      Enumeration_knob(f, &whiteIdx_, Constants::WHITEPOINT, kname("illuminant"), "");
      Tooltip(f, "Whitepoint (illuminant)");
      ClearFlags(f, Knob::STARTLINE);
      if(hidden_) SetFlags(f, Knob::INVISIBLE);

      Enumeration_knob(f, &primIdx_, Constants::PRIMARY_RGB, kname("primaries"), "");
      Tooltip(f, "RGB primaries");
      ClearFlags(f, Knob::STARTLINE);
      if(hidden_) SetFlags(f, Knob::INVISIBLE);
    }

    bool knobChanged(DD::Image::Knob* k)
    {
      return k->is(kname("colorspace")) || k->is(kname("illuminant")) || k->is(kname("primaries"));
    }

    int curveIndex() const { return curveIdx_; }
    int whitepointIndex() const { return whiteIdx_; }
    int primaryIndex() const { return primIdx_; }

   private:
    const char* prefix_;
    const char* label_;
    bool hidden_ = false;

    int curveIdx_ = Constants::COLOR_LINEAR;
    int whiteIdx_ = Constants::WHITE_D65;
    int primIdx_ = Constants::PRIM_COLOR_SRGB;

    const char* kname(const char* base) const
    {
      static char buf[64];
      snprintf(buf, sizeof(buf), "%s%s", prefix_, base);
      return buf;
    }
  };

  // ─────────────────────────────────────────────────────────────────────────
  // KnobPair — in row + out row + swap button + single Bradford toggle
  // ─────────────────────────────────────────────────────────────────────────

  class KnobPair
  {
   public:
    KnobPair() : in_("in_", "in"), out_("out_", "out") {}

    KnobPair(const KnobPair&) = delete;
    KnobPair& operator=(const KnobPair&) = delete;

    // ── Flags — call before addKnobs() ───────────────────────────────────────

    /// Hides the entire input slot (in_colorspace, in_illuminant, in_primaries).
    void hideInputSlot() { in_.hide(); }

    /// Hides the entire output slot (out_colorspace, out_illuminant, out_primaries).
    void hideOutputSlot() { out_.hide(); }

    /// Hides the swap in/out button.
    void hideSwap() { hideSwap_ = true; }

    /// Hides the Bradford matrix toggle.
    void hideBradford() { hideBradford_ = true; }

    // ── Knob registration ────────────────────────────────────────────────────

    void addKnobs(DD::Image::Knob_Callback f)
    {
      using namespace DD::Image;

      in_.addKnobs(f);
      out_.addKnobs(f);

      Button(f, "cs_swap", "swap in/out");
      SetFlags(f, Knob::STARTLINE);
      if(hideSwap_) SetFlags(f, Knob::INVISIBLE);

      Bool_knob(f, (bool*)&bradford_, "bradford_matrix", "Bradford matrix");
      Tooltip(f,
              "Apply Bradford chromatic adaptation between illuminants. "
              "Leave off to match Nuke's native Colorspace node.");
      if(hideBradford_) SetFlags(f, Knob::INVISIBLE);
    }

    // ── knob_changed ─────────────────────────────────────────────────────────

    bool knobChanged(DD::Image::Knob* k, DD::Image::Op* op)
    {
      if(k->is("cs_swap")) {
        swapKnobs(op);
        return true;
      }
      if(in_.knobChanged(k) || out_.knobChanged(k) || k->is("bradford_matrix")) return true;

      return false;
    }

    // ── Transform builders ───────────────────────────────────────────────────

    /// in -> out.  Call once before the pixel loop.
    ColorTransform buildTransform() const
    {
      ColorTransform xf;
      xf.curveIn = out_.curveIndex();  // encoded->linear uses out curve
      xf.whiteIn = in_.whitepointIndex();
      xf.primIn = in_.primaryIndex();
      xf.curveOut = in_.curveIndex();  // linear->encoded uses in curve
      xf.whiteOut = out_.whitepointIndex();
      xf.primOut = out_.primaryIndex();
      xf.useBradfordCAT = (bradford_ != 0);
      xf.buildMatrix();
      return xf;
    }

    /// out -> in  (inverse of buildTransform).  Call once before the pixel loop.
    ColorTransform buildTransformInverse() const
    {
      ColorTransform xf;
      xf.curveIn = in_.curveIndex();  // swap vs buildTransform
      xf.whiteIn = out_.whitepointIndex();
      xf.primIn = out_.primaryIndex();
      xf.curveOut = out_.curveIndex();  // swap vs buildTransform
      xf.whiteOut = in_.whitepointIndex();
      xf.primOut = in_.primaryIndex();
      xf.useBradfordCAT = (bradford_ != 0);
      xf.buildMatrix();
      return xf;
    }

    KnobSet& inSlot() { return in_; }
    KnobSet& outSlot() { return out_; }

   private:
    KnobSet in_;
    KnobSet out_;
    int bradford_ = 0;
    bool hideSwap_ = false;
    bool hideBradford_ = false;

    void swapKnobs(DD::Image::Op* op)
    {
      int ci = in_.curveIndex(), wi = in_.whitepointIndex(), pi = in_.primaryIndex();
      int co = out_.curveIndex(), wo = out_.whitepointIndex(), po = out_.primaryIndex();

      auto set = [&](const char* name, int val) {
        DD::Image::Knob* k = op->knob(name);
        if(k) k->set_value(val);
      };

      set("in_colorspace", co);
      set("out_colorspace", ci);
      set("in_illuminant", wo);
      set("out_illuminant", wi);
      set("in_primaries", po);
      set("out_primaries", pi);
    }
  };

}  // namespace colorspace