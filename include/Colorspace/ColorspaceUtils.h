#ifndef COLORSPACE_UTILS_H
#define COLORSPACE_UTILS_H

#include <algorithm>
#include <array>
#include <unordered_set>

#include "Colorspace/ColorspaceAliases.h"
#include "Colorspace/ColorspaceConstants.h"

bool isInXYZMatrix(double cs)
{
  static const std::unordered_set<ColorLut> colors = {ColorLut::COLOR_CIE_XYZ,
                                                      ColorLut::COLOR_CIE_YXY, ColorLut::COLOR_LAB,
                                                      ColorLut::COLOR_CIE_LCH};

  return std::find(colors.begin(), colors.end(), cs) != colors.end();
}

RGBcolor removeExp(const RGBcolor& p)
{
  RGBcolor rgb = {0.0f, 0.0f, 0.0f};
  for(size_t i = 0; i < 3; ++i) {
    rgb[i] = std::abs(p[i]) < 1e-10 ? 0.0f : p[i];
  }

  return rgb;
}

#endif  // COLORSPACE_UTILS_H