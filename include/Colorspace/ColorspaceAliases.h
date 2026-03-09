#ifndef COLORSPACE_ALIASES_H
#define COLORSPACE_ALIASES_H

#include <array>

#include "Colorspace/ColorspaceConstants.h"

using ColorLut = Constants::Colorspaces;
using RGBcolor = std::array<float, 3>;
using XYZMat = std::array<float, 9>;
using TransformDispatcher = RGBcolor (*)(const RGBcolor&);

#endif  // COLORSPACE_ALIASES_H