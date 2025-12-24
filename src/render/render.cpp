#include "pch.h"
#include "font.h"
#include "profile.h"
#include "render.h"
#include "ui.h"
#include "shader_globals.h"
#include "shader_binding_model.h"
#include "command_buffer.h"
#include "render_target.h"
#include "render_frame.h"
#include "render_batch.h"
#include "viewport.h"
#include "vertex_descriptor.h"
#include "debug_geometry.h"
#include "texture.h"
#include "shader.h"
#include "material.h"
#include "triangle_mesh.h"
#include "flip_book.h"
#include "editor.h"
#include "game.h"
#include "world.h"
#include "file_system.h"
#include "input.h"
#include "atomic.h"
#include "window.h"
#include "text_file.h"
#include "slang.h"
#include "slang-com-ptr.h"
#include <sstream>
#include <algorithm>

extern bool init_render_backend(Window *window);
extern bool post_init_render_backend();

void *gpu_allocator_proc(Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *allocator_data) {
    Assert(allocator_data);
    auto buffer = (Gpu_Buffer *)allocator_data;

    switch (mode) {
    case ALLOCATE: { return alloc(buffer, size); }
    case RESIZE:   { Assert(0, "Gpu allocator does not support memory resize"); }
    case FREE:     { Assert(0, "Gpu allocator does not support memory release"); }
    case FREE_ALL: { Assert(0, "Gpu allocator does not support all memory release"); }
    default:       { unreachable_code_path(); return null; }
    }
}

bool init_render_context(Window *window) {
    if (!init_render_backend(window)) return false;

    // @Cleanup @Speed: gpu data read is used extremely small portion of the buffer,
    // it would be nice to have separate general buffers (read/write only, read and write).
    constexpr u32 gpu_bits = GPU_DYNAMIC_BIT | GPU_READ_BIT | GPU_WRITE_BIT | GPU_PERSISTENT_BIT | GPU_COHERENT_BIT;
    
     gpu_main_buffer = New(Gpu_Buffer);
    *gpu_main_buffer = make_gpu_buffer(Megabytes(16), gpu_bits);

    map(gpu_main_buffer);
    if (!gpu_main_buffer->mapped_pointer) return false;

    gpu_allocator = { .proc = gpu_allocator_proc, .data = gpu_main_buffer };
    
    if (!post_init_render_backend()) return false;
    
    return true;
}

static Render_Frame *render_frame = null;

void init_render_frame() {
    auto frame = render_frame = New(Render_Frame);

    frame->opaque_batch      = make_render_batch(256);
    frame->transparent_batch = make_render_batch(128);
    frame->hud_batch         = make_render_batch(128);

    for (auto i = 0; i < RENDER_FRAMES_IN_FLIGHT; ++i) {
        frame->syncs           [i] = GPU_HANDLE_NONE;
        frame->command_buffers [i] = make_command_buffer (512,           context.allocator);
        frame->indirect_buffers[i] = make_indirect_buffer(Kilobytes(64), gpu_allocator);
    }

    // @Cleanup: align data here to ensure submit buffers alignment for cbuffers, which is
    // kinda annoying, maybe create separate gpu buffer for cbuffer data submit.
    gpu_main_buffer->used = Align(gpu_main_buffer->used, CBUFFER_OFFSET_ALIGNMENT);
    
    for (auto i = 0; i < RENDER_FRAMES_IN_FLIGHT; ++i) {
        frame->submit_buffers[i] = make_wrap_buffer(Kilobytes(32), gpu_allocator);
    }
}

u32              get_draw_call_count   () { return render_frame->draw_call_count; }
void             inc_draw_call_count   () { render_frame->draw_call_count += 1; }
void             reset_draw_call_count () { render_frame->draw_call_count  = 0; }
Render_Batch    *get_opaque_batch      () { return &render_frame->opaque_batch; }
Render_Batch    *get_transparent_batch () { return &render_frame->transparent_batch; }
Render_Batch    *get_hud_batch         () { return &render_frame->hud_batch; }
Gpu_Handle      *get_render_frame_sync () { return &render_frame->syncs[frame_index % RENDER_FRAMES_IN_FLIGHT]; }
Command_Buffer  *get_command_buffer    () { return &render_frame->command_buffers[frame_index % RENDER_FRAMES_IN_FLIGHT]; }
Indirect_Buffer *get_indirect_buffer   () { return &render_frame->indirect_buffers[frame_index % RENDER_FRAMES_IN_FLIGHT]; }
Wrap_Buffer     *get_submit_buffer     () { return &render_frame->submit_buffers[frame_index % RENDER_FRAMES_IN_FLIGHT]; }

void post_render_cleanup() {
    auto indirect = get_indirect_buffer();
    auto submit   = get_submit_buffer();
    
    reset(indirect);
    reset(submit);
    
    ui.line_render.line_count = 0;
    ui.quad_render.quad_count = 0;
    ui.text_render.char_count = 0;
}

Gpu_Buffer *get_gpu_buffer        () { Assert(gpu_main_buffer); return gpu_main_buffer; }
u64         get_gpu_buffer_mark   () { return get_gpu_buffer()->used; }
Gpu_Handle  get_gpu_buffer_handle () { return get_gpu_buffer()->handle; }

u64 get_gpu_buffer_offset(const void *p) {
    auto buffer = get_gpu_buffer();
    Assert(p >= buffer->mapped_pointer);
    return (u8 *)p - (u8 *)buffer->mapped_pointer;
}

void *get_gpu_buffer_data(u64 offset, u64 size) {
    auto buffer = get_gpu_buffer();
    Assert(offset + size <= buffer->used);
    return (u8 *)buffer->mapped_pointer + offset;
}

void render_one_frame() {
    Profile_Zone(__func__);

    auto frame_sync = get_render_frame_sync();
    if (is_valid(*frame_sync)) {
        auto res = wait_client_sync(*frame_sync, GPU_WAIT_INFINITE);
        if (res == GPU_WAIT_FAILED) {
            log(LOG_MINIMAL, "Failed to wait on render frame gpu sync 0x%X", frame_sync);
        }

        delete_gpu_sync(*frame_sync);
        *frame_sync = GPU_HANDLE_NONE;
    }
    
    auto window            = get_window();
    auto manager           = get_entity_manager();
    auto buf               = get_command_buffer();
    auto opaque_batch      = get_opaque_batch();
    auto transparent_batch = get_transparent_batch();
    auto hud_batch         = get_hud_batch();
    
    auto &viewport = screen_viewport;
    auto &target   = viewport.render_target;
    auto &geo      = line_geometry;
    
    {
        const auto &res    = Vector2(screen_viewport.width, screen_viewport.height);
        const auto &ortho  = screen_viewport.orthographic_projection;
        const auto &camera = manager->camera;
        
        set_constant_value(cv_viewport_cursor_pos, screen_viewport.mouse_pos);
        set_constant_value(cv_viewport_resolution, res);
        set_constant_value(cv_viewport_ortho,      ortho);
        set_constant_value(cv_camera_position,     camera.position);
        set_constant_value(cv_camera_view,         camera.view);
        set_constant_value(cv_camera_proj,         camera.proj);
        set_constant_value(cv_camera_view_proj,    camera.view_proj);
    }

    {
        auto direct_infos = Array <Direct_Light_Info> { .allocator = __temporary_allocator };
        auto point_infos  = Array <Point_Light_Info>  { .allocator = __temporary_allocator };

        For_Entities (manager->entities, E_DIRECT_LIGHT) {
            Direct_Light_Info info;
            info.direction = forward(it.orientation);
            info.ambient   = it.ambient_factor;
            info.diffuse   = it.diffuse_factor;
            info.specular  = it.specular_factor;

            array_add(direct_infos, info);
        }

        For_Entities (manager->entities, E_POINT_LIGHT) {
            Point_Light_Info info;
            info.position              = it.position;
            info.ambient               = it.ambient_factor;
            info.diffuse               = it.diffuse_factor;
            info.specular              = it.specular_factor;
            info.attenuation_constant  = it.attenuation_constant;
            info.attenuation_linear    = it.attenuation_linear;
            info.attenuation_quadratic = it.attenuation_quadratic;

            array_add(point_infos, info);
        }
        
        set_constant_value(cv_direct_light_count, direct_infos.count);
        set_constant_value(cv_point_light_count,  point_infos.count);
        set_constant_value(cv_direct_lights,      direct_infos.items, direct_infos.count * sizeof(direct_infos.items[0]));
        set_constant_value(cv_point_lights,       point_infos.items, point_infos.count * sizeof(point_infos.items[0]));
    }

    {
        const auto &target     = viewport.render_target;
        const auto &resolution = Vector2(target.width, target.height);

        set_constant_value(cv_fb_transform,                viewport.transform);
        set_constant_value(cv_resolution,                  resolution);
        set_constant_value(cv_pixel_size,                  viewport.pixel_size);
        set_constant_value(cv_curve_distortion_factor,     viewport.curve_distortion_factor);
        set_constant_value(cv_chromatic_aberration_offset, viewport.chromatic_aberration_offset);
        set_constant_value(cv_quantize_color_count,        viewport.quantize_color_count);
        set_constant_value(cv_noise_blend_factor,          viewport.noise_blend_factor);
        set_constant_value(cv_scanline_count,              viewport.scanline_count);
        set_constant_value(cv_scanline_intensity,          viewport.scanline_intensity);
    }

    {
        Profile_Zone("render_game_world");

        // @Cleanup: lazy draw everything in one loop for now.
        For (manager->entities) {
            render_entity(&it);
        }

#if DEVELOPER
        //ui_world_line(Vector3_zero, Vector3_right, rgba_yellow);
        
        if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
            const auto player = get_player(manager);
            
            const auto c  = player->position + Vector3(0.0f, player->scale.y * 0.5f, 0.0f);
            const auto vn = normalize(player->velocity);

            draw_arrow(c, c + vn * 0.5f, rgba_red);
        }
    
        if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
            constexpr u32 color_from_e_type[E_COUNT] = {
                rgba_black,
                rgba_red,
                rgba_cyan,
                rgba_red,
                rgba_white,
                rgba_white,
                rgba_blue,
                rgba_purple,
            };
            
            For (manager->entities) {
                if (it.bits & E_OVERLAP_BIT) {
                    draw_aabb(it.aabb, rgba_green);
                } else {
                    draw_aabb(it.aabb, color_from_e_type[it.type]);
                }
            }
        }
#endif
    }

#if DEVELOPER
    {
        Profile_Zone("render_dev_stuff");
        draw_dev_stats();
        update_console();
        draw(Memory_profiler);
    }
#endif

    {
        Profile_Zone("flush_game_world");
        
        set_render_target(buf, &viewport.render_target);
        set_polygon      (buf, game_state.polygon_mode);
        set_topology     (buf, TOPOLOGY_TRIANGLES);
        set_viewport     (buf, 0, 0, target.width, target.height);
        set_scissor      (buf, 0, 0, target.width, target.height);
        set_cull         (buf, CULL_BACK, WINDING_CCW);
        set_blend_func   (buf, BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
        set_depth_write  (buf, true);
        set_depth_func   (buf, DEPTH_LESS);
        set_stencil_mask (buf, 0x00);
        set_stencil_func (buf, STENCIL_ALWAYS, 1, 0xFF);
        set_stencil_op   (buf, STENCIL_KEEP, STENCIL_REPLACE, STENCIL_KEEP);
        clear            (buf, 1.0f, 1.0f, 1.0f, 1.0f, CLEAR_ALL_BITS);

        set_cbuffer_instance(buf, &cbi_global_parameters);
        set_cbuffer_instance(buf, &cbi_level_parameters);
        
        set_blend_write(buf, false);
        flush(opaque_batch);
        
        set_blend_write(buf, true);
        flush(transparent_batch);
    }
    
    if (geo.vertex_count > 0) {
        Profile_Zone("flush_line_geometry");
                
        static auto shader = get_shader(S("geometry"));
        
        set_topology         (buf, TOPOLOGY_LINES);
        set_shader           (buf, shader);
        set_vertex_descriptor(buf, &geo.vertex_descriptor);
        draw                 (buf, geo.vertex_count, 1, 0, 0);

        geo.vertex_count = 0;
    }

    {
        static auto shader = get_shader(S("frame_buffer"));
        static auto quad   = get_mesh(S("quad"));
        
        const auto attachment = viewport.render_target.color_attachments[0];

        set_render_target    (buf, &screen_render_target);
        set_topology         (buf, TOPOLOGY_TRIANGLES);
        set_polygon          (buf, POLYGON_FILL);
        set_viewport         (buf, viewport.x, viewport.y, viewport.width, viewport.height);
        set_scissor          (buf, viewport.x, viewport.y, viewport.width, viewport.height);
        clear                (buf, 0.0f, 0.0f, 0.0f, 1.0f, CLEAR_ALL_BITS);
        set_depth_write      (buf, false);
        set_shader           (buf, shader);
        set_vertex_descriptor(buf, &quad->vertex_descriptor);
        set_cbuffer_instance (buf, &cbi_frame_buffer_constants);
        // @Cleanup: 1 because cbuffer before sampler takes 0 in shader right now,
        // its hardcoded which is kinda bad.
        set_texture          (buf, 1, attachment);
        draw_indexed         (buf, quad->index_count, 1, quad->first_index, 0);

        if (0) {
            static auto cbi = make_constant_buffer_instance(cbi_frame_buffer_constants.constant_buffer);
            For (cbi_frame_buffer_constants.value_table) {
                set_constant(&cbi, it.value.constant->name, it.value.data, it.value.constant->size);
            }

            const auto &target     = viewport.render_target;
            const auto &resolution = Vector2(target.width, target.height);

            const auto scale = Vector3(0.51f);
            const auto pos   = Vector3(0.0f);
            const auto transform = make_transform(pos, {}, scale);
            set_constant(&cbi, S("transform"),  transform);
            set_constant(&cbi, S("resolution"), Vector2(resolution.x * scale.x, resolution.y * scale.y));

            const auto attachment = viewport.render_target.color_attachments[1];
            set_cbuffer_instance (buf, &cbi);
            // @Cleanup: 1 because cbuffer before sampler takes 0 in shader right now,
            // its hardcoded which is kinda bad.
            set_texture          (buf, 1, attachment);
            draw_indexed         (buf, quad->index_count, 1, quad->first_index, 0);
        }
    }

    {
        Profile_Zone("flush_hud");
                
        set_polygon         (buf, POLYGON_FILL);
        set_viewport        (buf, viewport.x, viewport.y, viewport.width, viewport.height);
        set_scissor         (buf, viewport.x, viewport.y, viewport.width, viewport.height);
        set_cull            (buf, CULL_BACK, WINDING_CCW);
        set_blend_write     (buf, true);
        set_blend_func      (buf, BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
        set_depth_write     (buf, false);
        set_cbuffer_instance(buf, &cbi_global_parameters);
        
        flush(hud_batch);
    }

    {
        Profile_Zone("flush_command_buffer");
        flush(buf);
    }

    *frame_sync = gpu_fence_sync();

    swap_buffers(window);
    post_render_cleanup();
}

void init(Viewport &viewport, u16 width, u16 height) {
    viewport.resolution_scale = 1.0f;
    viewport.quantize_color_count = 256;

    const Texture_Format_Type formats[] = { TEX_RGB_8 };
    viewport.render_target = make_render_target(width, height, formats, carray_count(formats), TEX_DEPTH_24_STENCIL_8);
    
#if 0
    viewport.pixel_size                  = 1.0f;
    viewport.curve_distortion_factor     = 0.25f;
    viewport.chromatic_aberration_offset = 0.002f;
    viewport.quantize_color_count        = 16;
    viewport.noise_blend_factor          = 0.1f;
    viewport.scanline_count              = 64;
    viewport.scanline_intensity          = 0.95f;
#endif

    resize(viewport, width, height);
}

void resize(Viewport &viewport, u16 width, u16 height) {
	switch (viewport.aspect_type) {
    case VIEWPORT_FILL_WINDOW:
        viewport.width = width;
		viewport.height = height;
        break;
	case VIEWPORT_4X3:
		viewport.width = width;
		viewport.height = height;

		if (width * 3 > height * 4) {
			viewport.width = height * 4 / 3;
			viewport.x = (width - viewport.width) / 2;
		} else {
			viewport.height = width * 3 / 4;
			viewport.y = (height - viewport.height) / 2;
		}

		break;
	default:
		log(LOG_MINIMAL, "Failed to resize viewport with unknown aspect type %d", viewport.aspect_type);
		break;
	}

    viewport.orthographic_projection = make_orthographic(0, viewport.width, 0, viewport.height, -1, 1);
    
    const u16 target_width  = (u16)((f32)viewport.width  * viewport.resolution_scale);
    const u16 target_height = (u16)((f32)viewport.height * viewport.resolution_scale);
    
    resize(viewport.render_target, target_width, target_height);
    log("Resized viewport 0x%X to %dx%d", &viewport, target_width, target_height);
}

static Render_Key get_entity_render_key(Entity *e) {
    Render_Key key;
    key.screen_layer = SCREEN_GAME_LAYER;

    const bool transparent = has_transparency(get_material(e->material));
    if (transparent) {
        key.translucency = NORM_TRANSLUCENT;
    } else {
        key.translucency = NOT_TRANSLUCENT;
    }
        
    if (e->type == E_SKYBOX) {
        // Draw skybox at the very end.
        key.depth = (1u << RENDER_KEY_DEPTH_BITS) - 1;
    } else {
        const auto manager = get_entity_manager();
        const auto &camera = manager->camera;
        const f32 dsqr = length_sqr(e->position - camera.position);
        const f32 norm = dsqr / (camera.far_plane * camera.far_plane);

        Assert(norm >= 0.0f);
        Assert(norm <= 1.0f);
        
        u32 bits = *(u32 *)&norm;
        if (transparent) bits = ~bits;
        
        key.depth = (bits >> 8);
    }
    
    return key;
}

void render_entity(Entity *e) {
    if (!e->mesh)     return;
    if (!e->material) return;

    auto mesh = get_mesh(e->mesh);
    if (!mesh) return;

    auto material = get_material(e->material);
    if (!material) return;
    
    auto manager = get_entity_manager();
    auto &camera = manager->camera;
    
    // @Todo: outline mouse picked entity as before.
    auto mouse_picked = e->bits & E_MOUSE_PICKED_BIT;

    auto shader     = material->shader;
    auto &cbi_table = material->cbi_table;
    auto &vertex_descriptor = mesh->vertex_descriptor;
    
    Render_Primitive prim;
    prim.topology_mode = TOPOLOGY_TRIANGLES;
    prim.vertex_descriptor = &vertex_descriptor;
    prim.shader = shader;

    if (material->diffuse_texture) {
        prim.texture = material->diffuse_texture;
    }

    prim.cbis.allocator = __temporary_allocator;
    array_realloc(prim.cbis, cbi_table.count);
    
    if (auto cbi = table_find(cbi_table, S("Entity_Constants"))) {
        Material_Info info;
        info.ambient   = material->ambient;
        info.diffuse   = material->diffuse;
        info.specular  = material->specular;
        info.shininess = material->shininess;
            
        set_constant(cbi, S("material"), info);

        array_add(prim.cbis, cbi);
    }
        
    if (auto cbi = table_find(cbi_table, S("Entity_Parameters"))) {
        set_constant(cbi, S("object_to_world"), e->object_to_world);
        set_constant(cbi, S("uv_scale"),        e->uv_scale);
        set_constant(cbi, S("uv_offset"),       e->uv_offset);

        array_add(prim.cbis, cbi);
    }
    
    if (mesh->index_count > 0) {
        prim.indexed = true;
        prim.first_element = mesh->first_index;
        prim.element_count = mesh->index_count;
    } else {
        prim.first_element = 0;
        prim.element_count = mesh->vertex_count;
    }

    auto submit_buffer = get_submit_buffer();
    prim.is_entity  = true;
    prim.eid_offset = get_gpu_buffer_offset(submit_buffer->data) + submit_buffer->used;

    auto gpu_eid = alloc(submit_buffer, sizeof(Pid));
    *(Pid *)gpu_eid = e->eid;

    auto render_batch = has_transparency(material) ? get_transparent_batch() : get_opaque_batch();
    add_primitive(render_batch, prim, get_entity_render_key(e));
    
    if (mouse_picked) {
        draw_cross(e->position, 0.5f);
    }
    
    /*
      if (e->flags & ENTITY_FLAG_SELECTED_IN_EDITOR) {
      command.flags &= ~RENDER_FLAG_CULL_FACE;
      command.flags |= RENDER_FLAG_STENCIL;
      command.stencil.operation.stencil_failed = R_KEEP;
      command.stencil.operation.depth_failed   = R_KEEP;
      command.stencil.operation.both_passed    = R_REPLACE;
      command.stencil.function.type       = R_ALWAYS;
      command.stencil.function.comparator = 1;
      command.stencil.function.mask       = 0xFF;
      command.stencil.mask = 0xFF;

      geo_draw_cross(e->position, 0.5f);
      }

    if (e->flags & ENTITY_FLAG_SELECTED_IN_EDITOR) { // draw outline
        command.flags &= ~RENDER_FLAG_DEPTH;
        command.flags &= ~RENDER_FLAG_CULL_FACE;
        command.stencil.function.type       = R_NEQUAL;
        command.stencil.function.comparator = 1;
        command.stencil.function.mask       = 0xFF;
        command.stencil.mask                = 0x00;
        
        const auto *camera = desired_camera(world);
        const u32 color = rgba_yellow;
        const mat4 mvp = mat4_transform(e->position, e->rotation, e->scale * 1.02f) * camera->view_proj;

        auto &material = asset_table.materials[SID_MATERIAL_OUTLINE];
        set_material_uniform_value(&material, "u_color",     &color, sizeof(color));
        set_material_uniform_value(&material, "u_transform", &mvp,   sizeof(mvp));

        command.sid_material = SID_MATERIAL_OUTLINE;
        
        r_enqueue(&entity_render_queue, &command);
    }
    */
}

void init_line_geometry() {
    auto &geo = line_geometry;
    
    const u64 position_buffer_offset = get_gpu_buffer_mark();
    geo.positions = Gpu_New(Vector3, geo.MAX_VERTICES);
    
    const u64 color_buffer_offset = get_gpu_buffer_mark();;
    geo.colors = Gpu_New(u32,  geo.MAX_VERTICES);
    
    geo.vertex_count = 0;

    const Vertex_Binding bindings[2] = {
        {
            .binding_index = 0,
            .offset = position_buffer_offset,
            .component_count = 1,
            .components = {
                { .type = VC_F32_3, .advance_rate = 0, .normalize = false }
            },
        },
        {
            .binding_index = 1,
            .offset = color_buffer_offset,
            .component_count = 1,
            .components = {
                { .type = VC_U32, .advance_rate = 0, .normalize = false }
            },
        },
    };
    
    geo.vertex_descriptor = make_vertex_descriptor(bindings, carray_count(bindings));;
}

void draw_line(Vector3 start, Vector3 end, u32 color) {
    auto &geo = line_geometry;
    if (geo.vertex_count + 2 > geo.MAX_VERTICES) {
        log(LOG_MINIMAL, "Can't draw more line geometry, limit of %d lines is reached", geo.MAX_LINES);
        return;
    }

    Vector3 *vl = geo.positions + geo.vertex_count;
    u32  *vc = geo.colors    + geo.vertex_count;
        
    vl[0] = start;
    vl[1] = end;
    vc[0] = color;
    vc[1] = color;

    geo.vertex_count += 2;
}

void draw_arrow(Vector3 start, Vector3 end, u32 color) {
    static const f32 size = 0.04f;
    static const f32 arrow_step = 30.0f; // in degrees
    static const f32 arrow_sin[45] = {
        0.0f, 0.5f, 0.866025f, 1.0f, 0.866025f, 0.5f, -0.0f, -0.5f, -0.866025f,
        -1.0f, -0.866025f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };
    static const f32 arrow_cos[45] = {
        1.0f, 0.866025f, 0.5f, -0.0f, -0.5f, -0.866026f, -1.0f, -0.866025f, -0.5f, 0.0f,
        0.5f, 0.866026f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
    };

    draw_line(start, end, color);

    Vector3 forward = normalize(end - start);
	const Vector3 right = Abs(forward.y) > 1.0f - F32_EPSILON
        ? normalize(cross(Vector3_right, forward))
        : normalize(cross(Vector3_up, forward));
	const Vector3 up = cross(forward, right);
    forward *= size;
    
    f32 degrees = 0.0f;
    for (int i = 0; degrees < 360.0f; degrees += arrow_step, ++i) {
        f32 scale;
        Vector3 v1, v2;

        scale = 0.5f * size * arrow_cos[i];
        v1 = (end - forward) + (right * scale);

        scale = 0.5f * size * arrow_sin[i];
        v1 += up * scale;

        scale = 0.5f * size * arrow_cos[i + 1];
        v2 = (end - forward) + (right * scale);

        scale = 0.5f * size * arrow_sin[i + 1];
        v2 += up * scale;

        draw_line(v1, end, color);
        draw_line(v1, v2,  color);
    }
}

void draw_cross(Vector3 position, f32 size) {
    draw_arrow(position, position + Vector3_up * size,      rgba_blue);
    draw_arrow(position, position + Vector3_right * size,   rgba_green);
    draw_arrow(position, position + Vector3_forward * size, rgba_red);
}

void draw_box(const Vector3 points[8], u32 color) {
    for (int i = 0; i < 4; ++i) {
        draw_line(points[i],     points[(i + 1) % 4],       color);
        draw_line(points[i + 4], points[((i + 1) % 4) + 4], color);
        draw_line(points[i],     points[i + 4],             color);
    }
}

void draw_aabb(const AABB &aabb, u32 color) {
    const Vector3 bb[2] = { aabb.c - aabb.r, aabb.c + aabb.r };
    Vector3 points[8];

    for (int i = 0; i < 8; ++i) {
        points[i].x = bb[(i ^ (i >> 1)) % 2].x;
        points[i].y = bb[(i >> 1) % 2].y;
        points[i].z = bb[(i >> 2) % 2].z;
    }

    draw_box(points, color);
}

void draw_ray(const Ray &ray, f32 length, u32 color) {
    const Vector3 start = ray.origin;
    const Vector3 end   = ray.origin + ray.direction * length;
    draw_line(start, end, color);
}

static void preload_mesh(const File_Callback_Data *data) {
    START_TIMER(0);

    const auto path = data->path;
    if (new_mesh(path)) {
        log("Created %S in %.2fms", path, CHECK_TIMER_MS(0));
    }
}

void preload_all_meshes() {
    SCOPE_TIMER("Preloaded all meshes in");
    
    constexpr String path = PATH_MESH("");
    visit_directory(path, preload_mesh);
}

Triangle_Mesh *new_mesh(String path) {
    auto contents = read_file(path, __temporary_allocator);
    if (!is_valid(contents)) return null;
    return new_mesh(path, contents);
}

struct Obj_Vertex_Key { s32 pos; s32 norm; s32 uv; };
bool operator==(const Obj_Vertex_Key& a, const Obj_Vertex_Key& b) {
    return a.pos == b.pos && a.norm == b.norm && a.uv == b.uv;
}

Triangle_Mesh *new_mesh(String path, Buffer contents) {
    constexpr String obj_ext = S("obj"); 
    
    path = copy_string(path);
    auto name = get_file_name_no_ext(path);
    auto ext  = get_extension(path);

    auto &tri_mesh = triangle_mesh_table[name];
    tri_mesh.path = path;
    tri_mesh.name = name;
    
    if (ext == obj_ext) {
        constexpr auto LOG_IDENT_TINYOBJ = S("tinyobj");
        const auto obj_data = std::string((const char *)contents.data, contents.size);

        std::istringstream stream(obj_data);
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warning;
        std::string err;
        tinyobj::MaterialFileReader mat_file_reader(PATH_MATERIAL("").data);
        
        const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &err, &stream, &mat_file_reader);

        if (!warning.empty()) {
            log(LOG_IDENT_TINYOBJ, LOG_VERBOSE, "%s", warning.c_str());
        }

        if (!err.empty()) {
            log(LOG_IDENT_TINYOBJ, LOG_MINIMAL, "%s", err.c_str());
            return null;
        }

        if (!ret) {
            log(LOG_MINIMAL, "Failed to load %S", path);
            return null;
        }

        tri_mesh.index_count = 0;
        For (shapes) {
            //auto &tri_shape = array_add(tri_mesh.shapes);
            For (it.mesh.num_face_vertices) {
                //tri_shape.vertex_count += it;
                tri_mesh.index_count += it;
            }
        }
        
        auto positions = Array <Vector3> { .allocator = __temporary_allocator };
        auto normals   = Array <Vector3> { .allocator = __temporary_allocator };
        auto uvs       = Array <Vector2> { .allocator = __temporary_allocator };
        auto indices   = Array <u32>  { .allocator = __temporary_allocator };

        array_realloc(positions, tri_mesh.index_count);
        array_realloc(normals,   tri_mesh.index_count);
        array_realloc(uvs,       tri_mesh.index_count);
        array_realloc(indices,   tri_mesh.index_count);

        auto vertex_table = Table <Obj_Vertex_Key, u32> { .allocator = __temporary_allocator };
        table_realloc (vertex_table, tri_mesh.index_count);
        table_set_hash(vertex_table, [](const Obj_Vertex_Key& k) -> u64 {
            // Simple hash combine.
            return (k.pos * 73856093) ^ (k.uv * 19349663) ^ (k.norm * 83492791);
        });
        
        For (shapes) {
            const auto &mesh = it.mesh;
            
            //auto &tri_shape = array_add(tri_mesh.shapes);
            //tri_shape.name = str_format(M_global, "%S%s", name, it.name.c_str());

            u32 index_offset = 0;
            For (mesh.num_face_vertices) {
                //tri_shape.vertex_count += it;
                
                for (u32 v = 0; v < it; ++v) {
                    const auto obj_index = mesh.indices[index_offset + v];
                    const auto key = Obj_Vertex_Key { obj_index.vertex_index,
                                                      obj_index.normal_index,
                                                      obj_index.texcoord_index };

                    u32 unified_index = 0;
                    auto *found_index = table_find(vertex_table, key);
                    
                    if (!found_index) {
                        unified_index = positions.count;

                        Vector3 pos = {
                            attrib.vertices[3 * obj_index.vertex_index + 0],
                            attrib.vertices[3 * obj_index.vertex_index + 1],
                            attrib.vertices[3 * obj_index.vertex_index + 2]
                        };

                        Vector3 norm = Vector3_zero;
                        if (obj_index.normal_index >= 0) {
                            norm = {
                                attrib.normals[3 * obj_index.normal_index + 0],
                                attrib.normals[3 * obj_index.normal_index + 1],
                                attrib.normals[3 * obj_index.normal_index + 2]
                            };
                        }

                        Vector2 uv = Vector2_zero;
                        if (obj_index.texcoord_index >= 0) {
                            uv = {
                                attrib.texcoords[2 * obj_index.texcoord_index + 0],
                                attrib.texcoords[2 * obj_index.texcoord_index + 1],
                            };
                        }

                        array_add(positions, pos);
                        array_add(normals,   norm);
                        array_add(uvs,       uv);

                        vertex_table[key] = unified_index;
                    } else {
                        unified_index = *found_index;
                    }

                    array_add(indices, unified_index);
                }

                index_offset += it;
            }
        }

        // Submit collected obj mesh data to gpu.

        const u32 positions_size = positions.count * sizeof(positions[0]);
        const u32 normals_size   = normals  .count * sizeof(normals  [0]);
        const u32 uvs_size       = uvs      .count * sizeof(uvs      [0]);

        const auto positions_offset = get_gpu_buffer_mark();
        auto gpu_positions = alloc(positions_size, gpu_allocator);

        const auto normals_offset = get_gpu_buffer_mark();
        auto gpu_normals = alloc(normals_size, gpu_allocator);

        const auto uvs_offset = get_gpu_buffer_mark();
        auto gpu_uvs = alloc(uvs_size, gpu_allocator);

        copy(gpu_positions, positions.items, positions_size);
        copy(gpu_normals, normals.items, normals_size);
        copy(gpu_uvs, uvs.items, uvs_size);
        
        const Vertex_Binding bindings[] = {
            {
                .binding_index = 0,
                .offset = positions_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_F32_3, .advance_rate = 0, .normalize = false },
                },
            },
            {
                .binding_index = 1,
                .offset = normals_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_F32_3, .advance_rate = 0, .normalize = false },
                },
            },
            {
                .binding_index = 2,
                .offset = uvs_offset,
                .component_count = 1,
                .components = {
                    { .type = VC_F32_2, .advance_rate = 0, .normalize = false },
                },
            },
        };
        
        tri_mesh.vertex_count = positions.count;
        tri_mesh.vertex_descriptor = make_vertex_descriptor(bindings, carray_count(bindings));

        const u64 index_offset = get_gpu_buffer_mark();
        const u32 indices_size = indices.count * sizeof(indices[0]);

        auto gpu_indices = alloc(indices_size, gpu_allocator);
        copy(gpu_indices, indices.items, indices_size);

        tri_mesh.first_index = (u32)(index_offset) / sizeof(u32);
    } else {
        table_remove(triangle_mesh_table, name);
        log(LOG_MINIMAL, "Unsupported mesh extension in %S", path);
        return null;
    }

    return &tri_mesh;
}

Triangle_Mesh *get_mesh(String name) {
    return table_find(triangle_mesh_table, name);
}

u16 get_vertex_component_dimension(Vertex_Component_Type type) {
    constexpr u16 lut[VC_COUNT] = { 0, 1, 1, 1, 2, 3, 4 };
    return lut[type];
}

u16 get_vertex_component_size(Vertex_Component_Type type) {
    constexpr u16 lut[VC_COUNT] = { 0, 4, 4, 4, 8, 12, 16 };
    return lut[type];
}

Render_Batch make_render_batch(u32 capacity, Allocator alc) {
    Render_Batch batch;
    batch.primitives = New(Render_Primitive,   capacity, alc);
    batch.entries    = New(Render_Batch_Entry, capacity, alc);
    batch.capacity   = capacity;
    return batch;
}

void flush(Render_Batch *batch) {
    std::stable_sort(batch->entries, batch->entries + batch->count);
    
    auto buf             = get_command_buffer();
    auto indirect        = get_indirect_buffer();
    auto indirect_offset = indirect->used;
            
    for (u32 i = 0; i < batch->count; ++i) {
        const auto prim = batch->entries[i].primitive;
        if (prim->indexed) {
            Assert(prim->instance_count != 0);
            
            Indirect_Draw_Indexed_Command cmd;
            cmd.index_count    = prim->element_count;
            cmd.instance_count = prim->instance_count;
            cmd.first_index    = prim->first_element;
            cmd.vertex_offset  = 0;
            cmd.first_instance = prim->first_instance;

            add_command(indirect, cmd);
        } else {
            Assert(prim->instance_count != 0);

            Indirect_Draw_Command cmd;
            cmd.vertex_count   = prim->element_count;
            cmd.instance_count = prim->instance_count;
            cmd.first_vertex   = prim->first_element;
            cmd.first_instance = prim->first_instance;

            add_command(indirect, cmd);
        }
    }

    u32 merge_count = 0;
    for (u32 i = 0; i < batch->count; i += merge_count) {
        merge_count = 0;
        for (u32 j = i; j < batch->count; ++j) {
            if (!can_be_merged(batch->entries[i], batch->entries[j])) break;
            merge_count += 1;
        }
        
        auto prim = batch->entries[i].primitive;
        
        // @Todo: textures should be done differently - they should be referenced by
        // passed "address" to shader and obtained from global sampler array or smth.
        if (prim->texture) {
            For (prim->shader->resource_table) {
                if (it.value.type == GPU_TEXTURE_2D) {
                    if (it.value.name == S("depth_buffer")) continue;
                    
                    // @Cleanup: find first available texture slot, it will cause
                    // bugs if shader has several texture slots.
                    set_texture(buf, it.value.binding, prim->texture);
                }
            }
        }

        set_topology         (buf, prim->topology_mode);
        set_shader           (buf, prim->shader);
        set_vertex_descriptor(buf, prim->vertex_descriptor);
            
        if (prim->is_entity) {
            // For entity render we need extra vertex binding for eid.
            auto binding = New(Vertex_Binding, 1, __temporary_allocator);

            binding->binding_index = prim->vertex_descriptor->binding_count;
            binding->offset = prim->eid_offset;
            binding->component_count = 1;
            binding->components[0] = { .type = VC_U32, .advance_rate = 99 };
            
            set_vertex_binding(buf, binding, prim->vertex_descriptor->handle);
        }

        For (prim->cbis) {
            set_cbuffer_instance(buf, it);
        }
        
        if (prim->indexed) {
            draw_indirect_indexed(buf, indirect, indirect_offset, merge_count, 0);
            indirect_offset += merge_count * sizeof(Indirect_Draw_Indexed_Command);
        } else {            
            draw_indirect(buf, indirect, indirect_offset, merge_count, 0);
            indirect_offset += merge_count * sizeof(Indirect_Draw_Command);
        };
    }
    
    batch->count = 0;
}

void add_primitive(Render_Batch *batch, const Render_Primitive &prim, Render_Key key) {
    Assert(batch->count < batch->capacity);

    batch->primitives[batch->count] = prim;
    batch->entries   [batch->count] = { .render_key = key, .primitive = batch->primitives + batch->count };

    batch->count += 1;
}

bool can_be_merged(const Render_Primitive &a, const Render_Primitive &b) {
    return a.shader            == b.shader
        && a.texture           == b.texture
        && a.vertex_descriptor == b.vertex_descriptor
        && a.topology_mode     == b.topology_mode
        && a.indexed           == b.indexed;
}

bool can_be_merged(const Render_Batch_Entry &a, const Render_Batch_Entry &b) {
    return can_be_merged(*a.primitive, *b.primitive);
}

Command_Buffer make_command_buffer(u32 capacity, Allocator alc) {
    Command_Buffer buffer;
    buffer.capacity = capacity;
    buffer.commands = New(Command, capacity, alc);
    
    return buffer;
}

void add_cmd(Command_Buffer *buf, const Command &cmd) {
    Assert(cmd.type != CMD_NONE);
    Assert(buf->count < buf->capacity);
    
    buf->commands[buf->count] = cmd;
    buf->count += 1;
}

void set_polygon(Command_Buffer *buf, Polygon_Mode mode) {
    // Unfortunately, C++ designated initializers can't even be used in a bit more
    // complicated scenario, so this snippet produces internal compiler error...
    //
    // add_cmd(buf, { .type = CMD_POLYGON, .polygon_mode = mode });

    Command cmd;
    cmd.type = CMD_POLYGON;
    cmd.polygon_mode = mode;
    
    add_cmd(buf, cmd);
}

void set_topology(Command_Buffer *buf, Topology_Mode mode) {
    Command cmd;
    cmd.type = CMD_TOPOLOGY;
    cmd.topology_mode = mode;
    
    add_cmd(buf, cmd);
}

void set_viewport(Command_Buffer *buf, s32 x, s32 y, u32 w, u32 h) {
    Command cmd;
    cmd.type = CMD_VIEWPORT;
    cmd.x = x;
    cmd.y = y;
    cmd.w = w;
    cmd.h = h;
    
    add_cmd(buf, cmd);
}

void set_scissor(Command_Buffer *buf, s32 x, s32 y, u32 w, u32 h) {
    Command cmd;
    cmd.type = CMD_SCISSOR;
    cmd.x = x;
    cmd.y = y;
    cmd.w = w;
    cmd.h = h;
    
    add_cmd(buf, cmd);
}

void set_cull(Command_Buffer *buf, Cull_Face face, Winding_Type winding) {
    Command cmd;
    cmd.type = CMD_CULL;
    cmd.cull_face = face;
    cmd.cull_winding = winding;
    
    add_cmd(buf, cmd);
}

void set_blend_write(Command_Buffer *buf, bool write) {
    Command cmd;
    cmd.type = CMD_BLEND_WRITE;
    cmd.blend_write = write;
    
    add_cmd(buf, cmd);
}

void set_blend_func(Command_Buffer *buf, Blend_Func src, Blend_Func dst) {
    Command cmd;
    cmd.type = CMD_BLEND_FUNC;
    cmd.blend_src = src;
    cmd.blend_dst = dst;
    
    add_cmd(buf, cmd);
}

void set_depth_write(Command_Buffer *buf, bool write) {
    Command cmd;
    cmd.type = CMD_DEPTH_WRITE;
    cmd.depth_write = write;
    
    add_cmd(buf, cmd);
}

void set_depth_func(Command_Buffer *buf, Depth_Func func) {
    Command cmd;
    cmd.type = CMD_DEPTH_FUNC;
    cmd.depth_func = func;
    
    add_cmd(buf, cmd);
}

void set_stencil_mask(Command_Buffer *buf, u32 mask) {
    Command cmd;
    cmd.type = CMD_STENCIL_MASK;
    cmd.stencil_mask = mask;
    
    add_cmd(buf, cmd);
}

void set_stencil_func(Command_Buffer *buf, Stencil_Func func, u32 cmp, u32 mask) {
    Command cmd;
    cmd.type = CMD_STENCIL_FUNC;
    cmd.stencil_func = func;
    cmd.stencil_func_cmp = cmp;
    cmd.stencil_func_mask = mask;
    
    add_cmd(buf, cmd);
}

void set_stencil_op(Command_Buffer *buf, Stencil_Func fail, Stencil_Func pass, Stencil_Func depth_fail) {
    Command cmd;
    cmd.type = CMD_STENCIL_OP;
    cmd.stencil_op_fail = fail;
    cmd.stencil_op_pass = pass;
    cmd.stencil_op_depth_fail = depth_fail;
    
    add_cmd(buf, cmd);
}

void clear(Command_Buffer *buf, f32 r, f32 g, f32 b, f32 a, u32 bits) {
    Command cmd;
    cmd.type = CMD_CLEAR;
    cmd.clear_r = r;
    cmd.clear_g = g;
    cmd.clear_b = b;
    cmd.clear_a = a;
    cmd.clear_bits = bits;
    
    add_cmd(buf, cmd);
}

void set_shader(Command_Buffer *buf, Shader *shader) {
    Command cmd;
    cmd.type = CMD_SHADER;
    cmd.shader = shader;
    
    add_cmd(buf, cmd);
}

void set_texture(Command_Buffer *buf, u32 binding, Texture *texture) {
    Command cmd;
    cmd.type = CMD_TEXTURE;
    cmd.texture = texture;
    cmd.texture_binding = binding;
    
    add_cmd(buf, cmd);
}

void set_vertex_descriptor(Command_Buffer *buf, Vertex_Descriptor *descriptor) {
    Command cmd;
    cmd.type = CMD_VERTEX_DESCRIPTOR;
    cmd.vertex_descriptor = descriptor;
    
    add_cmd(buf, cmd);
}

void set_vertex_binding(Command_Buffer *buf, Vertex_Binding *binding, Gpu_Handle descriptor_handle) {
    Command cmd;
    cmd.type = CMD_VERTEX_BINDING;
    cmd.vertex_binding = binding;
    cmd.vertex_descriptor_handle = descriptor_handle;
    
    add_cmd(buf, cmd);
}

void set_render_target(Command_Buffer *buf, Render_Target *target) {
    Command cmd;
    cmd.type = CMD_RENDER_TARGET;
    cmd.render_target = target;
    
    add_cmd(buf, cmd);
}

void set_cbuffer_instance(Command_Buffer *buf, Constant_Buffer_Instance *instance) {
    Command cmd;
    cmd.type = CMD_CBUFFER_INSTANCE;
    cmd.cbuffer_instance = instance;
    
    add_cmd(buf, cmd);    
}

void draw(Command_Buffer *buf, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
    Command cmd;
    cmd.type = CMD_DRAW;
    cmd.vertex_count = vertex_count;
    cmd.instance_count = instance_count;
    cmd.first_vertex = first_vertex;
    cmd.first_instance = first_instance;
    
    add_cmd(buf, cmd);
}

void draw_indexed(Command_Buffer *buf, u32 index_count, u32 instance_count, u32 first_index, u32 first_instance) {
    Command cmd;
    cmd.type = CMD_DRAW_INDEXED;
    cmd.index_count = index_count;
    cmd.instance_count = instance_count;
    cmd.first_index = first_index;
    cmd.first_instance = first_instance;
    
    add_cmd(buf, cmd);
}

void draw_indirect(Command_Buffer *buf, Indirect_Buffer *indirect, u32 offset, u32 count, u32 stride) {
    Command cmd;
    cmd.type = CMD_DRAW_INDIRECT;
    cmd.indirect_buffer = indirect;
    cmd.indirect_offset = offset;
    cmd.indirect_count  = count;
    cmd.indirect_stride = stride;
    
    add_cmd(buf, cmd);
}

void draw_indirect_indexed(Command_Buffer *buf, Indirect_Buffer *indirect, u32 offset, u32 count, u32 stride) {
    Command cmd;
    cmd.type = CMD_DRAW_INDEXED_INDIRECT;
    cmd.indirect_buffer = indirect;
    cmd.indirect_offset = offset;
    cmd.indirect_count  = count;
    cmd.indirect_stride = stride;
    
    add_cmd(buf, cmd);
}

void map(Gpu_Buffer *buffer) {
    // Map using the same bits that were used to create the buffer,
    // except for dynamic one which is buffer storage exclusive.
    const u32 bits = buffer->bits & ~GPU_DYNAMIC_BIT;
    map(buffer, buffer->size, bits);
}

void *alloc(Gpu_Buffer *buffer, u64 size) {
    Assert(buffer->mapped_pointer);
    Assert(buffer->used + size < buffer->size);
    auto data = (u8 *)buffer->mapped_pointer + buffer->used;
    buffer->used += size;
    return data;
}

Indirect_Buffer make_indirect_buffer(u32 size, Allocator alc) {
    Indirect_Buffer indirect;
    indirect.data = alloc(size, alc);
    indirect.size = size;
    indirect.used = 0;
    
    return indirect;
}

void reset(Indirect_Buffer *buffer) {
    buffer->used = 0;
}

void add_command(Indirect_Buffer *buffer, const Indirect_Draw_Command &cmd) {
    Assert(buffer->used + sizeof(cmd) < buffer->size);
    *(Indirect_Draw_Command *)((u8 *)buffer->data + buffer->used) = cmd;
    buffer->used += sizeof(cmd);
}

void add_command(Indirect_Buffer *buffer, const Indirect_Draw_Indexed_Command &cmd) {
    Assert(buffer->used + sizeof(cmd) < buffer->size);
    *(Indirect_Draw_Indexed_Command *)((u8 *)buffer->data + buffer->used) = cmd;
    buffer->used += sizeof(cmd);
}

// flip book

Flip_Book *new_flip_book(String path) {
    auto contents = read_text_file(path, __temporary_allocator);
    if (!contents) return null;
    return new_flip_book(path, contents);
}

Flip_Book *new_flip_book(String path, String contents) {
    Text_File_Handler text;
    defer { reset(&text); };
    init_from_memory(&text, contents);

    path = copy_string(path);
    auto name = get_file_name_no_ext(path);

    auto &book = flip_book_table[name];
    book.name = name;
    book.path = path;
    array_clear(book.frames);
    
    while (1) {
        auto line = read_line(&text);        
        if (!line) break;

        constexpr auto delims = S(" ");
        auto tokens = split(line, delims);
        Assert(tokens.count == 3, "Wrong flip book text data layout");

        constexpr auto frame_token = S("f");

        if (tokens[0] == frame_token) {
            auto frame_time   = string_to_float(tokens[1]);
            auto texture_name = get_file_name_no_ext(tokens[2]);

            Flip_Book_Frame frame;
            frame.texture    = get_texture(texture_name);
            frame.frame_time = (f32)frame_time;

            array_add(book.frames, frame);
        } else {
            log(LOG_MINIMAL, "First token is not known flip book token in %S", path);
        }
    }

    return &book;
}

Flip_Book *get_flip_book(String name) {
    return table_find(flip_book_table, name);
}

Flip_Book_Frame *get_current_frame(Flip_Book *flip_book) {
    return &flip_book->frames[flip_book->current_frame];
}

void update(Flip_Book *flip_book, f32 dt) {
    flip_book->frame_time += dt;

    auto frame = get_current_frame(flip_book);
    if (flip_book->frame_time >= frame->frame_time) {
        flip_book->current_frame = (flip_book->current_frame + 1) % flip_book->frames.count;
        flip_book->frame_time = 0.0f;
    }
}

// gpu resource

Gpu_Resource make_gpu_resource(String name, Gpu_Resource_Type type, u16 binding) {
    Gpu_Resource resource;
    resource.name    = name;
    resource.type    = type;
    resource.binding = binding;
    return resource;
}

// shader

static Slang::ComPtr<slang::IGlobalSession> slang_global_session;
static Slang::ComPtr<slang::ISession>       slang_local_session;

void init_shader_platform() {
    SCOPE_TIMER("Initialized shader platform in");

    auto &platform = shader_platform;
        
    table_realloc(platform.shader_file_table,     64);
    table_realloc(platform.shader_table,          64);
    table_realloc(platform.constant_buffer_table, 64);
    
    slang::createGlobalSession(slang_global_session.writeRef());

    init_slang_local_session();
}

void init_slang_local_session() {    
    slang::TargetDesc target_desc = {};
#if 1
    target_desc.format  = SLANG_GLSL;
    target_desc.profile = slang_global_session->findProfile("glsl_450");
#else
    target_desc.format  = SLANG_SPIRV;
    target_desc.profile = slang_global_session->findProfile("spirv_1_5");
#endif
    target_desc.flags = 0;

    constexpr String dir = PATH_SHADER("");

    char s_max_direct_lights[16];
    char s_max_point_lights [16];

    stbsp_snprintf(s_max_direct_lights, carray_count(s_max_direct_lights), "%u", MAX_DIRECT_LIGHTS);
    stbsp_snprintf(s_max_point_lights,  carray_count(s_max_point_lights),  "%u", MAX_POINT_LIGHTS);
    
    const slang::PreprocessorMacroDesc macros[] = {
        { "MAX_DIRECT_LIGHTS", s_max_direct_lights },
        { "MAX_POINT_LIGHTS",  s_max_point_lights  },
    };
    
    slang::SessionDesc session_desc = {};
    session_desc.targets = &target_desc;
    session_desc.targetCount = 1;
    session_desc.compilerOptionEntryCount = 0;
    session_desc.searchPaths = &dir.data;
    session_desc.searchPathCount = 1;
    session_desc.preprocessorMacros = macros;
    session_desc.preprocessorMacroCount = carray_count(macros);
    session_desc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_ROW_MAJOR;
    
    slang_global_session->createSession(session_desc, slang_local_session.writeRef());
}

Shader *new_shader(String path) {
    auto *shader_file = new_shader_file(path);
    if (!shader_file) return null;
    return new_shader(*shader_file);
}

Shader *new_shader(String path, String source) {
    auto *shader_file = new_shader_file(path, source);
    if (!shader_file) return null;
    return new_shader(*shader_file);
}

Shader *new_shader(Shader_File &shader_file) {
    if (shader_file.type != SHADER_SOURCE) return null;
    
    auto compiled_shader = compile_shader_file(shader_file);
    if (!is_valid(compiled_shader)) return null;

    return new_shader(compiled_shader);
}

Shader *get_shader(String name) {
    return table_find(shader_platform.shader_table, name);
}

static Shader_Type get_shader_type(SlangStage stage) {
    switch (stage) {
    case SLANG_STAGE_VERTEX:   return SHADER_VERTEX;
    case SLANG_STAGE_HULL:     return SHADER_HULL;
    case SLANG_STAGE_DOMAIN:   return SHADER_DOMAIN;
    case SLANG_STAGE_GEOMETRY: return SHADER_GEOMETRY;
    case SLANG_STAGE_FRAGMENT: return SHADER_PIXEL;
    case SLANG_STAGE_COMPUTE:  return SHADER_COMPUTE;
    default:
        log(LOG_MINIMAL, "Failed to get shader type from slang shader stage %d", stage);
        return SHADER_NONE;
    }
}

Shader_File *new_shader_file(String path) {
    const auto source = read_text_file(path);
    if (!is_valid(source)) {
        log(LOG_MINIMAL, "Failed to read shader file %S", path);
        return null;
    }

    return new_shader_file(path, source);
}

Shader_File *new_shader_file(String path, String source) {
    auto &platform = shader_platform;
    path = copy_string(path);
    
    auto &shader_file = platform.shader_file_table[path];

    if (is_shader_header_path(path)) {
        shader_file.type = SHADER_HEADER;
    } else if (is_shader_source_path(path)) {
        shader_file.type = SHADER_SOURCE;
    } else {
        table_remove(platform.shader_file_table, path);
        log(LOG_MINIMAL, "Invalid extension in shader file path %S", path);
        return null;
    }

    shader_file.path = copy_string(path);
    shader_file.name = get_file_name_no_ext(shader_file.path);

    const char *cpath = temp_c_string(path);
    Slang::ComPtr<slang::IBlob> diagnostics;
    auto *module = slang_loadModuleFromSource(slang_local_session, cpath, cpath, source.data, source.count, diagnostics.writeRef());

    if (diagnostics) {
        log(LOG_MINIMAL, "Slang: %s", (const char *)diagnostics->getBufferPointer());
    }

    if (!module) {
        table_remove(platform.shader_file_table, path);
        log(LOG_MINIMAL, "Failed to load slang shader module from %S", path);
        return null;
    }
    
    shader_file.module            = module;
    shader_file.entry_point_count = module->getDefinedEntryPointCount();
    shader_file.entry_points      = New(Shader_Entry_Point, shader_file.entry_point_count);
    
    for (u32 i = 0; i < shader_file.entry_point_count; ++i) {
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        module->getDefinedEntryPoint(i, entry_point.writeRef());

        slang::ProgramLayout        *layout           = entry_point->getLayout();
        slang::EntryPointReflection *entry_reflection = layout->getEntryPointByIndex(0);

        const char *name  = entry_reflection->getName();
        SlangStage  stage = entry_reflection->getStage();

        auto &shader_entry = shader_file.entry_points[i];
        shader_entry.name = copy_string(name);
        shader_entry.type = get_shader_type(stage);
    }
    
    return &shader_file;
}

static u16 infer_element_count(slang::TypeLayoutReflection *layout) {
    const auto kind = layout->getKind();
    switch (kind) {
    case slang::TypeReflection::Kind::Array: return (u16)layout->getElementCount();
    default:                                 return 1;
    }
}

static Constant_Type infer_constant_type(slang::TypeLayoutReflection *layout) {
    // @See slang::ScalarType.
    constexpr Constant_Type scalar_lut[] = {
        CT_ERROR, // None
        CT_VOID,  // Void
        CT_ERROR, // Bool
        CT_S32,   // Int32
        CT_ERROR, // UInt32
        CT_ERROR, // Int64
        CT_ERROR, // UInt64
        CT_ERROR, // Float16
        CT_F32,   // Float32
        CT_ERROR, // Float64
        CT_ERROR, // Int8
        CT_ERROR, // UInt8
        CT_ERROR, // Int16
        CT_ERROR, // UInt16
    };

    constexpr Constant_Type Vectortor_lut[] = {
        CT_ERROR, CT_ERROR, CT_VECTOR2, CT_VECTOR3, CT_VECTOR4
    };

    const auto kind = layout->getKind();
    switch (kind) {
    case slang::TypeReflection::Kind::Vector: {
        const auto scalar = layout->getElementTypeLayout()->getScalarType();
        const auto type   = scalar_lut[scalar];
        if (type == CT_ERROR) return CT_ERROR;
        return Vectortor_lut[layout->getElementCount()];
    }
    case slang::TypeReflection::Kind::Matrix: {
        const auto scalar = layout->getElementTypeLayout()->getScalarType();
        const auto type   = scalar_lut[scalar];
        if (type == CT_ERROR) return CT_ERROR;

        const auto rows = layout->getRowCount();
        const auto cols = layout->getColumnCount();

        if (rows == 4 && cols == 4) return CT_MAT4;
        
        return CT_ERROR;
    }
    case slang::TypeReflection::Kind::Array: {
        const auto element_layout = layout->getElementTypeLayout();
        return infer_constant_type(element_layout);
    }
    case slang::TypeReflection::Kind::Scalar: return scalar_lut[layout->getScalarType()];
    case slang::TypeReflection::Kind::Struct: return CT_CUSTOM;
    default:                                  return CT_ERROR;
    }
}

Compiled_Shader compile_shader_file(Shader_File &shader_file) {
    auto &platform = shader_platform;

    const auto &path = shader_file.path;
    auto module = shader_file.module;

    Compiled_Shader compiled_shader = {};
    compiled_shader.shader_file      = &shader_file;
    compiled_shader.compiled_sources = New(Buffer, shader_file.entry_point_count, __temporary_allocator);
    compiled_shader.resources.allocator = __temporary_allocator;

    array_realloc(compiled_shader.resources, 8);

    for (u32 i = 0; i < shader_file.entry_point_count; ++i) {
        const auto &shader_entry = shader_file.entry_points[i];
        const char *cname = temp_c_string(shader_entry.name);
        
        Slang::ComPtr<slang::IEntryPoint> entry_point;
        module->findEntryPointByName(cname, entry_point.writeRef());

        Assert(entry_point); // this entry point must exist

        slang::IComponentType *components[] = { module, entry_point };
        Slang::ComPtr<slang::IComponentType> program;
        slang_local_session->createCompositeComponentType(components, carray_count(components), program.writeRef());

        if (!program) {
            log(LOG_MINIMAL, "Failed to create shader program %s from %S", cname, path);
            return {};
        }

        Slang::ComPtr<slang::IBlob> diagnostics;
        Slang::ComPtr<slang::IComponentType> linked_program;
        program->link(linked_program.writeRef(), diagnostics.writeRef());

        if (diagnostics) {
            log(LOG_MINIMAL, "Slang: %s", (const char *)diagnostics->getBufferPointer());
        }

        if (!linked_program) {
            log(LOG_MINIMAL, "Failed to link shader program %s for %S", cname, path);
            return {};
        }
        
        // We do not need to collect shader reflection data from each entry point, as
        // global slang parameters are the same for all shader entries (at least for
        // vertex and pixel, thats what was actually checked), so we can collect data
        // during the very first iteration.
        if (i == 0) {
            auto program_layout = linked_program->getLayout();
            auto var_layout = program_layout->getGlobalParamsVarLayout();
            auto type_layout = var_layout->getTypeLayout();

            switch (type_layout->getKind()) {
            case slang::TypeReflection::Kind::Struct: {
                const s32 global_field_count = type_layout->getFieldCount();
                for (s32 i = 0; i < global_field_count; i++) {
                    auto field_var_layout  = type_layout->getFieldByIndex(i);   // global
                    auto field_type_layout = field_var_layout->getTypeLayout(); // global

                    auto element_var_layout  = field_type_layout->getElementVarLayout(); 
                    auto element_type_layout = element_var_layout->getTypeLayout();

                    const char *cname = field_var_layout->getName();
                    const auto kind   = field_type_layout->getKind();
                    const u32 binding = field_var_layout->getBindingIndex();
                    const u32 space   = field_var_layout->getBindingSpace();
                    const s32 field_count = element_type_layout->getFieldCount();

                    #if DEBUG_SHADER_REFLECTION
                    log("%s: kind %d, binding %d, space %d", cname, kind, binding, space);
                    #endif
                    
                    switch (kind) {
                    case slang::TypeReflection::Kind::ConstantBuffer: {
                        const u32 size = (u32)field_type_layout->getElementTypeLayout()->getSize();

                        auto name = copy_string(cname);
                        auto resource = make_gpu_resource(name, GPU_CONSTANT_BUFFER, binding);
                        array_add(compiled_shader.resources, resource);
                        
                        auto cb = new_constant_buffer(name, size, binding, field_count);
                        if (!cb) {
                            log(LOG_MINIMAL, "Failed to create constant buffer %S for shader %S", name, path);
                            continue;
                        }
                            
                        for (s32 i = 0; i < field_count; i++) {
                            auto var_layout  = element_type_layout->getFieldByIndex(i);
                            auto type_layout = var_layout->getTypeLayout();

                            const char *cname = var_layout->getName();
                            const auto name   = copy_string(cname);
                            const auto kind   = field_type_layout->getKind();
                            const u16 count   = (u16)infer_element_count(type_layout);
                            const u32 offset  = (u32)var_layout->getOffset();
                            const u32 size    = (u32)type_layout->getSize();

                            #if DEBUG_SHADER_REFLECTION
                            log(" %s: offset %u, size %u, align %u, count %u",
                                cname, offset, size, type_layout->getAlignment(), count);
                            #endif

                            auto type = infer_constant_type(type_layout);
                            add_constant(cb, name, type, count, offset, size);
                        }
                        break;
                    }
                    case slang::TypeReflection::Kind::Resource: {
                        auto shape       = field_type_layout->getResourceShape();
                        auto access      = field_type_layout->getResourceAccess();
                        auto result_type = field_type_layout->getResourceResultType();

                        #if DEBUG_SHADER_REFLECTION
                        log(" shape %d, access %d, result type %d", shape, access, result_type);
                        #endif
                        
                        auto name = copy_string(cname);
                        
                        switch (shape & SLANG_RESOURCE_BASE_SHAPE_MASK) {
                        case SLANG_TEXTURE_2D: {
                            auto resource = make_gpu_resource(name, GPU_TEXTURE_2D, binding);
                            array_add(compiled_shader.resources, resource);
                            break;
                        }
                        case SLANG_TEXTURE_2D_ARRAY: {
                            auto resource = make_gpu_resource(name, GPU_TEXTURE_2D_ARRAY, binding);
                            array_add(compiled_shader.resources, resource);
                            break;
                        }
                        case SLANG_BYTE_ADDRESS_BUFFER: {
                            auto resource = make_gpu_resource(name, GPU_MUTABLE_BUFFER, binding);
                            array_add(compiled_shader.resources, resource);
                            break;
                        }
                        }
                    }
                    }
                }
                
                break;
            }
            }
        }
        
        s32 entry_point_index = 0; // only one entry point
        s32 target_index      = 0; // only one target
        Slang::ComPtr<slang::IBlob> kernel_blob;
        linked_program->getEntryPointCode(entry_point_index, target_index,
                                          kernel_blob.writeRef(), diagnostics.writeRef());
    
        if (diagnostics) {
            log(LOG_MINIMAL, "Slang: %s", (const char *)diagnostics->getBufferPointer());
        }

        if (!kernel_blob) {
            log(LOG_MINIMAL, "Failed to get compiled shader code from %S", path);
            return {};
        }
    
        Buffer source;
        source.size = kernel_blob->getBufferSize();
        source.data = (u8 *)talloc(source.size);
        copy(source.data, kernel_blob->getBufferPointer(), source.size);

        compiled_shader.compiled_sources[i] = source;
    }

    return compiled_shader;
}

bool is_shader_source_path(String path) {
    const auto ext = slice(path, '.', S_SEARCH_REVERSE_BIT);
    if (!is_valid(ext)) {
        log(LOG_MINIMAL, "Failed to find shader extension from path %S", path);
        return false;
    }
    
    return ext == SHADER_SOURCE_EXT;
}

bool is_shader_header_path(String path) {
    const auto ext = slice(path, '.', S_SEARCH_REVERSE_BIT);
    if (!is_valid(ext)) {
        log(LOG_MINIMAL, "Failed to find shader extension from path %S", path);
        return false;
    }
    
    return ext == SHADER_HEADER_EXT;
}

bool is_valid(const Shader_File &shader_file) {
    return shader_file.module && is_valid(shader_file.path);
}

bool is_valid(const Compiled_Shader &compiled_shader) {
    const auto *shader_file = compiled_shader.shader_file;
    return shader_file && is_valid(*shader_file);
}

bool is_valid(const Shader &shader) {
    const auto *shader_file = shader.shader_file;
    return shader.linked_program != GPU_HANDLE_NONE && shader_file && is_valid(*shader_file);
}

Constant_Buffer *new_constant_buffer(String name, u32 size, u16 binding, u16 constant_count) {
    // @Todo: maybe it should be done automatically in backend implementation.
    // As we store all constant buffers in one big gpu buffer, we need to respect
    // provided alignment rules by a backend.
    size = Align(size, CBUFFER_OFFSET_ALIGNMENT);
    
    auto &platform = shader_platform;
    auto &cb = platform.constant_buffer_table[name];

    cb.name    = name;
    cb.binding = binding;
    cb.size    = size;

    cb.constant_table.allocator = context.allocator;
        
    table_clear   (cb.constant_table);
    table_realloc (cb.constant_table, constant_count);
    
    return &cb;
}

Constant_Buffer *get_constant_buffer(String name) {
    auto &platform = shader_platform;
    return table_find(platform.constant_buffer_table, name);
}

Constant *add_constant(Constant_Buffer *cb, String name, Constant_Type type, u16 count, u32 offset, u32 size) {
    Constant c;

    if (type == CT_ERROR) {
        log(LOG_MINIMAL, "Can't add constant %S to constant buffer %S (0x%X), it's type is CT_ERROR", name, cb->name, cb);
        return null;
    }
        
    if (type != CT_CUSTOM) {
        const u32 calc_size = c.type_sizes[type] * count; 
        if (size != calc_size) {
            log(LOG_MINIMAL, "Given constant %S size %u does not correspond to it's size %u, inferred from type %u and count %u",
                  name, size, calc_size, type, count);
            return null;
        }
    }
    
    c.name   = name;
    c.type   = type;
    c.count  = count;
    c.offset = offset;
    c.size   = size;

    return &table_add(cb->constant_table, name, c);
}

Constant_Buffer_Instance make_constant_buffer_instance(Constant_Buffer *cb) {
    auto &platform = shader_platform;

    Constant_Buffer_Instance cbi;
    cbi.constant_buffer = cb;
    cbi.value_table.allocator = context.allocator;
    
    table_clear   (cbi.value_table);
    table_realloc (cbi.value_table, cb->constant_table.count);
        
    For (cb->constant_table) {
        auto &c  = it.value;
        auto &cv = cbi.value_table[c.name];
        cv.constant = &c;
        cv.data     = alloc(c.size);
    }

    return cbi;
}

void set_constant(Constant_Buffer_Instance *cbi, String name, const void *data, u32 size) {
    auto cb = cbi->constant_buffer;
    auto cv = table_find(cbi->value_table, name);
    
    if (!cv) {
        log(LOG_MINIMAL, "Failed to find constant value %S in constant buffer instance %S 0x%X", name, cb->name, cbi);
        return;
    }

    set_constant_value(cv, data, size);
}

void set_constant_value(Constant_Value *cv, const void *data, u32 size) {
    Assert(size <= cv->constant->size);
    copy(cv->data, data, size);
}

// texture

static void preload_texture(const File_Callback_Data *data) {
    START_TIMER(0);

    const auto path = data->path;
    if (!new_texture(path)) return;
    
    log("Created %S in %.2fms", path, CHECK_TIMER_MS(0));
}

void preload_all_textures() {    
    SCOPE_TIMER("Preloaded all textures in");
    constexpr String path = PATH_TEXTURE("");
    visit_directory(path, preload_texture);

    global_textures.error = get_texture(S("error"));
}

Texture *new_texture(String path) {
    const auto loaded_texture = load_texture(path);
    if (!is_valid(loaded_texture)) return null;    
    return new_texture(loaded_texture);
}

Texture *new_texture(String path, Buffer contents) {
    const auto loaded_texture = load_texture(path, contents);
    if (!is_valid(loaded_texture)) return null;
    return new_texture(loaded_texture);
}

Texture *get_texture(String name) {
    return table_find(texture_table, name);
}

static Texture_Format_Type get_texture_format_from_channel_count(s32 channel_count) {
    switch (channel_count) {
    case 1: return TEX_RED_8;
    case 3: return TEX_RGB_8;
    case 4: return TEX_RGBA_8;
    default:
        log(LOG_MINIMAL, "Failed to get texture format from channel count %d", channel_count);
        return TEX_FORMAT_NONE;
    }
}

Loaded_Texture load_texture(String path) {
    auto contents = read_file(path, __temporary_allocator);
    if (!is_valid(contents)) return {};
    return load_texture(path, contents);
}

Loaded_Texture load_texture(String path, Buffer contents) {
    s32 width, height, channel_count;
    u8 *data = stbi_load_from_memory(contents.data, (s32)contents.size, &width, &height, &channel_count, 0);
    if (!data) {
        log(S("stbi"), LOG_MINIMAL, "%s", stbi_failure_reason());
    }
    
    Loaded_Texture tex;
    tex.path = path;
    tex.contents.data = data;
    tex.contents.size = width * height * channel_count;
    tex.width  = width;
    tex.height = height;
    tex.type   = TEX_2D; // @Todo: actually detect somehow
    tex.format = get_texture_format_from_channel_count(channel_count);

    return tex;
}

bool is_valid(const Loaded_Texture &loaded_texture) {
    return is_valid(loaded_texture.path)
        && is_valid(loaded_texture.contents)
        && loaded_texture.width > 0 && loaded_texture.height > 0
        && loaded_texture.type   != TEX_TYPE_NONE
        && loaded_texture.format != TEX_FORMAT_NONE;
}

bool is_valid(const Texture &texture) {
    // @Todo: check texture address.
    return is_valid(texture.path)
        && texture.handle != GPU_HANDLE_NONE
        && texture.width > 0 && texture.height > 0
        && texture.type   != TEX_TYPE_NONE
        && texture.format != TEX_FORMAT_NONE;
}

bool is_texture_path(String path) {
    const auto ext = get_extension(path);
    if (!ext) return false;

    for (s32 i = 0; i < carray_count(TEXTURE_EXTS); ++i) {
        if (ext == TEXTURE_EXTS[i]) return true;
    }
    
    return false;
}

// material

Material *new_material(String path) {
    auto contents = read_text_file(path, __temporary_allocator);
    if (!contents) return null;
    return new_material(path, contents);
}

Material *new_material(String path, String contents) {
    Text_File_Handler text;
    defer { reset(&text); };
    init_from_memory(&text, contents);

    if (get_extension(path) != MATERIAL_EXT) {
        log(LOG_MINIMAL, "Failed to create new material with unsupported extension %S", path);
        return null;
    }
    
    path = copy_string(path);
    auto name = get_file_name_no_ext(path);

    auto &material = material_table[name];
    material.name = name;
    material.path = path;

    table_clear  (material.cbi_table);
    table_realloc(material.cbi_table, 8);

    while (1) {
        auto line = read_line(&text);        
        if (!line) break;

        constexpr auto delims = S(" ");
        auto tokens = split(line, delims);

        constexpr auto shader_token           = S("s");
        constexpr auto color_token            = S("c");
        constexpr auto ambient_texture_token  = S("ta");
        constexpr auto diffuse_texture_token  = S("td");
        constexpr auto specular_texture_token = S("ts");
        constexpr auto ambient_light_token    = S("la");
        constexpr auto diffuse_light_token    = S("ld");
        constexpr auto specular_light_token   = S("ls");
        constexpr auto shininess_token        = S("shine");
        constexpr auto blend_token            = S("blend");

        if (tokens[0] == shader_token) {
            auto shader_name = get_file_name_no_ext(tokens[1]);
            material.shader = get_shader(shader_name);

            // @Cleanup: not sure whether its a good idea to create cbuffer instance for
            // each cbuffer in a shader, maybe we should add a lazy version that will create
            // instances when needed or something.
            For (material.shader->resource_table) {
                if (it.value.type == GPU_CONSTANT_BUFFER) {
                    auto cb = get_constant_buffer(it.key);
                    if (!cb) {
                        log(LOG_MINIMAL, "Failed to find cbuffer %S during new material %S creation", it.key, name);
                        continue;
                    }
                    
                    material.cbi_table[it.key] = make_constant_buffer_instance(cb);
                }
            }
        } else if (tokens[0] == color_token) {
            auto r = (f32)string_to_float(tokens[1]);
            auto g = (f32)string_to_float(tokens[2]);
            auto b = (f32)string_to_float(tokens[3]);
            auto a = (f32)string_to_float(tokens[3]);
            material.base_color = Vector4(r, g, b, a);
        } else if (tokens[0] == ambient_texture_token) {
            auto texture_name = get_file_name_no_ext(tokens[1]);
            material.ambient_texture = get_texture(texture_name);
        } else if (tokens[0] == diffuse_texture_token) {
            auto texture_name = get_file_name_no_ext(tokens[1]);
            material.diffuse_texture = get_texture(texture_name);
        } else if (tokens[0] == specular_texture_token) {
            auto texture_name = get_file_name_no_ext(tokens[1]);
            material.specular_texture = get_texture(texture_name);
        } else if (tokens[0] == ambient_light_token) {
            auto x = (f32)string_to_float(tokens[1]);
            auto y = (f32)string_to_float(tokens[2]);
            auto z = (f32)string_to_float(tokens[3]);
            material.ambient = Vector3(x, y, z);
        } else if (tokens[0] == diffuse_light_token) {
            auto x = (f32)string_to_float(tokens[1]);
            auto y = (f32)string_to_float(tokens[2]);
            auto z = (f32)string_to_float(tokens[3]);
            material.diffuse = Vector3(x, y, z);
        } else if (tokens[0] == specular_light_token) {
            auto x = (f32)string_to_float(tokens[1]);
            auto y = (f32)string_to_float(tokens[2]);
            auto z = (f32)string_to_float(tokens[3]);
            material.specular = Vector3(x, y, z);
        } else if (tokens[0] == shininess_token) {
            auto v = (f32)string_to_float(tokens[1]);
            material.shininess = v;
        } else if (tokens[0] == blend_token) {
            auto v = (bool)string_to_integer(tokens[1]);
            material.use_blending = v;
        } else {
            // @Hack: skip extra empty lines logging.
            if (tokens[0].data[0] != '\n' && tokens[0].data[0] != '\r') {
                log(LOG_MINIMAL, "First token %S is not known material token in %S", tokens[0], path);
            }
        }
    }

    return &material;
}

// Material *new_material(String name, Shader *shader) {
//     auto &material = material_table[name];
//     material.shader = shader;
//     material.name = copy_string(name);
//     material.cbi_table.allocator = context.allocator;
    
//     table_clear  (material.cbi_table);
//     table_realloc(material.cbi_table, 8);

//     if (shader) {
//         For (shader->constant_buffers) {
//             material.cbi_table[it->name] = make_constant_buffer_instance(it);
//         }
//     }
    
//     return &material;
// }

Material *get_material(String name) {
    return table_find(material_table, name);
}

bool has_transparency(const Material *material) {
    return material->use_blending
        || material->base_color.w < 1.0f;
}
