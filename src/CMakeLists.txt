set(GAME_EXE "scopecorp")

if (GFX_API IN_LIST GFX_API_LIST)
else ()
   message(FATAL_ERROR "Invalid GFX_API")
endif()

add_executable(${GAME_EXE}
                assert.cpp camera.cpp collision.cpp flip_book.cpp font.cpp log.cpp main.cpp memory_storage.cpp pch.cpp profile.cpp
                # work_queue.cpp work_queue.h
                os/file.cpp
                math/matrix.cpp math/math.cpp math/quat.cpp math/vector.cpp
                audio/al.cpp audio/sound.cpp
                game/game.cpp game/player_control.cpp game/world.cpp
                editor/hot_reload.cpp
                render/debug_draw.cpp render/draw.cpp render/gl.cpp render/glad.cpp render/render_registry.cpp render/shader.cpp render/text.cpp render/vertex.cpp)

target_precompile_headers(${GAME_EXE} PUBLIC pch.h)
target_include_directories(${GAME_EXE} PRIVATE "${PROJECT_SOURCE_DIR}/src")

if (WIN32)
   target_sources(${GAME_EXE} PRIVATE os/win32.cpp)
   target_link_libraries(${GAME_EXE} PRIVATE ${RUN_TREE_DIR}/openal32.lib)
   
   if (GFX_API STREQUAL OPEN_GL)
      target_sources(${GAME_EXE} PRIVATE os/win32_gl.cpp)
      target_link_libraries(${GAME_EXE} PRIVATE opengl32.lib)
      target_compile_definitions(${GAME_EXE} PRIVATE OPEN_GL)
   endif()
endif()

if (MSVC)
   target_compile_options(${GAME_EXE} PUBLIC /Zc:__cplusplus /W3)
endif()

if (INCLUDE_TRACY)
   target_sources(${GAME_EXE} PRIVATE tracy/TracyClient.cpp)
   target_compile_definitions(${GAME_EXE} PRIVATE TRACY_ENABLE)
endif()

set_target_properties(${GAME_EXE} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}"
        LIBRARY_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}"
        RUNTIME_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}")

target_compile_definitions(${GAME_EXE} PRIVATE
                               _CRT_SECURE_NO_WARNINGS
                               # Platform
                               $<$<PLATFORM_ID:Windows>:WIN32>
                               # Configuration
                               $<$<CONFIG:Debug>:DEBUG>
                               $<$<CONFIG:Release>:RELEASE>
                               # Directories
                               DIR_SHADERS="${RUN_TREE_DIR}/data/shaders/"
                               DIR_TEXTURES="${RUN_TREE_DIR}/data/textures/"
                               DIR_SOUNDS="${RUN_TREE_DIR}/data/sounds/"
                               DIR_FONTS="${RUN_TREE_DIR}/data/fonts/")
