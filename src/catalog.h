#pragma once

#include "hash_table.h"
#include "file_system.h"

struct Catalog_Entry {
    String name; // file name from path with no extension
    String path;

    u64 last_write_time = 0;
};

struct Catalog {
    Array <Catalog_Entry> entries;
    Table <String, Catalog_Entry *> lookup_by_name;
    Table <String, Catalog_Entry *> lookup_by_path;
};

inline void add_entry(Catalog *catalog, const Catalog_Entry &entry) {
    auto &e = array_add(catalog->entries, entry);
    table_add(catalog->lookup_by_name, e.name, &e);
    table_add(catalog->lookup_by_path, e.path, &e);
}

inline void add_directory_files(Catalog *catalog, String path, bool recursive = true) {
    visit_directory(path, [] (const File_Callback_Data *data) {
        auto catalog = (Catalog *)data->user_data;

        Catalog_Entry entry;
        entry.path = copy_string(data->path);
        entry.name = get_file_name_no_ext(entry.path);
        entry.last_write_time = data->last_write_time;
        
        add_entry(catalog, entry);
    }, recursive, catalog);
}

inline Catalog_Entry *find_by_name(Catalog *catalog, String name) {
    if (auto p = table_find(catalog->lookup_by_name, name)) return *p;
    return null;
}

inline Catalog_Entry *find_by_path(Catalog *catalog, String path) {
    if (auto p = table_find(catalog->lookup_by_path, path)) return *p;
    return null;
}
