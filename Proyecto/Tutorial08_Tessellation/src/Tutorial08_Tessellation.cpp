#include "Tutorial08_Tessellation.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ColorConversion.h"
#include "ShaderMacroHelper.hpp"
#include "imgui.h"

namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial08_Tessellation();
}

namespace
{

struct GlobalConstants
{
    unsigned int NumHorzBlocks; // Number of blocks along the horizontal edge
    unsigned int NumVertBlocks; // Number of blocks along the horizontal edge
    float        fNumHorzBlocks;
    float        fNumVertBlocks;

    float fBlockSize;
    float LengthScale;
    float HeightScale;
    float LineWidth;

    float  TessDensity;
    int    AdaptiveTessellation;
    float  ErosionLevel; // Añadir nivel de erosión aquí
    float2 Dummy2;

    float4x4 WorldView;
    float4x4 WorldViewProj;
    float4x4 World; // Añadido: matriz de mundo para cálculos de iluminación
    float4   ViewportSize;
};

// Estructura para las constantes de iluminación que se enviarán al shader
struct LightConstants
{
    float4 Direction;    // xyz: dirección, w: no usado
    float4 Intensity;    // xyz: color/intensidad, w: no usado
    float4 AmbientColor; // xyz: color ambiental, w: no usado
};

} // namespace

void Tutorial08_Tessellation::CreatePipelineStates()
{



    // Al definir las variables estáticas
    ShaderResourceVariableDesc Vars[] =
        {
            {SHADER_TYPE_HULL | SHADER_TYPE_DOMAIN, "g_HeightMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
            {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
        };


    const bool bWireframeSupported = m_pDevice->GetDeviceInfo().Features.GeometryShaders;

    // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSOCreateInfo.PSODesc.Name = "Terrain PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology type defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
    // Cull back faces. For some reason, in OpenGL the order is reversed
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = m_pDevice->GetDeviceInfo().IsGLDevice() ? CULL_MODE_FRONT : CULL_MODE_BACK;
    // Enable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on

    // Create dynamic uniform buffer that will store shader constants
    CreateUniformBuffer(m_pDevice, sizeof(GlobalConstants), "Global shader constants CB", &m_ShaderConstants);
    // Crear un buffer para las constantes de iluminación
    CreateUniformBuffer(m_pDevice, sizeof(LightConstants), "Light attributes CB", &m_LightAttribsCB);

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.Desc.UseCombinedTextureSamplers = true;

    // Pack matrices in row-major order
    ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    // Define shader macros
    ShaderMacroHelper Macros;
    Macros.Add("BLOCK_SIZE", m_BlockSize);

    // Presentation engine always expects input in gamma space. Normally, pixel shader output is
    // converted from linear to gamma space by the GPU. However, some platforms (e.g. Android in GLES mode,
    // or Emscripten in WebGL mode) do not support gamma-correction. In this case the application
    // has to do the conversion manually.
    Macros.Add("CONVERT_PS_OUTPUT_TO_GAMMA", m_ConvertPSOutputToGamma);

    ShaderCI.Macros = Macros;

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);
    ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "TerrainVS";
        ShaderCI.Desc.Name       = "Terrain VS";
        ShaderCI.FilePath        = "terrain.vsh";

        m_pDevice->CreateShader(ShaderCI, &pVS);
    }


    // Create a geometry shader
    RefCntAutoPtr<IShader> pGS;
    if (bWireframeSupported)
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_GEOMETRY;
        ShaderCI.EntryPoint      = "TerrainGS";
        ShaderCI.Desc.Name       = "Terrain GS";
        ShaderCI.FilePath        = "terrain.gsh";

        m_pDevice->CreateShader(ShaderCI, &pGS);
    }

    // Create a hull shader
    RefCntAutoPtr<IShader> pHS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_HULL;
        ShaderCI.EntryPoint      = "TerrainHS";
        ShaderCI.Desc.Name       = "Terrain HS";
        ShaderCI.FilePath        = "terrain.hsh";

        m_pDevice->CreateShader(ShaderCI, &pHS);
    }

    // Create a domain shader
    RefCntAutoPtr<IShader> pDS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_DOMAIN;
        ShaderCI.EntryPoint      = "TerrainDS";
        ShaderCI.Desc.Name       = "Terrain DS";
        ShaderCI.FilePath        = "terrain.dsh";

        m_pDevice->CreateShader(ShaderCI, &pDS);
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS, pWirePS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "TerrainPS";
        ShaderCI.Desc.Name       = "Terrain PS";
        ShaderCI.FilePath        = "terrain.psh";

        m_pDevice->CreateShader(ShaderCI, &pPS);

        if (bWireframeSupported)
        {
            ShaderCI.EntryPoint = "WireTerrainPS";
            ShaderCI.Desc.Name  = "Wireframe Terrain PS";
            ShaderCI.FilePath   = "terrain_wire.psh";

            m_pDevice->CreateShader(ShaderCI, &pWirePS);
        }
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pHS = pHS;
    PSOCreateInfo.pDS = pDS;
    PSOCreateInfo.pPS = pPS;

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    ShaderResourceVariableDesc PSOVars[] = 
    {
        {SHADER_TYPE_HULL | SHADER_TYPE_DOMAIN,  "g_HeightMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL,                      "g_Texture",   SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = PSOVars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(PSOVars);

    // clang-format off
    // Define immutable sampler for g_HeightMap and g_Texture. Immutable samplers should be used whenever possible
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_HULL | SHADER_TYPE_DOMAIN, "g_HeightMap", SamLinearClampDesc},
        {SHADER_TYPE_PIXEL,                     "g_Texture",   SamLinearClampDesc}
    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO[0]);

    if (bWireframeSupported)
    {
        PSOCreateInfo.pGS = pGS;
        PSOCreateInfo.pPS = pWirePS;
        m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO[1]);
    }

    for (Uint32 i = 0; i < _countof(m_pPSO); ++i)
    {
        if (m_pPSO[i])
        {
            m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_VERTEX, "VSConstants")->Set(m_ShaderConstants);
            m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_HULL, "HSConstants")->Set(m_ShaderConstants);
            m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_DOMAIN, "DSConstants")->Set(m_ShaderConstants);

            // Añadir también el buffer de constantes globales al pixel shader
            auto* pConstVar = m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_PIXEL, "PSConstants");
            if (pConstVar != nullptr)
                pConstVar->Set(m_ShaderConstants);

            // Y el buffer de luz
            auto* pLightVar = m_pPSO[i]->GetStaticVariableByName(SHADER_TYPE_PIXEL, "PSLightAttribs");
            if (pLightVar != nullptr)
                pLightVar->Set(m_LightAttribsCB);
        }
    }
    if (m_pPSO[1])
    {
        // clang-format off
        m_pPSO[1]->GetStaticVariableByName(SHADER_TYPE_GEOMETRY, "GSConstants")->Set(m_ShaderConstants);
        m_pPSO[1]->GetStaticVariableByName(SHADER_TYPE_PIXEL,    "PSConstants")->Set(m_ShaderConstants);
        // clang-format on
    }
}

void Tutorial08_Tessellation::LoadTextures()
{
    {
        // Load texture
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = false;
        loadInfo.Name   = "Terrain height map";
        RefCntAutoPtr<ITexture> HeightMap;
        CreateTextureFromFile("ps_height_1k.png", loadInfo, m_pDevice, &HeightMap);
        const auto& HMDesc = HeightMap->GetDesc();
        m_HeightMapWidth   = HMDesc.Width;
        m_HeightMapHeight  = HMDesc.Height;
        // Get shader resource view from the texture
        m_HeightMapSRV = HeightMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    {
        TextureLoadInfo loadInfo;
        loadInfo.IsSRGB = true;
        loadInfo.Name   = "Terrain color map";
        RefCntAutoPtr<ITexture> ColorMap;
        CreateTextureFromFile("ps_texture_2k.png", loadInfo, m_pDevice, &ColorMap);
        // Get shader resource view from the texture
        m_ColorMapSRV = ColorMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    for (size_t i = 0; i < _countof(m_pPSO); ++i)
    {
        if (m_pPSO[i])
        {
            m_pPSO[i]->CreateShaderResourceBinding(&m_SRB[i], true);
            // Set texture SRV in the SRB
            // clang-format off
            m_SRB[i]->GetVariableByName(SHADER_TYPE_PIXEL,  "g_Texture")->Set(m_ColorMapSRV);
            m_SRB[i]->GetVariableByName(SHADER_TYPE_DOMAIN, "g_HeightMap")->Set(m_HeightMapSRV);
            m_SRB[i]->GetVariableByName(SHADER_TYPE_HULL,   "g_HeightMap")->Set(m_HeightMapSRV);
            // clang-format on
        }
    }
}

void Tutorial08_Tessellation::UpdateUI()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Checkbox("Animate", &m_Animate);
        ImGui::Checkbox("Adaptive tessellation", &m_AdaptiveTessellation);
        if (m_pPSO[1])
            ImGui::Checkbox("Wireframe", &m_Wireframe);

        ImGui::Separator();
        ImGui::Text("Erosion Simulation");

        if (ImGui::SliderFloat("Erosion Level", &m_ErosionLevel, 0.0f, 1.0f))
        {
            // Apply erosion effects manually when slider changes
            if (!m_SimulatingErosion)
                ApplyErosionEffects();
        }

        ImGui::SliderFloat("Erosion Speed", &m_ErosionSpeed, 0.05f, 1.0f);

        // Erosion simulation buttons
        if (!m_SimulatingErosion)
        {
            if (ImGui::Button("Simular Erosion"))
            {
                m_SimulatingErosion = true;
                m_ErosionLevel      = 0.0f;
                ApplyErosionEffects();
            }
        }
        else
        {
            if (ImGui::Button("Detener Simulacion"))
            {
                m_SimulatingErosion = false;
            }
            // Show erosion progress
            ImGui::ProgressBar(m_ErosionLevel, ImVec2(0.0f, 0.0f));
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::Text("%.1f%%", m_ErosionLevel * 100.0f);
        }

        ImGui::Separator();
        ImGui::Text("Light Settings");

        // Control para la dirección de la luz
        float lightDir[3] = {m_LightAttribs.Direction.x, m_LightAttribs.Direction.y, m_LightAttribs.Direction.z};
        if (ImGui::SliderFloat3("Light Direction", lightDir, -1.0f, 1.0f))
        {
            m_LightAttribs.Direction = float3(lightDir[0], lightDir[1], lightDir[2]);
            // Normalizar la dirección
            float len = sqrt(m_LightAttribs.Direction.x * m_LightAttribs.Direction.x +
                             m_LightAttribs.Direction.y * m_LightAttribs.Direction.y +
                             m_LightAttribs.Direction.z * m_LightAttribs.Direction.z);
            if (len > 0.0001f)
            {
                m_LightAttribs.Direction.x /= len;
                m_LightAttribs.Direction.y /= len;
                m_LightAttribs.Direction.z /= len;
            }
        }

        // Control para la intensidad de la luz
        float lightInt[3] = {m_LightAttribs.Intensity.x, m_LightAttribs.Intensity.y, m_LightAttribs.Intensity.z};
        ImGui::SliderFloat3("Light Intensity", lightInt, 0.0f, 1.0f);
        m_LightAttribs.Intensity = float3(lightInt[0], lightInt[1], lightInt[2]);

        // Control para el color ambiental
        float ambientColor[3] = {m_LightAttribs.AmbientColor.x, m_LightAttribs.AmbientColor.y, m_LightAttribs.AmbientColor.z};
        ImGui::SliderFloat3("Ambient Color", ambientColor, 0.0f, 0.5f);
        m_LightAttribs.AmbientColor = float3(ambientColor[0], ambientColor[1], ambientColor[2]);

        ImGui::Separator();
        ImGui::Text("Manual Controls");

        ImGui::SliderFloat("Tess density", &m_TessDensity, 1.f, 64.f);
        ImGui::SliderInt("Block size", &m_BlockSize, 16, 128);
        ImGui::SliderFloat("Height scale", &m_HeightScale, 1.0f, 25.0f);
        ImGui::SliderFloat("Length scale", &m_LengthScale, 5.0f, 15.0f);
        ImGui::SliderFloat("Distance", &m_Distance, 1.f, 20.f);
    }
    ImGui::End();
}

void Tutorial08_Tessellation::ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs)
{
    SampleBase::ModifyEngineInitInfo(Attribs);

    Attribs.EngineCI.Features.Tessellation    = DEVICE_FEATURE_STATE_ENABLED;
    Attribs.EngineCI.Features.GeometryShaders = DEVICE_FEATURE_STATE_OPTIONAL;
}

void Tutorial08_Tessellation::Initialize(const SampleInitInfo& InitInfo)
{
    SampleBase::Initialize(InitInfo);

    // Inicializar los valores predeterminados de la luz
    m_LightAttribs.Direction    = float3(-0.5f, -0.6f, -0.6f);
    m_LightAttribs.Intensity    = float3(1.0f, 0.9f, 0.8f);
    m_LightAttribs.AmbientColor = float3(0.15f, 0.15f, 0.2f);

    CreatePipelineStates();
    LoadTextures();
}

// Render a frame
void Tutorial08_Tessellation::Render()
{
    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    float4 ClearColor = {0.350f, 0.350f, 0.350f, 1.0f};
    if (m_ConvertPSOutputToGamma)
    {
        // If manual gamma correction is required, we need to clear the render target with sRGB color
        ClearColor = LinearToSRGB(ClearColor);
    }
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor.Data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    unsigned int NumHorzBlocks = m_HeightMapWidth / m_BlockSize;
    unsigned int NumVertBlocks = m_HeightMapHeight / m_BlockSize;
    {
        // Map the buffer and write rendering data
        MapHelper<GlobalConstants> Consts(m_pImmediateContext, m_ShaderConstants, MAP_WRITE, MAP_FLAG_DISCARD);
        Consts->fBlockSize     = static_cast<float>(m_BlockSize);
        Consts->NumHorzBlocks  = NumHorzBlocks;
        Consts->NumVertBlocks  = NumVertBlocks;
        Consts->fNumHorzBlocks = static_cast<float>(NumHorzBlocks);
        Consts->fNumVertBlocks = static_cast<float>(NumVertBlocks);

        Consts->LengthScale = m_LengthScale;
        Consts->HeightScale = Consts->LengthScale / m_HeightScale;

        Consts->WorldView     = m_WorldViewMatrix;
        Consts->WorldViewProj = m_WorldViewProjMatrix;
        Consts->World         = m_WorldMatrix; // Añadido: matriz de mundo para cálculos de iluminación

        Consts->TessDensity          = m_TessDensity;
        Consts->AdaptiveTessellation = m_AdaptiveTessellation ? 1 : 0;
        Consts->ErosionLevel         = m_ErosionLevel; // Añadido: pasar nivel de erosión al shader

        const auto& SCDesc   = m_pSwapChain->GetDesc();
        Consts->ViewportSize = float4(static_cast<float>(SCDesc.Width), static_cast<float>(SCDesc.Height), 1.f / static_cast<float>(SCDesc.Width), 1.f / static_cast<float>(SCDesc.Height));

        Consts->LineWidth = 3.0f;
    }

    // Actualizar los atributos de la luz
    {
        MapHelper<LightConstants> LightConsts(m_pImmediateContext, m_LightAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
        LightConsts->Direction    = float4(m_LightAttribs.Direction, 0.0f);
        LightConsts->Intensity    = float4(m_LightAttribs.Intensity, 1.0f);
        LightConsts->AmbientColor = float4(m_LightAttribs.AmbientColor, 1.0f);
    }

    // Set the pipeline state
    m_pImmediateContext->SetPipelineState(m_pPSO[m_Wireframe ? 1 : 0]);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.
    m_pImmediateContext->CommitShaderResources(m_SRB[m_Wireframe ? 1 : 0], RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = NumHorzBlocks * NumVertBlocks;
    DrawAttrs.Flags       = DRAW_FLAG_VERIFY_ALL;
    m_pImmediateContext->Draw(DrawAttrs);
}

void Tutorial08_Tessellation::Update(double CurrTime, double ElapsedTime)
{
    SampleBase::Update(CurrTime, ElapsedTime);

    // Update erosion simulation
    if (m_SimulatingErosion)
    {
        m_ErosionLevel += static_cast<float>(ElapsedTime) * m_ErosionSpeed;

        if (m_ErosionLevel >= 1.0f)
        {
            m_ErosionLevel      = 1.0f;
            m_SimulatingErosion = false;
        }

        ApplyErosionEffects();
    }

    // Detectar clic derecho + movimiento para rotar la dirección de la luz
    const auto& MouseState = m_InputController.GetMouseState();
    if (MouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT)
    {
        if (m_LastMousePos.x != -1 && m_LastMousePos.y != -1)
        {
            float dx = MouseState.PosX - m_LastMousePos.x;
            float dy = MouseState.PosY - m_LastMousePos.y;

            // Aplicar rotación a la dirección de la luz basada en el movimiento del mouse
            const float RotationSpeed = 0.005f;
            m_LightRotationX += dx * RotationSpeed;
            m_LightRotationY += dy * RotationSpeed;

            // Crear matriz de rotación y aplicarla a la dirección de la luz
            float4x4 RotX     = float4x4::RotationX(m_LightRotationY);
            float4x4 RotY     = float4x4::RotationY(m_LightRotationX);
            float4x4 Rotation = RotX * RotY;

            float4 NewDir            = float4(0, 0, -1, 0) * Rotation;
            m_LightAttribs.Direction = float3(NewDir.x, NewDir.y, NewDir.z);
        }

        m_LastMousePos.x = static_cast<float>(MouseState.PosX);
        m_LastMousePos.y = static_cast<float>(MouseState.PosY);
    }
    else
    {
        m_LastMousePos = float2(-1, -1);
    }

    UpdateUI();

    if (m_Animate)
    {
        m_RotationAngle += static_cast<float>(ElapsedTime) * 0.2f;
        if (m_RotationAngle > PI_F * 2.f)
            m_RotationAngle -= PI_F * 2.f;
    }

    float4x4 ModelMatrix = float4x4::RotationY(m_RotationAngle) * float4x4::RotationX(-PI_F * 0.1f);
    // Camera is at (0, 0, -m_Distance) looking along Z axis
    float4x4 ViewMatrix = float4x4::Translation(0.f, 0.0f, m_Distance);

    // Get pretransform matrix that rotates the scene according the surface orientation
    auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    // Get projection matrix adjusted to the current screen orientation
    auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 1000.f);

    m_WorldMatrix     = ModelMatrix;
    m_WorldViewMatrix = ModelMatrix * ViewMatrix * SrfPreTransform;

    // Compute world-view-projection matrix
    m_WorldViewProjMatrix = m_WorldViewMatrix * Proj;
}

void Tutorial08_Tessellation::ApplyErosionEffects()
{
    // Store original values for reference
    static const float originalHeightScale = 5.0f;
    static const float originalTessDensity = 32.0f;
    static const int   originalBlockSize   = 32;
    static const float originalLengthScale = 10.0f;

    // Valores originales de luz para referencia
    static const float3 originalLightDir     = float3(-0.5f, -0.6f, -0.6f);
    static const float3 originalLightColor   = float3(1.0f, 0.9f, 0.8f);
    static const float3 originalAmbientColor = float3(0.15f, 0.15f, 0.2f);

    float erosionFactor = m_ErosionLevel;

    // Apply erosion effects to terrain:
    m_HeightScale = originalHeightScale + (erosionFactor * 15.0f);
    m_TessDensity = originalTessDensity + (erosionFactor * 24.0f);
    m_BlockSize   = static_cast<int>(originalBlockSize - (erosionFactor * 16));
    m_LengthScale = originalLengthScale - (erosionFactor * 1.0f);

    // Aplicar efectos de erosión a la iluminación:

    // 1. Dirección de la luz: simular cambio gradual como atardecer/amanecer
    // Rotamos ligeramente la luz hacia abajo y cambiamos su ángulo horizontal
    float    rotationAngle = erosionFactor * 0.4f; // Hasta 0.4 radianes (~23 grados)
    float4x4 rotationY     = float4x4::RotationY(rotationAngle);
    float4x4 rotationX     = float4x4::RotationX(erosionFactor * 0.2f); // Hasta 0.2 radianes (~11 grados)

    float4 newDirection      = float4(originalLightDir, 0.0f) * rotationY * rotationX;
    m_LightAttribs.Direction = float3(newDirection.x, newDirection.y, newDirection.z);

    // 2. Color de la luz: cambiar hacia tonos más anaranjados/rojizos (atardecer)
    // A medida que aumenta la erosión, simulamos más partículas en suspensión
    m_LightAttribs.Intensity.x = originalLightColor.x;                                 // Mantener rojo
    m_LightAttribs.Intensity.y = originalLightColor.y * (1.0f - erosionFactor * 0.3f); // Reducir verde
    m_LightAttribs.Intensity.z = originalLightColor.z * (1.0f - erosionFactor * 0.5f); // Reducir azul más

    // 3. Luz ambiental: aumentar ligeramente para compensar pérdida de luz directa
    // Y añadir un tono azulado para simular atmósfera con polvo
    m_LightAttribs.AmbientColor.x = originalAmbientColor.x * (1.0f - erosionFactor * 0.1f);
    m_LightAttribs.AmbientColor.y = originalAmbientColor.y * (1.0f - erosionFactor * 0.05f);
    m_LightAttribs.AmbientColor.z = originalAmbientColor.z * (1.0f + erosionFactor * 0.3f); // Aumentar azul

    // Aplicar límites a los valores
    m_HeightScale = std::max(1.0f, std::min(25.0f, m_HeightScale));
    m_TessDensity = std::max(1.0f, std::min(64.0f, m_TessDensity));
    m_BlockSize   = std::max(16, std::min(128, m_BlockSize));
    m_LengthScale = std::max(5.0f, std::min(15.0f, m_LengthScale));
}

} // namespace Diligent