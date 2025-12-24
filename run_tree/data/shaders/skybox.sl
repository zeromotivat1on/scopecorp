#include "globals.slh"

struct In_Vertex {
    float3 position;
    float3 normal;
    float2 uv;
    int    eid;
};

struct Out_Vertex {
    float4 position : SV_Position;
    float2 uv;
    int    eid;
};

struct Out_Pixel {
    float4 color : SV_Target0;
    int    eid   : SV_Target1;
};

Sampler2D S2D;

[shader("vertex")]
Out_Vertex main_vertex(In_Vertex in) {
    Out_Vertex out;
    
    out.position = float4(in.position, 1.0f);
    
    float depth_scale = 1.0f - (uv_offset.z * 0.005f);
    float2 uv = in.uv - 0.5f;
    uv *= depth_scale;
    uv += 0.5f;
    out.uv = (uv + uv_offset.xy * 0.01f) * uv_scale;
    
    out.eid = in.eid;

    return out;
}

[shader("pixel")]
Out_Pixel main_pixel(Out_Vertex in) {
    Out_Pixel out;

    //out.color = float4(1, 0, 0, 1);
    out.color = S2D.Sample(in.uv);
    out.eid   = in.eid;

    return out;
}
