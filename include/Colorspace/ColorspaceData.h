#ifndef COLORSPACE_DATA_H
#define COLORSPACE_DATA_H

// Nuke XYZ chromatic adaptation matrices
const float matIdentity[] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

const float matXYZToSRGB[] = {0.41241086f, 0.35758454f, 0.18045381f, 0.21264936f, 0.71516907f,
                              0.07218152f, 0.01933176f, 0.1191948f,  0.95038998f};

const float matSRGBToXYZ[] = {3.24081278f, -1.53730845f, -0.49858657f, -0.96924323f, 1.87596655f,
                              0.04155507f, 0.05563836f,  -0.20400739f, 1.05712962f};

const float matXYZToSRGB_B[] = {0.4339115f,  0.37622687f, 0.1898616f,  0.21264936f, 0.71516907f,
                                0.07218152f, 0.01775321f, 0.10946182f, 0.87278491f};

const float matSRGBToXYZ_B[] = {3.08022809f, -1.53730845f, -0.54291916f, -0.92121649f, 1.87596655f,
                                0.04525001f, 0.05288144f,  -0.20400739f, 1.15112591f};

// CIE 1931 2deg Whitepoint
const float white_A[] = {0.44757f, 0.40745f};
const float white_B[] = {0.34842f, 0.35161f};
const float white_C[] = {0.31006f, 0.31616f};
const float white_D50[] = {0.34567f, 0.35850f};
const float white_D55[] = {0.33242f, 0.34743f};
const float white_d58[] = {0.32577f, 0.34152f};
const float white_D65[] = {0.31271f, 0.32902f};
const float white_D75[] = {0.29902f, 0.31485f};
const float white_9300[] = {0.28329f, 0.29640f};
const float white_E[] = {1.f / 3.f, 1.f / 3.f};
const float white_F2[] = {0.37208f, 0.37529f};
const float white_F7[] = {0.31292f, 0.32933f};
const float white_F11[] = {0.38052f, 0.37713f};
const float white_DCIP3[] = {0.31400f, 0.35100f};
const float white_ACES[] = {0.32168f, 0.33767f};

// Cone Response Matrices
const float cat02[] = {0.7328f, 0.4296f, -0.1624f, -0.7036f, 1.6975f,
                       0.0061f, 0.003f,  0.0136f,  0.9834f};
const float bradford[] = {0.8951f, 0.2664f, -0.1614f, -0.7502f, 1.7135f,
                          0.0367f, 0.0389f, -0.0685f, 1.0296f};

// Primary colors *some primaries were normalized from D50 to D65 by using the bradford matrix*
// https://github.com/colour-science/colour/tree/v0.3.15/colour/models/rgb/datasets
// http://www.brucelindbloom.com/index.html?WorkingSpaceInfo.html

// Adobe rgb D65
const float adobeRgb_1998[] = {0.64f, 0.33f, 0.21f, 0.71f, 0.15f, 0.06f};
// Apple rgb D65
const float appleRgb[] = {0.625f, 0.34f, 0.28f, 0.595f, 0.155f, 0.07f};
// *Best rgb D50 -> D65
const float bestRgb[] = {0.73565f, 0.260527f, 0.195348f, 0.803867f, 0.131876f, 0.036689f};
// *Beta rgb D50 -> D65
const float betaRgb[] = {0.686787f, 0.311f, 0.178532f, 0.772016f, 0.129284f, 0.037049f};
// Bruce rgb D65
const float bruceRgb[] = {0.64f, 0.33f, 0.28f, 0.65f, 0.15f, 0.06f};
// *CIE rgb 1931 E -> D65
const float cieRgb[] = {0.73817f, 0.260646f, 0.247533f, 0.744224f, 0.163897f, 0.009706f};
// *ColorMatch rgb D50 -> D65
const float colorMatchRgb[] = {0.619006f, 0.338742f, 0.27336f, 0.602807f, 0.145538f, 0.067042f};
// DCI-P3 rgb *no official whitepoint ref* using default D65
const float dciP3Rgb[] = {0.68f, 0.32f, 0.265f, 0.69f, 0.15f, 0.06f};
// *Don rgb4 D50 -> D65
const float donRrgb4[] = {0.693469f, 0.29794f, 0.194673f, 0.79131f, 0.131734f, 0.037075f};
// *Eci Rgb D50 -> D65
const float eciRgb[] = {0.667022f, 0.331416f, 0.189994f, 0.715244f, 0.13813f, 0.071059f};
// *Ekta Space PS5 rgb D50 -> D65
const float ektaRgbPS5[] = {0.69331f, 0.304262f, 0.240272f, 0.716695f, 0.118341f, 0.015102f};
// *NTSC Rgb C -> D65
const float ntscRgb[] = {0.669306f, 0.330849f, 0.202425f, 0.719882f, 0.139873f, 0.083718f};
// PAL/SECAM Rgb D65
const float palsecamRgb[] = {0.64f, 0.33f, 0.29f, 0.60f, 0.15f, 0.06f};
// *ProPhoto D50 -> D65
const float prophotoRgb[] = {0.735128f, 0.261061f, 0.138352f, 0.877531f, 0.068644f, 0.013841f};
// SMPTE-C D65
const float smptecRgb[] = {0.630f, 0.340f, 0.310f, 0.595f, 0.155f, 0.070f};
// sRGB D65
const float srgbRgb[] = {0.64f, 0.33f, 0.3f, 0.6f, 0.15f, 0.06f};
// *Wide Gamut Adobe D50 -> D65
const float widegamutRgb[] = {0.735136f, 0.261056f, 0.095681f, 0.841494f, 0.151207f, 0.023121f};
// Alexa wide gamut D65
const float arriWideGamutRgb[] = {0.684f, 0.313f, 0.221f, 0.848f, 0.0861f, -0.102f};
// Sony S Gamut D65
const float sonySGamutRgb[] = {0.73f, 0.28f, 0.14f, 0.855f, 0.1f, -0.05f};
// *ACES rgb ACES -> D65
const float acesRgb[] = {0.734857f, 0.264225f, -0.006172f, 1.011305f, 0.0f, 0.0f};
// Rec 2020 D65
const float rec2020Rgb[] = {0.708f, 0.292f, 0.17f, 0.797f, 0.131f, 0.046f};
// Arri Wide Gamut 4 D65
// https://www.arri.com/resource/blob/278790/dc29f7399c1dc9553d329e27f1409a89/2022-05-arri-logc4-specification-data.pdf
const float arriWideGamut4Rgb[] = {0.7347f, 0.2653f, 0.1424f, 0.8576f, 0.0991f, -0.0308f};
#endif  // COLORSPACE_DATA_H