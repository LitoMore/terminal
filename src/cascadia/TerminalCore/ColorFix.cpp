#include "pch.h"

#include <Windows.h>
#include <math.h>
#include "ColorFix.hpp"

const double gMinThreshold = 12.0;
const double gExpThreshold = 20.0;
const double gLStep = 5.0;

constexpr double rad006 = 0.104719755119659774;
constexpr double rad025 = 0.436332312998582394;
constexpr double rad030 = 0.523598775598298873;
constexpr double rad060 = 1.047197551196597746;
constexpr double rad063 = 1.099557428756427633;
constexpr double rad180 = 3.141592653589793238;
constexpr double rad275 = 4.799655442984406336;
constexpr double rad360 = 6.283185307179586476;

double GetHPrimeFn(double x, double y)
{
    if (x == 0 && y == 0)
    {
        return 0;
    }

    const auto hueAngle = atan2(x, y);
    return hueAngle >= 0 ? hueAngle : hueAngle + rad360;
}

// DeltaE 2000
// Source: https://github.com/zschuessler/DeltaE
double ColorFix::GetDeltaE(ColorFix x1, ColorFix x2)
{
    constexpr double kSubL = 1;
    constexpr double kSubC = 1;
    constexpr double kSubH = 1;

    // Delta L Prime
    const double deltaLPrime = x2.L - x1.L;

    // L Bar
    const double lBar = (x1.L + x2.L) / 2;

    // C1 & C2
    const double c1 = sqrt(pow(x1.A, 2) + pow(x1.B, 2));
    const double c2 = sqrt(pow(x2.A, 2) + pow(x2.B, 2));

    // C Bar
    const double cBar = (c1 + c2) / 2;

    // A Prime 1
    const double aPrime1 = x1.A + (x1.A / 2) * (1 - sqrt(pow(cBar, 7) / (pow(cBar, 7) + pow(25.0, 7))));

    // A Prime 2
    const double aPrime2 = x2.A + (x2.A / 2) * (1 - sqrt(pow(cBar, 7) / (pow(cBar, 7) + pow(25.0, 7))));

    // C Prime 1
    const double cPrime1 = sqrt(pow(aPrime1, 2) + pow(x1.B, 2));

    // C Prime 2
    const double cPrime2 = sqrt(pow(aPrime2, 2) + pow(x2.B, 2));

    // C Bar Prime
    const double cBarPrime = (cPrime1 + cPrime2) / 2;

    // Delta C Prime
    const double deltaCPrime = cPrime2 - cPrime1;

    // S sub L
    const double sSubL = 1 + ((0.015 * pow(lBar - 50, 2)) / sqrt(20 + pow(lBar - 50, 2)));

    // S sub C
    const double sSubC = 1 + 0.045 * cBarPrime;

    // h Prime 1
    const double hPrime1 = GetHPrimeFn(x1.B, aPrime1);

    // h Prime 2
    const double hPrime2 = GetHPrimeFn(x2.B, aPrime2);

    // Delta H Prime
    const double deltaHPrime = 0 == c1 || 0 == c2 ? 0 : 2 * sqrt(cPrime1 * cPrime2) * sin(abs(hPrime1 - hPrime2) <= rad180 ? hPrime2 - hPrime1 : (hPrime2 <= hPrime1 ? hPrime2 - hPrime1 + rad360 : hPrime2 - hPrime1 - rad360) / 2);

    // H Bar Prime
    const double hBarPrime = (abs(hPrime1 - hPrime2) > rad180) ? (hPrime1 + hPrime2 + rad360) / 2 : (hPrime1 + hPrime2) / 2;

    // T
    const double t = 1 - 0.17 * cos(hBarPrime - rad030) + 0.24 * cos(2 * hBarPrime) + 0.32 * cos(3 * hBarPrime + rad006) - 0.20 * cos(4 * hBarPrime - rad063);

    // S sub H
    const double sSubH = 1 + 0.015 * cBarPrime * t;

    // R sub T
    const double rSubT = -2 * sqrt(pow(cBarPrime, 7) / (pow(cBarPrime, 7) + pow(25.0, 7))) * sin(rad060 * exp(-pow((hBarPrime - rad275) / rad025, 2)));

    // Put it all together!
    const double lightness = deltaLPrime / (kSubL * sSubL);
    const double chroma = deltaCPrime / (kSubC * sSubC);
    const double hue = deltaHPrime / (kSubH * sSubH);

    return sqrt(pow(lightness, 2) + pow(chroma, 2) + pow(hue, 2) + rSubT * chroma * hue);
}

ColorFix::ColorFix()
{
    rgb = 0;
    L = 0;
    A = 0;
    B = 0;
}

ColorFix::ColorFix(COLORREF color)
{
    rgb = color;
    _ToLab();
}

// Method Description:
// - Populates our L, A, B values, based on our r, g, b values
// - Converts a color in rgb format to a color in lab format
// - Reference: http://www.easyrgb.com/index.php?X=MATH&H=01#text1
void ColorFix::_ToLab()
{
    double var_R = r / 255.0;
    double var_G = g / 255.0;
    double var_B = b / 255.0;

    if (var_R > 0.04045)
        var_R = pow(((var_R + 0.055) / 1.055), 2.4);
    else
        var_R = var_R / 12.92;
    if (var_G > 0.04045)
        var_G = pow(((var_G + 0.055) / 1.055), 2.4);
    else
        var_G = var_G / 12.92;
    if (var_B > 0.04045)
        var_B = pow(((var_B + 0.055) / 1.055), 2.4);
    else
        var_B = var_B / 12.92;

    var_R = var_R * 100.;
    var_G = var_G * 100.;
    var_B = var_B * 100.;

    //Observer. = 2 degrees, Illuminant = D65
    double X = var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805;
    double Y = var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722;
    double Z = var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505;

    double var_X = X / 95.047; //ref_X =  95.047   (Observer= 2 degrees, Illuminant= D65)
    double var_Y = Y / 100.000; //ref_Y = 100.000
    double var_Z = Z / 108.883; //ref_Z = 108.883

    if (var_X > 0.008856)
        var_X = pow(var_X, (1. / 3.));
    else
        var_X = (7.787 * var_X) + (16. / 116.);
    if (var_Y > 0.008856)
        var_Y = pow(var_Y, (1. / 3.));
    else
        var_Y = (7.787 * var_Y) + (16. / 116.);
    if (var_Z > 0.008856)
        var_Z = pow(var_Z, (1. / 3.));
    else
        var_Z = (7.787 * var_Z) + (16. / 116.);

    L = (116. * var_Y) - 16.;
    A = 500. * (var_X - var_Y);
    B = 200. * (var_Y - var_Z);
}

// Method Description:
// - Populates our r, g, b values, based on our L, A, B values
// - Converts a color in lab format to a color in rgb format
// - Reference: http://www.easyrgb.com/index.php?X=MATH&H=01#text1
void ColorFix::_ToRGB()
{
    double var_Y = (L + 16.) / 116.;
    double var_X = A / 500. + var_Y;
    double var_Z = var_Y - B / 200.;

    if (pow(var_Y, 3) > 0.008856)
        var_Y = pow(var_Y, 3);
    else
        var_Y = (var_Y - 16. / 116.) / 7.787;
    if (pow(var_X, 3) > 0.008856)
        var_X = pow(var_X, 3);
    else
        var_X = (var_X - 16. / 116.) / 7.787;
    if (pow(var_Z, 3) > 0.008856)
        var_Z = pow(var_Z, 3);
    else
        var_Z = (var_Z - 16. / 116.) / 7.787;

    double X = 95.047 * var_X; //ref_X =  95.047     (Observer= 2 degrees, Illuminant= D65)
    double Y = 100.000 * var_Y; //ref_Y = 100.000
    double Z = 108.883 * var_Z; //ref_Z = 108.883

    var_X = X / 100.; //X from 0 to  95.047      (Observer = 2 degrees, Illuminant = D65)
    var_Y = Y / 100.; //Y from 0 to 100.000
    var_Z = Z / 100.; //Z from 0 to 108.883

    double var_R = var_X * 3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
    double var_G = var_X * -0.9689 + var_Y * 1.8758 + var_Z * 0.0415;
    double var_B = var_X * 0.0557 + var_Y * -0.2040 + var_Z * 1.0570;

    if (var_R > 0.0031308)
        var_R = 1.055 * pow(var_R, (1 / 2.4)) - 0.055;
    else
        var_R = 12.92 * var_R;
    if (var_G > 0.0031308)
        var_G = 1.055 * pow(var_G, (1 / 2.4)) - 0.055;
    else
        var_G = 12.92 * var_G;
    if (var_B > 0.0031308)
        var_B = 1.055 * pow(var_B, (1 / 2.4)) - 0.055;
    else
        var_B = 12.92 * var_B;

    r = _Clamp(var_R * 255.);
    g = _Clamp(var_G * 255.);
    b = _Clamp(var_B * 255.);
}

// Method Description:
// - Given a background color, change the foreground color to make it more perceivable if necessary
// - Arguments:
// - back: the color to compare against
// - pColor: where to store the resulting color
// - Return Value:
// - True if we changed our color, false otherwise
COLORREF ColorFix::GetPerceivableColor(COLORREF fg, COLORREF bg)
{
    ColorFix backLab(bg);
    ColorFix frontLab(fg);
    double de1 = GetDeltaE(frontLab, backLab);
    if (de1 < gMinThreshold)
    {
        for (int i = 0; i <= 1; i++)
        {
            double step = (i == 0) ? gLStep : -gLStep;
            frontLab.L += step;

            while (((i == 0) && (frontLab.L <= 100)) || ((i == 1) && (frontLab.L >= 0)))
            {
                double de2 = GetDeltaE(frontLab, backLab);
                if (de2 >= gExpThreshold)
                {
                    frontLab._ToRGB();
                    return frontLab.rgb;
                }
                frontLab.L += step;
            }
        }
    }
    return frontLab.rgb;
}

// Method Description:
// - Clamps the given value to be between 0-255 inclusive
// - Converts the result to BYTE
// Arguments:
// - v: the value to clamp
// Return Value:
// - The clamped value
BYTE ColorFix::_Clamp(double v)
{
    if (v <= 0)
        return 0;
    else if (v >= 255)
        return 255;
    else
        return (BYTE)v;
}
