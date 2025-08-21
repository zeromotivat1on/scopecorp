set(GAME "scopecorp")

if (GFX_API IN_LIST GFX_API_LIST)
else ()
   message(FATAL_ERROR "Invalid GFX_API ${GFX_API}")
endif()

add_executable(${GAME})
set_target_properties(${GAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}"
    LIBRARY_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}"
    RUNTIME_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}")

target_sources(${GAME} PRIVATE
                asset.cpp basic.cpp camera.cpp collision.cpp font.cpp main.cpp
                # work_queue.cpp
                os/os.cpp
                audio/al.cpp audio/audio.cpp
                game/game.cpp
                editor/editor.cpp
                math/math.cpp
                render/render.cpp render/shader.cpp render/ui.cpp)
                
target_precompile_headers(${GAME} PUBLIC pch.h)
target_include_directories(${GAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(${GAME} PRIVATE ${RUN_TREE_DIR}/slang.lib)
add_custom_command(
    TARGET ${GAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RUN_TREE_DIR}/slang.dll
        $<TARGET_FILE_DIR:${GAME}>)
                
if (GFX_API STREQUAL OPEN_GL)
    target_sources(${GAME} PRIVATE render/gl.cpp render/glad.cpp)
    target_compile_definitions(${GAME} PRIVATE OPEN_GL)
endif()
   
if (WIN32)
    target_sources(${GAME} PRIVATE os/win32.cpp)
    target_link_libraries(${GAME} PRIVATE ${RUN_TREE_DIR}/openal32.lib shlwapi.lib)

    if (GFX_API STREQUAL OPEN_GL)
        target_sources(${GAME} PRIVATE render/win32_gl.cpp)
        target_link_libraries(${GAME} PRIVATE opengl32.lib)
    endif()
endif()

if (MSVC)
    target_compile_options(${GAME} PUBLIC /Zc:__cplusplus /W3)
endif()

if (DEVELOPER)
    target_compile_definitions(${GAME} PRIVATE DEVELOPER)
endif()

target_compile_definitions(${GAME} PRIVATE
                               _CRT_SECURE_NO_WARNINGS
                               # Platform
                               $<$<PLATFORM_ID:Windows>:WIN32>
                               # Configuration
                               $<$<CONFIG:Debug>:DEBUG>
                               $<$<CONFIG:Release>:RELEASE>
                               # Directories
                               DIR_RUN_TREE="${RUN_TREE_DIR}"
                               DIR_DATA="${RUN_TREE_DIR}/data/"
                               DIR_SHADERS="${RUN_TREE_DIR}/data/shaders/"
                               DIR_TEXTURES="${RUN_TREE_DIR}/data/textures/"
                               DIR_MATERIALS="${RUN_TREE_DIR}/data/materials/"
                               DIR_MESHES="${RUN_TREE_DIR}/data/meshes/"
                               DIR_SOUNDS="${RUN_TREE_DIR}/data/sounds/"
                               DIR_FONTS="${RUN_TREE_DIR}/data/fonts/"
                               DIR_FLIP_BOOKS="${RUN_TREE_DIR}/data/flip_books/"
                               DIR_LEVELS="${RUN_TREE_DIR}/data/levels/"
                               # Other
                               GAME_NAME="${GAME}"
                               GAME_VERSION="${CMAKE_PROJECT_VERSION}")
