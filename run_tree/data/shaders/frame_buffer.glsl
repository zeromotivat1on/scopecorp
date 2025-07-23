#begin vertex
#version 460 core

layout (location = 0) in vec2 v_location;
layout (location = 1) in vec2 v_uv;

layout (location = 0) out vec2 f_uv;

void main() {
    gl_Position = vec4(v_location, 0.0f, 1.0f);
    f_uv = v_uv;
}
#end vertex

#begin fragment
#version 460 core

layout (location = 0) in vec2 f_uv;

layout (location = 0) out vec4 out_color;

uniform sampler2D u_sampler;
uniform vec2      u_resolution;
uniform float     u_pixel_size;
uniform float     u_curve_distortion_factor;
uniform float     u_chromatic_aberration_offset;
uniform uint      u_quantize_color_count;
uniform float     u_noise_blend_factor;
uniform uint      u_scanline_count;
uniform float     u_scanline_intensity;

void main() {
    vec2 uv = f_uv;

    {   // Pixelate
        const float pixel_size = 1.0f;
        const vec2 normalized_pixel_size = u_pixel_size / u_resolution;
        uv = normalized_pixel_size * floor(uv / normalized_pixel_size);
    }

    {   // Curve distortion
        const float curve_distortion_factor = 0.25f;

        vec2 curve_uv = uv * 2.0f - 1.0f;
        const vec2 offset = curve_uv.yx * u_curve_distortion_factor;
        
        curve_uv += curve_uv * offset * offset;
        curve_uv = curve_uv * 0.5f + 0.5f;
  
        uv = curve_uv;
    }
    
    vec4 color = texture(u_sampler, uv);

    {   // Chromatic aberration
        const float chromatic_aberration_offset = 0.002f;
        color.r = texture(u_sampler, uv + vec2(u_chromatic_aberration_offset, 0.0)).r;
        color.g = texture(u_sampler, uv).g;
        color.b = texture(u_sampler, uv - vec2(u_chromatic_aberration_offset, 0.0)).b;
    }
    
    {   // Quantize color
        const int quantize_color_count = 16;
        color.rgb = floor(color.rgb * (u_quantize_color_count - 1)) / (u_quantize_color_count - 1);
    }

    {   // Noise
        const float noise_blend_factor = 0.1f;
        const float noise = fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
        color.rgb = mix(color.rgb, color.rgb * noise, u_noise_blend_factor);
    }

    {   // Scanline
        const int scanline_count = 64;
        const float scanline_intensity = 0.95f;
        const float scan = cos(uv.y * u_scanline_count * 3.14159);
        color.rgb *= mix(u_scanline_intensity, 1.0, step(0.0, scan));
    }
    
    out_color = color;
    //out_color = texture(u_sampler, f_uv);
    //out_color = vec4(1, 0, 0, 1);
}
#end fragment
