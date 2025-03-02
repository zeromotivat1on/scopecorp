add_executable(${PROJECT_NAME}
                camera.cpp collision.cpp flip_book.cpp font.cpp log.cpp main.cpp memory_storage.cpp pch.cpp profile.cpp
                # work_queue.cpp work_queue.h
                os/file.cpp
                math/matrix.cpp math/math.cpp math/quat.cpp math/vector.cpp
                audio/al.cpp audio/sound.cpp
                game/game.cpp game/player_control.cpp game/world.cpp
                render/draw.cpp render/gl.cpp render/glad.cpp render/render_registry.cpp render/text.cpp render/vertex.cpp)

if (WIN32)
   target_link_libraries(${PROJECT_NAME} PRIVATE opengl32.lib)
   target_link_libraries(${PROJECT_NAME} PRIVATE ${RUN_TREE_DIR}/openal32.lib)
   target_sources(${PROJECT_NAME} PRIVATE os/win32.cpp os/win32_gl.cpp)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}"
        LIBRARY_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}"
        RUNTIME_OUTPUT_DIRECTORY "${RUN_TREE_DIR}/build/${CMAKE_SYSTEM_NAME}/${CONFIG_TYPE}")
        
target_precompile_headers(${PROJECT_NAME} PUBLIC pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/src")

if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /Zc:__cplusplus /W3)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
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
