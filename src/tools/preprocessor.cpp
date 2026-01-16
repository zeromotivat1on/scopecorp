#define SPRINTF_CUSTOM_STRING

#include "basic.cpp"
#include "os.cpp"

#ifdef _WIN32
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "dbghelp.lib")
#include "win32.cpp"
#endif

#include "reflection.h"
#include "profile.h"

struct Reflection_Target {
    String filepath;
    String struct_names[8];
    u32    struct_count;
};

String reflect_struct(String code, String struct_name) {
    static const auto token_field_modifier_no_serialize = S("@no_serialize");
    
    const auto search = tprint("%s %S {", "struct", struct_name);
    
    auto s = slice(code, search, S_INDEX_PLUS_ONE_BIT);
    if (!s) {
        log(LOG_ERROR, "Failed to find '%S' in given code string", search);
        return {};
    }

    s.data += search.size + 1; // include new line
    s.size -= search.size + 1; // include new line
    
    String_Builder sb;
    sb.allocator = __temporary_allocator;

    codegen(sb, "Begin_Reflection(%S)", struct_name);
    
    while (1) {
        auto line = slice(s, '\n', S_LEFT_SLICE_BIT);
        if (!line || line.size < 5) break;
        
        const auto trimed_line = trim(line);
        // Check for end of struct definition.
        if (trimed_line == S("};\n")) break;

        s.data += line.size;
        s.size -= line.size;

        // Skip comments.
        const auto first_two = make_string(trimed_line.data, 2);
        if (first_two == S("//") || first_two == S("/*")) continue;
                
        auto tokens = split(line, S(" ;/")); // skip line semicolon and comments as well

        if (tokens.count < 2) continue;

        // Simple check if its a function/operator/etc.
        bool is_struct_field = true;
        For (tokens) {
            const auto s = slice(it, S("("));
            if (s) {
                is_struct_field = false;
                break;
            }
        }

        if (!is_struct_field) continue;
        
        auto token_type     = trim(tokens[0]);
        auto token_variable = trim(tokens[1]);

        const auto reflection_type      = get_reflection_type(token_type);
        const auto reflection_type_name = to_string(reflection_type);

        bool serializable = true;
        for (u32 i = 2; i < tokens.count; ++i) {
            const auto token = trim(tokens[i]);
            if (serializable && token == token_field_modifier_no_serialize) {
                serializable = false;
            }
        }
        
        codegen(sb, "Add_Reflection_Field(%S, %S, %S, %S)",
                         struct_name, token_variable, reflection_type_name,
                         serializable ? S("1") : S("0"));
    }

    codegen(sb, "End_Reflection(%S)", struct_name);
        
    const auto reflection = builder_to_string(sb);
    return reflection;
}

void reflection_pass() {
    START_TIMER(0);

    Reflection_Target targets[2];
    targets[0].filepath = PATH_SRC("game/entity_manager.h");
    targets[0].struct_count = 1;
    targets[0].struct_names[0] = S("Entity");
    targets[1].filepath = PATH_SRC("pch.h");
    targets[1].struct_count = 1;
    targets[1].struct_names[0] = S("Camera");
    
    String_Builder sb;
    sb.allocator = __temporary_allocator;

    codegen(sb, "#pragma once");

    static const auto to_pretty_code_path = [](String path) -> String {
        return make_string(path.data + 3, path.size - 3);
    };
    
    for (u32 i = 0; i < carray_count(targets); ++i) {
        const auto &target = targets[i];
        const auto code = read_text_file(target.filepath, __temporary_allocator);

        const auto path = to_pretty_code_path(target.filepath);
        codegen(sb, "\n// %S", path);

        for (u32 j = 0; j < target.struct_count; ++j) {
            const auto name = target.struct_names[j];
            const auto reflection = reflect_struct(code, name);
            if (reflection) {
                append(sb, reflection);
            }
        }
    }

    const auto output_path     = PATH_CODEGEN("reflection_meta.h");
    const auto output_contents = builder_to_string(sb);
    write_text_file(output_path, output_contents);

    for (u32 i = 0; i < carray_count(targets); ++i) {
        const auto &target = targets[i];
        const auto path = to_pretty_code_path(target.filepath);
        print("%S, ", path);

        for (u32 j = 0; j < target.struct_count; ++j) {
            const auto name = target.struct_names[j];
            print("%S, ", name);
        }
    }
    print("\n\n");
    
    const auto pretty_output_path = to_pretty_code_path(output_path);
    print("Generated %S %.2fms\n", pretty_output_path, CHECK_TIMER_MS(0));
}

s32 main() {
    set_process_cwd(get_process_directory());

    reflection_pass();

    return 0;
}
