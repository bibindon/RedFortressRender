#include "RenderSettingsDialogInternal.h"
namespace NSRender
{
namespace RenderSettingsDialogInternal
{
void SyncDebugGBufferCheckboxes(HWND hWnd, Render* render)
{
    const DebugGBufferView view = render->GetDebugGBufferView();
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_DEBUG_WORLD_POS,
                        view == DebugGBufferView::WorldPos);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_DEBUG_NORMAL,
                        view == DebugGBufferView::Normal);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_DEBUG_DEPTH,
                        view == DebugGBufferView::Depth);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_DEBUG_THICKNESS,
                        view == DebugGBufferView::Thickness);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_DEBUG_BACK_DEPTH,
                        view == DebugGBufferView::BackDepth);
}

void SyncRenderSettingsDialogFromRender(HWND hWnd)
{
    RenderSettingsDialogState* state = reinterpret_cast<RenderSettingsDialogState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    Render* render = (state != nullptr) ? state->render : nullptr;
    if (render == nullptr)
    {
        return;
    }
    SetSettingsComboTextSelection(hWnd, 31000, FormatResolutionLabel(Common::ScreenW(), Common::ScreenH()));
    SetSettingsComboSelection(hWnd, 31001, RenderingQualityToComboIndex(render->GetRenderQuality()));
    SetSettingsCheckbox(hWnd, 31002, render->IsShowFPS());
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_POINT_LIGHT_DISABLE,
                        !render->IsPointLightEnabled());
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_FRAME_RATE_SLEEP_DISABLE,
                        !render->IsFrameRateSleepEnabled());
    SetSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_GBUFFER_ENABLE, render->IsGBufferEnabled());
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_FRONT_BACKFACE_CULLING_ENABLE,
                        render->IsGBufferFrontBackfaceCullingEnabled());
    SyncDebugGBufferCheckboxes(hWnd, render);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_DEPTH_R32F,
                        render->GetGBufferDepthFormat() == GBufferScalarFormat::R32F);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_DEPTH_R16F,
                        render->GetGBufferDepthFormat() == GBufferScalarFormat::R16F);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_FOG_DEPTH_R32F,
                        render->GetGBufferFogDepthFormat() == GBufferScalarFormat::R32F);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_FOG_DEPTH_R16F,
                        render->GetGBufferFogDepthFormat() == GBufferScalarFormat::R16F);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_POSITION_RGBA16F,
                        render->GetGBufferPositionFormat() == GBufferVectorFormat::A16B16G16R16F);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_POSITION_ABGR8,
                        render->GetGBufferPositionFormat() == GBufferVectorFormat::A8B8G8R8);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_NORMAL_RGBA16F,
                        render->GetGBufferNormalFormat() == GBufferVectorFormat::A16B16G16R16F);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_NORMAL_ABGR8,
                        render->GetGBufferNormalFormat() == GBufferVectorFormat::A8B8G8R8);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_THICKNESS_R32F,
                        render->GetGBufferThicknessFormat() == GBufferScalarFormat::R32F);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_THICKNESS_R16F,
                        render->GetGBufferThicknessFormat() == GBufferScalarFormat::R16F);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_BACK_DEPTH_R32F,
                        render->GetGBufferBackDepthFormat() == GBufferScalarFormat::R32F);
    SetSettingsCheckbox(hWnd,
                        IDC_RENDER_SETTINGS_GBUFFER_BACK_DEPTH_R16F,
                        render->GetGBufferBackDepthFormat() == GBufferScalarFormat::R16F);
    SetSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_SATURATE_ENABLE, render->IsPostEffectSaturateEnabled());
    SetSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_GAUSSIAN_ENABLE, render->IsPostEffectGaussianFilterEnabled());
    SetSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_BLOOM_ENABLE, render->IsPostEffectBloomEnabled());
    SetSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_SSAO_ENABLE, render->IsPostEffectSSAOEnabled());
    SetSettingsCheckbox(hWnd, IDC_RENDER_SETTINGS_FOG_ENABLE, render->IsPostEffectFogEnabled());
    SetSettingsCheckbox(hWnd, 31006, render->IsPostEffectDepthBufferShadowEnabled());
    SetSettingsCheckbox(hWnd, 31007, render->IsPostEffectSSGIEnabled());
    SetSettingsCheckbox(hWnd, 31008, render->IsPostEffectHeightFogEnabled());
    SetSettingsCheckbox(hWnd, 31012, render->IsPostEffectStarBurstEnabled());
    SetSettingsCheckbox(hWnd, 31013, render->IsPostEffectGodRayEnabled());
    SetSettingsCheckbox(hWnd, 31014, render->IsPostEffectHaloEnabled());
    SetSettingsCheckbox(hWnd, 31120, render->IsMeshMixSSSEnabled());
    SetSettingsCheckbox(hWnd, 31121, render->IsPhongTreatTextureAsWhiteEnabled());
    SetSettingsCheckbox(hWnd, 31122, render->IsMeshMixSpecularIntensityOverrideEnabled());
    SetSettingsCheckbox(hWnd, 31123, render->IsMeshMixSpecularEdgeOverrideEnabled());
    SetSettingsCheckbox(hWnd, 31302, render->IsMeshMixSkinAnimAlphaClipEnabled());
    SetSettingsCheckbox(hWnd, 31303, render->IsMeshMixSkinAnimIgnoreTransparentMaterialEnabled());
    SetSettingsCheckbox(hWnd, 31702, render->IsPostEffectSSGIBlurEnabled());
    SetSettingsCheckbox(hWnd, 31704, render->IsPostEffectSSGISeparableBlurEnabled());
    SetSettingsCheckbox(hWnd, 31806, render->IsPostEffectSSAOCompositeGaussian3x3Enabled());
    SetSettingsCheckbox(hWnd, 31807, render->IsPostEffectSSAOSeparableBlurEnabled());
    SetSettingsCheckbox(hWnd, 31808, render->IsPostEffectSSAOBlurEnabled());
    SetSettingsCheckbox(hWnd, 31809, render->IsPostEffectSSAODepthScaledSampleDistanceEnabled());
    SetSettingsCheckbox(hWnd, 31810, render->IsPostEffectSSAORandomSamplingDirectionEnabled());
    SetSettingsCheckbox(hWnd, 31811, render->IsPostEffectSSAOMaxDarknessClampEnabled());
    SetSettingsCheckbox(hWnd, 32020, render->IsPostEffectGaussianFilterEnabled());
    SetSettingsCheckbox(hWnd, 32030, render->IsPostEffectMaskedGaussianFilterEnabled());
    SetSettingsCheckbox(hWnd, 32100, render->IsPostEffectAAEnabled());
    SetSettingsCheckbox(hWnd, 32101, render->IsPostEffectTAAEnabled());
    SetSettingsCheckbox(hWnd, 32112, render->IsPostEffectMotionBlurCameraEnabled());
    const DepthOfFieldMode dofMode = render->GetPostEffectDepthOfFieldMode();
    SetSettingsCheckbox(hWnd, 31009, dofMode == DepthOfFieldMode::Disabled);
    SetSettingsCheckbox(hWnd, 31010, dofMode == DepthOfFieldMode::Enabled);
    SetSettingsCheckbox(hWnd, 31011, dofMode == DepthOfFieldMode::AutoNear);
    state->cameraNearPlane = render->GetCameraNearPlane();
    state->cameraFarPlane = render->GetCameraFarPlane();
    state->cameraHorizontalFovDegrees = render->GetCameraHorizontalFovDegrees();
    state->cameraShakeDuration = render->GetCameraShakeDuration();
    state->cameraShakeIntensity = render->GetCameraShakeIntensity();
    state->gBufferNearPlane = render->GetGBufferNearPlane();
    state->gBufferFarPlane = render->GetGBufferFarPlane();
    state->lightColor = render->GetLightColor();
    state->ambientLightColor = render->GetAmbientLightColor();
    state->fogColor = render->GetPostEffectFogColor();
    state->godRayColor = render->GetPostEffectGodRayLightColor();
    state->godRayPos = render->GetPostEffectGodRayLightPos();
    SetSettingsEditFloat(hWnd, 41000, state->cameraNearPlane, L"%.3f");
    SetSettingsEditFloat(hWnd, 41001, state->cameraFarPlane, L"%.3f");
    SetSettingsEditFloat(hWnd, 41003, state->cameraHorizontalFovDegrees, L"%.0f");
    SetSettingsEditFloat(hWnd, 41004, state->cameraShakeDuration, L"%.3f");
    SetSettingsEditFloat(hWnd, 41005, state->cameraShakeIntensity, L"%.2f");
    SetTrackbarFromFloat(hWnd, 31003, state->cameraHorizontalFovDegrees, 1.0f, 180.0f);
    SetTrackbarFromFloat(hWnd, 31004, state->cameraShakeDuration, 0.1f, 5.0f);
    SetTrackbarFromFloat(hWnd, 31005, state->cameraShakeIntensity, 0.0f, 1.0f);
    SetSettingsEditFloat(hWnd, 41010, state->gBufferNearPlane, L"%.3f");
    SetSettingsEditFloat(hWnd, 41011, state->gBufferFarPlane, L"%.3f");
    SetSettingsEditFloat(hWnd, 41100, render->GetLightBrightness(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41101, render->GetAmbientLightBrightness(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41102, render->GetMeshMixSaturateShadowIntensity());
    SetSettingsEditFloat(hWnd, 41103, render->GetMeshMixShadowDarkness());
    SetSettingsEditFloat(hWnd, 41104, render->GetMeshMixSpecularIntensity());
    SetSettingsEditFloat(hWnd, 41105, render->GetMeshMixSpecularEdge());
    SetSettingsEditFloat(hWnd, 41106, render->GetMeshMixEnvMapBlend());
    SetSettingsEditFloat(hWnd, 41107, render->GetMeshMixSSSIntensity());
    SetSettingsEditFloat(hWnd, 41124, render->GetMeshMixFresnelIntensity());
    SetTrackbarFromFloat(hWnd, 31100, render->GetLightBrightness(), 0.0f, 5.0f);
    SetTrackbarFromFloat(hWnd, 31101, render->GetAmbientLightBrightness(), 0.0f, 5.0f);
    SetTrackbarFromFloat(hWnd, 31102, render->GetMeshMixSaturateShadowIntensity(), 0.0f, 10.0f);
    SetTrackbarFromFloat(hWnd, 31103, render->GetMeshMixShadowDarkness(), 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31104, render->GetMeshMixSpecularIntensity(), 0.0f, 2.0f);
    SetTrackbarFromFloat(hWnd, 31105, render->GetMeshMixSpecularEdge(), 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31106, render->GetMeshMixEnvMapBlend(), 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31107, render->GetMeshMixSSSIntensity(), 0.0f, 30.0f);
    SetTrackbarFromFloat(hWnd, 31124, render->GetMeshMixFresnelIntensity(), 0.0f, 2.0f);
    SetSettingsEditFloat(hWnd, 41200, state->lightColor.r);
    SetSettingsEditFloat(hWnd, 41201, state->lightColor.g);
    SetSettingsEditFloat(hWnd, 41202, state->lightColor.b);
    SetSettingsEditFloat(hWnd, 41203, state->ambientLightColor.r);
    SetSettingsEditFloat(hWnd, 41204, state->ambientLightColor.g);
    SetSettingsEditFloat(hWnd, 41205, state->ambientLightColor.b);
    SetTrackbarFromFloat(hWnd, 31200, state->lightColor.r, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31201, state->lightColor.g, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31202, state->lightColor.b, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31203, state->ambientLightColor.r, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31204, state->ambientLightColor.g, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31205, state->ambientLightColor.b, 0.0f, 1.0f);
    SetSettingsEditFloat(hWnd, 41500, render->GetMeshPBRRoughness(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41501, render->GetMeshPBRMetallic(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41502, render->GetMeshPBREnvReflectionIntensity(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41503, render->GetMeshPBREnvMaxMipLevel(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41504, render->GetMeshPBREnvDiffuseIntensity(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41505, render->GetMeshPBREnvDiffuseMipLevel(), L"%.3f");
    SetSettingsEditTextIfNotFocused(hWnd, 32212, render->GetMeshPBREnvMapTexturePath().c_str());
    SetTrackbarFromFloat(hWnd, 31500, render->GetMeshPBRRoughness(), 0.04f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31501, render->GetMeshPBRMetallic(), 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31502, render->GetMeshPBREnvReflectionIntensity(), 0.0f, 3.0f);
    SetTrackbarFromFloat(hWnd, 31503, render->GetMeshPBREnvMaxMipLevel(), 0.0f, 10.0f);
    SetTrackbarFromFloat(hWnd, 31504, render->GetMeshPBREnvDiffuseIntensity(), 0.0f, 3.0f);
    SetTrackbarFromFloat(hWnd, 31505, render->GetMeshPBREnvDiffuseMipLevel(), 0.0f, 10.0f);
    SetSettingsEditFloat(hWnd, 41600, render->GetPostEffectDepthBufferShadowIntensity());
    SetSettingsEditFloat(hWnd, 41601, render->GetPostEffectDepthBufferShadowSaturationBoost());
    SetSettingsEditFloat(hWnd, 41602, render->GetPostEffectDepthBufferShadowCoverage());
    SetSettingsEditFloat(hWnd, 41603, render->GetPostEffectDepthBufferShadowBias(), L"%.5f");
    SetSettingsEditFloat(hWnd, 42310, render->GetPostEffectDepthBufferShadowBiasFar(), L"%.5f");
    SetTrackbarFromFloat(hWnd, 31600, render->GetPostEffectDepthBufferShadowIntensity(), 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31601, render->GetPostEffectDepthBufferShadowSaturationBoost(), 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31602, render->GetPostEffectDepthBufferShadowCoverage(), 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31603, render->GetPostEffectDepthBufferShadowBias(), 0.0f, 0.03f);
    SetTrackbarFromFloat(hWnd, 32310, render->GetPostEffectDepthBufferShadowBiasFar(), 0.0f, 0.03f);
    SetSettingsComboSelection(hWnd, 31610, TapCountToComboIndex(render->GetPostEffectDepthBufferShadowPcfTapCount()));
    SetSettingsComboSelection(hWnd, 31611, TapCountToComboIndex(render->GetPostEffectDepthBufferShadowCompositeTapCount()));
    SetSettingsComboSelection(hWnd, 31612, TexSizeDivisorToComboIndex(render->GetPostEffectDepthBufferShadowTexSizeDivisor()));
    SetSettingsCheckbox(hWnd, 31613, render->IsPostEffectDepthBufferShadowDebugLightDepthEnabled());
    SetSettingsCheckbox(hWnd, 31615, render->IsPostEffectDepthBufferShadowFarEnabled());
    SetSettingsComboSelection(hWnd, 31700, SampleCountToComboIndex(render->GetPostEffectSSGISampleCount()));
    SetSettingsEditFloat(hWnd, 41710, render->GetPostEffectSSGIIndirectLightStrength());
    SetSettingsEditFloat(hWnd, 41711, render->GetPostEffectSSGIIndirectLightMaxContribution());
    SetSettingsEditFloat(hWnd, 41701, render->GetPostEffectSSGISampleRadius());
    SetTrackbarFromFloat(hWnd, 31701, render->GetPostEffectSSGISampleRadius(), 0.1f, 10.0f);
    SetSettingsComboSelection(hWnd, 31703, BlurKernelSizeToComboIndex(render->GetPostEffectSSGIBlurKernelSize()));
    SetSettingsEditFloat(hWnd, 41800, render->GetPostEffectSSAOSampleRadius());
    SetSettingsEditFloat(hWnd, 41803, render->GetPostEffectSSAOShadowStrength());
    SetSettingsEditFloat(hWnd, 41804, render->GetPostEffectSSAOSaturationBoost());
    SetTrackbarFromFloat(hWnd, 31800, render->GetPostEffectSSAOSampleRadius(), 0.05f, 10.0f);
    SetTrackbarFromFloat(hWnd, 31803, render->GetPostEffectSSAOShadowStrength(), 0.0f, 4.0f);
    SetTrackbarFromFloat(hWnd, 31804, render->GetPostEffectSSAOSaturationBoost(), 0.0f, 5.0f);
    SetSettingsComboSelection(hWnd, 31801, TexSizeDivisorToComboIndex(render->GetPostEffectSSAOTexSizeDivisor()));
    SetSettingsComboSelection(hWnd, 31802, BlurKernelSizeToComboIndex(render->GetPostEffectSSAOBlurKernelSize()));
    SetSettingsComboSelection(hWnd, 31805, SampleCountToComboIndex(render->GetPostEffectSSAOSampleCount()));
    SetSettingsEditFloat(hWnd, 41900, render->GetPostEffectFogIntensity(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41901, state->fogColor.r);
    SetSettingsEditFloat(hWnd, 41902, state->fogColor.g);
    SetSettingsEditFloat(hWnd, 41903, state->fogColor.b);
    SetTrackbarFromFloat(hWnd, 31900, render->GetPostEffectFogIntensity(), 0.0f, 20.0f);
    SetTrackbarFromFloat(hWnd, 31901, state->fogColor.r, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31902, state->fogColor.g, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 31903, state->fogColor.b, 0.0f, 1.0f);
    SetSettingsEditFloat(hWnd, 41920, render->GetPostEffectHeightFogIntensity());
    SetSettingsEditFloat(hWnd, 41921, render->GetPostEffectHeightFogStart(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41922, render->GetPostEffectHeightFogMax(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41923, render->GetPostEffectHeightFogDistanceStart(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41924, render->GetPostEffectHeightFogDistanceMax(), L"%.3f");
    SetTrackbarFromFloat(hWnd, 31920, render->GetPostEffectHeightFogIntensity(), 0.0f, 5.0f);
    SetTrackbarFromFloat(hWnd, 31921, render->GetPostEffectHeightFogStart(), -50000.0f, 50000.0f);
    SetTrackbarFromFloat(hWnd, 31922, render->GetPostEffectHeightFogMax(), -50000.0f, 50000.0f);
    SetTrackbarFromFloat(hWnd, 31923, render->GetPostEffectHeightFogDistanceStart(), 0.0f, 100000.0f);
    SetTrackbarFromFloat(hWnd, 31924, render->GetPostEffectHeightFogDistanceMax(), 0.0f, 100000.0f);
    SetSettingsEditFloat(hWnd, 41940, render->GetPostEffectSaturate(), L"%.3f");
    SetTrackbarFromFloat(hWnd, 31940, render->GetPostEffectSaturate(), 0.0f, 4.0f);
    SetSettingsEditFloat(hWnd, 41950, render->GetPostEffectDepthOfFieldFocalDistance(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41951, render->GetPostEffectDepthOfFieldMaxBlurDistance(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41952, render->GetPostEffectDepthOfFieldAutoActivationDistance(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41953, render->GetPostEffectDepthOfFieldStartNear(), L"%.3f");
    SetTrackbarFromFloat(hWnd, 31950, render->GetPostEffectDepthOfFieldFocalDistance(), 0.5f, 50.0f);
    SetTrackbarFromFloat(hWnd, 31951, render->GetPostEffectDepthOfFieldMaxBlurDistance(), 0.5f, 50.0f);
    SetTrackbarFromFloat(hWnd, 31952, render->GetPostEffectDepthOfFieldAutoActivationDistance(), 0.5f, 50.0f);
    SetTrackbarFromFloat(hWnd, 31953, render->GetPostEffectDepthOfFieldStartNear(), 0.0f, 50.0f);
    SetSettingsEditFloat(hWnd, 41970, render->GetPostEffectBloomThreshold(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41971, render->GetPostEffectBloomWeightSum(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41972, render->GetPostEffectHaloThreshold(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41980, render->GetPostEffectStarBurstThreshold(), L"%.3f");
    SetSettingsEditFloat(hWnd, 41981, render->GetPostEffectStarBurstDistanceFade());
    SetTrackbarFromFloat(hWnd, 31970, render->GetPostEffectBloomThreshold(), 0.0f, 5.0f);
    SetTrackbarFromFloat(hWnd, 31971, render->GetPostEffectBloomWeightSum(), 1.0f, 100.0f);
    SetTrackbarFromFloat(hWnd, 31972, render->GetPostEffectHaloThreshold(), 0.0f, 5.0f);
    SetTrackbarFromFloat(hWnd, 31980, render->GetPostEffectStarBurstThreshold(), 0.0f, 5.0f);
    SetTrackbarFromFloat(hWnd, 31981, render->GetPostEffectStarBurstDistanceFade(), 0.0f, 1.0f);
    SetSettingsEditFloat(hWnd, 42000, state->godRayColor.x);
    SetSettingsEditFloat(hWnd, 42001, state->godRayColor.y);
    SetSettingsEditFloat(hWnd, 42002, state->godRayColor.z);
    SetSettingsEditFloat(hWnd, 42003, render->GetPostEffectGodRayIntensity());
    SetSettingsEditFloat(hWnd, 42004, render->GetPostEffectGodRayVirtualProximityStrength());
    SetSettingsEditFloat(hWnd, 42005, state->godRayPos.x, L"%.3f");
    SetSettingsEditFloat(hWnd, 42006, state->godRayPos.y, L"%.3f");
    SetSettingsEditFloat(hWnd, 42007, state->godRayPos.z, L"%.3f");
    SetTrackbarFromFloat(hWnd, 32000, state->godRayColor.x, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 32001, state->godRayColor.y, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 32002, state->godRayColor.z, 0.0f, 1.0f);
    SetTrackbarFromFloat(hWnd, 32003, render->GetPostEffectGodRayIntensity(), 0.0f, 3.0f);
    SetTrackbarFromFloat(hWnd, 32004, render->GetPostEffectGodRayVirtualProximityStrength(), 0.0f, 5.0f);
    SetTrackbarFromFloat(hWnd, 32005, state->godRayPos.x, -200.0f, 200.0f);
    SetTrackbarFromFloat(hWnd, 32006, state->godRayPos.y, -200.0f, 200.0f);
    SetTrackbarFromFloat(hWnd, 32007, state->godRayPos.z, -200.0f, 200.0f);
    SetSettingsEditFloat(hWnd, 42021, render->GetPostEffectGaussianStrength());
    SetTrackbarFromFloat(hWnd, 32021, render->GetPostEffectGaussianStrength(), 0.0f, 1.0f);
    SetSettingsEditTextIfNotFocused(hWnd, 32220, render->GetPostEffectMaskedGaussianMaskPath().c_str());
    SetSettingsEditFloat(hWnd, 42101, render->GetPostEffectTAAHistoryWeight());
    SetSettingsEditFloat(hWnd, 42110, render->GetPostEffectMotionBlurCameraMaxBlurPixels(), L"%.0f");
    SetSettingsEditInt(hWnd, 42111, render->GetPostEffectMotionBlurCameraSampleCount());
    SetTrackbarFromFloat(hWnd, 32110, render->GetPostEffectMotionBlurCameraMaxBlurPixels(), 1.0f, 64.0f);
    SetTrackbarFromInt(hWnd, 32111, render->GetPostEffectMotionBlurCameraSampleCount(), 2, 21);
    SetSettingsEditInt(hWnd, 42120, render->GetPostEffectFXAAQuality());
    SetTrackbarFromInt(hWnd, 32120, render->GetPostEffectFXAAQuality(), 1, 8);
    SetSettingsEditInt(hWnd, 42130, render->GetPostEffectFontSampleSize());
    SetTrackbarFromInt(hWnd, 32130, render->GetPostEffectFontSampleSize(), 1, 21);
    SetSettingsEditFloat(hWnd, 42142, render->GetExplosionScale(), L"%.2f");
    SetTrackbarFromFloat(hWnd, 32142, render->GetExplosionScale(), 0.1f, 10.0f);
    SetSettingsEditFloat(hWnd, 42143, render->GetDamageScale(), L"%.2f");
    SetTrackbarFromFloat(hWnd, 32143, render->GetDamageScale(), 0.1f, 10.0f);
    SyncLoadedModelsFromRender(state);
    UpdatePointLightsList(state);
    UpdateSettingsTextList(state);
}
}
}
