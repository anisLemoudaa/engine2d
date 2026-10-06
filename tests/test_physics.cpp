// Plain CHECK macro (assert() is compiled out in Release builds!)
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include "engine/Engine.hpp"
using namespace eng;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

static void fall_and_rest() {
    Engine e(800, 600, "t", true);
    auto ground = e.createBox(0, 5, 20, 1, false, sf::Color::White, 0, 1, 0.3f, 0.f);
    auto box    = e.createBox(0, 0, 1, 1, true, sf::Color::Red, 0, 1, 0.3f, 0.f);
    auto g0 = e.position(ground);
    float y0 = e.position(box).second;
    for (int i = 0; i < 30; ++i) e.step(Engine::DT);
    CHECK(e.position(box).second > y0 + 0.5f);               // gravity pulls the dynamic body down
    int hits = 0;
    e.setCollisionCallback([&](EntityId, EntityId) { ++hits; });
    for (int i = 0; i < 300; ++i) e.step(Engine::DT);
    CHECK(std::fabs(e.position(box).second - 4.0f) < 0.1f);  // rests on ground top (4.5) - half height (0.5)
    CHECK(hits >= 1);                                        // collision event delivered
    CHECK(e.position(ground) == g0);                         // static body never moves
}

static void collision_response_bounce() {
    Engine e(800, 600, "t", true);
    e.createBox(0, 5, 20, 1, false, sf::Color::White, 0, 1, 0.3f, 0.f);
    auto ball = e.createCircle(0, 0, 0.5f, true, sf::Color::Blue, 0, 1, 0.3f, 1.0f);   // restitution 1
    float prev = 0; bool bounced = false;
    for (int i = 0; i < 240; ++i) {
        e.step(Engine::DT);
        float vy = e.velocity(ball).second;
        if (prev > 2.0f && vy < -2.0f) bounced = true;       // velocity reversed by the contact
        prev = vy;
    }
    CHECK(bounced);
}

static void forces_and_velocity() {
    Engine e(800, 600, "t", true);
    e.setGravity(0, 0);
    auto b = e.createBox(0, 0, 1, 1, true, sf::Color::White, 0, 1, 0.3f, 0.f);
    e.applyImpulse(b, 5, 0);
    for (int i = 0; i < 60; ++i) e.step(Engine::DT);
    CHECK(e.position(b).first > 1.0f);
    e.setVelocity(b, 0, 0);
    e.applyTorque(b, 50);
    for (int i = 0; i < 30; ++i) e.step(Engine::DT);
    CHECK(std::fabs(e.angle(b)) > 0.01f);
}

static void errors_and_lifetime() {
    Engine e(800, 600, "t", true);
    bool threw = false;
    try { e.position(999999); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { e.createBox(0, 0, -1, 1, true, sf::Color::White, 0, 1, 0.3f, 0.f); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);

    auto a = e.createBox(0, 0, 1, 1, true, sf::Color::White, 0, 1, 0.3f, 0.f);
    CHECK(e.entityCount() == 1);
    e.destroy(a);
    e.destroy(a);                                             // double destroy must be harmless
    CHECK(e.entityCount() == 0);
    threw = false;
    try { e.position(a); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
}

static void destroy_inside_collision_callback() {
    Engine e(800, 600, "t", true);
    e.createBox(0, 5, 20, 1, false, sf::Color::White, 0, 1, 0.3f, 0.f);
    auto box = e.createBox(0, 3, 1, 1, true, sf::Color::White, 0, 1, 0.3f, 0.f);
    e.setCollisionCallback([&](EntityId a, EntityId b) { e.destroy(a); e.destroy(b); });
    for (int i = 0; i < 120; ++i) e.step(Engine::DT);         // must not crash (callbacks run after the world step)
    CHECK(e.entityCount() == 0);
    (void)box;
}

int main() {
    fall_and_rest();
    collision_response_bounce();
    forces_and_velocity();
    errors_and_lifetime();
    destroy_inside_collision_callback();
    if (failures) { std::fprintf(stderr, "%d check(s) failed\n", failures); return 1; }
    std::puts("physics OK");
    return 0;
}
