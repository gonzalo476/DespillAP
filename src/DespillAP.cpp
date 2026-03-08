
#include "DespillAP.h"

#include "Color.h"
#include "Constants.h"
#include "Pixel.h"

enum inputs {
  inputSource = 0,
  inputLimit = 1,
  inputColor = 2,
  inputRespill = 3,
};

DespillAPIop::DespillAPIop(Node *node) : Iop(node)
{
  inputs(3);
  k_limitChannel = Chan_Alpha;
  k_outputSpillChannel = Chan_Alpha;
  k_spillPick[0] = 0.0f;
  k_spillPick[1] = 1.0f;
  k_spillPick[2] = 0.0f;
  k_colorType = 3;
  k_absMode = 0;
  k_respillColor[0] = 1.0f;
  k_respillColor[1] = 1.0f;
  k_respillColor[2] = 1.0f;
  k_outputType = 0;
  k_outputAlpha = 1;
  k_invertAlpha = 1;
  k_despillMath = 0;
  k_customWeight = 0.0f;
  k_hueOffset = 0.0f;
  k_hueLimit = 1.0f;
  k_respillMath = 0;
  k_protectColor[0] = 0.0f;
  k_protectColor[1] = 0.0f;
  k_protectColor[2] = 0.0f;
  k_protectTolerance = 0.2f;
  k_protectFalloff = 2.0f;
  k_protectEffect = 1.0f;
  k_invertLimitMask = 1;
  k_blackPoint = 0.0f;
  k_whitePoint = 1.0f;

  isSourceConnected = false;
  isLimitConnected = false;
  isColorConnected = false;
  isRespillConnected = false;

  _returnColor = 0;
}

void DespillAPIop::knobs(Knob_Callback f)
{
  Divider(f);
  Enumeration_knob(f, &k_colorType, Constants::COLOR_TYPES, "color");
  Tooltip(f,
          "Select spill color: Red, Green, Blue channels, or use Color Picker. Disabled when Color "
          "input is connected");

  ClearFlags(f, Knob::STARTLINE);
  Bool_knob(f, &k_absMode, "absolute_mode", "Absolute Mode");
  Tooltip(
      f,
      "Normalize spill relative to picked color intensity. When off, uses raw spill calculation");

  Knob *pick_knob = Color_knob(f, k_spillPick, "pick");
  ClearFlags(f, Knob::MAGNITUDE | Knob::SLIDER);
  Tooltip(f,
          "Pick specific spill color. Automatically calculates hue shift from red reference. "
          "Disabled when Color input connected or when using channel buttons");

  Enumeration_knob(f, &k_despillMath, Constants::DESPILL_MATH_TYPES, "despill_math", "math");
  Tooltip(f, "Algorithm for despill calculation. Custom math enables the weight parameter below");

  Float_knob(f, &k_customWeight, IRange(-1, 1), "custom_weight", "");
  SetFlags(f, Knob::DISABLED);
  Tooltip(f, "Custom weight for despill calculation. Only active when Math is set to Custom");

  Divider(f, "<b>Hue</b>");

  Float_knob(f, &k_hueOffset, IRange(-30, 30), "hue_offset", "offset");
  Tooltip(f,
          "Fine-tune hue angle in degrees. Added to automatic shift from picked color, or used "
          "directly with channel selection");

  Float_knob(f, &k_hueLimit, IRange(0, 2), "hue_limit", "limit");
  Tooltip(f,
          "Maximum despill strength. Multiplied by limit mask if connected, controls how "
          "aggressive the despill can be");

  Input_Channel_knob(f, &k_limitChannel, 1, 1, "limit_channel", "mask");
  Tooltip(f,
          "Channel from Limit input to control despill strength per pixel. White = full strength, "
          "black = no despill");

  Bool_knob(f, &k_invertLimitMask, "invert_limit_mask", "invert");
  SetFlags(f, Knob::ENDLINE);
  Tooltip(f, "Invert limit mask values. Black areas get despill instead of white areas");

  Bool_knob(f, &k_protectTones, "protect_tones", "Protect Tones");
  Tooltip(f, "Enable protection of specific colors (like skin tones) from being despilled");

  Bool_knob(f, &k_protectPrev, "protect_preview", "Preview");
  SetFlags(f, Knob::DISABLED);
  ClearFlags(f, Knob::STARTLINE);
  Tooltip(
      f,
      "Preview protection matte. Shows protected areas multiplied by protection effect strength");

  BeginGroup(f, "Protect Tones");
  SetFlags(f, Knob::CLOSED);

  Knob *protectColor_knob = Color_knob(f, k_protectColor, "protect_color", "color");
  ClearFlags(f, Knob::MAGNITUDE | Knob::SLIDER);
  SetFlags(f, Knob::DISABLED);
  Tooltip(f,
          "Reference color to protect from despill (typically skin tone or important foreground "
          "color)");

  Float_knob(f, &k_protectTolerance, IRange(0, 1), "protect_tolerance", "tolerance");
  SetFlags(f, Knob::DISABLED);
  Tooltip(f,
          "Color similarity threshold for protection. Higher values protect more similar colors");

  Float_knob(f, &k_protectFalloff, IRange(0, 4), "protect_falloff", "falloff");
  SetFlags(f, Knob::DISABLED);
  Tooltip(f, "Softness of protection transition between protected and unprotected areas");

  Float_knob(f, &k_protectEffect, IRange(0, 10), "protect_effect", "effect");
  SetFlags(f, Knob::DISABLED);
  Tooltip(f,
          "Strength of protection effect. In preview mode, shows as multiplication factor for "
          "protected areas");

  EndGroup(f);

  Divider(f, "<b>Respill</b>");

  Enumeration_knob(f, &k_respillMath, Constants::RESPILL_MATH_TYPES, "respill_math", "math");
  Tooltip(f, "Algorithm for calculating luminance of spill and respill colors");

  Knob *respillColor_knob = Color_knob(f, k_respillColor, IRange(0, 4), "respill_color", "color");
  ClearFlags(f, Knob::MAGNITUDE | Knob::SLIDER);
  Tooltip(
      f,
      "Replacement color added where spill was removed. Multiplied by Respill input if connected");

  Float_knob(f, &k_blackPoint, IRange(0, 1), "luma_black", "blackpoint");
  Tooltip(f, "Lower luminance bound. Pixels below this value are fully clipped to 0.");

  Float_knob(f, &k_whitePoint, IRange(0, 1), "luma_white", "whitepoint");
  Tooltip(f, "Upper luminance bound. Pixels above this value are fully clipped to 1.");

  Divider(f, "<b>Output</b>");

  Enumeration_knob(f, &k_outputType, Constants::OUTPUT_TYPES, "output_despill", "output");
  Tooltip(f, "Output: Despilled image with respill color added, or raw spill matte");

  Bool_knob(f, &k_outputAlpha, "output_alpha", "Output Spill Alpha");
  ClearFlags(f, Knob::STARTLINE);
  Tooltip(
      f, "Generate alpha channel from spill amount. When off, passes through original input alpha");

  Bool_knob(f, &k_invertAlpha, "invert_alpha", "Invert");
  SetFlags(f, Knob::ENDLINE);
  Tooltip(f, "Invert spill alpha: spill areas become transparent (0) instead of opaque (1)");

  Input_Channel_knob(f, &k_outputSpillChannel, 1, 1, "output_spill_channel", "channel");
  SetFlags(f, Knob::ENDLINE);
  Tooltip(f,
          "Target channel for spill alpha output. Written as clamped values between 0.0 and 1.0");

  Spacer(f, 0);
}

int DespillAPIop::knob_changed(Knob *k)
{
  if(k->is("despill_math")) {
    Knob *despillMath_knob = k->knob("despill_math");
    Knob *customWeight_knob = k->knob("custom_weight");

    if(despillMath_knob->get_value() == 3) {
      customWeight_knob->enable();
    }
    else {
      customWeight_knob->disable();
    }
    return 1;
  }

  if(k->is("color")) {
    if(knob("color")->get_value() != 3) {
      knob("pick")->disable();
    }
    else {
      knob("pick")->enable();
    }
    return 1;
  }

  if(k->is("protect_tones")) {
    Knob *protectTones_knob = k->knob("protect_tones");
    Knob *protectColor_knob = k->knob("protect_color");
    Knob *protectTolerance_knob = k->knob("protect_tolerance");
    Knob *protectFalloff_knob = k->knob("protect_falloff");
    Knob *protectEffect_knob = k->knob("protect_effect");
    Knob *protectPreview_knob = k->knob("protect_preview");

    if(protectTones_knob->get_value() == 1) {
      protectColor_knob->enable();
      protectTolerance_knob->enable();
      protectFalloff_knob->enable();
      protectEffect_knob->enable();
      protectPreview_knob->enable();
    }
    else {
      protectColor_knob->disable();
      protectTolerance_knob->disable();
      protectFalloff_knob->disable();
      protectEffect_knob->disable();
      protectPreview_knob->disable();
    }
    return 1;
  }
  knob("tile_color")->set_value(0x8b8b8bff);  // node color
  return 0;
}

const char *DespillAPIop::input_label(int n, char *) const
{
  switch(n) {
    case 0:
      return "Source";
    case 1:
      return "Limit";
    case 2:
      return "Color";
    case 3:
      return "Respill";
    default:
      return 0;
  }
}

void DespillAPIop::set_input(int i, Op *inputOp, int input, int offset)
{
  Iop::set_input(i, inputOp, input, offset);
  bool isConnected = (inputOp && inputOp->node_name() != std::string("Black in root"));

  switch(i) {
    case inputSource:
      isSourceConnected = isConnected;
      break;
    case inputLimit:
      isLimitConnected = isConnected;
      break;
    case inputColor:
      isColorConnected = isConnected;
      break;
    case inputRespill:
      isRespillConnected = isConnected;
      break;
  }

  if(!isColorConnected) {
    knob("color")->enable();
    if(knob("color")->get_value() == 3) {
      knob("pick")->enable();
    }
  }
  else {
    knob("pick")->disable();
    knob("color")->disable();
  }
}

void DespillAPIop::_validate(bool for_real)
{
  copy_info(0);

  // setup output channels:
  // include all requested channels plus our spill output channel
  nuke::ChannelSet outChannels = channels();
  outChannels += k_outputSpillChannel;
  set_out_channels(outChannels);
  info_.turn_on(outChannels);

  // initialize normalization vector for colorspace calcs
  normVec = Vector3(1.0f, 1.0f, 1.0f);

  // get the picked spill color
  Vector3 pickSpill(k_spillPick);

  // determine color selection mode
  // and setup internal variables
  if(isColorConnected) {
    // color input is connected:
    // use automatic color detection
    _clr = 0;             // red channel
    _usePickedColor = 1;  // flag to use picked color
  }
  else if(k_colorType != Constants::COLOR_PICK) {
    // manual channel selection (Red/Green/Blue butons)
    _usePickedColor = 0;  // use channel selection, not picked color
    _clr = k_colorType;   // use selected channel (0=Red, 1=Green, 2=Blue)
  }
  else if(pickSpill.x == pickSpill.y && pickSpill.x == pickSpill.z) {
    // if picked color is grayscale (all rgb values equal)
    // this means that no valid color was picked,
    // so pass trought input unchanged
    _returnColor = 1;  // bypass all processing
  }
  else {
    // valid color was picked from picker knob
    _usePickedColor = 1;  // use the picked color
    _clr = 0;             // default processing channel
  }

  // calculate hue shift for non connected color mode
  if(!isColorConnected) {
    float _autoShift = 0.0f;

    if(_usePickedColor == 1) {
      // calculate automatic hue shift based on picked color
      // convert picked color and red reference to plane vectors for angle calc
      Vector3 v1 = color::VectorToPlane(k_spillPick, normVec);
      Vector3 v2 = color::VectorToPlane(Vector3(1.0f, 0.0f, 0.0f), normVec);  // red reference

      // calculate angle between picked color and red reference
      _autoShift = color::ColorAngle(v1, v2);
      _autoShift = _autoShift * 180.0f / M_PI_F;  // rads to deg
    }

    // final hue shift: user offset - automatic shift
    // this allows user to fine-tune the calculated shift
    _hueShift = k_hueOffset - _autoShift;
  }
}

void DespillAPIop::_request(int x, int y, int r, int t, ChannelMask channels, int count)
{
  // ensure RGB channels are always requested for processing
  nuke::ChannelSet requestedChannels = channels;
  requestedChannels += Mask_RGB;

  // request data fron input 'Source'
  input(0)->request(x, y, r, t, requestedChannels, count);

  // request limit matte if its connected to input 'Limit'
  // take only what fits from the Op format, based on the input limit.
  if(input(inputLimit) != nullptr) {
    input(inputLimit)->request(x, y, r, t, Mask_All, count);
  }

  // request color reference if its connected to input 'Color'
  if(input(inputColor) != nullptr) {
    input(inputColor)->request(x, y, r, t, Mask_RGB, count);
  }

  // request respill color if its connected to input 'Respill'
  if(input(inputRespill) != nullptr) {
    input(inputRespill)->request(x, y, r, t, Mask_RGB, count);
  }
}

void DespillAPIop::engine(int y, int x, int r, ChannelMask channels, Row &row)
{
  callCloseAfter(0);
  ProcessCPU(y, x, r, channels, row);
}

void DespillAPIop::ProcessCPU(int y, int x, int r, ChannelMask channels, Row &row)
{
  // FETCH INPUT ROWS

  ChannelSet requestedChannels = channels;
  requestedChannels += Mask_RGB;
  row.get(input0(), y, x, r, requestedChannels);

  // Copy non-RGB, non-spill-output channels straight through
  ChannelSet copyMask = channels - Mask_RGB - k_outputSpillChannel;
  row.pre_copy(row, copyMask);
  row.copy(row, copyMask, x, r);

  // Color reference row — always constructed, only fetched if connected
  Row color_row(x, r);
  if(input(inputColor) != nullptr) color_row.get(*input(inputColor), y, x, r, Mask_RGB);

  // Respill row — always constructed, only fetched if connected
  Row respill_row(x, r);
  if(input(inputRespill) != nullptr) respill_row.get(*input(inputRespill), y, x, r, Mask_RGB);

  // Limit matte row — always constructed, only fetched if connected
  Row limit_matte_row(x, r);
  if(input(inputLimit) != nullptr) limit_matte_row.get(*input(inputLimit), y, x, r, Mask_All);

  // PIXEL CURSORS

  // Primary source: read + write RGB
  pixel::PixelCursor src(row, x);
  for(auto ch : pixel::rgbChannels()) {
    src.addReadChannel(ch);
    src.addWriteChannel(ch);
  }

  // Color reference — always read (row is zeroed when not connected, matching original)
  pixel::RowReader colorRd(color_row, x);
  for(auto ch : pixel::rgbChannels()) colorRd.add(ch);

  // Respill — always read (same reason)
  pixel::RowReader respillRd(respill_row, x);
  for(auto ch : pixel::rgbChannels()) respillRd.add(ch);

  // Alpha pass-through
  pixel::RowReader alphaRd(row, x);
  alphaRd.add(Chan_Alpha);

  // Limit — always read; value is ignored when not connected (original uses limitPtr unconditionally)
  pixel::RowReader limitRd(limit_matte_row, x);
  limitRd.add(k_limitChannel);

  // Spill output channel — base pointer offset to row start, indexed by (x0 - x) like original
  float *spillOutBase =
      (channels & k_outputSpillChannel) ? row.writable(k_outputSpillChannel) + x : nullptr;

  // PIXEL LOOP

  for(int x0 = x; x0 < r; ++x0, src.advance(), colorRd.advance(), respillRd.advance(),
          alphaRd.advance(), limitRd.advance()) {
    // Read current pixel — always from all rows, matching original
    Vector3 rgb(src.read(0), src.read(1), src.read(2));
    Vector3 colorRgb(colorRd[0], colorRd[1], colorRd[2]);
    Vector3 respillRgb(respillRd[0], respillRd[1], respillRd[2]);

    // Early exit: _returnColor bypasses all processing.
    // Do NOT explicitly write output — Nuke's writable() already aliases
    // the input memory in this case, so the pixel passes through untouched.
    if(_returnColor == 1) continue;

    float spillMatte = 0.0f;
    float hueShift = 0.0f;
    float autoShift = 0.0f;
    Vector3 despillColor;

    // Determine despill color and hue shift — identical to original
    if(isColorConnected) {
      despillColor = colorRgb;
      Vector3 v1 = color::VectorToPlane(despillColor);
      Vector3 v2 = color::VectorToPlane(Vector3(1.0f, 0.0f, 0.0f));
      autoShift = color::ColorAngle(v1, v2);
      autoShift = autoShift * 180.0f / M_PI_F;
      hueShift = k_hueOffset - autoShift;
    }
    else {
      if(_usePickedColor == 1) {
        despillColor = Vector3(k_spillPick);
      }
      else {
        despillColor =
            Vector3(_clr == 0 ? 1.0f : 0.0f, _clr == 1 ? 1.0f : 0.0f, _clr == 2 ? 1.0f : 0.0f);
      }
      hueShift = _hueShift;
    }

    // Limit matte — read unconditionally, applied only when isLimitConnected
    float invertInputLimit = k_invertLimitMask ? (1.0f - limitRd[0]) : limitRd[0];
    float limitResult = isLimitConnected ? k_hueLimit * invertInputLimit : k_hueLimit;

    // Core despill
    Vector4 rawDespilled = color::Despill(rgb, hueShift, _clr, k_despillMath, limitResult,
                                          k_customWeight, k_protectTones, k_protectColor,
                                          k_protectTolerance, k_protectEffect, k_protectFalloff);

    // Protection preview mode
    if(k_protectPrev && k_protectTones) {
      float mask = clamp(rawDespilled.w * k_protectEffect, 0.0f, 1.0f);
      src.write(0, rgb.x * mask);
      src.write(1, rgb.y * mask);
      src.write(2, rgb.z * mask);
      continue;
    }

    // Spill vector (difference between original and despilled)
    Vector3 spillVec(rgb[0] - rawDespilled.x, rgb[1] - rawDespilled.y, rgb[2] - rawDespilled.z);

    float spillLuma = color::GetLuma(spillVec, k_respillMath);

    // Relative vs absolute mode
    Vector3 despilledRGB;
    Vector4 spillFull;
    float spillLumaFull;

    if(!k_absMode) {
      despilledRGB = Vector3(rawDespilled.x, rawDespilled.y, rawDespilled.z);
      spillFull = Vector4(spillVec.x, spillVec.y, spillVec.z, 0.0f);
      spillLumaFull = spillLuma;
    }
    else {
      Vector4 pickDespilled = color::Despill(
          despillColor, hueShift, _clr, k_despillMath, limitResult, k_customWeight, k_protectTones,
          k_protectColor, k_protectTolerance, k_protectEffect, k_protectFalloff);

      Vector3 pickSpill(despillColor.x - pickDespilled.x, despillColor.y - pickDespilled.y,
                        despillColor.z - pickDespilled.z);

      float pickSpillLuma = color::GetLuma(pickSpill, k_respillMath);

      spillLumaFull = (pickSpillLuma == 0.0f) ? 0.0f : spillLuma / pickSpillLuma;
      Vector3 scaledSpill = despillColor * spillLumaFull;
      spillFull = Vector4(scaledSpill.x, scaledSpill.y, scaledSpill.z, 0.0f);
      despilledRGB =
          Vector3(rgb[0] - scaledSpill.x, rgb[1] - scaledSpill.y, rgb[2] - scaledSpill.z);
      spillMatte = pickDespilled.w;
    }

    // Respill color — original always builds both and then picks one
    Vector3 respillBase(k_respillColor[0], k_respillColor[1], k_respillColor[2]);
    Vector3 respillInput(respillRgb.x, respillRgb.y, respillRgb.z);
    Vector3 finalRespill = isRespillConnected ? respillInput : respillBase;

    // Output type
    Vector4 result;
    if(k_outputType == Constants::OUTPUT_DESPILL) {
      float rangeLuma = color::LumaRange(spillLumaFull, k_blackPoint, k_whitePoint);
      result = Vector4(despilledRGB.x + finalRespill.x * rangeLuma,
                       despilledRGB.y + finalRespill.y * rangeLuma,
                       despilledRGB.z + finalRespill.z * rangeLuma, 0.0f);
      spillLumaFull = rangeLuma;
    }
    else {
      result = spillFull;
    }

    // Alpha output
    if(!k_outputAlpha) {
      spillMatte = alphaRd[0];
    }
    else if(!k_invertAlpha) {
      spillMatte = spillLumaFull;
    }
    else {
      spillMatte = 1.0f - spillLumaFull;
    }

    // Write spill output channel — indexed as (x0 - x) to match original's x0 offset
    if(spillOutBase != nullptr) {
      spillOutBase[x0 - x] = clamp(spillMatte, 0.0f, 1.0f);
    }

    // Write RGB
    src.write(0, result.x);
    src.write(1, result.y);
    src.write(2, result.z);
  }
}

// gpu
const char *DespillAPIop::gpuEngine_decl() const
{
  return "uniform float $$hueShift;      \n"
         "uniform float $$hueLimit;      \n"
         "uniform float $$customWeight;  \n"
         "uniform vec3  $$despillColor;  \n"
         "uniform vec3  $$respillColor;  \n"
         "uniform float $$blackPoint;    \n"
         "uniform float $$whitePoint;    \n"
         "uniform vec3  $$protectColor;  \n"
         "uniform float $$protectTol;    \n"
         "uniform float $$protectFall;   \n"
         "uniform float $$protectEffect; \n";
}

// ── gpuEngine_shader_hash_at ──────────────────────────────────────────────────

Hash DespillAPIop::gpuEngine_shader_hash_at(double time)
{
  Hash hash;
  // Only knobs that affect shader *structure* go here.
  // Uniforms (floats, colors) do NOT go here.
  hash.append(knob("absolute_mode")->get_value_at(time));
  hash.append(knob("protect_tones")->get_value_at(time));
  hash.append(knob("protect_preview")->get_value_at(time));
  hash.append(knob("output_despill")->get_value_at(time));
  hash.append(knob("invert_alpha")->get_value_at(time));
  hash.append(knob("output_alpha")->get_value_at(time));
  hash.append(knob("invert_limit_mask")->get_value_at(time));
  hash.append(knob("despill_math")->get_value_at(time));
  hash.append(knob("respill_math")->get_value_at(time));
  hash.append(knob("color")->get_value_at(time));  // determines _clr -> chan indices
  return hash;
}

// ── gpuEngine_body ────────────────────────────────────────────────────────────

const char *DespillAPIop::gpuEngine_body() const
{
  std::stringstream s;

  // Channel indices from _clr — horneados en el source
  // _clr: 0=R, 1=G, 2=B
  // chan0/chan1: the two non-spill channels
  int chan0, chan1;
  if(_clr == Constants::COLOR_RED) {
    chan0 = 1;
    chan1 = 2;  // G, B
  }
  else if(_clr == Constants::COLOR_GREEN) {
    chan0 = 0;
    chan1 = 2;  // R, B
  }
  else {
    chan0 = 0;
    chan1 = 1;  // R, G
  }

  s << "{ \n"

       // ── hueRotate — replica color::HueRotate() ───────────────────────────────
       // GLSL no permite funciones locales, se declaran como funciones globales en el body
       "vec3 hueRotate(vec3 rgb, float angle) { \n"
       "  if (abs(angle) < 0.0001) return rgb; \n"
       "  float cosA   = cos(angle * 0.01745329252); \n"
       "  float sinA   = sin(angle * 0.01745329252); \n"
       "  float sqrt3  = 1.73205080757; \n"
       "  float common = (rgb.r + rgb.g + rgb.b) * (1.0 - cosA) / 3.0; \n"
       "  return vec3( \n"
       "    common + rgb.r * cosA + (-rgb.g / sqrt3 + rgb.b / sqrt3) * sinA, \n"
       "    common + rgb.g * cosA + ( rgb.r / sqrt3 - rgb.b / sqrt3) * sinA, \n"
       "    common + rgb.b * cosA + (-rgb.r / sqrt3 + rgb.g / sqrt3) * sinA  \n"
       "  ); \n"
       "} \n"

       // ── despillLimit — math horneado ─────────────────────────────────────────
       "float despillLimit(vec3 rgb) { \n";

  if(k_despillMath == Constants::DESPILL_AVERAGE) {
    s << "  return (rgb[" << chan0 << "] + rgb[" << chan1 << "]) / 2.0; \n";
  }
  else if(k_despillMath == Constants::DESPILL_MAX) {
    s << "  return max(rgb[" << chan0 << "], rgb[" << chan1 << "]); \n";
  }
  else if(k_despillMath == Constants::DESPILL_MIN) {
    s << "  return min(rgb[" << chan0 << "], rgb[" << chan1 << "]); \n";
  }
  else {  // DESPILL_CUSTOM
    s << "  float w = ($$customWeight + 1.0) / 2.0; \n"
      << "  return rgb[" << chan0 << "] * w + rgb[" << chan1 << "] * (1.0 - w); \n";
  }

  s << "} \n"

       // ── getLuma — math horneado ──────────────────────────────────────────────
       "float getLuma(vec3 rgb) { \n";

  if(k_respillMath == Constants::LUMA_REC709) {
    s << "  return rgb.r * 0.2126 + rgb.g * 0.7152 + rgb.b * 0.0722; \n";
  }
  else if(k_respillMath == Constants::LUMA_CCIR601) {
    s << "  return rgb.r * 0.299 + rgb.g * 0.587 + rgb.b * 0.114; \n";
  }
  else if(k_respillMath == Constants::LUMA_REC2020) {
    s << "  return rgb.r * 0.2627 + rgb.g * 0.6780 + rgb.b * 0.0593; \n";
  }
  else if(k_respillMath == Constants::LUMA_AVERAGE) {
    s << "  return (rgb.r + rgb.g + rgb.b) / 3.0; \n";
  }
  else {  // LUMA_MAX
    s << "  return max(rgb.r, max(rgb.g, rgb.b)); \n";
  }

  s << "} \n"

       // ── lumaRange — replica color::LumaRange() ───────────────────────────────
       "float lumaRange(float luma) { \n"
       "  float bp = $$blackPoint; \n"
       "  float wp = $$whitePoint; \n"
       "  if (bp <= 0.0 && wp >= 1.0) return luma; \n"
       "  float range = wp - bp; \n"
       "  if (range <= 0.0) return luma; \n"
       "  return clamp((luma - bp) / range, 0.0, 1.0); \n"
       "} \n"

       // ── doDespill — replica color::Despill() ─────────────────────────────────
       // Returns vec4(despilledRGB, protectResult)
       "vec4 doDespill(vec3 rgb, float limit) { \n"
       "  vec3  hued        = hueRotate(rgb, $$hueShift); \n"
       "  float limitResult = despillLimit(hued) * limit; \n";

  // Protect tones — horneado
  if(k_protectTones) {
    s << "  float len = length(rgb) * length($$protectColor) + 0.000001; \n"
         "  float cosAngle     = clamp(dot(rgb, $$protectColor) / len, 0.0, 1.0); \n"
         "  float protectResult = pow(cosAngle, 1.0 / pow(clamp($$protectTol, 0.0001, 1.0), "
         "$$protectFall)); \n"
         "  limitResult = limitResult * (1.0 + protectResult * $$protectEffect); \n";
  }
  else {
    s << "  float protectResult = 0.0; \n";
  }

  s << "  hued[" << _clr << "] = min(hued[" << _clr
    << "], limitResult); \n"
       "  vec3 result = hueRotate(hued, -$$hueShift); \n"
       "  return vec4(result, protectResult); \n"
       "} \n"

       // ── Main ─────────────────────────────────────────────────────────────────
       "  float savedAlpha = OUT.a; \n"
       "  vec3  rgb        = OUT.rgb; \n"
       "  float spillMatte = 0.0; \n";

  // Protection preview — horneado, salida temprana
  if(k_protectPrev && k_protectTones) {
    s << "  vec4  despilled = doDespill(rgb, $$hueLimit); \n"
         "  float mask      = clamp(despilled.w * $$protectEffect, 0.0, 1.0); \n"
         "  OUT = vec4(rgb * mask, savedAlpha); \n"
         "} \n";
    _shaderBodyText = s.str();
    return _shaderBodyText.c_str();
  }

  s << "  vec4  rawDespilled = doDespill(rgb, $$hueLimit); \n"
       "  vec3  spillVec     = rgb - rawDespilled.rgb; \n"
       "  float spillLuma    = getLuma(spillVec); \n"
       "  vec3  despilledRGB; \n"
       "  float spillLumaFull; \n";

  // Absolute vs relative — horneado
  if(!k_absMode) {
    s << "  despilledRGB  = rawDespilled.rgb; \n"
         "  spillLumaFull = spillLuma; \n";
  }
  else {
    // despillColor viene como uniform — soporta color picker y canal puro
    s << "  vec4  pickDespilled  = doDespill($$despillColor, $$hueLimit); \n"
         "  vec3  pickSpill      = $$despillColor - pickDespilled.rgb; \n"
         "  float pickSpillLuma  = getLuma(pickSpill); \n"
         "  spillLumaFull        = (pickSpillLuma == 0.0) ? 0.0 : spillLuma / pickSpillLuma; \n"
         "  vec3  scaledSpill    = $$despillColor * spillLumaFull; \n"
         "  despilledRGB         = rgb - scaledSpill; \n"
         "  spillMatte           = pickDespilled.w; \n";
  }

  // Output type — horneado
  if(k_outputType == Constants::OUTPUT_DESPILL) {
    s << "  float rangeLuma = lumaRange(spillLumaFull); \n"
         "  vec3  outRGB    = despilledRGB + $$respillColor * rangeLuma; \n"
         "  spillLumaFull   = rangeLuma; \n";
  }
  else {
    s << "  vec3 outRGB = spillVec; \n";
  }

  // Alpha — horneado
  if(!k_outputAlpha) {
    s << "  spillMatte = savedAlpha; \n";
  }
  else if(!k_invertAlpha) {
    s << "  spillMatte = spillLumaFull; \n";
  }
  else {
    s << "  spillMatte = 1.0 - spillLumaFull; \n";
  }

  s << "  OUT = vec4(outRGB, clamp(spillMatte, 0.0, 1.0)); \n"
       "} \n";

  _shaderBodyText = s.str();
  return _shaderBodyText.c_str();
}

// ── gpuEngine_GL_begin — sube los uniforms cada frame ────────────────────────

void DespillAPIop::gpuEngine_GL_begin(GPUContext *context)
{
  // Scalars — bind por valor segun firma: bind(name, float v)
  context->bind("$$hueShift", _hueShift);
  context->bind("$$hueLimit", k_hueLimit);
  context->bind("$$blackPoint", k_blackPoint);
  context->bind("$$whitePoint", k_whitePoint);
  context->bind("$$protectEffect", k_protectEffect);
  context->bind("$$protectTol", k_protectTolerance);
  context->bind("$$protectFall", k_protectFalloff);
  context->bind("$$customWeight", (k_customWeight + 1.0f) / 2.0f);

  // vec3 — bind(name, siz, count, float[]) requiere array no-const
  float respill[3] = {k_respillColor[0], k_respillColor[1], k_respillColor[2]};
  float protect[3] = {k_protectColor[0], k_protectColor[1], k_protectColor[2]};
  context->bind("$$respillColor", 3, 1, respill);
  context->bind("$$protectColor", 3, 1, protect);

  // despillColor — replica la logica de _validate() y ProcessCPU()
  float despillColor[3];
  if(_usePickedColor == 1) {
    despillColor[0] = k_spillPick[0];
    despillColor[1] = k_spillPick[1];
    despillColor[2] = k_spillPick[2];
  }
  else {
    despillColor[0] = _clr == 0 ? 1.0f : 0.0f;
    despillColor[1] = _clr == 1 ? 1.0f : 0.0f;
    despillColor[2] = _clr == 2 ? 1.0f : 0.0f;
  }
  context->bind("$$despillColor", 3, 1, despillColor);
}

void DespillAPIop::gpuEngine_GL_end(GPUContext *context)
{
  // nada que limpiar
}

static Iop *build(Node *node)
{
  return (new NukeWrapper(new DespillAPIop(node)))->channelsRGBoptionalAlpha();
}
const Iop::Description DespillAPIop::d("DespillAP", "Keyer/DespillAP", build);