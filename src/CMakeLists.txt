add_executable(${PROJECT_NAME}
                array.h camera.cpp camera.h file.cpp file.h font.cpp font.h input.h main.cpp matrix.cpp matrix.h memory.h my_math.cpp my_math.h pch.cpp profile.h quat.cpp quat.h stb_image.h stb_truetype.h thread.h time.h vector.cpp vector.h window.h
                # work_queue.cpp work_queue.h
                editor/hot_reload.h
                game/entities.h game/game.cpp game/game.h game/player_control.cpp game/player_control.h game/world.cpp game/world.h
                editor/hot_reload.h
                render/gl.cpp render/gl.h render/glad.cpp render/glad.h render/khrplatform.h render/shader.h render/shader.cpp render/texture.h render/texture.cpp)

if (WIN32)
   target_link_libraries(${PROJECT_NAME} PRIVATE opengl32.lib)
   target_sources(${PROJECT_NAME} PRIVATE os/wglext.h os/win32_gl.cpp os/win32_hot_reload.cpp os/win32_input.cpp os/win32_memory.cpp os/win32_thread.cpp os/win32_time.cpp os/win32_window.cpp os/win32_window.h)
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
                               # Platform
                               $<$<PLATFORM_ID:Windows>:WIN32>
                               # Configuration
                               $<$<CONFIG:Debug>:DEBUG>
                               $<$<CONFIG:Release>:RELEASE>
                               # Directories
                               DIR_SHADERS="${RUN_TREE_DIR}/data/shaders/"
                               DIR_TEXTURES="${RUN_TREE_DIR}/data/textures/")
