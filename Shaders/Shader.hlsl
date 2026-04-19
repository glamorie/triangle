struct vertex
{
  float2 Position : POSITION;
  float3 Color    : COLOR;
};

struct pixel
{
  float4 Position : SV_Position;
  float3 Color    : COLOR;
};

pixel VertexMain(vertex Vertex)
{
  pixel Out;
  Out.Position = float4(Vertex.Position, 0.0f, 1.0f);
  Out.Color = Vertex.Color;
  return Out;
}

float4 PixelMain(pixel Pixel) : SV_Target
{
  return float4(Pixel.Color, 1.0);
}