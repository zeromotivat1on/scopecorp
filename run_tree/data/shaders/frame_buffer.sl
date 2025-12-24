cbuffer Frame_Buffer_Constants {
    float4x4 transform;
    float2   resolution;
    float    pixel_size;
    float    curve_distortion_factor;
    float    chromatic_aberration_offset;
    int      quantize_color_count;
    float    noise_blend_factor;
    int      scanline_count;
    float    scanline_intensity;
};

// Basically the same vertex as for deafult mesh, as we are using quad obj.
struct In_Vertex {
    float3 position;
    float3 normal;
    float2 uv;
};

struct Out_Vertex {
    float4 position : SV_Position;
    float2 uv;
};

struct Out_Pixel {
    float4 color : SV_Target0;
};

Sampler2D S2D;

[shader("vertex")]
Out_Vertex main_vertex(In_Vertex in) {
    Out_Vertex out;
    
    out.position = mul(float4(in.position.xy, 0.0f, 1.0f), transform);
    out.uv = in.uv;

    return out;
}

[shader("pixel")]
Out_Pixel main_pixel(Out_Vertex in) {
    Out_Pixel out;

    float2 uv = in.uv;

    {   // Pixelate
        const float pixel_size = 1.0f;
        const float2 normalized_pixel_size = pixel_size / resolution;
        uv = normalized_pixel_size * floor(uv / normalized_pixel_size);
    }

    {   // Curve distortion
        float2 curve_uv = uv * 2.0f - 1.0f;
        const float2 offset = curve_uv.yx * curve_distortion_factor;
        
        curve_uv += curve_uv * offset * offset;
        curve_uv = curve_uv * 0.5f + 0.5f;
  
        uv = curve_uv;
    }
    
    float4 color = S2D.Sample(uv);

    {   // Chromatic aberration
        color.r = S2D.Sample(uv + float2(chromatic_aberration_offset, 0.0)).r;
        color.g = S2D.Sample(uv).g;
        color.b = S2D.Sample(uv - float2(chromatic_aberration_offset, 0.0)).b;
    }
    
    {   // Quantize color
        color.rgb = floor(color.rgb * (quantize_color_count - 1)) / (quantize_color_count - 1);
    }

    {   // Noise
        const float noise = fract(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
        color.rgb = lerp(color.rgb, color.rgb * noise, noise_blend_factor);
    }

    {   // Scanline
        const float scan = cos(uv.y * scanline_count * 3.14159);
        color.rgb = color.rgb * lerp(scanline_intensity, 1.0, step(0.0, scan));
    }
    
    out.color = color;
    //out.color = S2D.Sample(in.uv);
    
    return out;
}
