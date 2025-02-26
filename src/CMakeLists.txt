add_executable(${PROJECT_NAME}
                camera.cpp camera.h flip_book.cpp flip_book.h font.cpp main.cpp pch.cpp profile.h stb_image.h stb_truetype.h log.h log.cpp
                # work_queue.cpp work_queue.h
                os/file.cpp os/file.h os/input.h os/memory.h os/thread.h os/time.h os/window.h
                math/matrix.cpp math/matrix.h math/math.cpp math/math.h math/quat.cpp math/quat.h math/vector.cpp math/vector.h
                audio/al.h audio/alc.h audio/alext.h audio/sound.cpp audio/sound.h
                editor/hot_reload.h
                game/entities.h game/game.cpp game/game.h game/player_control.cpp game/player_control.h game/world.cpp game/world.h
                editor/hot_reload.h
                render/gl.cpp render/glad.cpp render/render_registry.cpp render/shader.cpp render/text.cpp render/texture.cpp render/vertex.cpp)

if (WIN32)
   target_link_libraries(${PROJECT_NAME} PRIVATE opengl32.lib)
   target_link_libraries(${PROJECT_NAME} PRIVATE ${RUN_TREE_DIR}/openal32.lib)
   target_sources(${PROJECT_NAME} PRIVATE os/win32.cpp)
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
                               DIR_SOUNDS="${RUN_TREE_DIR}/data/sounds/")
