
#include "include/DespillAP.h"

#include "include/Color.h"
#include "include/Constants.h"

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
  // get main input data
  ChannelSet requestedChannels = channels;
  requestedChannels += Mask_RGB;  // Add RGB
  row.get(input0(), y, x, r, requestedChannels);

  // copy all non rgb channels
  ChannelSet copyMask = channels - Mask_RGB - k_outputSpillChannel;
  row.pre_copy(row, copyMask);
  row.copy(row, copyMask, x, r);

  // get input color reference (for atm color detection)
  Row color_row(x, r);
  if(input(inputColor) != nullptr) {
    color_row.get(*input(inputColor), y, x, r, Mask_RGB);
  }

  // get optional respill color input (custom replacement color)
  Row respill_row(x, r);
  if(input(inputRespill) != nullptr) {
    respill_row.get(*input(inputRespill), y, x, r, Mask_RGB);
  }

  // get limit matte input
  Row limit_matte_row(x, r);
  const float *limitPtr;
  if(input(inputLimit) != nullptr) {
    limit_matte_row.get(*input(inputLimit), y, x, r, Mask_All);
  }
  if(input(inputLimit) != nullptr) {
    limitPtr = limit_matte_row[k_limitChannel] + x;
  }

  Vector3 rgb;
  Vector3 colorRgb;
  Vector3 respillRgb;

  // pixel pointers for multichannel processing
  std::array<const float *, 3> inPtr;
  std::array<const float *, 3> colorPtr;
  std::array<const float *, 3> respillPtr;
  std::array<float *, 3> outPtr;

  // get pointer to input alpha channel for pass-throught
  const float *input_alpha = row[Chan_Alpha] + x;

  // lambda to increment all pixel pointers
  auto incrementPointers = [&]() {
    for(int i = 0; i < 3; ++i) {
      ++inPtr[i];
      ++outPtr[i];
      if(input(inputColor) != nullptr) {
        ++colorPtr[i];
      }
      if(input(inputRespill) != nullptr) {
        ++respillPtr[i];
      }
    }
    ++input_alpha;
    if(input(inputLimit) != nullptr) {
      ++limitPtr;
    }
  };

  // set pixel pointers to point to RGB channels
  for(int i = 0; i < 3; ++i) {
    auto chan = static_cast<Channel>(i + 1);
    inPtr[i] = row[chan] + x;
    outPtr[i] = row.writable(chan) + x;
    colorPtr[i] = color_row[chan] + x;
    respillPtr[i] = respill_row[chan] + x;
  }

  // Main pixel loop
  for(int x0 = x; x0 < r; ++x0) {
    // read rgb values from the current pixel
    for(int i = 0; i < 3; i++) {
      rgb[i] = *inPtr[i];
      colorRgb[i] = *colorPtr[i];
      respillRgb[i] = *respillPtr[i];
    }

    // early exit if color is unchanged
    if(_returnColor == 1) {
      incrementPointers();
      continue;
    }

    float spillMatte = 0.0f;
    float hueShift = 0.0f;
    float autoShift = 0.0f;
    Vector3 despillColor;

    // determine despill color and hue shift
    if(isColorConnected) {
      // use color from connected input for atm color detection
      despillColor = colorRgb;
      Vector3 v1 = color::VectorToPlane(despillColor);
      Vector3 v2 = color::VectorToPlane(Vector3(1.0f, 0.0f, 0.0f));
      autoShift = color::ColorAngle(v1, v2);
      autoShift = autoShift * 180.0f / M_PI_F;  // rad to deg
      hueShift = k_hueOffset - autoShift;
    }
    else {
      // use manual color detection
      if(_usePickedColor == 1) {
        despillColor = Vector3(k_spillPick);
      }
      else {
        // create a color constant based on selected channel
        // 0=red, 1=green, 2=blue
        despillColor =
            Vector3(_clr == 0 ? 1.0f : 0.0f, _clr == 1 ? 1.0f : 0.0f, _clr == 2 ? 1.0f : 0.0f);
      }
      hueShift = _hueShift;
    }

    // apply limit matte if connected
    float invertInputLimit = k_invertLimitMask ? (1.0f - (*limitPtr)) : *limitPtr;
    float limitResult = isLimitConnected ? k_hueLimit * invertInputLimit : k_hueLimit;

    // perform limit operation
    Vector4 rawDespilled = color::Despill(rgb, hueShift, _clr, k_despillMath, limitResult,
                                          k_customWeight, k_protectTones, k_protectColor,
                                          k_protectTolerance, k_protectEffect, k_protectFalloff);

    // case: if tones are protected, output protection matte
    if(k_protectPrev && k_protectTones) {
      for(int i = 0; i < 3; i++) {
        *outPtr[i] = rgb[i] * clamp(rawDespilled.w * k_protectEffect, 0.0f, 1.0f);
      }
      // move to next pixel channel
      incrementPointers();
      continue;
    }

    // calculate spill amount (difference between rgb and raw despilled)
    Vector3 spillVec = {
        rgb[0] - rawDespilled.x,
        rgb[1] - rawDespilled.y,
        rgb[2] - rawDespilled.z,
    };

    float spillLuma = color::GetLuma(spillVec, k_respillMath);

    // process key generation and normalization
    Vector3 despilledRGB;
    Vector4 spillFull;
    float spillLumaFull;

    if(!k_absMode) {
      // relative mode: use calculated values
      despilledRGB = {rawDespilled.x, rawDespilled.y, rawDespilled.z};
      spillFull = Vector4(spillVec.x, spillVec.y, spillVec.z, 0.0f);
      spillLumaFull = spillLuma;
    }
    else {
      // absolute mode: normalize spill relative to pícked color
      // calculate how much the pícked color would be despilled
      Vector4 pickDespilled = color::Despill(
          despillColor, hueShift, _clr, k_despillMath, limitResult, k_customWeight, k_protectTones,
          k_protectColor, k_protectTolerance, k_protectEffect, k_protectFalloff);

      Vector3 pickSpill = {
          despillColor.x - pickDespilled.x,
          despillColor.y - pickDespilled.y,
          despillColor.z - pickDespilled.z,
      };

      float pickSpillLuma = color::GetLuma(pickSpill, k_respillMath);

      // normalize current spill relative to picked color spill
      spillLumaFull = (pickSpillLuma == 0.0f) ? 0.0f : spillLuma / pickSpillLuma;
      Vector3 scaledSpill = despillColor * spillLumaFull;
      spillFull = Vector4(scaledSpill.x, scaledSpill.y, scaledSpill.z, 0.0f);
      despilledRGB = {rgb[0] - scaledSpill.x, rgb[1] - scaledSpill.y, rgb[2] - scaledSpill.z};
      spillMatte = pickDespilled.w;
    }

    // calculate final respill color (replacement color for removed spill)
    Vector3 respillBase = {k_respillColor[0], k_respillColor[1], k_respillColor[2]};
    Vector3 respillInput = respillRgb;
    Vector3 finalRespill =
        isRespillConnected ? Vector3(respillInput.x, respillInput.y, respillInput.z) : respillBase;

    // output type: despilled image or spill matte
    Vector4 result;
    if(k_outputType == Constants::OUTPUT_DESPILL) {
      // output despilled image with respill color added back
      float rangeLuma = color::LumaRange(spillLumaFull, k_blackPoint, k_whitePoint);
      result = {despilledRGB.x + finalRespill.x * rangeLuma,
                despilledRGB.y + finalRespill.y * rangeLuma,
                despilledRGB.z + finalRespill.z * rangeLuma, 0.0f};
      spillLumaFull = rangeLuma;
    }
    else {
      result = spillFull;
    }

    // determine alpha output value
    if(!k_outputAlpha) {
      // pass the original input alpha channel
      spillMatte = *input_alpha;
    }
    else if(!k_invertAlpha) {
      // output spill amount as alpha channel
      spillMatte = spillLumaFull;
    }
    else {
      // output inverted spill amount as alpha channel
      spillMatte = 1.0f - spillLumaFull;
    }

    // write alpha channel to specified output channel
    // if 'channels' contains 'k_outputSpillChannel' or the selected output channel
    if(channels & k_outputSpillChannel) {
      *(row.writable(k_outputSpillChannel) + x0) = clamp(spillMatte, 0.0f, 1.0f);
    }

    // write RGB channels to output
    for(int i = 0; i < 3; i++) {
      *outPtr[i] = result[i];
    }

    // move to next pixel channel
    incrementPointers();
  }
}

static Iop *build(Node *node)
{
  return (new NukeWrapper(new DespillAPIop(node)))->noChannels();
}
const Iop::Description DespillAPIop::d("DespillAP", "Keyer/DespillAP", build);