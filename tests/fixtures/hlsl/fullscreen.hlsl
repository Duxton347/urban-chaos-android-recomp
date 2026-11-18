struct Push { float depth; uint discard_all; };
[[vk::push_constant]] ConstantBuffer<Push> draw;
float4 vs(uint id : SV_VertexID) : SV_Position {
    float2 positions[3] = {float2(-1,-1), float2(3,-1), float2(-1,3)};
    return float4(positions[id], draw.depth, 1);
}
float4 ps() : SV_Target {
    if (draw.discard_all != 0) discard;
    return float4(1,0,0,1);
}
