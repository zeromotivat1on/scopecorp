#include "pch.h"
#include "matrix.h"
#include "viewport.h"

void update_matrices(Camera &c) {
    c.view = make_view(c.position, c.look_at_position, c.up_vector);

    if (c.mode == PERSPECTIVE) {
		c.proj = make_perspective(To_Radians(c.fov), c.aspect, c.near_plane, c.far_plane);
    } else if (c.mode == ORTHOGRAPHIC) {
		c.proj = make_orthographic(c.left, c.right, c.bottom, c.top, c.near_plane, c.far_plane);
    }
    
    c.view_proj = c.view * c.proj;

    c.inv_view = inverse(c.view);
    c.inv_proj = inverse(c.proj);
}

void on_viewport_resize(Camera &c, const Viewport &vp) {
	c.aspect = (f32)vp.width / vp.height;
	c.left   = (f32)vp.x;
	c.right  = (f32)vp.x + vp.width;
	c.bottom = (f32)vp.y;
	c.top    = (f32)vp.y + vp.height;
}

Vector2 world_to_screen(Vector3 location, const Camera &camera, const Rect &rect) {
    const auto view_proj = camera.view_proj;

    const auto clip = Vector4(location, 1.0f) * view_proj;
    if (clip.w <= 0.0f) return Vector2(-F32_MAX, -F32_MAX);

    const auto ndc = Vector2(clip.x / clip.w, clip.y / clip.w);
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f)
        return Vector2(-F32_MAX, -F32_MAX);

    Vector2 pos;
    pos.x = rect.x + (ndc.x * 0.5f + 0.5f) * rect.w;
    pos.y = rect.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * rect.h;

    return pos;
}

Vector3 screen_to_world(Vector2 pos, const Camera &camera, const Rect &rect) {
    Vector3 ndc;
    ndc.x = -1.0f + (2.0f * (pos.x - rect.x)) / rect.w;
    ndc.y = -1.0f + (2.0f * (pos.y - rect.y)) / rect.h;
    ndc.z = 1.0f;

    const Vector4 clip = Vector4(ndc.xy, -1.0f, 1.0f);
    Vector4 eye = camera.inv_proj * clip;
    eye.z = -1.0f;
    eye.w =  0.0f;

    const Vector4 location = camera.inv_view * eye;
    return location.xyz;
}
