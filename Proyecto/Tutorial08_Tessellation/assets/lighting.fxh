// lighting.fxh - Definiciones y funciones para cálculos de iluminación

// Estructura para datos de iluminación
struct LightAttribs
{
    float4 Direction;       // xyz: dirección normalizada de la luz, w: no usado
    float4 Intensity;       // xyz: color/intensidad de la luz, w: no usado  
    float4 AmbientColor;    // xyz: color de la luz ambiental, w: no usado
};

// Función básica de iluminación direccional con componente ambiental
float3 CalculateLighting(float3 Normal, float3 Albedo, LightAttribs Light)
{
    // Asegurar que la normal esté normalizada
    Normal = normalize(Normal);
    
    // Calcular el producto punto entre la normal y la dirección de la luz (invertida)
    // Usamos max para evitar valores negativos (partes del modelo orientadas en dirección opuesta a la luz)
    float NdotL = max(0.0, dot(Normal, -Light.Direction.xyz));
    
    // Calcular el componente difuso
    float3 DiffuseLight = Light.Intensity.xyz * NdotL;
    
    // Calcular el componente ambiental
    float3 AmbientLight = Light.AmbientColor.xyz;
    
    // Combinar los componentes difuso y ambiental con el color del material
    float3 FinalColor = Albedo * (DiffuseLight + AmbientLight);
    
    return FinalColor;
}