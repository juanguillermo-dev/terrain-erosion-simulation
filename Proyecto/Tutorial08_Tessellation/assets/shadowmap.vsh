#include "structures.fxh"

cbuffer VSConstants
{
    GlobalConstants g_Constants;
};

// Para el shadow pass, solo necesitamos calcular la posición desde la perspectiva de la luz
void ShadowMapVS(in  uint    BlockID : SV_VertexID,
                out float4   Pos     : SV_Position)
{
    uint BlockHorzOrder = BlockID % g_Constants.NumHorzBlocks;
    uint BlockVertOrder = BlockID / g_Constants.NumHorzBlocks;
    
    float2 BlockOffset = float2(
        float(BlockHorzOrder) / g_Constants.fNumHorzBlocks,
        float(BlockVertOrder) / g_Constants.fNumVertBlocks
    );
    
    float2 UV = BlockOffset;
    float2 XY = (UV - float2(0.5, 0.5)) * g_Constants.LengthScale;
    float Height = g_HeightMap.SampleLevel(g_HeightMap_sampler, UV, 0) * g_Constants.HeightScale;
    float4 PosWorld = float4(XY.x, Height, XY.y, 1.0);
    
    // Usamos la matriz de vista/proyección de la luz
    Pos = mul(PosWorld, g_Constants.LightViewProj);
}