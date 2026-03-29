#pragma once
#include "SampleBase.hpp"
#include "BasicMath.hpp"
namespace Diligent
{
class Tutorial08_Tessellation final : public SampleBase
{
public:
    virtual void        ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs) override final;
    virtual void        Initialize(const SampleInitInfo& InitInfo) override final;
    virtual void        Render() override final;
    virtual void        Update(double CurrTime, double ElapsedTime) override final;
    virtual const Char* GetSampleName() const override final { return "Tutorial08: Tessellation"; }

private:
    void CreatePipelineStates();
    void LoadTextures();
    void UpdateUI();
    void ApplyErosionEffects();
    void CreateShadowMap();
    void RenderShadowMap();

    // Estructura para almacenar atributos de la luz y sombras
    struct LightAttribs
    {
        float3 Direction    = float3(-0.5f, -0.6f, -0.6f); // Dirección de la luz
        float3 Intensity    = float3(1.0f, 0.9f, 0.8f);    // Intensidad/color de la luz
        float3 AmbientColor = float3(0.15f, 0.15f, 0.2f);  // Color ambiental

        // Propiedades de sombras
        float ShadowDepthBias = 0.005f;
        int   PCFFilterSize   = 3;
        float ShadowIntensity = 0.7f;
    };

    RefCntAutoPtr<IPipelineState>         m_pPSO[2];
    RefCntAutoPtr<IPipelineState>         m_pShadowMapPSO;
    RefCntAutoPtr<IShaderResourceBinding> m_SRB[2];
    RefCntAutoPtr<IShaderResourceBinding> m_ShadowMapSRB;
    RefCntAutoPtr<IBuffer>                m_ShaderConstants;
    RefCntAutoPtr<ITextureView>           m_HeightMapSRV;
    RefCntAutoPtr<ITextureView>           m_ColorMapSRV;

    // Recursos para shadow mapping
    RefCntAutoPtr<ITexture>     m_ShadowMap;
    RefCntAutoPtr<ITextureView> m_ShadowMapSRV;
    RefCntAutoPtr<ITextureView> m_ShadowMapDSV;
    RefCntAutoPtr<ISampler>     m_ShadowMapSampler;

    // Buffer para las constantes de iluminación
    RefCntAutoPtr<IBuffer> m_LightAttribsCB;

    float4x4 m_WorldViewProjMatrix;
    float4x4 m_WorldViewMatrix;
    float4x4 m_WorldMatrix;
    float4x4 m_LightViewProjMatrix; // Matriz para renderizado desde la perspectiva de la luz

    bool  m_Animate              = true;
    bool  m_Wireframe            = false;
    float m_RotationAngle        = 0;
    float m_TessDensity          = 32;
    float m_Distance             = 10.f;
    bool  m_AdaptiveTessellation = true;
    int   m_BlockSize            = 32;
    float m_HeightScale          = 5.0f;
    float m_LengthScale          = 10.0f;
    float m_ErosionLevel         = 0.0f;
    bool  m_SimulatingErosion    = false;
    float m_ErosionSpeed         = 0.1f;

    // Variables para control de la luz
    LightAttribs m_LightAttribs;
    float        m_LightRotationX = 0.0f;
    float        m_LightRotationY = 0.0f;
    bool         m_EnableShadows  = true;
    Uint32       m_ShadowMapSize  = 2048;

    // Mouse state para manipulación de la luz
    float2 m_LastMousePos = float2(0, 0);

    unsigned int m_HeightMapWidth  = 0;
    unsigned int m_HeightMapHeight = 0;
};
} // namespace Diligent