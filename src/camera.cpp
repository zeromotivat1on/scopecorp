#include "pch.h"
#include "camera.h"
#include "math/math.h"

mat4 camera_view(const Camera* c)
{
    return mat4_view(c->eye, c->at, c->up);
}

mat4 camera_projection(const Camera* c)
{
    if (c->mode == MODE_PERSPECTIVE)
        return mat4_perspective(rad(c->fov), c->aspect, c->near, c->far);

    if (c->mode == MODE_ORTHOGRAPHIC)
        return mat4_orthographic(c->left, c->right, c->bottom, c->top, c->near, c->far);

    assert(false); // unknown camera mode
    return mat4_identity();
}
