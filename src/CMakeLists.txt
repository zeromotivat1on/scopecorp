set(GAME_EXE "scopecorp")

if (GFX_API IN_LIST GFX_API_LIST)
else ()
   message(FATAL_ERROR "Invalid GFX_API ${GFX_API}")
endif()

add_executable(${GAME_EXE})
set_target_properties(${GAME_EXE} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}"
        LIBRARY_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}"
        RUNTIME_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}")

target_sources(${GAME_EXE} PRIVATE
                asset.cpp camera.cpp collision.cpp flip_book.cpp font.cpp main.cpp pch.cpp profile.cpp str.cpp
                # work_queue.cpp
                os/os.cpp
                audio/al.cpp audio/audio.cpp
                game/game.cpp
                editor/editor.cpp
                math/matrix.cpp math/math.cpp math/quat.cpp math/vector.cpp
                render/render.cpp render/shader.cpp)
                
target_precompile_headers(${GAME_EXE} PUBLIC pch.h)
target_include_directories(${GAME_EXE} PRIVATE "${PROJECT_SOURCE_DIR}/src")

if (GFX_API STREQUAL OPEN_GL)
   target_sources(${GAME_EXE} PRIVATE render/gl.cpp render/glad.cpp)
   target_compile_definitions(${GAME_EXE} PRIVATE OPEN_GL)
endif()
   
if (WIN32)
   target_sources(${GAME_EXE} PRIVATE os/win32.cpp)
   target_link_libraries(${GAME_EXE} PRIVATE ${RUN_TREE_DIR}/openal32.lib shlwapi.lib)

   if (GFX_API STREQUAL OPEN_GL)
      target_sources(${GAME_EXE} PRIVATE os/win32_gl.cpp)
      target_link_libraries(${GAME_EXE} PRIVATE opengl32.lib)
   endif()
endif()

if (MSVC)
   target_compile_options(${GAME_EXE} PUBLIC /Zc:__cplusplus /W3)
endif()

if (DEVELOPER)
   target_compile_definitions(${GAME_EXE} PRIVATE DEVELOPER)
endif()

target_compile_definitions(${GAME_EXE} PRIVATE
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
                               GAME_NAME="${GAME_EXE}")
