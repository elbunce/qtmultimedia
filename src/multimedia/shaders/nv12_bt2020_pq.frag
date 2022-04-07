#version 440
#extension GL_GOOGLE_include_directive : enable

#include "uniformbuffer.glsl"
#include "colortransfer.glsl"
#include "colorconvert.glsl"
#include "hdrtonemapper.glsl"

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D plane1Texture;
layout(binding = 2) uniform sampler2D plane2Texture;

void main()
{
    float Y = texture(plane1Texture, texCoord).r;
    vec2 UV = texture(plane2Texture, texCoord).rg;
    fragColor = vec4(Y, UV.x, UV.y, 1.);
    // map to Rec.2020 color space
    fragColor = ubuf.colorMatrix * fragColor;
    fragColor = convertPQToLinear(fragColor);
    fragColor = convertRec2020ToSRGB(fragColor);

#ifdef QMM_OUTPUTSURFACE_LINEAR
    fragColor = tonemapBT2390(fragColor, ubuf.masteringWhite, ubuf.maxLumPQ);
#else
    fragColor = fragColor*.7; // scale down slightly to reduce overexposure
    fragColor = tonemapBT2390(fragColor, ubuf.masteringWhite, ubuf.maxLumPQ);
    fragColor = convertSRGBFromLinear(fragColor);
#endif
    fragColor *= ubuf.opacity;
}