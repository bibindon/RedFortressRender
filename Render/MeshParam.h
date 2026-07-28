#pragma once

#include "Common.h"

namespace NSRender
{

struct stMeshParam
{
    bool ambient = true;
    DWORD ambientColor = 0x101010ff;
    DWORD specularColor = 0xffffffff;
    float specularIntensity = 0.0f;
    float specularEdge = 0.0f;
    bool fresnel = true;
    float fresnelIntensity = 0.08f;
    bool specularIntensityOverrideEnabled = false;
    bool specularEdgeOverrideEnabled = false;
    bool treatTextureAsWhite = false;
    bool fogDistance = true;
    float fogDistanceLength = 10000.0f;
    float fogDistanceSpeed = 0.0f;
    DWORD fogDistanceColor = 0x7f7fffff;
    bool fogHeight = false;
    bool smooth = false;
    bool shadow = true;
    bool saturateShadow = true;
    float saturateShadowIntensity = 1.2f;
    float shadowDarkness = 1.0f;
    bool cubeMapping = false;
    float cubeMappingRate = 1.0f;
    float cubeMappingGauss = 0.0f;
    bool autoHide = false;
    bool parallaxOcclusionMapping = false;
    bool normalMapping = false;
    bool glass = false;
    bool mirror = false;
    bool waterMirror = false;
    bool emit = false;
    float emitIntensity = 1.0f;
    DWORD emitColor = 0x00ffffff;
    float emitPointLightIntensity = 0.1f;
    float emitPointLightRange = 6.0f;
    bool wave = false;
    float waveIntensity = 0.1f;
    float waveSpeed = 5.0f;
    float waveDensity = 20.0f;
    float waterReflectionStrength = 0.25f;
    float waterReflectionTint = 0.2f;
    bool sway = false;
    float swayIntensity = 0.1f;
    bool pointLight = true;
    bool ssao = true;
    bool collision = false;
    float collisionRadius = 2.0f;
    bool sss = false;
    float sssIntensity = 1.0f;
    DWORD sssColor = 0xffff80;
};

enum class eMeshParamPreset
{
    TREE,
    GRASS,
    STONE,
    MIRROR,
    GLASS,
    SKIN,
    HAIR,
    WAVE,
    CLOTH,
    METAL,
    RUBBER
};

stMeshParam GetMeshParamPreset(eMeshParamPreset preset);

}
