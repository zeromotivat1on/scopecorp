#include "pch.h"
#include "camera.h"
#include "log.h"

#include "math/math_basic.h"
#include "math/matrix.h"

#include "render/r_viewport.h"

void update_matrices(Camera &c) {
    c.view = mat4_view(c.eye, c.at, c.up);

    if (c.mode == MODE_PERSPECTIVE) {
		c.proj = mat4_perspective(Rad(c.fov), c.aspect, c.near, c.far);
    } else if (c.mode == MODE_ORTHOGRAPHIC) {
		c.proj = mat4_orthographic(c.left, c.right, c.bottom, c.top, c.near, c.far);
    }
    
    c.view_proj = c.view * c.proj;
}

void on_viewport_resize(Camera &c, const R_Viewport &vp) {
	c.aspect = (f32)vp.width / vp.height;
	c.left   = vp.x;
	c.right  = (f32)vp.x + vp.width;
	c.bottom = vp.y;
	c.top    = (f32)vp.y + vp.height;
}

// @Todo: fix world_to_screen convertion.
#include "editor/editor.h"
vec2 world_to_screen(const Rect &rect, const mat4 &view_proj, vec3 loc, bool report) {
    const vec4 clip = view_proj * vec4(loc, 1.0f);

    if (Abs(clip.x) > clip.w || Abs(clip.y) > clip.w || Abs(clip.z) > clip.w) {
        return vec2_zero;
    }
    
    const vec3 ndc = clip.xyz / clip.w;
    const vec2 pos = vec2(rect.x + ((ndc.x + 1.0f) * 0.5f * rect.w),
                          rect.y + ((ndc.y + 1.0f) * 0.5f * rect.h));

    if (report) {
        editor_report("loc %s, clip %s, ndc %s, pos %s",
                      to_string(loc), to_string(clip), to_string(ndc), to_string(pos));
    }
    
    return pos;
}

vec3 screen_to_world(const Rect &rect, const mat4 &inv_view, const mat4 &inv_proj, vec2 pos) {
    vec3 ndc;
    ndc.x = -1.0f + (2.0f * (pos.x - rect.x)) / rect.w;
    ndc.y = -1.0f + (2.0f * (pos.y - rect.y)) / rect.h;
    ndc.z = 1.0f;

    const vec4 clip = vec4(ndc.xy, -1.0f, 1.0f);
    vec4 eye = inv_proj * clip;
    eye.z = -1.0f;
    eye.w =  0.0f;

    const vec4 location = inv_view * eye;
    return location.xyz;
}
