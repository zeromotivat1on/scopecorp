#include "globals.slh"
#include "light.slh"
#include "color.slh"

struct In_Vertex {
    float3 position;
    float3 normal;
    float2 uv;
    uint   eid;
};

struct Out_Vertex {
    float4 position : SV_Position;
    float3 normal;
    float2 uv;
    uint   eid;
    float3 pixel_world_position;
};

struct Out_Pixel {
    float4 color : SV_Target0;
};

cbuffer Entity_Constants {
    Material material;
};

Sampler2D S2D;
// @Cleanup: remove glsl binding.
layout (binding = 8) RWByteAddressBuffer picking_buffer; // float|depth + uint|eid

[shader("vertex")]
Out_Vertex main_vertex(In_Vertex in) {
    Out_Vertex out;

    out.position = mul(mul(float4(in.position, 1.0f), object_to_world), camera_view_proj);
    out.normal   = in.normal;
    out.uv       = in.uv * uv_scale;
    out.eid      = in.eid;
    out.pixel_world_position = float3(mul(float4(in.position, 1.0f), object_to_world).xyz);

    return out;
}

void update_picking_buffer(uint2 pos, uint eid, float z, float opacity) 
{
    if (opacity < 0.01f) return;
    
    uint2 cursor_pos = (uint2)viewport_cursor_pos;
    if (pos.x != cursor_pos.x || pos.y != cursor_pos.y) return;

    // @Todo: does not work :( Probably due to usage of backend internal depth buffer.
    // Compare pixel z against the currently stored z-value in the
    // picking_buffer. If it's greater or equal (behind) -- return.
    uint d = (uint)z;
    if (d > picking_buffer.Load(0)) return;
    
    picking_buffer.Store2(0, uint2(d, eid));
}

[shader("pixel")]
Out_Pixel main_pixel(Out_Vertex in) {
    Out_Pixel out;

    // @Cleanup: its kinda bad to do it every pixel shader call.
    update_picking_buffer((uint2)in.position.xy, in.eid, in.position.z, 1);

    //const float3 normal = in.normal;
    const float3 normal = float3(0, 1, 0);

    float3 phong = float3(0);
    const float3 view_direction = normalize(camera_position - in.pixel_world_position);

    for (int i = 0; i < direct_light_count; ++i) {
        phong += get_direct_light(normal, view_direction, direct_lights[i], material);
    }

    for (int i = 0; i < point_light_count; ++i) {
        phong += get_point_light(normal, view_direction, in.pixel_world_position,
                                 point_lights[i], material);
    }

    const float4 color = S2D.Sample(in.uv) * float4(phong, 1.0f);
    //color = float4(1, 0, 0, 0.5) * float4(phong, 1.0f);
    out.color = color;
    
    return out;
}
