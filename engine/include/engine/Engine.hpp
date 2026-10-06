#pragma once
#include <entt/entt.hpp>
#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Components.hpp"

namespace eng {

using EntityId = std::uint32_t;

struct ContactRecorder : b2ContactListener {
    std::vector<std::pair<EntityId, EntityId>> begins;
    void BeginContact(b2Contact* c) override {
        auto a = c->GetFixtureA()->GetBody()->GetUserData().pointer;
        auto b = c->GetFixtureB()->GetBody()->GetUserData().pointer;
        begins.emplace_back(static_cast<EntityId>(a), static_cast<EntityId>(b));
    }
};

class Engine {
public:
    static constexpr float PPM = 80.f;       // pixels per meter
    static constexpr float DT  = 1.f / 60.f; // fixed timestep

    Engine(int width, int height, const std::string& title, bool headless = false);
    ~Engine();

    // --- entities ---
    EntityId createBox(float x, float y, float w, float h, bool dynamic, sf::Color c,
                       int layer, float density, float friction, float restitution);
    EntityId createCircle(float x, float y, float r, bool dynamic, sf::Color c,
                          int layer, float density, float friction, float restitution);
    void destroy(EntityId id);

    // --- physics ---
    void setGravity(float gx, float gy);
    void applyForce(EntityId id, float fx, float fy);
    void applyImpulse(EntityId id, float ix, float iy);
    void applyTorque(EntityId id, float t);
    void setVelocity(EntityId id, float vx, float vy);
    std::pair<float, float> position(EntityId id) const;
    std::pair<float, float> velocity(EntityId id) const;
    float angle(EntityId id) const;

    // --- input / camera ---
    bool keyDown(const std::string& name) const;
    bool mouseDown(const std::string& button) const;
    std::pair<float, float> mouseWorld() const;
    void setCamera(float cx, float cy, float zoom);
    void setSprite(EntityId id, const std::string& path);   // throws on load failure
    std::array<int, 3> pixelAt(int x, int y);               // renders a frame, reads one pixel (for render tests)
    bool hasWindow() const { return window_ != nullptr; }
    void clearCallbacks() { onUpdate_ = nullptr; onCollision_ = nullptr; }

    // --- loop ---
    void run(std::function<void(float)> onUpdate);   // opens the window loop
    void step(float dt);                              // one fixed step (also used headless)
    void quit() { running_ = false; }
    void setCollisionCallback(std::function<void(EntityId, EntityId)> cb) { onCollision_ = std::move(cb); }
    std::size_t entityCount() const { return count_; }

private:
    b2Body* bodyOf(EntityId id) const;
    EntityId addBody(const Transform&, const Renderable&, bool dynamic, const b2Shape&,
                     float density, float friction, float restitution);
    void draw();     // draws the scene into the back buffer
    void render();   // draw + display

    entt::registry reg_;
    ContactRecorder contacts_;
    b2World world_;
    std::unique_ptr<sf::RenderWindow> window_;
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures_;  // destroyed before window_
    sf::View view_;
    bool running_ = false;
    std::size_t count_ = 0;   // live entities
    std::function<void(float)> onUpdate_;
    std::function<void(EntityId, EntityId)> onCollision_;
};
}
