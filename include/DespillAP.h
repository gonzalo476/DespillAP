
#ifndef DESPILL_AP_H
#define DESPILL_AP_H

#include "DDImage/DDMath.h"
#include "DDImage/Iop.h"
#include "DDImage/NukeWrapper.h"
using namespace DD::Image;
#include "DDImage/Format.h"
#include "DDImage/Knobs.h"
#include "DDImage/Row.h"
#include "DDImage/Tile.h"
#include "DDImage/Vector3.h"
#include "DDImage/Vector4.h"

#define HELP                                                                   \
  "DespillAP v1.0\n"                                                           \
  "\n"                                                                         \
  "DespillAP is a native Nuke node designed to remove color spill from "       \
  "images with precision and efficiency.\n"                                    \
  "\n"                                                                         \
  "Based on the algorithms and conceptual design of Adrian Pueyo's "           \
  "apDespill, DespillAP incorporates advanced features to provide both "       \
  "creative and technical control over the despill process.\n"                 \
  "\n"                                                                         \
  "Despill Color knob : selects the color you want to remove from the "        \
  "image.\n"                                                                   \
  "Absolute Mode knob : performs a despill operation toward a specific color " \
  "or emulates a key, similar to tools like Keylight.\n"                       \
  "Image Inputs       : allows connection of image inputs to define the "      \
  "despill color, respill color, or limits for a fully image-based despill "   \
  "workflow.\n"                                                                \
  "Tone Protection     : preserves key tones in the image during the despill " \
  "process.\n"                                                                 \
  "\n"                                                                         \
  "Tip: Default settings are optimized to avoid extra calculations, "          \
  "providing quick and effective results.\n"                                   \
  "\n"                                                                         \
  "Copyright 2025. Developed by Gonzalo Rojas.\n";

static const char *const CLASS = "DespillAP";

class DespillAPIop : public Iop
{
 public:
  // constructor
  DespillAPIop(Node *node);

  // destructor
  ~DespillAPIop() {}

  int minimum_inputs() const { return 4; }
  int maximum_inputs() const { return 4; }

  void knobs(Knob_Callback f);
  int knob_changed(Knob *k);

  void _validate(bool);

  void _request(int x, int y, int r, int t, ChannelMask channels, int count);

  void engine(int y, int l, int r, ChannelMask channels, Row &row);

  void ProcessCPU(int y, int x, int r, ChannelMask channels, Row &row);

  const char *input_label(int n, char *) const;
  void set_input(int i, Op *op, int input, int offset);

  static const Iop::Description d;

  const char *Class() const { return d.name; }
  const char *node_help() const { return HELP; }

 private:
  // spill knobs
  bool k_absMode;
  int k_colorType;
  int k_despillMath;
  float k_spillPick[3];
  float k_customWeight;

  // hue knobs
  float k_hueOffset;
  float k_hueLimit;
  Channel k_limitChannel;
  Knob *limitChannel_knob;
  Knob *invertLimitMask_knob;
  bool k_invertLimitMask;

  // protect tones knobs
  float k_protectColor[3];
  bool k_protectTones;
  float k_protectTolerance;
  float k_protectFalloff;
  bool k_protectPrev;
  float k_protectEffect;

  // respill knobs
  float k_respillColor[3];
  int k_respillMath;
  float k_blackPoint;
  float k_whitePoint;

  // output knobs
  int k_outputType;
  bool k_outputAlpha;
  bool k_invertAlpha;
  Channel k_outputSpillChannel;

  // connected inputs
  bool isSourceConnected;
  bool isLimitConnected;
  bool isColorConnected;
  bool isRespillConnected;

  // internal variables
  float _hueShift;
  int _clr;
  int _usePickedColor;
  int _returnColor;
  Vector3 normVec;
};

#endif  // DESPILL_AP_H