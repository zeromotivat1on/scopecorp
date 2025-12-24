#include "globals.slh"
#include "color.slh"

struct In_Vertex {
    float2   pos;
    float4   uv_rect;
    uint     color;
    uint     charcode;
    float4x4 transform;
};

struct Out_Vertex {
    float4 position : SV_Position;
    float2 uv;
    float4 color;
    uint   charcode;
};

struct Out_Pixel {
    float4 color : SV_Target0;
};

Sampler2D font_atlas;

[shader("vertex")]
Out_Vertex main_vertex(In_Vertex in) {
    Out_Vertex out;

    const float2 uv = float2(lerp(in.uv_rect.x, in.uv_rect.z, in.pos.x),
                             lerp(1.0f - in.uv_rect.y, 1.0f - in.uv_rect.w, 1.0f - in.pos.y));
    
    out.position = mul(mul(float4(in.pos, 0.0f, 1.0f), in.transform), viewport_ortho);
    out.uv       = float2(uv.x, uv.y);
    out.color    = rgba_unpack(in.color);
    out.charcode = in.charcode;

    return out;
}

[shader("pixel")]
Out_Pixel main_pixel(Out_Vertex in) {
    Out_Pixel out;
    
    float alpha = font_atlas.Sample(in.uv).r;
    out.color = in.color * float4(1.0f, 1.0f, 1.0f, alpha);

    return out;
}
