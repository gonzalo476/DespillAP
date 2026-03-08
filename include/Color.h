#ifndef COLOR_H
#define COLOR_H

#include <algorithm>
#include <cmath>

#include "Constants.h"
#include "DDImage/DDMath.h"
#include "DDImage/Vector3.h"
#include "DDImage/Vector4.h"

namespace nuke = DD::Image;

inline float magnitude(const Vector3 v)
{
  return std::sqrt(v.dot(v));
}

inline float cosAngleBetween(const Vector3 a, const Vector3 b)
{
  float magA = magnitude(a);
  float magB = magnitude(b);
  if(magA == 0.0f || magB == 0.0f) return 0.0f;
  return a.dot(b) / (magA * magB);
}

namespace color
{
  enum ScreenColor {
    kScreenColorRed = 0,
    kScreenColorGreen = 1,
    kScreenColorBlue = 2,
  };
  namespace luma
  {
    float ToLumaRec709(const float (&rgb)[3])
    {
      float l = rgb[0] * 0.2126f + rgb[1] * 0.7152f + rgb[2] * 0.0722f;
      return l;
    }

    float ToLumaCcir601(const float (&rgb)[3])
    {
      float l = rgb[0] * 0.299f + rgb[1] * 0.587f + rgb[2] * 0.114f;
      return l;
    }

    float ToLumaRec2020(const float (&rgb)[3])
    {
      float l = rgb[0] * 0.2627f + rgb[1] * 0.6780f + rgb[2] * 0.0593f;
      return l;
    }

    float ToLumaAverage(const float (&rgb)[3])
    {
      float l = (rgb[0] + rgb[1] + rgb[2]) / 3.0f;
      return l;
    }

    float ToLumaMax(const float (&rgb)[3])
    {
      float l = std::max({rgb[0], rgb[1], rgb[2]});
      return l;
    }
  }  // namespace luma

  Vector3 HueRotate(const Vector3 rgb, const float &angle)
  {
    Vector3 hue;

    if(angle == 0.0f) {
      return rgb;
    }

    float cosA = std::cos(angle * M_PI_F / 180.0f);
    float sinA = std::sin(angle * M_PI_F / 180.0f);
    float sqrt3 = std::sqrt(3.0f);
    float common = (rgb.x + rgb.y + rgb.z) * (1.0f - cosA) / 3.0f;

    hue[0] = common + rgb.x * cosA + (-rgb.y / sqrt3 + rgb.z / sqrt3) * sinA;
    hue[1] = common + rgb.y * cosA + (rgb.x / sqrt3 - rgb.z / sqrt3) * sinA;
    hue[2] = common + rgb.z * cosA + (-rgb.x / sqrt3 + rgb.y / sqrt3) * sinA;

    return hue;
  }

  Vector3 VectorToPlane(const Vector3 v1, const Vector3 v2 = {1.0f, 1.0f, 1.0f})
  {
    Vector3 proj;
    float scale;

    scale = v2.dot(v1) / v2.dot(v2);
    proj = v2 * scale;

    return v1 - proj;
  }

  float ColorAngle(const Vector3 v1, const Vector3 v2)
  {
    Vector3 normal(1.0f, 1.0f, 1.0f);

    float mag1 = v1.dot(v1);
    float mag2 = v2.dot(v2);

    float angle = std::acos(v1.dot(v2) / std::sqrt(mag1 * mag2));

    Vector3 crs = v1.cross(v2);

    if(normal.dot(crs) > 0.0f) {
      angle = -angle;
    }

    return angle;
  }

  Vector4 Despill(const Vector3 rgb, float hueShift, int _clr, int despillMath, float limit,
                  float customWeight, bool protectTones, Vector3 protectColor,
                  float protectTolerance, float protectEffect, float protectFalloff)
  {
    Vector3 hueIn = HueRotate(rgb, hueShift);
    Vector4 despilled(hueIn[0], hueIn[1], hueIn[2], 0.0f);
    customWeight = (customWeight + 1) / 2;

    float limitResult = 0.0f;
    int chans[2];

    if(_clr == Constants::COLOR_RED) {
      chans[0] = Constants::COLOR_GREEN;
      chans[1] = Constants::COLOR_BLUE;
    }
    else if(_clr == Constants::COLOR_GREEN) {
      chans[0] = Constants::COLOR_RED;
      chans[1] = Constants::COLOR_BLUE;
    }
    else if(_clr == Constants::COLOR_BLUE) {
      chans[0] = Constants::COLOR_RED;
      chans[1] = Constants::COLOR_GREEN;
    }

    if(despillMath == Constants::DESPILL_AVERAGE) {
      limitResult = (despilled[chans[0]] + despilled[chans[1]]) / 2;
    }
    else if(despillMath == Constants::DESPILL_MAX) {
      limitResult = MAX(despilled[chans[0]], despilled[chans[1]]);
    }
    else if(despillMath == Constants::DESPILL_MIN) {
      limitResult = MIN(despilled[chans[0]], despilled[chans[1]]);
    }
    else {
      limitResult = despilled[chans[0]] * customWeight + despilled[chans[1]] * (1 - customWeight);
    }

    float protectResult = 0.0f;
    bool isProtectDifferent = (protectColor[0] != protectColor[1]) ||
                              (protectColor[0] != protectColor[2]) ||
                              (protectColor[1] != protectColor[2]);

    if(protectTones && isProtectDifferent) {
      float cosProtectAngle;
      cosProtectAngle = cosAngleBetween(rgb, protectColor);
      cosProtectAngle = clamp(cosProtectAngle, 0.0f, 1.0f);
      protectResult = std::pow(cosProtectAngle, 1 / std::pow(protectTolerance, protectFalloff));
      limitResult = limitResult * (1 + protectResult * protectEffect);
    }

    for(int c = 0; c < 3; c++) {
      despilled[c] = c == _clr ? MIN(despilled[c], limitResult * limit) : despilled[c];
    }

    Vector3 rgbDespilled(despilled.x, despilled.y, despilled.z);
    rgbDespilled = HueRotate(rgbDespilled, -hueShift);
    despilled.x = rgbDespilled.x;
    despilled.y = rgbDespilled.y;
    despilled.z = rgbDespilled.z;
    despilled.w = protectResult;  // alpha

    return despilled;
  }

  float GetLuma(const Vector3 rgb, int math)
  {
    float luma;
    switch(math) {
      case Constants::LUMA_REC709:
        luma = luma::ToLumaRec709({rgb.x, rgb.y, rgb.z});
        break;
      case Constants::LUMA_CCIR601:
        luma = luma::ToLumaCcir601({rgb.x, rgb.y, rgb.z});
        break;
      case Constants::LUMA_REC2020:
        luma = luma::ToLumaRec2020({rgb.x, rgb.y, rgb.z});
        break;
      case Constants::LUMA_AVERAGE:
        luma = luma::ToLumaAverage({rgb.x, rgb.y, rgb.z});
        break;
      case Constants::LUMA_MAX:
        luma = luma::ToLumaMax({rgb.x, rgb.y, rgb.z});
        break;
      default:
        luma = luma::ToLumaRec709({rgb.x, rgb.y, rgb.z});
        break;
    }
    return luma;
  }

  float LumaRange(float spillLuma, float blackPoint = 0.0f, float whitePoint = 1.0f)
  {
    if(blackPoint <= 0.0f && whitePoint >= 1.0f) return spillLuma;

    float range = whitePoint - blackPoint;
    if(range <= 0.0f) return spillLuma;

    float normalized = (spillLuma - blackPoint) / range;
    return clamp(normalized, 0.0f, 1.0f);
  }
}  // namespace color

#endif  // COLOR_H