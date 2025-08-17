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

    c.inv_view = inverse(c.view);
    c.inv_proj = inverse(c.proj);
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

vec2 world_to_screen(vec3 location, const Camera &camera, const Rect &rect) {
    const vec4 clip = vec4(location, 1.0f) * camera.proj * camera.view;
    const vec3 ndc = clip.xyz / clip.w;
    const vec2 pos = vec2((ndc.x + 1.0f) * 0.5f * rect.w,
                          (ndc.y + 1.0f) * 0.5f * rect.h);
    return pos;
}

vec3 screen_to_world(vec2 pos, const Camera &camera, const Rect &rect) {
    vec3 ndc;
    ndc.x = -1.0f + (2.0f * (pos.x - rect.x)) / rect.w;
    ndc.y = -1.0f + (2.0f * (pos.y - rect.y)) / rect.h;
    ndc.z = 1.0f;

    const vec4 clip = vec4(ndc.xy, -1.0f, 1.0f);
    vec4 eye = camera.inv_proj * clip;
    eye.z = -1.0f;
    eye.w =  0.0f;

    const vec4 location = camera.inv_view * eye;
    return location.xyz;
}
