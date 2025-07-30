#include "pch.h"
#include "log.h"
#include "str.h"
#include "font.h"
#include "profile.h"
#include "asset.h"

#include "render/render.h"
#include "render/ui.h"
#include "render/r_table.h"
#include "render/r_command.h"
#include "render/r_target.h"
#include "render/r_pass.h"
#include "render/r_stats.h"
#include "render/r_storage.h"
#include "render/r_viewport.h"
#include "render/r_vertex_descriptor.h"
#include "render/r_geo.h"
#include "render/r_texture.h"
#include "render/r_shader.h"
#include "render/r_material.h"
#include "render/r_uniform.h"
#include "render/r_mesh.h"
#include "render/r_flip_book.h"

#include "editor/editor.h"

#include "game/game.h"
#include "game/world.h"
#include "game/entity.h"

#include "os/input.h"
#include "os/atomic.h"
#include "os/window.h"

#include "math/math_basic.h"

void r_create_table(R_Table &t) {
    reserve(t.arena, MB(4));
    
    sparse_reserve(t.arena, t.targets,    t.MAX_TARGETS);
    sparse_reserve(t.arena, t.passes,     t.MAX_PASSES);
    sparse_reserve(t.arena, t.textures,   t.MAX_TEXTURES);
    sparse_reserve(t.arena, t.shaders,    t.MAX_SHADERS);
    sparse_reserve(t.arena, t.uniforms,   t.MAX_UNIFORMS);
    sparse_reserve(t.arena, t.materials,  t.MAX_MATERIALS);
    sparse_reserve(t.arena, t.meshes,     t.MAX_MESHES);
    sparse_reserve(t.arena, t.flip_books, t.MAX_FLIP_BOOKS);
    sparse_reserve(t.arena, t.vertex_descriptors, t.MAX_VERTEX_DESCRIPTORS);
    
    // Add dummies at index 0.
    sparse_push(t.targets);
    sparse_push(t.passes);
    sparse_push(t.textures);
    sparse_push(t.shaders);
    sparse_push(t.uniforms);
    sparse_push(t.materials);
    sparse_push(t.meshes);
    sparse_push(t.vertex_descriptors);
    sparse_push(t.flip_books);
}

void r_destroy_table(R_Table &t) {
    release(t.arena);
    t = {};
}

void r_init_global_uniforms() {
    RID_UNIFORM_BUFFER = r_create_uniform_buffer(MAX_UNIFORM_BUFFER_SIZE);
        
    constexpr u32 MAX_UNIFORM_LIGHTS = 64; // must be the same as in shaders
    static_assert(MAX_UNIFORM_LIGHTS >= World.MAX_POINT_LIGHTS + World.MAX_DIRECT_LIGHTS);

    const Uniform_Block_Field camera_fields[] = {
        { R_F32_3,   1 },
        { R_F32_4X4, 1 },
        { R_F32_4X4, 1 },
        { R_F32_4X4, 1 },
    };

    const Uniform_Block_Field viewport_fields[] = {
        { R_F32_2,   1 },
        { R_F32_4X4, 1 },
    };
         
    const Uniform_Block_Field direct_light_fields[] = {
        { R_U32,   1 },
        { R_F32_3, World.MAX_DIRECT_LIGHTS },
        { R_F32_3, World.MAX_DIRECT_LIGHTS },
        { R_F32_3, World.MAX_DIRECT_LIGHTS },
        { R_F32_3, World.MAX_DIRECT_LIGHTS },
    };

    const Uniform_Block_Field point_light_fields[] = {
        { R_U32,   1 },
        { R_F32_3, World.MAX_POINT_LIGHTS },
        { R_F32_3, World.MAX_POINT_LIGHTS },
        { R_F32_3, World.MAX_POINT_LIGHTS },
        { R_F32_3, World.MAX_POINT_LIGHTS },
        { R_F32,   World.MAX_POINT_LIGHTS },
        { R_F32,   World.MAX_POINT_LIGHTS },
        { R_F32,   World.MAX_POINT_LIGHTS },
    };

    r_add_uniform_block(RID_UNIFORM_BUFFER,
                        UNIFORM_BLOCK_BINDING_CAMERA,
                        UNIFORM_BLOCK_NAME_CAMERA,
                        camera_fields, COUNT(camera_fields),
                        &uniform_block_camera);

    r_add_uniform_block(RID_UNIFORM_BUFFER,
                        UNIFORM_BLOCK_BINDING_VIEWPORT,
                        UNIFORM_BLOCK_NAME_VIEWPORT,
                        viewport_fields, COUNT(viewport_fields),
                        &uniform_block_viewport);

    r_add_uniform_block(RID_UNIFORM_BUFFER,
                        UNIFORM_BLOCK_BINDING_DIRECT_LIGHTS,
                        UNIFORM_BLOCK_NAME_DIRECT_LIGHTS,
                        direct_light_fields, COUNT(direct_light_fields),
                        &uniform_block_direct_lights);

    r_add_uniform_block(RID_UNIFORM_BUFFER,
                        UNIFORM_BLOCK_BINDING_POINT_LIGHTS,
                        UNIFORM_BLOCK_NAME_POINT_LIGHTS,
                        point_light_fields, COUNT(point_light_fields),
                        &uniform_block_point_lights);
}

void r_resize_viewport(R_Viewport &viewport, u16 width, u16 height) {
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
		error("Failed to resize viewport with unknown aspect type %d", viewport.aspect_type);
		break;
	}

    const u16 r_width  = (u16)((f32)viewport.width  * viewport.resolution_scale);
    const u16 r_height = (u16)((f32)viewport.height * viewport.resolution_scale);
    
    r_resize_render_target(viewport.render_target, r_width, r_height);
    log("Resized viewport %dx%d", r_width, r_height);
}

void draw_world(const Game_World &world) {
    PROFILE_SCOPE(__FUNCTION__);
    
	draw_entity(world.skybox);

	For (world.static_meshes) {
		draw_entity(it);
    }
    
	draw_entity(world.player);
}

static R_Sort_Key entity_sort_key(const Entity &e) {
    R_Sort_Key sort_key;
    sort_key.screen_layer = R_GAME_LAYER;
    
    if (e.type == E_SKYBOX) {
        // Draw skybox at the very end.
        sort_key.depth = 0x00FFFFFF;
    } else {
        const auto &camera = active_camera(World);
        const f32 dsqr = length_sqr(e.location - camera.eye);
        const f32 norm = dsqr / (camera.far * camera.far);

        Assert(norm >= 0.0f);
        Assert(norm <= 1.0f);
        
        const u32 bits = *(u32 *)&norm;
        sort_key.depth = bits >> 8;
    }

    if (e.type == E_PLAYER) {
        // @Cleanup @Temp: only player has translucency.
        sort_key.translucency = R_NORM_TRANSLUCENT;
    } else {
        sort_key.translucency = R_OPAQUE;
    }

    // @Todo: invert depth to sort (back-to-front) translucenct entites.
    
    return sort_key;
}

void draw_entity(const Entity &e) {
    if (e.draw_data.sid_material == SID_NONE) return;
    if (e.draw_data.sid_mesh == SID_NONE)     return;

    const auto *mh = find_mesh(e.draw_data.sid_mesh);
    const auto *mt = find_material(e.draw_data.sid_material);

    if (mh == null || mt == null) return;
    
    // @Todo: outline mouse picked entity as before.
    
    const bool mouse_picked = e.bits & E_MOUSE_PICKED_BIT;
    
    R_Command cmd;
    cmd.mode = R_TRIANGLES;
    cmd.shader = mt->shader;
    cmd.texture = mt->texture;
    cmd.uniform_count = mt->uniform_count;
    cmd.uniforms = mt->uniforms;
    cmd.vertex_desc = mh->vertex_descriptor;
    
    if (mh->index_count > 0) {
        cmd.bits |= R_CMD_INDEXED_BIT;
        cmd.first = mh->first_index;
        cmd.count = mh->index_count;
    } else {
        cmd.first = 0;
        cmd.count = mh->vertex_count;
    }
    
#if DEVELOPER
    cmd.eid_offset = e.draw_data.eid_offset;
#endif

    if (mouse_picked) {
        r_geo_cross(e.location, 0.5f);
    }

    r_add(R_command_list, cmd, entity_sort_key(e));
    
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

        geo_draw_cross(e->location, 0.5f);
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
        const mat4 mvp = mat4_transform(e->location, e->rotation, e->scale * 1.02f) * camera->view_proj;

        auto &material = asset_table.materials[SID_MATERIAL_OUTLINE];
        set_material_uniform_value(&material, "u_color",     &color, sizeof(color));
        set_material_uniform_value(&material, "u_transform", &mvp,   sizeof(mvp));

        command.sid_material = SID_MATERIAL_OUTLINE;
        
        r_enqueue(&entity_render_queue, &command);
    }
    */
}

void r_geo_init() {
    constexpr u32 location_buffer_size = R_geo.MAX_VERTICES * sizeof(vec3);
    constexpr u32 color_buffer_size    = R_geo.MAX_VERTICES * sizeof(u32);

    const u32 location_buffer_offset  = R_vertex_map_range.offset + R_vertex_map_range.size;
    const u32 color_buffer_offset     = location_buffer_offset + location_buffer_size;

    R_geo.locations  = (vec3 *)r_alloc(R_vertex_map_range, location_buffer_size).data;
    R_geo.colors     = (u32  *)r_alloc(R_vertex_map_range, color_buffer_size).data;
    R_geo.vertex_count = 0;
    
    R_Vertex_Binding bindings[2] = {};
    bindings[0].binding_index = 0;
    bindings[0].offset = location_buffer_offset;
    bindings[0].component_count = 1;
    bindings[0].components[0] = { R_F32_3, 0 };

    bindings[1].binding_index = 1;
    bindings[1].offset = color_buffer_offset;
    bindings[1].component_count = 1;
    bindings[1].components[0] = { R_U32, 0 };
    
    R_geo.vertex_descriptor = r_create_vertex_descriptor(COUNT(bindings), bindings);
    R_geo.sid_material = SID_MATERIAL_GEOMETRY;

    r_create_command_list(R_geo.MAX_COMMANDS, R_geo.command_list);
}

void r_geo_line(vec3 start, vec3 end, u32 color) {
    Assert(R_geo.vertex_count + 2 <= R_geo.MAX_VERTICES);

    auto &gdb = R_geo;

    vec3 *vl = gdb.locations + gdb.vertex_count;
    u32  *vc = gdb.colors    + gdb.vertex_count;
        
    vl[0] = start;
    vl[1] = end;
    vc[0] = color;
    vc[1] = color;

    gdb.vertex_count += 2;
}

void r_geo_arrow(vec3 start, vec3 end, u32 color) {
    static const f32 size = 0.04f;
    static const f32 arrow_step = 30.0f; // In degrees
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

    r_geo_line(start, end, color);

    vec3 forward = normalize(end - start);
	const vec3 right = Abs(forward.y) > 1.0f - F32_EPSILON
        ? normalize(cross(vec3_right, forward))
        : normalize(cross(vec3_up, forward));
	const vec3 up = cross(forward, right);
    forward *= size;
    
    f32 degrees = 0.0f;
    for (int i = 0; degrees < 360.0f; degrees += arrow_step, ++i) {
        f32 scale;
        vec3 v1, v2;

        scale = 0.5f * size * arrow_cos[i];
        v1 = (end - forward) + (right * scale);

        scale = 0.5f * size * arrow_sin[i];
        v1 += up * scale;

        scale = 0.5f * size * arrow_cos[i + 1];
        v2 = (end - forward) + (right * scale);

        scale = 0.5f * size * arrow_sin[i + 1];
        v2 += up * scale;

        r_geo_line(v1, end, color);
        r_geo_line(v1, v2,  color);
    }
}

void r_geo_cross(vec3 location, f32 size) {
    r_geo_arrow(location, location + vec3_up * size,      rgba_blue);
    r_geo_arrow(location, location + vec3_right * size,   rgba_green);
    r_geo_arrow(location, location + vec3_forward * size, rgba_red);
}

void r_geo_box(const vec3 points[8], u32 color) {
    for (int i = 0; i < 4; ++i) {
        r_geo_line(points[i],     points[(i + 1) % 4],       color);
        r_geo_line(points[i + 4], points[((i + 1) % 4) + 4], color);
        r_geo_line(points[i],     points[i + 4],             color);
    }
}

void r_geo_aabb(const AABB &aabb, u32 color) {
    const vec3 bb[2] = { aabb.min, aabb.max };
    vec3 points[8];

    for (int i = 0; i < 8; ++i) {
        points[i].x = bb[(i ^ (i >> 1)) % 2].x;
        points[i].y = bb[(i >> 1) % 2].y;
        points[i].z = bb[(i >> 2) % 2].z;
    }

    r_geo_box(points, color);
}

void r_geo_ray(const Ray &ray, f32 length, u32 color) {
    const vec3 start = ray.origin;
    const vec3 end   = ray.origin + ray.direction * length;
    r_geo_line(start, end, color);
}

void r_geo_flush() {
    PROFILE_SCOPE(__FUNCTION__);

    if (R_geo.vertex_count == 0) return;

    const auto &mt = *find_material(R_geo.sid_material);
    
    R_Command cmd = {};
    cmd.mode = R_LINES;
    cmd.shader = mt.shader;
    cmd.texture = mt.texture;
    cmd.uniform_count = mt.uniform_count;
    cmd.uniforms = mt.uniforms;
    cmd.vertex_desc = R_geo.vertex_descriptor;
    cmd.first = 0;
    cmd.count = R_geo.vertex_count;
    cmd.base_instance  = 0;
    cmd.instance_count = 1;
    
    r_add(R_geo.command_list, cmd);
    
    r_submit(R_geo.command_list);
    
    R_geo.vertex_count = 0;
}

static For_Result cb_draw_aabb(Entity *e, void *user_data) {
    auto *aabb = sparse_find(World.aabbs, e->aabb_index);
    if (aabb) {
        u32 aabb_color = rgba_black;
        switch (e->type) {
        case E_PLAYER:
        case E_STATIC_MESH: {
            aabb_color = rgba_red;
            break;
        }
        case E_SOUND_EMITTER_2D:
        case E_SOUND_EMITTER_3D: {
            aabb_color = rgba_blue;
            break;
        }
        case E_PORTAL: {
            aabb_color = rgba_purple;
            break;
        }
        case E_DIRECT_LIGHT:
        case E_POINT_LIGHT: {
            aabb_color = rgba_white;
            break;
        }
        }
        
        r_geo_aabb(*aabb, aabb_color);
    }

    return CONTINUE;
};

void r_geo_debug() {
    const auto &player = World.player;

    if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
        const vec3 center = player.location + vec3(0.0f, player.scale.y * 0.5f, 0.0f);
        r_geo_arrow(center, center + normalize(player.velocity) * 0.5f, rgba_red);
    }
    
    if (game_state.view_mode_flags & VIEW_MODE_FLAG_COLLISION) {
        for_each_entity(World, cb_draw_aabb);

        if (Editor.mouse_picked_entity) {
            const auto *e = Editor.mouse_picked_entity;
            const u32 mouse_picked_color = rgba_yellow;
            r_geo_aabb(World.aabbs[e->aabb_index], mouse_picked_color);
        }
        
        if (player.collide_aabb_index != INDEX_NONE) {
            r_geo_aabb(World.aabbs[player.aabb_index],         rgba_green);
            r_geo_aabb(World.aabbs[player.collide_aabb_index], rgba_green);
        }
    }
}

u16 r_create_mesh(u16 vertex_descriptor, u32 vertex_count, u32 first_index, u32 index_count) {
    R_Mesh mh;
    mh.vertex_descriptor = vertex_descriptor;
    mh.vertex_count = vertex_count;
    mh.first_index = first_index;
    mh.index_count = index_count;

    return sparse_push(R_table.meshes, mh);
}

u16 r_create_uniform(sid name, u16 type, u16 count) {
    R_Uniform un;
    un.name = name;
    un.type = type;
    un.count = count;
    un.size = count * r_uniform_type_size(type);

    auto &cache = uniform_value_cache;
    Assert(cache.size + un.size <= MAX_UNIFORM_VALUE_CACHE_SIZE);

    un.offset = cache.size;
    cache.size += un.size;

    return sparse_push(R_table.uniforms, un);
}

void r_set_uniform(u16 uniform, u32 offset, u32 size, const void *data) {
    auto &un = R_table.uniforms[uniform];
    Assert(offset + size <= un.size);

    auto &cache = uniform_value_cache;
    mem_copy((u8 *)cache.data + un.offset + offset, data, size);
}

u16 r_create_material(u16 shader, u16 texture, R_Light_Params lparams, u16 ucount, const u16 *uniforms) {
    Assert(ucount <= R_Material::MAX_UNIFORMS);

    R_Material mt;
    mt.shader = shader;
    mt.texture = texture;
    mt.light_params = lparams;
    mt.uniform_count = ucount;
    mem_copy(mt.uniforms, uniforms, ucount * sizeof(uniforms[0]));

    return sparse_push(R_table.materials, mt);
}

void r_set_material_uniform(u16 material, sid name, u32 offset, u32 size, const void *data) {
    const auto &mt = R_table.materials[material];
    for (u16 i = 0; i < mt.uniform_count; ++i) {
        const u16 uniform = mt.uniforms[i];
        const auto &un = R_table.uniforms[uniform];
        if (name == un.name) {
            r_set_uniform(uniform, offset, size, data);
            break;
        }
    }
}

u32 r_uniform_type_size(u16 type) {
    switch (type) {
	case R_U32:     return 1  * sizeof(u32);
	case R_F32:     return 1  * sizeof(f32);
	case R_F32_2:   return 2  * sizeof(f32);
	case R_F32_3:   return 3  * sizeof(f32);
	case R_F32_4:   return 4  * sizeof(f32);
	case R_F32_4X4: return 16 * sizeof(f32);
    default:
        error("Failed to get uniform size from type %d", type);
        return 0;
    }
}

u32 r_uniform_type_dimension(u16 type) {
    switch (type) {
	case R_U32:     return 1;
	case R_F32:     return 1;
	case R_F32_2:   return 2;
	case R_F32_3:   return 3;
	case R_F32_4:   return 4;
	case R_F32_4X4: return 16;
    default:
        error("Failed to get uniform size from type %d", type);
        return 0;
    }
}

u32 r_uniform_block_field_size(const Uniform_Block_Field &field) {
    return r_uniform_type_size(field.type) * field.count;
}

u16 r_vertex_component_dimension(u16 type) {
	switch (type) {
	case R_S32:   return 1;
	case R_U32:   return 1;
	case R_F32_2: return 2;
	case R_F32_3: return 3;
	case R_F32_4: return 4;
	default:
		error("Failed to get dimension from unknown vertex component type %d", type);
		return 0;
	}
}

u16 r_vertex_component_size(u16 type) {
	switch (type) {
    case R_S32:   return 1 * sizeof(s32);
    case R_U32:   return 1 * sizeof(u32);
	case R_F32_2: return 2 * sizeof(f32);
	case R_F32_3: return 3 * sizeof(f32);
	case R_F32_4: return 4 * sizeof(f32);
	default:
		error("Failed to get size from unknown vertex component type %d", type);
		return 0;
	}
}

void update_render_stats() {
    static f32 update_time = 0.0f;
    constexpr f32 update_interval = 0.2f;

    update_time += delta_time;
    
    constexpr s32 dt_frame_count = 512;
    static f32 previous_dt_table[dt_frame_count];
    
    frame_index++;
    previous_dt_table[frame_index % dt_frame_count] = delta_time;

    if (update_time > update_interval) {
        update_time = 0.0f;
        
        f32 dt_sum = 0.0f;
        for (s32 i = 0; i < dt_frame_count; ++i) {
            dt_sum += previous_dt_table[i];
        }
        
        average_dt  = dt_sum / dt_frame_count;
        average_fps = 1.0f / average_dt;
    }
}

u32 get_texture_format_from_channel_count(u32 channel_count) {
    switch (channel_count) {
    case 3: return R_RGB_8;
    case 4: return R_RGBA_8;
    default:
        constexpr u32 default_format = R_RGBA_8;
        error("%s: Using default texture format %u as texture channel count = %d", __func__, default_format, channel_count);
        return default_format;
    }
}

void r_add(R_Command_List &list, const R_Command &cmd, R_Sort_Key sort_key) {
    Assert(list.count < list.capacity);
    
    list.queued[list.count] = R_Command_List::Queued { sort_key, cmd };
    list.count += 1;
}

R_Alloc_Range r_alloc(R_Map_Range &map, u32 size) {
    Assert(map.size + size <= map.capacity);

    R_Alloc_Range alloc;
    alloc.map = &map;
    alloc.offset = map.size;
    alloc.size = 0;
    alloc.capacity = size;
    alloc.data = (u8 *)map.data + map.size;

    map.size += size;
    
    return alloc;
}

void *r_alloc(R_Alloc_Range &alloc, u32 size) {
    Assert(alloc.size + size <= alloc.capacity);

    void *data = (u8 *)alloc.data + alloc.size;
    alloc.size += size;

    return data;
}

u32 head_pointer(R_Alloc_Range &alloc) {
    return alloc.size + alloc.offset + alloc.map->offset;
}

u16 r_channel_count_from_format(u16 format) {
    switch (format) {
    case R_RED_32: return 1;
    case R_RGB_8:  return 3;
    case R_RGBA_8: return 4;
    default: return 0;
    }
}

u16 r_format_from_channel_count(u16 count) {
    switch (count) {
    case 1: return R_RED_32;
    case 3: return R_RGB_8;
    case 4: return R_RGBA_8;
    default: return 0;
    }
}

u16 r_create_flip_book(u32 count, const u16 *textures, f32 next_frame_time) {
    R_Flip_Book fp;
    fp.frame_count = count;
    fp.next_frame_time = next_frame_time;

    for (u32 i = 0; i < count; ++i) {
        fp.frames[i] = R_Flip_Book::Frame { textures[i] };
    }

    return sparse_push(R_table.flip_books, fp);
}

R_Flip_Book::Frame &r_get_current_flip_book_frame(u16 flip_book) {
    auto &fp = R_table.flip_books[flip_book];
	Assert(fp.current_frame_index < fp.frame_count);
	return fp.frames[fp.current_frame_index];
}

R_Flip_Book::Frame &r_advance_flip_book_frame(u16 flip_book) {
    auto &fp = R_table.flip_books[flip_book];
	Assert(fp.current_frame_index < fp.frame_count);

    auto &frame = fp.frames[fp.current_frame_index];
	fp.current_frame_index = (fp.current_frame_index + 1) % fp.frame_count;
    
	return frame;
}

void r_tick_flip_book(u16 flip_book, f32 dt) {
    auto &fp = R_table.flip_books[flip_book];
	fp.frame_time += dt;
    
	if (fp.frame_time > fp.next_frame_time) {
		r_advance_flip_book_frame(flip_book);
		fp.frame_time = 0.0f;
	}
}
