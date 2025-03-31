#include "pch.h"
#include "audio/sound.h"

void load_game_sounds(Sound_List *list) {
	// @Cleanup:: create_sound loads sound data directly to sound card memory,
	// so its bad to load big files in such case (they should be streamed then).
	// But its good for small, frequently used sounds like player steps.

	list->world             = create_sound(SOUND_PATH("C418_Shuniji.wav"), 0);
    list->player_steps      = create_sound(SOUND_PATH("player_steps.wav"), SOUND_FLAG_LOOP);
    list->player_steps_cute = create_sound(SOUND_PATH("player_steps_cute.wav"), SOUND_FLAG_LOOP);
}
