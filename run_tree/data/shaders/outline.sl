cbuffer Outline_Constants {
    float4x4 transform;
    float4   color;
};

struct In_Vertex {
    float3 position;
};

struct Out_Vertex {
    float4 position : SV_Position;
};

struct Out_Pixel {
    float4 color : SV_Target0;
};

[shader("vertex")]
Out_Vertex main_vertex(In_Vertex in) {
    Out_Vertex out;
    out.position = mul(float4(in.position, 1.0f), transform);
    return out;
}

[shader("pixel")]
Out_Pixel main_pixel(Out_Vertex in) {
    Out_Pixel out;
    out.color = color;
    return out;
}
