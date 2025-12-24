#pragma once

// #include "collision.h"
// #include "entity.h"
// #include "render_batch.h"

// typedef For_Result (*For_Each_Entity_Callback)(Entity *e, void *user_data);

// struct Game_World {
//     static constexpr u32 MAX_NAME_SIZE = 64;

//     static constexpr u32 MAX_STATIC_MESHES     = 1024;
//     static constexpr u32 MAX_DIRECT_LIGHTS     = ::MAX_DIRECT_LIGHTS;
//     static constexpr u32 MAX_POINT_LIGHTS      = ::MAX_POINT_LIGHTS;
//     static constexpr u32 MAX_SOUND_EMITTERS_2D = 32;
//     static constexpr u32 MAX_SOUND_EMITTERS_3D = 128;
//     static constexpr u32 MAX_PORTALS           = 128;
//     static constexpr u32 MAX_ENTITIES          = MAX_STATIC_MESHES + MAX_DIRECT_LIGHTS
//         + MAX_POINT_LIGHTS + MAX_SOUND_EMITTERS_2D + MAX_SOUND_EMITTERS_3D + MAX_PORTALS;
//     static constexpr u32 MAX_AABBS = 2048;
    
//     String name;

// 	f32 dt;

// 	Player player;
// 	Camera camera;
// 	Camera ed_camera;

// 	Skybox skybox;

// 	Array <Static_Mesh>      static_meshes;
//     Array <Point_Light>      point_lights;
//     Array <Direct_Light>     direct_lights;
//     Array <Sound_Emitter_2D> sound_emitters_2d;
//     Array <Sound_Emitter_3D> sound_emitters_3d;
//     Array <Portal>           portals;
    
// 	Array <AABB> aabbs;
    
//     Render_Batch opaque_batch;
//     Render_Batch transparent_batch;
// };

// inline Game_World World;

// void init       (Game_World &world);
// void save_level (Game_World &world);
// void load_level (Game_World &world, String path);

// Camera &active_camera(Game_World &world);

// Entity *create_entity(Game_World &world, Entity_Type e_type);
// Entity *find_entity_by_eid(Game_World &world, eid eid);

// void draw (Game_World &world);
// void draw (Entity &e);

// void for_each_entity(Game_World &world, For_Each_Entity_Callback callback, void *user_data = null);
