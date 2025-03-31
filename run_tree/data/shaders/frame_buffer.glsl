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

const bool barrel_distortion    = false;
const bool chromatic_aberration = false;
const bool quantize_color       = false;
const bool noise_and_grain      = false;
const bool scanline             = false;

void main() {
    vec2 uv = f_uv;
    
    if (barrel_distortion) {
        const float distortion_factor = 0.4f;
        const vec2 centered = f_uv - 0.5;
        const float dist = dot(centered, centered);
        const vec2 distorted = centered * (1.0 + distortion_factor * dist);
        uv = distorted + 0.5;
    }
    
    vec4 color = texture(u_sampler, uv);

    if (chromatic_aberration) {
        const float offset = 0.004f;
        color.r = texture(u_sampler, uv + vec2(offset, 0.0)).r;
        color.g = texture(u_sampler, uv).g;
        color.b = texture(u_sampler, uv - vec2(offset, 0.0)).b;
    }
    
    if (quantize_color) {
        const int color_level_count = 8;
        color.rgb = floor(color.rgb * color_level_count) / color_level_count;
    }

    if (noise_and_grain) {
        const float blend_factor = 0.3f;
        const float noise = fract(sin(dot(uv, vec2(12.9898,78.233))) * 43758.5453);
        color.rgb = mix(color.rgb, color.rgb * noise, blend_factor);
    }

    if (scanline) {
        const int scanline_count = 16;
        const float scanline_intensity = 0.92f;
        const float scan = cos(uv.y * scanline_count * 3.14159);
        color.rgb *= mix(scanline_intensity, 1.0, step(0.0, scan));
    }
    
    out_color = color;
}
#end fragment
