#pragma once

#include "hash_table.h"

inline const auto LEVEL_EXT = S("level");

struct Entity_Manager;

struct Level {
    Atom            name;
    Entity_Manager *entity_manager;
};

Level *new_level            (String path);
Level *new_level            (Atom name, String contents);
Level *get_level            (Atom name);
Level *get_current_level    ();
void   set_current_level    (Atom name);
void   save_current_level   ();
void   reload_current_level ();

inline Table <Atom, Level> level_table;
