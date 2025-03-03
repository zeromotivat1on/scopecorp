#include "pch.h"
#include "audio/al.h"
#include "audio/sound.h"
#include "math/vector.h"

void load_game_sounds(Sound_List *list) {
	// @Cleanup:: create_sound loads sound data directly to sound card memory,
	// so its bad to load big files in such case (they should be streamed then).
	// But its good for small, frequently used sounds like player steps.

	list->world = create_sound(DIR_SOUNDS "C418_Shuniji.wav", 0);
	list->player_steps = create_sound(DIR_SOUNDS "player_steps.wav", SOUND_FLAG_LOOP);
	list->player_steps_cute = create_sound(DIR_SOUNDS "player_steps_cute.wav", SOUND_FLAG_LOOP);
}

void set_listener_pos(vec3 pos) {
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
}
