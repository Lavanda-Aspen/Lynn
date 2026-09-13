cmake_minimum_required(VERSION 3.15)
project(Lynn C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# --- raylib ---
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SUPPORT_MODULE_RMODELS OFF CACHE BOOL "" FORCE)   
set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "" FORCE)    

# --- SDL2 ---
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.9
    GIT_SHALLOW TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_VIDEO OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib SDL2)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        raylib 
        SDL2-static
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} 
        PRIVATE 
            winmm 
            gdi32 
            imm32 
            ole32 
            oleaut32 
            version 
            uuid 
            setupapi
    )
    target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--subsystem,console")
endif()

if (APPLE)
    target_link_libraries(${PROJECT_NAME} 
        PRIVATE
            "-framework IOKit" 
            "-framework Cocoa" 
            "-framework OpenGL"
    )
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} 
        PRIVATE 
            m 
            pthread 
            dl
    )
endif()