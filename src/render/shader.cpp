#include "pch.h"
#include "render/r_shader.h"

#include "log.h"
#include "asset.h"

#include "os/file.h"

// Slang stuff to do.
#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

static Slang::ComPtr<slang::IGlobalSession> Global_session;
static Slang::ComPtr<slang::ISession>       Local_session;

void r_init_shader_compiler() {
    slang::createGlobalSession(Global_session.writeRef());

    slang::TargetDesc target_desc = {};    
    target_desc.format = SLANG_SPIRV;
    target_desc.profile = Global_session->findProfile("spirv_1_5");
    target_desc.flags = 0;

    slang::SessionDesc session_desc = {};
    session_desc.targets = &target_desc;
    session_desc.targetCount = 1;
    session_desc.compilerOptionEntryCount = 0;

    Global_session->createSession(session_desc, Local_session.writeRef());

    log("Initialized slang compiler");
}

void r_destroy_shader_compiler() {
    
}

Buffer r_compile_shader(Arena &a, String path) {
    char cpath[MAX_PATH_LENGTH];
    str_c(path, COUNT(cpath), cpath);
    
    Slang::ComPtr<slang::IBlob> diagnostics;
    auto *module = Local_session->loadModule(cpath, diagnostics.writeRef());

    if (diagnostics) {
        log("Slang diagnostic after module load: %s", (const char *)diagnostics->getBufferPointer());
    }
    
    if (!module) {
        error("Failed to load shader module %s", cpath);        
        return BUFFER_NONE;
    }

    constexpr String name = S("vertex_main");
    Slang::ComPtr<slang::IEntryPoint> entry_point;
    module->findEntryPointByName(name.value, entry_point.writeRef());

    if (!entry_point) {
        error("Failed to find entry point %s in shader %s", name.value, cpath);
        return BUFFER_NONE;
    }
    
    slang::IComponentType *components[] = { module, entry_point };
    Slang::ComPtr<slang::IComponentType> program;
    Local_session->createCompositeComponentType(components, COUNT(components), program.writeRef());

    if (!program) {
        error("Failed to create shader program from %s", cpath);
        return BUFFER_NONE;
    }
    
    Slang::ComPtr<slang::IComponentType> linked_program;
    program->link(linked_program.writeRef(), diagnostics.writeRef());

    if (diagnostics) {
        log("Slang diagnostic after program link: %s", (const char *)diagnostics->getBufferPointer());
    }

    if (!linked_program) {
        error("Failed to link shader program from %s", cpath);
        return BUFFER_NONE;
    }

    s32 entry_point_index = 0; // only one entry point
    s32 target_index      = 0; // only one target
    Slang::ComPtr<slang::IBlob> kernel_blob;
    linked_program->getEntryPointCode(entry_point_index, target_index,
                                      kernel_blob.writeRef(), diagnostics.writeRef());
    
    if (diagnostics) {
        log("Slang diagnostic after compiled code retreival: %s", (const char *)diagnostics->getBufferPointer());
    }

    if (!kernel_blob) {
        error("Failed to get compiled shader code from %s", cpath);
        return BUFFER_NONE;
    }

    // According to docs, slang caches all data in session object, so we don't
    // necessary need to push binary to arena, but we just want to store all
    // data used by the engine in our own memory storages.
    
    Buffer r;
    r.size = kernel_blob->getBufferSize();
    r.data = (u8 *)push(a, r.size);
    
    mem_copy(r.data, kernel_blob->getBufferPointer(), r.size);

    return r;
}

constexpr String DECL_BEGIN_VERTEX   = S("#begin vertex");
constexpr String DECL_END_VERTEX     = S("#end vertex");
constexpr String DECL_BEGIN_FRAGMENT = S("#begin fragment");
constexpr String DECL_END_FRAGMENT   = S("#end fragment");

static inline String get_begin_decl(Shader_Type type) {
    switch (type) {
    case SHADER_VERTEX:   return DECL_BEGIN_VERTEX;
    case SHADER_FRAGMENT: return DECL_BEGIN_FRAGMENT;
    default:
        error("Unknown shader type %d", type);
        return STRING_NONE;
    }
}

static inline String get_end_decl(Shader_Type type) {
    switch (type) {
    case SHADER_VERTEX:   return DECL_END_VERTEX;
    case SHADER_FRAGMENT: return DECL_END_FRAGMENT;
    default:
        error("Unknown shader type %d", type);
        return STRING_NONE;
    }
}

String parse_shader_region(Arena &a, String s, Shader_Type type) {
    Assert(type < SHADER_COUNT);
    
    const String decl_begin = get_begin_decl(type);
    const String decl_end   = get_end_decl(type);
    
    String begin = str_slice(s, decl_begin);
    String end   = str_slice(s, decl_end);
    
    if (!is_valid(begin)) {
        error("Failed to find shader region begin declaration '%s'", decl_begin.value);
        return STRING_NONE;
    }

    if (!is_valid(end)) {
        error("Failed to find shader region end declaration '%s'", decl_end.value);
        return STRING_NONE;
    }
    
    begin = str_slice(begin, ASCII_NEW_LINE, S_INDEX_PLUS_ONE_BIT);
    const auto region = String { begin.value, (u64)(end.value - begin.value) };
    
    return str_copy(a, region);
}

String parse_shader_includes(Arena &a, String s) {
    constexpr String INCLUDE = S("#include");

    Scratch scratch = local_scratch();
    defer { release(scratch); };
    
    String t = s;
    String inc = str_slice(t, INCLUDE);
    
    String_Builder sb;
    
    // @Todo: handle recursive includes.
    while (is_valid(inc)) {
        String inc_left = str_slice(t, INCLUDE, S_LEFT_SLICE_BIT | S_INDEX_MINUS_ONE_BIT);
        str_build(a, sb, inc_left);

        String inc_path = str_slice(inc,      '"', S_INDEX_PLUS_ONE_BIT);
        inc_path        = str_slice(inc_path, '"', S_LEFT_SLICE_BIT | S_INDEX_MINUS_ONE_BIT);
        
        String_Builder sb_path;
        str_build(scratch.arena, sb_path, DIR_SHADERS);
        str_build(scratch.arena, sb_path, inc_path);

        String path = str_build_finish(scratch.arena, sb_path);
        String inc_contents = os_read_text_file(scratch.arena, path);
        str_build(a, sb, inc_contents);

        // @Robustness: this may lead to undesired behavior if several
        // include statements will be on same line.
        t   = str_slice(inc, '\n', S_INDEX_PLUS_ONE_BIT);
        inc = str_slice(t, INCLUDE);
    }

    str_build(a, sb, t); // rest of shader code

    String r = str_build_finish(a, sb);
    if (r.length > R_Shader::MAX_FILE_SIZE) {
        error("Final shader source size %llu with parsed includes exceeds max shade file size %llu", r.length, R_Shader::MAX_FILE_SIZE);
    }
    
    return r;
}
