#include "pch.h"
#include "font.h"
#include "profile.h"
#include "render.h"
#include "ui.h"
#include "shader_globals.h"
#include "shader_binding_model.h"
#include "render_frame.h"
#include "render_batch.h"
#include "viewport.h"
#include "line_geometry.h"
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

bool init_render_context(Window *window) {
    if (!init_render_backend(window)) return false;

    gpu_init_backend();

    log(S("GPU"), "%S | %S | %S | %S", gpu.vendor, gpu.renderer, gpu.backend_version, gpu.shader_language_version);
    
    gpu_init_frontend();
    
    if (!post_init_render_backend()) return false;
    
    return true;
}

void gpu_init_frontend() {
    array_realloc(gpu.buffers,            64);
    array_realloc(gpu.images,             64);
    array_realloc(gpu.image_views,        64);
    array_realloc(gpu.samplers,           64);
    array_realloc(gpu.shaders,            64);
    array_realloc(gpu.framebuffers,       64);
    array_realloc(gpu.cmd_buffers,        64);
    array_realloc(gpu.vertex_inputs,      64);
    array_realloc(gpu.descriptors,        64);
    array_realloc(gpu.free_buffers,       16);
    array_realloc(gpu.free_images,        16);
    array_realloc(gpu.free_image_views,   16);
    array_realloc(gpu.free_samplers,      16);
    array_realloc(gpu.free_shaders,       16);
    array_realloc(gpu.free_framebuffers,  16);
    array_realloc(gpu.free_cmd_buffers,   16);
    array_realloc(gpu.free_vertex_inputs, 16);
    array_realloc(gpu.free_descriptors,   16);
    
    array_add(gpu.framebuffers,  {});
    array_add(gpu.cmd_buffers,   {});
    array_add(gpu.vertex_inputs, {});
    array_add(gpu.descriptors,   {});
    array_add(gpu.pipelines,     {});
    
    u8 pixels[] = { 0,0,0,       205,117,132, 0,0,
                    205,117,132, 0,0,0,       0,0,};
    const auto default_image_contents = make_buffer(pixels, sizeof(pixels));

    gpu.default_image      = gpu_new_image(GPU_IMAGE_TYPE_2D, GPU_IMAGE_FORMAT_RGB_8, 1, 2, 2, 1, default_image_contents);
    gpu.default_image_view = gpu_new_image_view(gpu.default_image, GPU_IMAGE_TYPE_2D, GPU_IMAGE_FORMAT_RGB_8, 0, 1, 0, 1);
     gpu.sampler_default_color = gpu_new_sampler(GPU_SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR,
                                                 GPU_SAMPLER_FILTER_NEAREST,
                                                 GPU_SAMPLER_WRAP_REPEAT,
                                                 GPU_SAMPLER_WRAP_REPEAT,
                                                 GPU_SAMPLER_WRAP_REPEAT,
                                                 GPU_SAMPLER_COMPARE_MODE_NONE,
                                                 GPU_SAMPLER_COMPARE_FUNCTION_NONE,
                                                 -1000.0f, 1000.0f, COLOR4F_BLACK);
    gpu.sampler_default_depth_stencil = gpu_new_sampler(GPU_SAMPLER_FILTER_NEAREST_MIPMAP_LINEAR,
                                                        GPU_SAMPLER_FILTER_NEAREST,
                                                        GPU_SAMPLER_WRAP_REPEAT,
                                                        GPU_SAMPLER_WRAP_REPEAT,
                                                        GPU_SAMPLER_WRAP_REPEAT,
                                                        GPU_SAMPLER_COMPARE_MODE_REF_TO_IMAGE,
                                                        GPU_SAMPLER_COMPARE_FUNCTION_LESS,
                                                        -1000.0f, 1000.0f, COLOR4F_BLACK);
    
    gpu_write_allocator.buffer = gpu_new_buffer(GPU_BUFFER_TYPE_STAGING_UNCACHED, Megabytes(8));
    gpu_read_allocator.buffer  = gpu_new_buffer(GPU_BUFFER_TYPE_STAGING_CACHED,   Megabytes(1));

    {
        Gpu_Vertex_Binding bindings[3];
        bindings[0].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
        bindings[0].index      = 0;
        bindings[0].stride     = 12;
        bindings[1].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
        bindings[1].index      = 1;
        bindings[1].stride     = 12;
        bindings[2].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
        bindings[2].index      = 2;
        bindings[2].stride     = 8;

        Gpu_Vertex_Attribute attributes[3];
        attributes[0].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V3;
        attributes[0].index   = 0;
        attributes[0].binding = 0;
        attributes[0].offset  = 0;
        attributes[1].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V3;
        attributes[1].index   = 1;
        attributes[1].binding = 1;
        attributes[1].offset  = 0;
        attributes[2].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V2;
        attributes[2].index   = 2;
        attributes[2].binding = 2;
        attributes[2].offset  = 0;
        
        gpu.vertex_input_entity = gpu_new_vertex_input(bindings, carray_count(bindings), attributes, carray_count(attributes));
    }
    
    {
        Gpu_Descriptor_Binding bindings[3];
        bindings[0].type  = GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER;
        bindings[0].index = 0;
        bindings[0].count = 1;
        bindings[1].type  = GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER;
        bindings[1].index = 1;
        bindings[1].count = 2;
        bindings[2].type  = GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER;
        bindings[2].index = 2;
        bindings[2].count = 1;
        
        gpu.descriptor_global = gpu_new_descriptor(bindings, carray_count(bindings));
    }

    {
        Gpu_Descriptor_Binding bindings[3];
        bindings[0].type  = GPU_DESCRIPTOR_BINDING_TYPE_UNIFORM_BUFFER;
        bindings[0].index = 3;
        bindings[0].count = 1;
        bindings[1].type  = GPU_DESCRIPTOR_BINDING_TYPE_SAMPLED_IMAGE;
        bindings[1].index = 4;
        bindings[1].count = 1;
        bindings[2].type  = GPU_DESCRIPTOR_BINDING_TYPE_STORAGE_BUFFER;
        bindings[2].index = 5;
        bindings[2].count = 1;
        
        gpu.descriptor_entity = gpu_new_descriptor(bindings, carray_count(bindings));
    }
}

Gpu_Allocation gpu_alloc(u64 size, Gpu_Allocator *alc) {
    auto &buffer = gpu.buffers[alc->buffer];
    Assert(alc->used + size <= buffer.size);
    
    Gpu_Allocation memory;
    memory.buffer      = alc->buffer;
    memory.size        = size;
    memory.used        = 0;
    memory.offset      = alc->used;
    memory.mapped_data = (u8 *)buffer.mapped_data + alc->used;

    alc->used += size;
    
    return memory;
}

Gpu_Allocation gpu_alloc(u64 size, u64 alignment, Gpu_Allocator *alc) {
    size = Align(size, alignment);
    return gpu_alloc(size, alc);
}

void gpu_release(Gpu_Allocation *memory, Gpu_Allocator *alc) {
    Assert(alc->used >= memory->size);
    Assert(memory->offset + memory->size == alc->used); // ensure it's last allocation
    
    alc->used -= memory->size;
    
    *memory = {};
}

void gpu_append(Gpu_Allocation *memory, const void *data, u64 size) {
    Assert(memory->used + size <= memory->size);

    auto gpu_data = (u8 *)memory->mapped_data + memory->used;
    copy(gpu_data, data, size);
    
    memory->used += size;
}

u32 gpu_max_mipmap_count(u32 width, u32 height) {
    return (u32)Floor(Log2((f32)Max(width, height))) + 1;
}

template <typename T>
static T *gpu_get_resource(u32 index, Array <T> &resources) {
    if (index >= resources.count) return &resources[0];
    return &resources[index];
}

Gpu_Buffer         *gpu_get_buffer         (u32 index) { return gpu_get_resource(index, gpu.buffers); }
Gpu_Image          *gpu_get_image          (u32 index) { return gpu_get_resource(index, gpu.images);}
Gpu_Image_View     *gpu_get_image_view     (u32 index) { return gpu_get_resource(index, gpu.image_views);}
Gpu_Sampler        *gpu_get_sampler        (u32 index) { return gpu_get_resource(index, gpu.samplers); }
Gpu_Shader         *gpu_get_shader         (u32 index) { return gpu_get_resource(index, gpu.shaders); }
Gpu_Framebuffer    *gpu_get_framebuffer    (u32 index) { return gpu_get_resource(index, gpu.framebuffers); }
Gpu_Command_Buffer *gpu_get_command_buffer (u32 index) { return gpu_get_resource(index, gpu.cmd_buffers); }
Gpu_Vertex_Input   *gpu_get_vertex_input   (u32 index) { return gpu_get_resource(index, gpu.vertex_inputs); }
Gpu_Descriptor     *gpu_get_descriptor     (u32 index) { return gpu_get_resource(index, gpu.descriptors); }
Gpu_Pipeline       *gpu_get_pipeline       (u32 index) { return gpu_get_resource(index, gpu.pipelines); }

static Render_Frame *render_frame = null;

void init_render_frame() {
    auto frame = render_frame = New(Render_Frame);

    frame->opaque_batch      = make_render_batch(256);
    frame->transparent_batch = make_render_batch(128);
    frame->hud_batch         = make_render_batch(4096);

    for (auto i = 0; i < RENDER_FRAMES_IN_FLIGHT; ++i) {
        frame->syncs           [i] = {};
        frame->command_buffers [i] = gpu_new_command_buffer(512);
        frame->gpu_indirect_allocations[i] = gpu_alloc(Kilobytes(64), &gpu_write_allocator);
    }

    // @Cleanup: align data here to ensure submit buffers alignment for cbuffers, which is
    // kinda annoying, maybe create separate gpu buffer for cbuffer data submit.
    auto write_buffer = gpu_get_buffer(gpu_write_allocator.buffer);
    write_buffer->used = Align(write_buffer->used, (u64)gpu_uniform_buffer_offset_alignment());
    
    for (auto i = 0; i < RENDER_FRAMES_IN_FLIGHT; ++i) {
        frame->gpu_submit_allocations[i] = gpu_alloc(Kilobytes(32), &gpu_write_allocator);
    }
}

u32              get_draw_call_count   () { return render_frame->draw_call_count; }
void             inc_draw_call_count   () { render_frame->draw_call_count += 1; }
void             reset_draw_call_count () { render_frame->draw_call_count  = 0; }
Render_Batch    *get_opaque_batch      () { return &render_frame->opaque_batch; }
Render_Batch    *get_transparent_batch () { return &render_frame->transparent_batch; }
Render_Batch    *get_hud_batch         () { return &render_frame->hud_batch; }
Handle          *get_render_frame_sync () { return &render_frame->syncs[frame_index % RENDER_FRAMES_IN_FLIGHT]; }
u32             get_command_buffer     () { return render_frame->command_buffers[frame_index % RENDER_FRAMES_IN_FLIGHT]; }
Gpu_Allocation *get_gpu_indirect_allocation () { return &render_frame->gpu_indirect_allocations[frame_index % RENDER_FRAMES_IN_FLIGHT]; }
Gpu_Allocation *get_gpu_submit_allocation   () { return &render_frame->gpu_submit_allocations[frame_index % RENDER_FRAMES_IN_FLIGHT]; }

void post_render_cleanup() {
    auto indirect = get_gpu_indirect_allocation();
    auto submit   = get_gpu_submit_allocation();
    
    indirect->used = 0;
    submit->used   = 0;
    
    ui.line_render.line_count = 0;
    ui.quad_render.quad_count = 0;
    ui.text_render.char_count = 0;
}

void render_one_frame() {
    Profile_Zone(__func__);

    auto frame_sync = get_render_frame_sync();
    if (frame_sync && *frame_sync) {
        auto res = wait_client_sync(*frame_sync, GPU_WAIT_INFINITE);
        if (res == GPU_SYNC_RESULT_WAIT_FAILED) {
            log(LOG_ERROR, "Failed to wait on render frame gpu sync 0x%X", frame_sync);
        }

        delete_gpu_sync(*frame_sync);
        *frame_sync = {};
    }
    
    auto window            = get_window();
    auto manager           = get_entity_manager();
    auto buf               = get_command_buffer();
    auto opaque_batch      = get_opaque_batch();
    auto transparent_batch = get_transparent_batch();
    auto hud_batch         = get_hud_batch();
    
    auto &viewport = screen_viewport;
    auto &geo      = line_geometry;
    
    {
        const auto &res    = Vector2((f32)screen_viewport.width, (f32)screen_viewport.height);
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
        const auto framebuffer = gpu_get_framebuffer(viewport.framebuffer);
        const auto image_view  = gpu_get_image_view(framebuffer->color_attachments[0]);
        const auto image       = gpu_get_image(image_view->image);
        const auto &resolution = Vector2((f32)image->width, (f32)image->height);

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

            draw_arrow(c, c + vn * 0.5f, COLOR32_RED);
        }
    
        if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
            constexpr Color32 color_from_e_type[E_COUNT] = {
                COLOR32_BLACK,
                COLOR32_RED,
                COLOR32_CYAN,
                COLOR32_RED,
                COLOR32_WHITE,
                COLOR32_WHITE,
                COLOR32_BLUE,
                COLOR32_PURPLE,
            };
            
            For (manager->entities) {
                if (it.bits & E_OVERLAP_BIT) {
                    draw_aabb(it.aabb, COLOR32_GREEN);
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
    }
#endif

    {
        Profile_Zone("flush_game_world");

        const auto framebuffer = gpu_get_framebuffer(viewport.framebuffer);
        const auto image_view  = gpu_get_image_view(framebuffer->color_attachments[0]);
        const auto image       = gpu_get_image(image_view->image);
        
        gpu_cmd_framebuffer (buf, viewport.framebuffer);
        gpu_cmd_polygon     (buf, game_state.polygon_mode);
        gpu_cmd_viewport    (buf, 0, 0, image->width, image->height);
        gpu_cmd_scissor     (buf, 0, 0, image->width, image->height);
        gpu_cmd_cull_face   (buf, GPU_CULL_FACE_BACK);
        gpu_cmd_winding     (buf, GPU_WINDING_COUNTER_CLOCKWISE);
        gpu_cmd_blend_func  (buf, GPU_BLEND_FUNCTION_SRC_ALPHA, GPU_BLEND_FUNCTION_ONE_MINUS_SRC_ALPHA);
        gpu_cmd_depth_write (buf, true);
        gpu_cmd_depth_func  (buf, GPU_DEPTH_FUNCTION_LESS);
        gpu_cmd_stencil_mask(buf, 0x00);
        gpu_cmd_stencil_func(buf, GPU_STENCIL_FUNCTION_ALWAYS, 1, 0xFF);
        gpu_cmd_stencil_op  (buf, GPU_STENCIL_FUNCTION_KEEP, GPU_STENCIL_FUNCTION_REPLACE, GPU_STENCIL_FUNCTION_KEEP);
        // @Temp: clear only color and depth, as no stencil buffer usage for now.
        gpu_cmd_scissor_test(buf, false);
        gpu_cmd_clear       (buf, COLOR4F_WHITE, GPU_CLEAR_COLOR_AND_DEPTH_BITS);
        gpu_cmd_scissor_test(buf, true);
        
        gpu_cmd_cbuffer_instance(buf, &cbi_global_parameters);
        gpu_cmd_cbuffer_instance(buf, &cbi_level_parameters);
        
        gpu_cmd_blend_test(buf, false);
        flush(opaque_batch);
        
        gpu_cmd_blend_test(buf, true);
        flush(transparent_batch);
    }
    
    if (geo.vertex_count > 0) {
        Profile_Zone("flush_line_geometry");
                
        static auto shader = get_shader(S("geometry"));
        
        gpu_cmd_shader       (buf, shader);
        gpu_cmd_vertex_input (buf, geo.vertex_input);
        gpu_cmd_vertex_buffer(buf, gpu_write_allocator.buffer, 0, geo.positions_offset, 12);
        gpu_cmd_vertex_buffer(buf, gpu_write_allocator.buffer, 1, geo.colors_offset, 4);
        gpu_cmd_index_buffer (buf, gpu_write_allocator.buffer);
        gpu_cmd_draw         (buf, GPU_TOPOLOGY_LINES, geo.vertex_count, 1, 0, 0);

        geo.vertex_count = 0;
    }

    {
        static auto shader = get_shader(S("frame_buffer"));
        static auto quad   = get_mesh(S("quad"));

        const auto framebuffer = gpu_get_framebuffer(viewport.framebuffer);
        
        gpu_cmd_framebuffer      (buf, gpu.default_framebuffer);
        gpu_cmd_polygon          (buf, GPU_POLYGON_FILL);
        gpu_cmd_viewport         (buf, viewport.x, viewport.y, viewport.width, viewport.height);
        gpu_cmd_scissor          (buf, viewport.x, viewport.y, viewport.width, viewport.height);
        // @Temp: clear only color and depth, as no stencil buffer usage for now.
        gpu_cmd_scissor_test     (buf, false);
        gpu_cmd_clear            (buf, COLOR4F_BLACK, GPU_CLEAR_COLOR_AND_DEPTH_BITS);
        gpu_cmd_scissor_test     (buf, true);
        gpu_cmd_depth_write      (buf, false);
        gpu_cmd_shader           (buf, shader);
        gpu_cmd_vertex_input     (buf, quad->vertex_input);

        // @Cleanup
        const auto vertex_input = gpu_get_vertex_input(quad->vertex_input);
        for (u32 i = 0; i < vertex_input->binding_count; ++i) {
            const auto &binding = vertex_input->bindings[i];
            const auto offset = quad->vertex_offsets[i];
            
            gpu_cmd_vertex_buffer(buf, gpu_write_allocator.buffer, binding.index, offset, binding.stride);
            gpu_cmd_index_buffer(buf, gpu_write_allocator.buffer);
        }
            
        gpu_cmd_cbuffer_instance (buf, &cbi_frame_buffer_constants);
        // @Cleanup: 1 because cbuffer before sampler takes 0 in shader right now,
        // its hardcoded which is kinda bad.
        gpu_cmd_image_view       (buf, 1, framebuffer->color_attachments[0]);
        gpu_cmd_sampler          (buf, 1, gpu.sampler_default_color);
        gpu_cmd_draw_indexed     (buf, GPU_TOPOLOGY_TRIANGLES, quad->index_count, 1, quad->first_index, 0);

        if (0) {
            static auto cbi = make_constant_buffer_instance(cbi_frame_buffer_constants.constant_buffer);
            For (cbi_frame_buffer_constants.value_table) {
                set_constant(&cbi, it.value.constant->name, it.value.data, it.value.constant->size);
            }

            //const auto &target     = viewport.render_target;
            //const auto &resolution = Vector2(target.width, target.height);

            const auto scale = Vector3(0.51f);
            const auto pos   = Vector3(0.0f);
            const auto transform = make_transform(pos, {}, scale);
            set_constant(&cbi, S("transform"),  transform);
            //set_constant(&cbi, S("resolution"), Vector2(resolution.x * scale.x, resolution.y * scale.y));

            //const auto attachment = viewport.render_target.color_attachments[1];
            gpu_cmd_cbuffer_instance(buf, &cbi);
            // @Cleanup: 1 because cbuffer before sampler takes 0 in shader right now,
            // its hardcoded which is kinda bad.
            //set_texture         (buf, 1, attachment);
            gpu_cmd_draw_indexed        (buf, GPU_TOPOLOGY_TRIANGLES, quad->index_count, 1, quad->first_index, 0);
        }
    }

    {
        Profile_Zone("flush_hud");
                
        gpu_cmd_polygon         (buf, GPU_POLYGON_FILL);
        gpu_cmd_viewport        (buf, viewport.x, viewport.y, viewport.width, viewport.height);
        gpu_cmd_scissor         (buf, viewport.x, viewport.y, viewport.width, viewport.height);
        gpu_cmd_cull_face       (buf, GPU_CULL_FACE_BACK);
        gpu_cmd_winding         (buf, GPU_WINDING_COUNTER_CLOCKWISE);
        gpu_cmd_blend_test      (buf, true);
        gpu_cmd_blend_func      (buf, GPU_BLEND_FUNCTION_SRC_ALPHA, GPU_BLEND_FUNCTION_ONE_MINUS_SRC_ALPHA);
        gpu_cmd_depth_write     (buf, false);
        gpu_cmd_cbuffer_instance(buf, &cbi_global_parameters);
        
        flush(hud_batch);
    }

    {
        Profile_Zone("flush_command_buffer");
        gpu_flush_cmd_buffer(buf);
    }

    *frame_sync = gpu_fence_sync();

    swap_buffers(window);
    post_render_cleanup();
}

void init(Viewport &viewport, u32 width, u32 height) {
    viewport.resolution_scale = 1.0f;
    viewport.quantize_color_count = 256;
    
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

void resize(Viewport &viewport, u32 width, u32 height) {
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
		log(LOG_ERROR, "Failed to resize viewport with unknown aspect type %d", viewport.aspect_type);
		break;
	}

    viewport.orthographic_projection = make_orthographic(0, (f32)viewport.width, 0, (f32)viewport.height, -1, 1);
    
    const auto internal_width  = (u32)((f32)viewport.width  * viewport.resolution_scale);
    const auto internal_height = (u32)((f32)viewport.height * viewport.resolution_scale);

    if (viewport.framebuffer) gpu_delete_framebuffer(viewport.framebuffer);
    
    const Gpu_Image_Format color_formats[] = { GPU_IMAGE_FORMAT_RGB_8 };
    viewport.framebuffer = gpu_new_framebuffer(color_formats, carray_count(color_formats),
                                               internal_width, internal_height,
                                               GPU_IMAGE_FORMAT_DEPTH_24_STENCIL_8,
                                               internal_width, internal_height);
    
    log("Resized viewport 0x%X to %dx%d", &viewport, internal_width, internal_height);
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
    
    Render_Primitive prim;
    prim.topology = GPU_TOPOLOGY_TRIANGLES;
    prim.vertex_input = mesh->vertex_input;
    prim.vertex_offsets = mesh->vertex_offsets;
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

    auto submit = get_gpu_submit_allocation();
    prim.is_entity  = true;
    prim.eid_offset = submit->offset + submit->used;
    gpu_append(submit, e->eid);

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
        const Color32 color = rgba_yellow;
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

    auto gpu_allocation = gpu_alloc(geo.MAX_VERTICES * sizeof(Vector3), &gpu_write_allocator);
    const auto position_buffer_offset = gpu_allocation.offset;
    geo.positions_offset = gpu_allocation.offset;
    geo.positions = (Vector3 *)gpu_allocation.mapped_data;

    gpu_allocation = gpu_alloc(geo.MAX_VERTICES * sizeof(u32), &gpu_write_allocator);
    const auto color_buffer_offset = gpu_allocation.offset;
    geo.colors_offset = gpu_allocation.offset;
    geo.colors = (Color32 *)gpu_allocation.mapped_data;
    
    geo.vertex_count = 0;

    Gpu_Vertex_Binding bindings[2];
    bindings[0].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
    bindings[0].index      = 0;
    bindings[0].stride     = 12;
    bindings[1].input_rate = GPU_VERTEX_INPUT_RATE_VERTEX;
    bindings[1].index      = 1;
    bindings[1].stride     = 4;

    Gpu_Vertex_Attribute attributes[2];
    attributes[0].type    = GPU_VERTEX_ATTRIBUTE_TYPE_V3;
    attributes[0].index   = 0;
    attributes[0].binding = 0;
    attributes[0].offset  = 0;
    attributes[1].type    = GPU_VERTEX_ATTRIBUTE_TYPE_U32;
    attributes[1].index   = 1;
    attributes[1].binding = 1;
    attributes[1].offset  = 0;
    
    geo.vertex_input = gpu_new_vertex_input(bindings, carray_count(bindings), attributes, carray_count(attributes));
}

void draw_line(Vector3 start, Vector3 end, Color32 color) {
    auto &geo = line_geometry;
    if (geo.vertex_count + 2 > geo.MAX_VERTICES) {
        log(LOG_ERROR, "Can't draw more line geometry, limit of %d lines is reached", geo.MAX_LINES);
        return;
    }

    auto vl = geo.positions + geo.vertex_count;
    auto vc = geo.colors    + geo.vertex_count;
        
    vl[0] = start;
    vl[1] = end;
    vc[0] = color;
    vc[1] = color;

    geo.vertex_count += 2;
}

void draw_arrow(Vector3 start, Vector3 end, Color32 color) {
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
    draw_arrow(position, position + Vector3_up * size,      COLOR32_BLUE);
    draw_arrow(position, position + Vector3_right * size,   COLOR32_GREEN);
    draw_arrow(position, position + Vector3_forward * size, COLOR32_RED);
}

void draw_box(const Vector3 points[8], Color32 color) {
    for (int i = 0; i < 4; ++i) {
        draw_line(points[i],     points[(i + 1) % 4],       color);
        draw_line(points[i + 4], points[((i + 1) % 4) + 4], color);
        draw_line(points[i],     points[i + 4],             color);
    }
}

void draw_aabb(const AABB &aabb, Color32 color) {
    const Vector3 bb[2] = { aabb.c - aabb.r, aabb.c + aabb.r };
    Vector3 points[8];

    for (int i = 0; i < 8; ++i) {
        points[i].x = bb[(i ^ (i >> 1)) % 2].x;
        points[i].y = bb[(i >> 1) % 2].y;
        points[i].z = bb[(i >> 2) % 2].z;
    }

    draw_box(points, color);
}

void draw_ray(const Ray &ray, f32 length, Color32 color) {
    const Vector3 start = ray.origin;
    const Vector3 end   = ray.origin + ray.direction * length;
    draw_line(start, end, color);
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
#define USE_TINYOBJLOADER 1
    
    const auto obj_ext = S("obj"); 
    
    path = copy_string(path);
    auto name = get_file_name_no_ext(path);
    auto ext  = get_extension(path);

    auto &tri_mesh = mesh_table[name];
    tri_mesh.path = path;
    tri_mesh.name = name;
    
    if (ext == obj_ext) {
#if USE_TINYOBJLOADER
        const auto LOG_IDENT_TINYOBJ = S("tinyobj");
        const auto obj_data = std::string((const char *)contents.data, contents.size);

        std::istringstream stream(obj_data);
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warning;
        std::string err;
        tinyobj::MaterialFileReader mat_file_reader((char *)PATH_MATERIAL("").data);

        const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &err, &stream, &mat_file_reader);
        
        if (!warning.empty()) {
            log(LOG_IDENT_TINYOBJ, LOG_WARNING, "%s", warning.c_str());
        }

        if (!err.empty()) {
            log(LOG_IDENT_TINYOBJ, LOG_ERROR, "%s", err.c_str());
            return &tri_mesh;
        }

        if (!ret) {
            log(LOG_ERROR, "Failed to load %S", path);
            return &tri_mesh;
        }

        tri_mesh.index_count = 0;
        For (shapes) {
            //auto &tri_shape = array_add(tri_mesh.shapes);
            For (it.mesh.num_face_vertices) {
                //tri_shape.vertex_count += it;
                tri_mesh.index_count += it;
            }
        }
#else
        const auto mark = get_temporary_storage_mark();
        defer { set_temporary_storage_mark(mark); };
        
        const auto obj = parse_obj_file(path, make_string(contents), __temporary_allocator);

        For (obj.faces) {
            const auto face_index_count = get_obj_face_index_count(it);
            tri_mesh.index_count += face_index_count;
        }
#endif
        
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

#if USE_TINYOBJLOADER
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
#else
        For (obj.faces) {
            const auto face_index_count = get_obj_face_index_count(it);

            for (u32 i = 0; i < face_index_count; ++i) {
                const auto position_index = it.position_indices[i];
                const auto texcoord_index = it.texcoord_indices[i];
                const auto normal_index   = it.normal_indices[i];
                
                const auto key = Obj_Vertex_Key { (s32)position_index, (s32)normal_index, (s32)texcoord_index };

                u32 unified_index = 0;
                auto *found_index = table_find(vertex_table, key);
                    
                if (!found_index) {
                    unified_index = positions.count;

                    const auto position = obj.positions[position_index];
                    const auto texcoord = obj.texcoords[texcoord_index];
                    const auto normal   = obj.normals[normal_index];

                    array_add(positions, position.xyz);
                    array_add(uvs,       texcoord.xy);
                    array_add(normals,   normal);
                    
                    vertex_table[key] = unified_index;
                } else {
                    unified_index = *found_index;
                }

                array_add(indices, unified_index);
            }
        }
#endif
        // Submit collected obj mesh data to gpu.

        const u32 positions_size = positions.count * sizeof(positions[0]);
        const u32 normals_size   = normals  .count * sizeof(normals  [0]);
        const u32 uvs_size       = uvs      .count * sizeof(uvs      [0]);

        auto gpu_allocation = gpu_alloc(positions_size, &gpu_write_allocator);
        const auto positions_offset = gpu_allocation.offset;
        auto gpu_positions = gpu_allocation.mapped_data;
        
        gpu_allocation = gpu_alloc(normals_size, &gpu_write_allocator);
        const auto normals_offset = gpu_allocation.offset;
        auto gpu_normals = gpu_allocation.mapped_data;

        gpu_allocation = gpu_alloc(uvs_size, &gpu_write_allocator);
        const auto uvs_offset = gpu_allocation.offset;
        auto gpu_uvs = gpu_allocation.mapped_data;

        copy(gpu_positions, positions.items, positions_size);
        copy(gpu_normals, normals.items, normals_size);
        copy(gpu_uvs, uvs.items, uvs_size);

        tri_mesh.vertex_input = gpu.vertex_input_entity;
        tri_mesh.vertex_offsets    = New(u64, 3);
        tri_mesh.vertex_offsets[0] = positions_offset;
        tri_mesh.vertex_offsets[1] = normals_offset;
        tri_mesh.vertex_offsets[2] = uvs_offset;
        
        tri_mesh.vertex_count = positions.count;

        gpu_allocation = gpu_alloc(indices.count * sizeof(indices[0]), &gpu_write_allocator);
        const auto index_offset = gpu_allocation.offset;
        const auto indices_size = indices.count * sizeof(indices[0]);

        auto gpu_indices = gpu_allocation.mapped_data;
        copy(gpu_indices, indices.items, indices_size);

        tri_mesh.index_offset = index_offset;
        tri_mesh.first_index  = (u32)(index_offset) / sizeof(u32);
    } else {
        log(LOG_ERROR, "Unsupported mesh extension in %S", path);
    }

    return &tri_mesh;
}

Triangle_Mesh *get_mesh(String name) {
    auto mesh = table_find(mesh_table, name);
    if (mesh) return mesh;

    log(LOG_ERROR, "Failed to find mesh %S", name);
    return global_meshes.missing;
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
    auto indirect        = get_gpu_indirect_allocation();
    auto indirect_offset = (u32)(indirect->offset + indirect->used);
            
    for (u32 i = 0; i < batch->count; ++i) {
        const auto prim = batch->entries[i].primitive;
        if (prim->indexed) {
            Assert(prim->instance_count);
            
            Gpu_Indirect_Draw_Indexed_Command cmd;
            cmd.index_count    = prim->element_count;
            cmd.instance_count = prim->instance_count;
            cmd.first_index    = prim->first_element;
            cmd.vertex_offset  = 0;
            cmd.first_instance = prim->first_instance;

            gpu_append(indirect, cmd);
        } else {
            Assert(prim->instance_count);

            Gpu_Indirect_Draw_Command cmd;
            cmd.vertex_count   = prim->element_count;
            cmd.instance_count = prim->instance_count;
            cmd.first_vertex   = prim->first_element;
            cmd.first_instance = prim->first_instance;

            gpu_append(indirect, cmd);
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
                    gpu_cmd_image_view(buf, it.value.binding, prim->texture->image_view);
                    gpu_cmd_sampler   (buf, it.value.binding, prim->texture->sampler);
                }
            }
        }

        gpu_cmd_shader      (buf, prim->shader);
        gpu_cmd_vertex_input(buf, prim->vertex_input);
        
        // @Cleanup
        const auto vertex_input = gpu_get_vertex_input(prim->vertex_input);
        for (u32 j = 0; j < vertex_input->binding_count; ++j) {
            const auto &binding = vertex_input->bindings[j];
            const auto offset = prim->vertex_offsets[j];
            
            gpu_cmd_vertex_buffer(buf, gpu_write_allocator.buffer, binding.index, offset, binding.stride);
            gpu_cmd_index_buffer (buf, gpu_write_allocator.buffer);
        }
        
        if (prim->is_entity) {
            // @Todo
            // For entity render we need extra vertex binding for eid.
            // auto binding = New(Vertex_Binding, 1, __temporary_allocator);

            // binding->binding_index = prim->vertex_descriptor->binding_count;
            // binding->offset = prim->eid_offset;
            // binding->component_count = 1;
            // binding->components[0] = { .type = VC_U32, .advance_rate = 1 };
            
            // gpu_cmd_vertex_binding(buf, binding, prim->vertex_descriptor->handle);
        }

        For (prim->cbis) {
            gpu_cmd_cbuffer_instance(buf, it);
        }
        
        if (prim->indexed) {
            gpu_cmd_draw_indirect_indexed(buf, prim->topology, indirect->mapped_data, indirect_offset, merge_count, 0);
            indirect_offset += merge_count * sizeof(Gpu_Indirect_Draw_Indexed_Command);
        } else {            
            gpu_cmd_draw_indirect(buf, prim->topology, indirect->mapped_data, indirect_offset, merge_count, 0);
            indirect_offset += merge_count * sizeof(Gpu_Indirect_Draw_Command);
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
        && a.vertex_input      == b.vertex_input
        && a.topology          == b.topology
        && a.indexed           == b.indexed
        // @Cleanup: this one should not be here, implement correct draw of merged primitives.
        && a.vertex_offsets[0] == b.vertex_offsets[0];
}

bool can_be_merged(const Render_Batch_Entry &a, const Render_Batch_Entry &b) {
    return can_be_merged(*a.primitive, *b.primitive);
}

void gpu_add_cmds(u32 cmd_buffer, Gpu_Command *cmds, u32 count) {    
    auto buffer = gpu_get_command_buffer(cmd_buffer);
    Assert(buffer->count + count <= buffer->capacity);
    
    copy(buffer->commands + buffer->count, cmds, count * sizeof(cmds[0]));
    buffer->count += count;
}

void gpu_cmd_polygon(u32 cmd_buffer, Gpu_Polygon_Mode mode) {
    // Unfortunately, C++ designated initializers can't even be used in a bit more
    // complicated scenario, so this snippet produces internal compiler error...
    //
    // add_cmd(buf, { .type = CMD_POLYGON, .polygon_mode = mode });

    Gpu_Command cmd;
    cmd.type    = GPU_CMD_POLYGON;
    cmd.polygon = mode;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_viewport(u32 cmd_buffer, s32 x, s32 y, u32 w, u32 h) {
    Gpu_Command cmd;
    cmd.type = GPU_CMD_VIEWPORT;
    cmd.x    = x;
    cmd.y    = y;
    cmd.w    = w;
    cmd.h    = h;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_scissor(u32 cmd_buffer, s32 x, s32 y, u32 w, u32 h) {
    Gpu_Command cmd;
    cmd.type = GPU_CMD_SCISSOR;
    cmd.x    = x;
    cmd.y    = y;
    cmd.w    = w;
    cmd.h    = h;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_cull_face(u32 cmd_buffer, Gpu_Cull_Face face) {
    Gpu_Command cmd;
    cmd.type      = GPU_CMD_CULL_FACE;
    cmd.cull_face = face;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_winding(u32 cmd_buffer, Gpu_Winding_Type winding) {
    Gpu_Command cmd;
    cmd.type    = GPU_CMD_WINDING;
    cmd.winding = winding;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_scissor_test(u32 cmd_buffer, bool test) {
    Gpu_Command cmd;
    cmd.type         = GPU_CMD_SCISSOR_TEST;
    cmd.scissor_test = test;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_blend_test(u32 cmd_buffer, bool test) {
    Gpu_Command cmd;
    cmd.type       = GPU_CMD_BLEND_TEST;
    cmd.blend_test = test;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_blend_func(u32 cmd_buffer, Gpu_Blend_Function src, Gpu_Blend_Function dst) {
    Gpu_Command cmd;
    cmd.type      = GPU_CMD_BLEND_FUNC;
    cmd.blend_src = src;
    cmd.blend_dst = dst;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_depth_test(u32 cmd_buffer, bool test) {
    Gpu_Command cmd;
    cmd.type       = GPU_CMD_DEPTH_TEST;
    cmd.depth_test = test;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_depth_write(u32 cmd_buffer, bool write) {
    Gpu_Command cmd;
    cmd.type        = GPU_CMD_DEPTH_WRITE;
    cmd.depth_write = write;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_depth_func(u32 cmd_buffer, Gpu_Depth_Function func) {
    Gpu_Command cmd;
    cmd.type       = GPU_CMD_DEPTH_FUNC;
    cmd.depth_func = func;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_stencil_mask(u32 cmd_buffer, u32 mask) {
    Gpu_Command cmd;
    cmd.type                = GPU_CMD_STENCIL_MASK;
    cmd.stencil_global_mask = mask;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_stencil_func(u32 cmd_buffer, Gpu_Stencil_Function func, u32 cmp, u32 mask) {
    Gpu_Command cmd;
    cmd.type         = GPU_CMD_STENCIL_FUNC;
    cmd.stencil_test = func;
    cmd.stencil_ref  = cmp;
    cmd.stencil_mask = mask;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_stencil_op(u32 cmd_buffer, Gpu_Stencil_Function fail, Gpu_Stencil_Function pass, Gpu_Stencil_Function depth_fail) {
    Gpu_Command cmd;
    cmd.type                = GPU_CMD_STENCIL_OP;
    cmd.stencil_fail        = fail;
    cmd.stencil_pass        = pass;
    cmd.stencil_depth_fail = depth_fail;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_clear(u32 cmd_buffer, Color4f color, u32 bits) {
    Gpu_Command cmd;
    cmd.type        = GPU_CMD_CLEAR;
    cmd.clear_color = color;
    cmd.clear_bits  = bits;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_shader(u32 cmd_buffer, Shader *shader) {
    Gpu_Command cmd;
    cmd.type            = GPU_CMD_SHADER;
    cmd.resource_handle = shader->linked_program;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

// void gpu_cmd_shader(u32 cmd_buffer, u32 shader) {
//     Gpu_Command cmd;
//     cmd.type = GPU_CMD_SHADER;
//     cmd.bind_resource = shader;
    
//     gpu_add_cmds(cmd_buffer, &cmd, 1);
// }

void gpu_cmd_image_view(u32 cmd_buffer, u32 binding, u32 image_view) {
    Gpu_Command cmd;
    cmd.type          = GPU_CMD_IMAGE_VIEW;
    cmd.bind_index    = binding;
    cmd.bind_resource = image_view;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_sampler(u32 cmd_buffer, u32 binding, u32 sampler) {
    Gpu_Command cmd;
    cmd.type          = GPU_CMD_SAMPLER;
    cmd.bind_index    = binding;
    cmd.bind_resource = sampler;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_vertex_input(u32 cmd_buffer, u32 vertex_input) {
    Gpu_Command cmd;
    cmd.type          = GPU_CMD_VERTEX_INPUT;
    cmd.bind_resource = vertex_input;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

// void set_vertex_binding(u32 cmd_buffer, Vertex_Binding *binding, Handle descriptor_handle) {
//     Gpu_Command cmd;
//     cmd.type = GPU_CMD_VERTEX_BINDING;
//     cmd.vertex_binding = binding;
//     cmd.vertex_descriptor_handle = descriptor_handle;
    
//     gpu_add_cmds(cmd_buffer, &cmd, 1);
// }

void gpu_cmd_vertex_buffer(u32 cmd_buffer, u32 buffer, u32 binding, u64 offset, u32 stride) {
    Gpu_Command cmd;
    cmd.type          = GPU_CMD_VERTEX_BUFFER;
    cmd.bind_index    = binding;
    cmd.bind_resource = buffer;
    cmd.bind_offset   = offset;
    cmd.bind_stride   = stride;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_index_buffer(u32 cmd_buffer, u32 buffer) {
    Gpu_Command cmd;
    cmd.type          = GPU_CMD_INDEX_BUFFER;
    cmd.bind_resource = buffer;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_framebuffer(u32 cmd_buffer, u32 framebuffer) {
    Gpu_Command cmd;
    cmd.type          = GPU_CMD_FRAMEBUFFER;
    cmd.bind_resource = framebuffer;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_cbuffer_instance(u32 cmd_buffer, Constant_Buffer_Instance *instance) {
    auto cbi = instance;
    auto cb  = cbi->constant_buffer;

    auto submit = get_gpu_submit_allocation();
    // Ensure next allocation for submit data is properly 
    // aligned with cbuffer offset requirements.
    submit->used = Align(submit->used, (u64)gpu_uniform_buffer_offset_alignment());
    const auto offset = submit->offset + submit->used;

    For (cbi->value_table) {
        auto data = it.value.data;
        auto c    = it.value.constant;

        auto gpu_data = (u8 *)submit->mapped_data + submit->used + c->offset;
        copy(gpu_data, data, c->size);
                
        //
        // This is an interesting discover about difference between updating gpu
        // memory through mapped pointer or by using direct gl call. The former
        // updates data as is and memory is visible at gpu execution time, not at
        // submission time, so there was a bug that all entities used last set
        // entity parameters data. The latter copies data into drivers internal
        // staging area from which gpu will read. So in some cases like frequent
        // update of the same gpu memory region its better to use direct gl call
        // to ensure correct data will be used by gpu later.
        //
        //                                                   -alex, Nov 24 2025
        //
        // Turned out that the latter version (glNamedBufferSubData) was slow...
        //
    }

    submit->used += cb->size;

    const auto buffer = gpu_get_buffer(submit->buffer);
    
    Gpu_Command cmd;
    cmd.type          = GPU_CMD_CBUFFER_INSTANCE;
    cmd.bind_resource = submit->buffer;
    cmd.bind_index    = cb->binding;
    cmd.bind_offset   = offset;
    cmd.bind_size     = cb->size;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_draw(u32 cmd_buffer, Gpu_Topology_Mode topology, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance) {
    Gpu_Command cmd;
    cmd.type           = GPU_CMD_DRAW;
    cmd.topology       = topology;
    cmd.draw_count     = vertex_count;
    cmd.instance_count = instance_count;
    cmd.first_draw     = first_vertex;
    cmd.first_instance = first_instance;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_draw_indexed(u32 cmd_buffer, Gpu_Topology_Mode topology, u32 index_count, u32 instance_count, u32 first_index, u32 first_instance) {
    Gpu_Command cmd;
    cmd.type           = GPU_CMD_DRAW_INDEXED;
    cmd.topology       = topology;
    cmd.draw_count     = index_count;
    cmd.instance_count = instance_count;
    cmd.first_draw     = first_index;
    cmd.first_instance = first_instance;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_draw_indirect(u32 cmd_buffer, Gpu_Topology_Mode topology, void *data, u32 offset, u32 count, u32 stride) {
    Gpu_Command cmd;
    cmd.type            = GPU_CMD_DRAW_INDIRECT;
    cmd.topology        = topology;
    cmd.indirect_data   = data;
    cmd.indirect_offset = offset;
    cmd.indirect_count  = count;
    cmd.indirect_stride = stride;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

void gpu_cmd_draw_indirect_indexed(u32 cmd_buffer, Gpu_Topology_Mode topology, void *data, u32 offset, u32 count, u32 stride) {
    Gpu_Command cmd;
    cmd.type            = GPU_CMD_DRAW_INDEXED_INDIRECT;
    cmd.topology        = topology;
    cmd.indirect_data   = data;
    cmd.indirect_offset = offset;
    cmd.indirect_count  = count;
    cmd.indirect_stride = stride;
    
    gpu_add_cmds(cmd_buffer, &cmd, 1);
}

u32 gpu_vertex_attribute_size(Gpu_Vertex_Attribute_Type type) {
    constexpr u32 lut[] = { 0, 4, 4, 4, 8, 12, 16 };
    return lut[type];
}

u32 gpu_vertex_attribute_dimension(Gpu_Vertex_Attribute_Type type) {
    constexpr u32 lut[] = { 0, 1, 1, 1, 2, 3, 4 };
    return lut[type];
}

// flip book

Flip_Book *new_flip_book(String path) {
    auto contents = read_text_file(path, __temporary_allocator);
    if (!contents) return null;
    return new_flip_book(path, contents);
}

Flip_Book *new_flip_book(String path, String contents) {
    Text_File_Handler text;
    text.allocator = __temporary_allocator;
    
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

        const auto delims = S(" ");
        auto tokens = split(line, delims);
        Assert(tokens.count == 3, "Wrong flip book text data layout");

        const auto frame_token = S("f");

        if (tokens[0] == frame_token) {
            auto frame_time   = string_to_float(tokens[1]);
            auto texture_name = get_file_name_no_ext(tokens[2]);

            Flip_Book_Frame frame;
            frame.texture    = get_texture(make_atom(texture_name));
            frame.frame_time = (f32)frame_time;

            array_add(book.frames, frame);
        } else {
            log(LOG_ERROR, "First token is not known flip book token in %S", path);
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

    const auto dir = PATH_SHADER("");
    const auto cdir = (const char *)dir.data;
    
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
    session_desc.searchPaths = &cdir;
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
    auto shader = table_find(shader_platform.shader_table, name);
    if (shader) return shader;

    log(LOG_ERROR, "Failed to find shader %S", name);
    return global_shaders.missing;
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
        log(LOG_ERROR, "Failed to get shader type from slang shader stage %d", stage);
        return SHADER_NONE;
    }
}

Shader_File *new_shader_file(String path) {
    const auto source = read_text_file(path);
    if (!is_valid(source)) {
        log(LOG_ERROR, "Failed to read shader file %S", path);
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
        log(LOG_ERROR, "Invalid extension in shader file path %S", path);
        return null;
    }

    shader_file.path = copy_string(path);
    shader_file.name = get_file_name_no_ext(shader_file.path);

    const char *cpath = temp_c_string(path);
    Slang::ComPtr<slang::IBlob> diagnostics;
    auto *module = slang_loadModuleFromSource(slang_local_session, cpath, cpath, (char *)source.data, source.size, diagnostics.writeRef());

    if (diagnostics) {
        log(LOG_ERROR, "Slang: %s", (const char *)diagnostics->getBufferPointer());
    }

    if (!module) {
        table_remove(platform.shader_file_table, path);
        log(LOG_ERROR, "Failed to load slang shader module from %S", path);
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
            log(LOG_ERROR, "Failed to create shader program %s from %S", cname, path);
            return {};
        }

        Slang::ComPtr<slang::IBlob> diagnostics;
        Slang::ComPtr<slang::IComponentType> linked_program;
        program->link(linked_program.writeRef(), diagnostics.writeRef());

        if (diagnostics) {
            log(LOG_ERROR, "Slang: %s", (const char *)diagnostics->getBufferPointer());
        }

        if (!linked_program) {
            log(LOG_ERROR, "Failed to link shader program %s for %S", cname, path);
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
                            log(LOG_ERROR, "Failed to create constant buffer %S for shader %S", name, path);
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
            log(LOG_ERROR, "Slang: %s", (const char *)diagnostics->getBufferPointer());
        }

        if (!kernel_blob) {
            log(LOG_ERROR, "Failed to get compiled shader code from %S", path);
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
        log(LOG_ERROR, "Failed to find shader extension from path %S", path);
        return false;
    }
    
    return ext == SHADER_SOURCE_EXT;
}

bool is_shader_header_path(String path) {
    const auto ext = slice(path, '.', S_SEARCH_REVERSE_BIT);
    if (!is_valid(ext)) {
        log(LOG_ERROR, "Failed to find shader extension from path %S", path);
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
    return is_valid(shader.linked_program) && shader_file && is_valid(*shader_file);
}

Constant_Buffer *new_constant_buffer(String name, u32 size, u16 binding, u16 constant_count) {
    // @Cleanup @Memory: calculate cbuffer aligned memory size,
    //                   current alignment to cbuffer offset is a hack.
    //                   All constants should be passed here then.
    
    // As we store all constant buffers in one big gpu buffer, we need to respect
    // provided alignment rules by a backend.
    size = Align(size, gpu_uniform_buffer_offset_alignment());
    
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
        log(LOG_ERROR, "Can't add constant %S to constant buffer %S (0x%X), it's type is CT_ERROR", name, cb->name, cb);
        return null;
    }
        
    if (type != CT_CUSTOM) {
        const u32 calc_size = c.type_sizes[type] * count; 
        if (size != calc_size) {
            log(LOG_ERROR, "Given constant %S size %u does not correspond to it's size %u, inferred from type %u and count %u",
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
        log(LOG_ERROR, "Failed to find constant value %S in constant buffer instance %S 0x%X", name, cb->name, cbi);
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

Gpu_Image_Format gpu_image_format_from_channel_count(u32 channel_count) {
    switch (channel_count) {
    case 1: return GPU_IMAGE_FORMAT_RED_8;
    case 3: return GPU_IMAGE_FORMAT_RGB_8;
    case 4: return GPU_IMAGE_FORMAT_RGBA_8;
    default:
        log(S("GPU"), LOG_ERROR, "Failed to get image format from channel count %d", channel_count);
        return GPU_IMAGE_FORMAT_NONE;
    }
}

Texture *new_texture(String path) {
    auto contents = read_file(path, __temporary_allocator);
    auto name = get_file_name_no_ext(path);
    auto atom = make_atom(name);
    return new_texture(atom, contents);
}

Texture *new_texture(Atom name, Buffer file_contents) {
    s32 width = 0, height = 0, depth = 1, channel_count = 0;
    u8 *data = stbi_load_from_memory(file_contents.data, (s32)file_contents.size, &width, &height, &channel_count, 0);
    
    if (!data) log(S("stbi"), LOG_ERROR, "%s", stbi_failure_reason());

    auto &texture = texture_table[name];

    if (texture.image_view) gpu_delete_image_view(texture.image_view);
    if (texture.sampler)    gpu_delete_sampler(texture.sampler);

    // @Cleanup: for now assume its just 2D image.
    const auto type   = GPU_IMAGE_TYPE_2D;
    const auto format = gpu_image_format_from_channel_count(channel_count);
    const auto mipmap_count = gpu_max_mipmap_count(width, height);

    const auto contents = make_buffer(data, width * height * depth * channel_count);
    const auto image = gpu_new_image(type, format, mipmap_count, width, height, depth, contents);
    texture.image_view = gpu_new_image_view(image, type, format, 0, mipmap_count, 0, depth);

    texture.sampler = gpu.sampler_default_color;
    
    return &texture;
}

Texture *new_texture(Atom name, u32 image_view, u32 sampler) {
    auto &texture = texture_table[name];

    if (texture.image_view) gpu_delete_image_view(texture.image_view);
    
    texture.image_view = image_view;
    texture.sampler    = sampler;
    
    return &texture;
}

Texture *get_texture(Atom name) {
    auto tex = table_find(texture_table, name);
    if (tex) return tex;

    log(LOG_ERROR, "Failed to find texture %S", get_string(name));
    return global_textures.missing;
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
    const auto shader_token           = S("s");
    const auto color_token            = S("c");
    const auto ambient_texture_token  = S("ta");
    const auto diffuse_texture_token  = S("td");
    const auto specular_texture_token = S("ts");
    const auto ambient_light_token    = S("la");
    const auto diffuse_light_token    = S("ld");
    const auto specular_light_token   = S("ls");
    const auto shininess_token        = S("shine");
    const auto blend_token            = S("blend");

    Text_File_Handler text;
    text.allocator = __temporary_allocator;
    
    defer { reset(&text); };
    init_from_memory(&text, contents);

    if (get_extension(path) != MATERIAL_EXT) {
        log(LOG_ERROR, "Failed to create new material with unsupported extension %S", path);
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

        const auto delims = S(" ");
        auto tokens = split(line, delims);
        
        if (tokens[0] == shader_token) {
            // @Cleanup @Hack: this trim should not be here, its a temp fix of an issue
            // when returned token contains part of new line character '\' due to not
            // fully correct *split* function implementation.
            auto shader_name = trim(get_file_name_no_ext(tokens[1]));
            material.shader = get_shader(shader_name);

            // @Cleanup: not sure whether its a good idea to create cbuffer instance for
            // each cbuffer in a shader, maybe we should add a lazy version that will create
            // instances when needed or something.
            For (material.shader->resource_table) {
                if (it.value.type == GPU_CONSTANT_BUFFER) {
                    auto cb = get_constant_buffer(it.key);
                    if (!cb) {
                        log(LOG_ERROR, "Failed to find cbuffer %S during new material %S creation", it.key, name);
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
            // @Cleanup @Hack
            auto texture_name = trim(get_file_name_no_ext(tokens[1]));
            material.ambient_texture = get_texture(make_atom(texture_name));
        } else if (tokens[0] == diffuse_texture_token) {
            // @Cleanup @Hack
            auto texture_name = trim(get_file_name_no_ext(tokens[1]));
            material.diffuse_texture = get_texture(make_atom(texture_name));
        } else if (tokens[0] == specular_texture_token) {
            // @Cleanup @Hack
            auto texture_name = trim(get_file_name_no_ext(tokens[1]));
            material.specular_texture = get_texture(make_atom(texture_name));
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
                log(LOG_ERROR, "First token %S is not known material token in %S", tokens[0], path);
            }
        }
    }

    return &material;
}

Material *get_material(String name) {
    auto mat = table_find(material_table, name);
    if (mat) return mat;

    log(LOG_ERROR, "Failed to find material %S", name);
    return global_materials.missing;
}

bool has_transparency(const Material *material) {
    return material->use_blending
        || material->base_color.w < 1.0f;
}
