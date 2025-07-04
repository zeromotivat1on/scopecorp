#include "pch.h"
#include "camera.h"
#include "log.h"

#include "math/math_core.h"
#include "math/matrix.h"

#include "render/viewport.h"

void update_matrices(Camera *c) {
    c->view = mat4_view(c->eye, c->at, c->up);

    if (c->mode == MODE_PERSPECTIVE)
		c->proj = mat4_perspective(rad(c->fov), c->aspect, c->near, c->far);
    
	if (c->mode == MODE_ORTHOGRAPHIC)
		c->proj = mat4_orthographic(c->left, c->right, c->bottom, c->top, c->near, c->far);

    c->view_proj = c->view * c->proj;
}

void on_viewport_resize(Camera *camera, const Viewport *viewport) {
	camera->aspect = (f32)viewport->width / viewport->height;
	camera->left = viewport->x;
	camera->right = (f32)viewport->x + viewport->width;
	camera->bottom = viewport->y;
	camera->top = (f32)viewport->y + viewport->height;
}
