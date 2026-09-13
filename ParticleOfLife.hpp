#pragma once

#include "raylib.h"
#include <vector>
#include <mutex>
#include <thread>
#include <iostream>
#include <cmath>

typedef struct Particle{
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> vel_x;
    std::vector<float> vel_y;
    std::vector<int> type;

    Particle(size_t size): x(size), y(size), vel_x(size), vel_y(size), type(size) {}
} Particle;

const int   MASS[6]   = {1, 2, 4, 8, 16, 32};
const float RADIUS[6] = {4, 4, 4, 4, 4, 4};
const Color COLOR[6] = {
    (Color){255, 255, 255, 255},
    (Color){255, 180, 40, 255},
    (Color){60, 140, 255, 255},
    (Color){255, 40, 90, 255}, 
    (Color){160, 40, 255, 255},
    (Color){0, 255, 0, 255},
};


class POL
{
private:
    bool is_running = false;
    std::mutex mtx;
    std::thread worker_thread;


    size_t MAX_PARTICLES;
    size_t NUM_TYPES;
    float R_MAX;
    float BETA;
    float FORCE_SCALE;
    float FRICTION;
    float MIN_DIST;
    uint16_t WIDTH;
    uint16_t HEIGHT;

    std::vector<float> ANGLE;
    std::vector<float> COS;
    std::vector<float> SIN;

    Particle particles;

    void Init() {
        for (int i = 0; i < NUM_TYPES * NUM_TYPES; i++) {
            ANGLE[i] = 0.0f;
            COS[i] = 1.0f;
            SIN[i] = 0.0f;
        }

        for (int i = 0; i < MAX_PARTICLES; i++) {
            particles.x[i] = static_cast<float>(GetRandomValue(20, WIDTH - 20));
            particles.y[i] = static_cast<float>(GetRandomValue(20, HEIGHT - 20));
            particles.vel_x[i] = 0.0f;
            particles.vel_y[i] = 0.0f;
            particles.type[i] = GetRandomValue(0, NUM_TYPES - 1);
        }
    }

    inline float Force_Func(float r) {
        if (r < BETA) {
            return r / BETA - 1.0f;
        } else if (r < 1.0f) {
            return (1.0f - fabsf(2.0f*r - 1.0f - BETA) / (1.0f - BETA));
        }
        return 0.0f;
    }

    void to_center() {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            particles.vel_x[i] += 0.075 * -(particles.x[i] - WIDTH/2);
            particles.vel_y[i] += 0.075 * -(particles.y[i] - HEIGHT/2);
        }
    }

    void Update_Physic(float dt) {
        float fx[MAX_PARTICLES] = {0};
        float fy[MAX_PARTICLES] = {0};

        for (int i = 0; i < MAX_PARTICLES; i++) {
            for (int j = 0; j < MAX_PARTICLES; j++) {
                if (i == j) continue;

                float dx = particles.x[j] - particles.x[i];
                float dy = particles.y[j] - particles.y[i];
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist > R_MAX) continue;

                int id = particles.type[i] * NUM_TYPES + particles.type[j];

                float r = dist / R_MAX;
                float F = Force_Func(r) * FORCE_SCALE;

                float ux = dx / dist, uy = dy / dist;

                if (r > BETA) {
                    ux = ux * COS[id]  - uy * SIN[id];
                    uy = uy * COS[id]  + ux * SIN[id];
                }
                
                fx[i] += F * ux;
                fy[i] += F * uy;
            }
        }

        for (int i = 0; i < MAX_PARTICLES; i++) {

            particles.vel_x[i] += (fx[i] / MASS[particles.type[i]]) * dt;
            particles.vel_y[i] += (fy[i] / MASS[particles.type[i]]) * dt;

            particles.vel_x[i] *= FRICTION;
            particles.vel_y[i] *= FRICTION;

            particles.x[i] += particles.vel_x[i] * dt;
            particles.y[i] += particles.vel_y[i] * dt;

            if (particles.x[i] < -40) {
                particles.x[i] = WIDTH + 30;
            } else if (particles.x[i] > WIDTH + 40) {
                particles.x[i] = -30;
            }

            if (particles.y[i] < -40) {
                particles.y[i] = HEIGHT + 30;
            } else if (particles.y[i] > HEIGHT + 40) particles.y[i] = -30;
        }
    }

    void Render() {
        InitWindow(WIDTH, HEIGHT, "Lynn");
        SetTargetFPS(60);

        while (!WindowShouldClose() && is_running) {
            float dt = GetFrameTime();
            if (dt > 0.05f) dt = 0.05f;

            to_center();

            Update_Physic(dt);

            BeginDrawing();
            ClearBackground(BLACK);

            BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < MAX_PARTICLES; i++) {
                Color c = COLOR[particles.type[i]];
                c.a = 220;
                DrawCircleV((Vector2){particles.x[i], particles.y[i]}, RADIUS[particles.type[i]], c);
            }
            EndBlendMode();

            DrawFPS(10, 10);
            EndDrawing();
        }
        CloseWindow();
        is_running = false;
    }

public:
    POL(size_t MP, size_t NT, float RM, float beta, float FS, float friction, float MD, uint16_t width, uint16_t height):
    MAX_PARTICLES(MP), NUM_TYPES(NT), R_MAX(RM), BETA(beta), FORCE_SCALE(FS), FRICTION(friction), MIN_DIST(MD), WIDTH(width), HEIGHT(height),
    particles(MAX_PARTICLES), ANGLE(NUM_TYPES * NUM_TYPES), COS(NUM_TYPES * NUM_TYPES), SIN(NUM_TYPES * NUM_TYPES) {
        Init();
    }

    void start() {
        is_running = true;
        worker_thread = std::thread(&POL::Render, this);
    }

    void stop() {
        if (is_running) {
            is_running = false;
            if (worker_thread.joinable()) {
                worker_thread.join();
            }
        }
    }

    bool Active() const { return is_running;}

    void Drift_Angle(const std::vector<float> delta_angle) {
        std::lock_guard lock(mtx);

        if (delta_angle.size() != NUM_TYPES * NUM_TYPES) {
            std::cerr << "Invalid vector size for Drift_Angle" << std::endl;
            return;
        }

        for (int i = 0; i < NUM_TYPES * NUM_TYPES; i++) {

            ANGLE[i] += delta_angle[i] / 50.0f;

            if (ANGLE[i] > PI) {
                ANGLE[i] -= 2 * PI;
            } else if (ANGLE[i] < -PI) {
                ANGLE[i] += 2 * PI;
            }

            COS[i] = std::cos(ANGLE[i]);
            SIN[i] = std::sin(ANGLE[i]);
        }
    }

    ~POL() {
        stop();
    }
    
};