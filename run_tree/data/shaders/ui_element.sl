#include "globals.slh"
#include "color.slh"

struct In_Vertex {
    float2 position;
    uint   color;
};

struct Out_Vertex {
    float4 position : SV_Position;
    float4 color;
};

struct Out_Pixel {
    float4 color : SV_Target0;
};

[shader("vertex")]
Out_Vertex main_vertex(In_Vertex in) {
    Out_Vertex out;

    out.position = mul(float4(in.position, 0.0f, 1.0f), viewport_ortho);
    out.color    = rgba_unpack(in.color);
    
    return out;
}

[shader("pixel")]
Out_Pixel main_pixel(Out_Vertex in) {
    Out_Pixel out;
    out.color = in.color;
    return out;
}
