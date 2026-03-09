// in: LinToColor
// out: ColorToLin

#ifndef COLORSPACE_LUT_H
#define COLORSPACE_LUT_H

#include <float.h>

#include <algorithm>
#include <array>
#include <cmath>

#include "Colorspace/ColorspaceAliases.h"
#include "Colorspace/ColorspaceData.h"
#include "DDImage/DDMath.h"

constexpr float CIN_BLACKPOINT = 95.0f;
constexpr float CIN_WHITEPOINT = 685.0f;
constexpr float CIN_GAMMA = 0.6f;
constexpr float _PI = 3.1415926f;

// rgb to mat func
RGBcolor toXYZMat(const float* mat, const RGBcolor& p)
{
  RGBcolor xyz = {0.0f, 0.0f, 0.0f};

  float r = p[0];
  float g = p[1];
  float b = p[2];

  xyz[0] = mat[0] * r + mat[1] * g + mat[2] * b;
  xyz[1] = mat[3] * r + mat[4] * g + mat[5] * b;
  xyz[2] = mat[6] * r + mat[7] * g + mat[8] * b;

  return xyz;
}

// CIE XYZ
RGBcolor CIEXyzToLin(const RGBcolor& p)
{
  RGBcolor rgb = toXYZMat(matXYZToSRGB, p);
  return rgb;
}

RGBcolor LinToCIEXyz(const RGBcolor& p)
{
  RGBcolor rgb = toXYZMat(matSRGBToXYZ, p);
  return rgb;
}

// CIE Yxy
RGBcolor CIEYxyToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  RGBcolor xyz = toXYZMat(matXYZToSRGB, p);

  float X = xyz[0];  // r
  float Y = xyz[1];  // g
  float Z = xyz[2];  // b

  float sum = X + Y + Z;

  rgb[0] = Y;

  if(sum > 1e-6f) {
    rgb[1] = X / sum;
    rgb[2] = Y / sum;
  }

  return rgb;
}

RGBcolor LinToCIEYxy(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  float x = p[0];  // r
  float y = p[1];  // g
  float z = p[2];  // b

  float d = 0.0f;
  if(z > 1e-6f) {
    d = x / z;
  }

  rgb[0] = y * d;
  rgb[1] = x;
  rgb[2] = (1.0f - y - z) * d;

  RGBcolor xyz = toXYZMat(matSRGBToXYZ, rgb);

  return xyz;
}

// CIE L*a*b
RGBcolor LinToCIELab(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  auto f = [](float v) {
    const float e = 0.008856f;
    const float k = 7.787f;
    return (v > 0.206893f) ? v * v * v : (v - 0.137931f) / k;
  };

  float fy = (p[0] + 0.16f) / 1.16f;
  float fx = fy + (p[1] / 5.0f);
  float fz = fy - (p[2] / 2.0f);

  rgb[0] = f(fx);
  rgb[1] = f(fy);
  rgb[2] = f(fz);

  return toXYZMat(matSRGBToXYZ_B, rgb);
}

RGBcolor CIELabToLin(const RGBcolor& p)
{
  RGBcolor xyz = toXYZMat(matXYZToSRGB_B, p);
  RGBcolor lab = {0.0f, 0.0f, 0.0f};

  auto f = [](float v) {
    const float e = 0.008856f;
    const float k = 7.787f;
    return (v > e) ? std::powf(v, 0.333333f) : ((k * v) + 0.137931f);
  };

  float fx = f(xyz[0]);
  float fy = f(xyz[1]);
  float fz = f(xyz[2]);

  lab[0] = 1.16f * fy - 0.16f;
  lab[1] = 5.0f * (fx - fy);
  lab[2] = 2.0f * (fy - fz);

  return lab;
}

// CIE L*C*h
RGBcolor LinToCIELCh(const RGBcolor& p)
{
  RGBcolor lch = {0.0f, 0.0f, 0.0f};

  float r = p[0];
  float g = p[1];
  float b = p[2] * 3.60f;

  float rad = (b * _PI) / 1.80f;

  lch[0] = r;
  lch[1] = g * std::cosf(rad);
  lch[2] = g * std::sinf(rad);

  return LinToCIELab(lch);
}

RGBcolor CIELChToLin(const RGBcolor& p)
{
  RGBcolor lab = CIELabToLin(p);
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  float l = lab[0];
  float a = lab[1];
  float b = lab[2];

  rgb[0] = l;
  rgb[1] = std::sqrtf(a * a + b * b);
  rgb[2] = std::atan2f(b, a) * 1.80f / _PI;

  if(rgb[2] < 0.0f) rgb[2] += 3.60;

  rgb[2] = rgb[2] / 3.60f;

  return rgb;
}

// Gamma 1.8
RGBcolor Gamma180ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(p[i], 1.0f / 1.80f);
  }

  return rgb;
}

RGBcolor LinToGamma180(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(p[i], 1.80f);
  }

  return rgb;
}

// Gamma 2.2
RGBcolor Gamma220ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(p[i], 1.0f / 2.20f);
  }

  return rgb;
}

RGBcolor LinToGamma220(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(p[i], 2.20f);
  }

  return rgb;
}

// Gamma 2.4
RGBcolor Gamma240ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(p[i], 1.0f / 2.40f);
  }

  return rgb;
}

RGBcolor LinToGamma240(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(p[i], 2.40f);
  }

  return rgb;
}

// Gamma 2.6
RGBcolor Gamma260ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(p[i], 1.0f / 2.60f);
  }

  return rgb;
}

RGBcolor LinToGamma260(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(p[i], 2.60f);
  }

  return rgb;
}

// Rec 709
RGBcolor LinToRec709(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v <= 0.081f)
      rgb[i] = v / 4.5f;
    else
      rgb[i] = std::powf((v + 0.099f) / 1.099f, 1.0f / 0.45f);
  }

  return rgb;
}

RGBcolor Rec709ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v <= 0.018f)
      rgb[i] = v * 4.5f;
    else
      rgb[i] = 1.099f * std::powf(v, 0.45f) - 0.099f;
  }

  return rgb;
}

// sRGB
RGBcolor sRGBToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v <= 0.0031308f)
      rgb[i] = 12.92f * v;
    else
      rgb[i] = 1.055f * std::powf(v, 1.0f / 2.4f) - 0.055f;
  }

  return rgb;
}

RGBcolor LinTosRGB(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v <= 0.04045f)
      rgb[i] = v / 12.92f;
    else
      rgb[i] = std::powf((v + 0.055f) / 1.055f, 2.4f);
  }

  return rgb;
}

// Cineon
RGBcolor LinToCineon(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  float offset = std::powf(10.f, (CIN_BLACKPOINT - CIN_WHITEPOINT) * 0.002f / CIN_GAMMA);
  float gain = 1.f / (1.f - offset);

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];

    rgb[i] = gain * (std::powf(10.f, (1023.f * v - CIN_WHITEPOINT) * 0.002f / CIN_GAMMA) - offset);
  }

  return rgb;
}

RGBcolor CineonToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  float offset = std::powf(10.f, (CIN_BLACKPOINT - CIN_WHITEPOINT) * 0.002f / CIN_GAMMA);
  float gain = 1.f / (1.f - offset);

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];

    rgb[i] = (std::log10f(v / gain + offset) / (0.002f / CIN_GAMMA) + CIN_WHITEPOINT) / 1023.f;
  }

  return rgb;
}

// HSV
RGBcolor HSVToLin(const RGBcolor& p)
{
  RGBcolor in = sRGBToLin(p);
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  float r = in[0];
  float g = in[1];
  float b = in[2];

  float cmax = std::max({r, g, b});
  float cmin = std::min({r, g, b});
  float delta = cmax - cmin;

  if(delta == 0.f)
    rgb[0] = 0.f;
  else if(cmax == r)
    rgb[0] = std::fmod((60.0f * ((g - b) / delta) + 360.0f), 360.0f) / 360.0f;
  else if(cmax == g)
    rgb[0] = std::fmod((60.0f * ((b - r) / delta) + 120.0f), 360.0f) / 360.0f;
  else if(cmax == b)
    rgb[0] = std::fmod((60.0f * ((r - g) / delta) + 240.0f), 360.0f) / 360.0f;

  rgb[1] = (cmax == 0.f) ? 0.f : (delta / cmax);
  rgb[2] = cmax;

  return rgb;
}

RGBcolor LinToHSV(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  float h = p[0] * 360.0f;
  float s = p[1];
  float v = p[2];

  float c = v * s;
  float x = c * (1 - std::fabs(std::fmod(h / 60.0f, 2) - 1));
  float m = v - c;

  if(h >= 0 && h < 60) {
    rgb[0] = c + m;
    rgb[1] = x + m;
    rgb[2] = m;
  }
  else if(h >= 60 && h < 120) {
    rgb[0] = x + m;
    rgb[1] = c + m;
    rgb[2] = m;
  }
  else if(h >= 120 && h < 180) {
    rgb[0] = m;
    rgb[1] = c + m;
    rgb[2] = x + m;
  }
  else if(h >= 180 && h < 240) {
    rgb[0] = m;
    rgb[1] = x + m;
    rgb[2] = c + m;
  }
  else if(h >= 240 && h < 300) {
    rgb[0] = x + m;
    rgb[1] = m;
    rgb[2] = c + m;
  }
  else if(h >= 300 && h < 360) {
    rgb[0] = c + m;
    rgb[1] = m;
    rgb[2] = x + m;
  }

  return LinTosRGB(rgb);
}

// HSL
RGBcolor LinToHSL(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  float r = p[0] * 360.0f;  // h
  float g = p[1];           // s
  float b = p[2];           // l

  float c = (1.0f - std::fabs(2.0f * b - 1.0f)) * g;  // chroma
  float x = c * (1.0f - std::fabs(std::fmod(r / 60.0f, 2.0f) - 1.0f));
  float m = b - c / 2.0f;

  if(r >= 0 && r < 60) {
    rgb[0] = c + m;
    rgb[1] = x + m;
    rgb[2] = m;
  }
  else if(r >= 60 && r < 120) {
    rgb[0] = x + m;
    rgb[1] = c + m;
    rgb[2] = m;
  }
  else if(r >= 120 && r < 180) {
    rgb[0] = m;
    rgb[1] = c + m;
    rgb[2] = x + m;
  }
  else if(r >= 180 && r < 240) {
    rgb[0] = m;
    rgb[1] = x + m;
    rgb[2] = c + m;
  }
  else if(r >= 240 && r < 300) {
    rgb[0] = x + m;
    rgb[1] = m;
    rgb[2] = c + m;
  }
  else if(r >= 300 && r < 360) {
    rgb[0] = c + m;
    rgb[1] = m;
    rgb[2] = x + m;
  }

  return LinTosRGB(rgb);
}

RGBcolor HSLToLin(const RGBcolor& p)
{
  RGBcolor in = sRGBToLin(p);
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  float r = in[0];
  float g = in[1];
  float b = in[2];

  float cmax = std::max({r, g, b});
  float cmin = std::min({r, g, b});
  float delta = cmax - cmin;

  // h
  if(delta == 0.0f)
    rgb[0] = 0.0f;
  else if(cmax == r)
    rgb[0] = std::fmod((60.0f * ((g - b) / delta) + 360.0f), 360.0f) / 360.0f;
  else if(cmax == g)
    rgb[0] = (60.0f * ((b - r) / delta) + 120.0f) / 360.0f;
  else if(cmax == b)
    rgb[0] = (60.0f * ((r - g) / delta) + 240.0f) / 360.0f;

  // l
  rgb[2] = (cmax + cmin) / 2.0f;

  // s
  rgb[1] = (delta == 0.0f) ? 0.0f : (delta / (1.0f - std::fabs(2.0f * rgb[2] - 1.0f)));

  return rgb;
}

// YPbPr
RGBcolor LinToYPbPr(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  float r = p[0];  // y
  float g = p[1];  // pb
  float b = p[2];  // pr

  rgb[0] = r + 1.5748f * b;
  rgb[1] = r - 0.1873f * g - 0.4681f * b;
  rgb[2] = r + 1.8556f * g;

  return LinTosRGB(rgb);
}

RGBcolor YPbPrToLin(const RGBcolor& p)
{
  RGBcolor in = sRGBToLin(p);
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  float r = in[0];
  float g = in[1];
  float b = in[2];

  rgb[0] = 0.2126f * r + 0.7152f * g + 0.0722f * b;  // Y
  rgb[1] = -0.1146f * r - 0.3854f * g + 0.5f * b;    // Pb
  rgb[2] = 0.5f * r - 0.4542f * g - 0.0458f * b;     // Pr

  return rgb;
}

// YCbCr BT.709
RGBcolor LinToYCbCr(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  float Y = p[0];
  float Cb = p[1];
  float Cr = p[2];

  const float Kr = 0.2126f;
  const float Kb = 0.0722f;

  float Y_min = 16.0f / 255.0f, Y_max = 235.0f / 255.0f;
  float C_min = 16.0f / 255.0f, C_max = 240.0f / 255.0f;

  Y -= Y_min;
  Cb -= (C_max + C_min) / 2.0f;
  Cr -= (C_max + C_min) / 2.0f;

  Y *= 1.0f / (Y_max - Y_min);
  Cb *= 1.0f / (C_max - C_min);
  Cr *= 1.0f / (C_max - C_min);

  float R = Y + (2.0f - 2.0f * Kr) * Cr;
  float B = Y + (2.0f - 2.0f * Kb) * Cb;
  float G = (Y - Kr * R - Kb * B) / (1.0f - Kr - Kb);

  rgb[0] = R;
  rgb[1] = G;
  rgb[2] = B;

  return LinTosRGB(rgb);
}

RGBcolor YCbCrToLin(const RGBcolor& p)
{
  RGBcolor in = sRGBToLin(p);
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  float R = in[0];
  float G = in[1];
  float B = in[2];

  const float Kr = 0.2126f;
  const float Kb = 0.0722f;

  float Y_min = 16.0f / 255.0f, Y_max = 235.0f / 255.0f;
  float C_min = 16.0f / 255.0f, C_max = 240.0f / 255.0f;

  float Y = Kr * R + (1 - Kr - Kb) * G + Kb * B;
  float Cb = 0.5f * (B - Y) / (1.0f - Kb);
  float Cr = 0.5f * (R - Y) / (1.0f - Kr);

  Y = Y * (Y_max - Y_min) + Y_min;
  Cb = Cb * (C_max - C_min) + (C_max + C_min) / 2.0f;
  Cr = Cr * (C_max - C_min) + (C_max + C_min) / 2.0f;

  rgb[0] = Y;
  rgb[1] = Cb;
  rgb[2] = Cr;

  return rgb;
}

// Panalog
RGBcolor PanalogToLin(const RGBcolor& p)  // to_func_Panalog
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = (444.0f * std::log10f(0.0408f + (1.0f - 0.0408f) * p[i]) + 681.0f) / 1023.0f;
  }

  return rgb;
}

RGBcolor LinToPanalog(const RGBcolor& p)  // from_func_Panalog
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = (std::powf(10.0f, (1023.0f * p[i] - 681.0f) / 444.0f) - 0.0408f) / (1.0f - 0.0408f);
  }

  return rgb;
}

// REDLog
RGBcolor REDLogToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  auto f = [](float v) {
    const float black_offset = pow(10.0f, (0.0f - 1023.0f) / 511.0f);
    return (1023.0f + 511.0f * log10(v * (1 - black_offset) + black_offset)) / 1023.0f;
  };

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = f(p[i]);
  }

  return rgb;
}

RGBcolor LinToREDLog(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  auto f = [](float v) {
    const float black_offset = pow(10.0f, (0.0f - 1023.0f) / 511.0f);
    return ((pow(10.f, ((1023.0f * v - 1023.0f) / 511.0f)) - black_offset) / (1.0f - black_offset));
  };

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = f(p[i]);
  }

  return rgb;
}

// ViperLog
RGBcolor ViperLogToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = (500.0f * std::log10f(p[i]) + 1023.0f) / 1023.0f;
    ;
  }

  return rgb;
}

RGBcolor LinToViperLog(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(10.f, (1023.f * p[i] - 1023.f) / 500.f);
    ;
  }

  return rgb;
}

// AlexaV3LogC
// "ALEXA LOG C Curve-Usage in VFX"
RGBcolor AlexaV3LogCToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v > 0.010591f)
      rgb[i] = 0.247190f * std::log10f(5.555556f * v + 0.052272f) + 0.385537f;
    else
      rgb[i] = v * 5.367655f + 0.092809f;
  }
  return rgb;
}

RGBcolor LinToAlexaV3LogC(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v > 0.1496582f)
      rgb[i] = std::powf(10.f, (v - 0.385537f) / 0.2471896f) * 0.18f - 0.00937677f;
    else
      rgb[i] = (v / 0.9661776f - 0.04378604f) * 0.18f - 0.00937677f;
  }

  return rgb;
}

// PLogLin
RGBcolor LinToPLog(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = 0.18f * std::powf(10.0f, (p[i] * 1023. - 445.0f) * 0.002f / 0.6f);
  }

  return rgb;
}

RGBcolor PLogToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = (445.0f + std::log10f(std::max(p[i], 1e-10f) / 0.18f) * 0.6f / 0.002f) / 1023.0f;
  }

  return rgb;
}

// SLog
RGBcolor SlogToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = (0.432699f * std::log10f(p[i] + 0.037584f) + 0.616596f) + 0.03f;
  }

  return rgb;
}

RGBcolor LinToSlog(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(10.0f, ((p[i] - 0.616596f - 0.03f) / 0.432699f)) - 0.037584f;
  }

  return rgb;
}

// SLog-1
RGBcolor Slog1ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v >= -0.00008153227156f)
      rgb[i] = ((std::log10f((v / 0.9f) + 0.037584f) * 0.432699f + 0.616596f + 0.03f) *
                    (940.0f - 64.0f) +
                64.0f) /
               1023.0f;
    else
      rgb[i] = (((v / 0.9f) * 5.0f + 0.030001222851889303f) * (940.0f - 64.0f) + 64.0f) / 1023.0f;
  }

  return rgb;
}

RGBcolor LinToSlog1(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v >= 90.0f / 1023.0f)
      rgb[i] = (std::powf(10., (((v * 1023.0f - 64.0f) / (940.0f - 64.0f) - 0.616596f - 0.03f) /
                                0.432699f)) -
                0.037584f) *
               0.9f;
    else
      rgb[i] = ((v * 1023.0f - 64.0f) / (940.0f - 64.0f) - 0.030001222851889303f) / 5.0f * 0.9f;
  }

  return rgb;
}

// SLog-2
RGBcolor Slog2ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v >= -0.00008153227156f)
      rgb[i] =
          ((std::log10f((v / 0.9f) * 155.0f / 219.0f + 0.037584f) * 0.432699f + 0.616596f + 0.03f) *
               (940.0f - 64.0f) +
           64.0f) /
          1023.0f;
    else
      rgb[i] =
          (((v / 0.9f) * 3.53881278538813f + 0.030001222851889303f) * (940.0f - 64.0f) + 64.0f) /
          1023.0f;
  }

  return rgb;
}

RGBcolor LinToSlog2(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v >= 90.0f / 1023.0f)
      rgb[i] = 219.0 *
               (std::powf(10.0f, (((v * 1023.0f - 64.0f) / (940.0f - 64.0f) - 0.616596f - 0.03f) /
                                  0.432699f)) -
                0.037584f) /
               155.0f * 0.9f;
    else
      rgb[i] = ((v * 1023.0f - 64.0f) / (940.0f - 64.0f) - 0.030001222851889303f) /
               3.53881278538813f * 0.9f;
  }

  return rgb;
}

// SLog-3
RGBcolor Slog3ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v >= 0.01125000f)
      rgb[i] = (420.0f + std::log10f((v + 0.01f) / (0.18f + 0.01f)) * 261.5f) / 1023.0f;
    else
      rgb[i] = (v * (171.2102946929f - 95.0f) / 0.01125000f + 95.0f) / 1023.0f;
  }

  return rgb;
}

RGBcolor LinToSlog3(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v >= 171.2102946929f / 1023.0f)
      rgb[i] = std::powf(10.0f, ((v * 1023.0f - 420.0f) / 261.5f)) * (0.18f + 0.01f) - 0.01f;
    else
      rgb[i] = (v * 1023.0f - 95.0f) * 0.01125000f / (171.2102946929f - 95.0f);
  }

  return rgb;
}

// CLog 10bit
// https://downloads.canon.com/nw/learn/white-papers/cinema-eos/WhitePaper_Clog_optoelectronic.pdf
RGBcolor ClogToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v < -0.0452664f || v > 8.00903f) {
      rgb[i] = 0.0f;
    }
    else if(v < 0.0f) {
      rgb[i] = -0.529136f * std::log10f(1.0f - 10.1596f * v) + 0.0730597f;
    }
    else {
      rgb[i] = 0.529136f * std::log10f(10.1596f * v + 1.0f) + 0.0730597f;
    }
  }

  return rgb;
}

RGBcolor LinToClog(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v < -0.0684932f || v > 1.08676f) {
      rgb[i] = 0.0f;
    }
    else if(v < 0.0730597f) {
      rgb[i] = (1.0f - std::powf(10.0f, (0.0730597f - v) / 0.529136f)) / 10.1596f;
    }
    else {
      rgb[i] = (std::powf(10.0f, (v - 0.0730597f) / 0.529136f) - 1.0f) / 10.1596f;
    }
  }

  return rgb;
}

// Log3G10 10 stops over mid grey
RGBcolor LinToLog3G10(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float a = 0.224282f;
  const float b = 155.975327f;
  const float c = 0.01f;
  const float g = 15.1927f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v < 0.0f)
      rgb[i] = (v / g) - c;
    else
      rgb[i] = ((std::powf(10.f, (v / a)) - 1.0f) / b) - c;
  }

  return rgb;
}

RGBcolor Log3G10ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float a = 0.224282f;
  const float b = 155.975327f;
  const float c = 0.01f;
  const float g = 15.1927f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i] + c;
    if(v < 0.0f)
      rgb[i] = v * g;
    else
      rgb[i] = a * std::log10f((v * b) + 1.0f);
  }

  return rgb;
}

// Log3G12 12 stops over mid grey
RGBcolor LinToLog3G12(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float a = 0.184904f;
  const float b = 347.189667f;
  const float c = 0.0f;
  const float g = 15.1927f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v < 0.0f)
      rgb[i] = (v / g) - c;
    else
      rgb[i] = ((std::powf(10.f, (v / a)) - 1.0f) / b) - c;
  }

  return rgb;
}

RGBcolor Log3G12ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float a = 0.184904f;
  const float b = 347.189667f;
  const float c = 0.0f;
  const float g = 15.1927f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i] + c;
    if(v < 0.0f)
      rgb[i] = v * g;
    else
      rgb[i] = a * std::log10f((v * b) + 1.0f);
  }

  return rgb;
}

// HybridLogGamma
RGBcolor LinToHybridLogGamma(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float a = 0.17883277f;
  const float b = 0.28466892f;
  const float c = 0.55991073f;
  const float t = 0.0833f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v <= std::sqrtf(3.0f * t)) {
      rgb[i] = (v * v) / 3.0f;
    }
    else {
      rgb[i] = (std::exp((v - c) / a) + b) / 12.0f;
    }
  }

  return rgb;
}

RGBcolor HybridLogGammaToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float a = 0.17883277f;
  const float b = 0.28466892f;
  const float c = 0.55991073f;
  const float t = 0.0833f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    if(v <= t)
      rgb[i] = std::sqrtf(3.0f * v);
    else
      rgb[i] = a * std::logf(12.0f * v - b) + c;
  }

  return rgb;
}

// Protune
RGBcolor LinToProtune(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = (std::powf(113.0f, p[i]) - 1.0f) / 112.0f;
  }

  return rgb;
}

RGBcolor ProtuneToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::logf(112.0f * p[i] + 1) / std::logf(113.0f);
  }

  return rgb;
}

// BT1886
RGBcolor LinToBT1886(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float white = 1.0f;
  const float black = 0.0f;
  const float gamma = 2.4f;

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf((p[i] - black) / (white - black), gamma);
  }

  return rgb;
}

RGBcolor BT1886ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float white = 1.0f;
  const float black = 0.0f;
  const float gamma = 2.4f;

  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::powf(p[i], 1.0 / gamma) * (white - black) + black;
  }

  return rgb;
}

// st2084
RGBcolor LinToSt2084(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float c1 = 0.8359375f;
  const float c2 = 18.8515625f;
  const float c3 = 18.6875f;
  const float m1 = 0.1593017578125f;
  const float m2 = 78.84375f;
  const float lum = 10000.0f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    rgb[i] = lum *
             std::powf(std::max(std::powf(v, 1.0f / m2) - c1, 0.0f) / (c2 - c3 * pow(v, 1.0f / m2)),
                       1.0f / m1);
  }

  return rgb;
}

RGBcolor St2084ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float c1 = 0.8359375f;
  const float c2 = 18.8515625f;
  const float c3 = 18.6875f;
  const float m1 = 0.1593017578125f;
  const float m2 = 78.84375f;
  const float lum = 10000.0f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    float normV = v / lum;
    rgb[i] = std::pow((c1 + c2 * std::pow(normV, m1)) / (1.0f + c3 * std::pow(normV, m1)), m2);
  }

  return rgb;
}

// Blackmagic Film Generation 5
RGBcolor LinToBFG5(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float a = 0.08692876065491224f;
  const float b = 0.005494072432257808f;
  const float c = 0.5300133392291939f;
  const float d = 8.283605932402494f;
  const float e = 0.09246575342465753f;
  const float cut = 0.005f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    rgb[i] = v < cut ? (v - e) / d : std::expf((v - c) / a) - b;
  }
  return rgb;
}

RGBcolor BFG5ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float a = 0.08692876065491224f;
  const float b = 0.005494072432257808f;
  const float c = 0.5300133392291939f;
  const float d = 8.283605932402494f;
  const float e = 0.09246575342465753f;
  const float cut = 0.005f;

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    rgb[i] = v < cut ? d * v + e : a * std::logf(v + b) + c;
  }
  return rgb;
}

// ARRILogC4
RGBcolor LinToARRILogC4(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};

  const float a = 2231.8263090676883f;    // (pow(2.0, 18.0) - 16.0) / 117.45
  const float b = 0.9071358748778103f;    // (1023.0 - 95.0) / 1023.0
  const float c = 0.09286412512218964f;   // 95.0 / 1023.0
  const float s = 0.1135972086105891f;    // (7 * log(2) * pow(2.0, 7 - 14 * c / b)) / (a * b)
  const float t = -0.01805699611991131f;  // (pow(2.0, 14.0 * (-c / b) + 6.0) - 64.0) / a

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    float vp = 14.0f * (v - c) / b + 6.0f;
    rgb[i] = v < 0.0f ? v * s + t : (std::powf(2.0f, vp) - 64.0f) / a;
  }

  return rgb;
}

RGBcolor ARRILogC4ToLin(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  const float a = 2231.8263090676883f;    // (pow(2.0, 18.0) - 16.0) / 117.45
  const float b = 0.9071358748778103f;    // (1023.0 - 95.0) / 1023.0
  const float c = 0.09286412512218964f;   // 95.0 / 1023.0
  const float s = 0.1135972086105891f;    // (7 * log(2) * pow(2.0, 7 - 14 * c / b)) / (a * b)
  const float t = -0.01805699611991131f;  // (pow(2.0, 14.0 * (-c / b) + 6.0) - 64.0) / a

  for(size_t i = 0; i < 3; ++i) {
    float v = p[i];
    rgb[i] = v < t ? (v - t) / s : (std::log2f(a * v + 64.0f) - 6.0f) / 14.0f * b + c;
  }

  return rgb;
}

#endif  // COLORSPACE_LUT_H