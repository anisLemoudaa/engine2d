#include "engine/Engine.hpp"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace eng {

Engine::Engine(int w, int h, const std::string& title, bool headless)
    : world_(b2Vec2(0.f, 10.f)) {                 // y points down (screen space)
    world_.SetContactListener(&contacts_);
    if (!headless) {
        window_ = std::make_unique<sf::RenderWindow>(sf::VideoMode(w, h), title);
        window_->setVerticalSyncEnabled(true);
    }
    view_.setSize(float(w), float(h));
    view_.setCenter(w / 2.f, h / 2.f);
}

Engine::~Engine() = default;

b2Body* Engine::bodyOf(EntityId id) const {
    auto e = static_cast<entt::entity>(id);
    if (!reg_.valid(e) || !reg_.all_of<Body>(e)) throw std::out_of_range("invalid entity id");
    return reg_.get<Body>(e).body;
}

EntityId Engine::addBody(const Transform& t, const Renderable& r, bool dynamic, const b2Shape& shape,
                         float density, float friction, float restitution) {
    auto e = reg_.create();
    b2BodyDef bd;
    bd.type = dynamic ? b2_dynamicBody : b2_staticBody;
    bd.position.Set(t.x, t.y);
    bd.userData.pointer = static_cast<uintptr_t>(entt::to_integral(e));
    b2Body* body = world_.CreateBody(&bd);
    b2FixtureDef fd;
    fd.shape = &shape; fd.density = density; fd.friction = friction; fd.restitution = restitution;
    body->CreateFixture(&fd);
    reg_.emplace<Transform>(e, t);
    reg_.emplace<Renderable>(e, r);
    reg_.emplace<Body>(e, Body{body});
    ++count_;
    return entt::to_integral(e);
}

EntityId Engine::createBox(float x, float y, float w, float h, bool dynamic, sf::Color c,
                           int layer, float density, float friction, float restitution) {
    if (w <= 0 || h <= 0) throw std::invalid_argument("box size must be positive");
    b2PolygonShape s; s.SetAsBox(w / 2, h / 2);
    Renderable r; r.type = ShapeType::Box; r.w = w; r.h = h; r.color = c; r.layer = layer;
    return addBody({x, y, 0}, r, dynamic, s, density, friction, restitution);
}

EntityId Engine::createCircle(float x, float y, float radius, bool dynamic, sf::Color c,
                              int layer, float density, float friction, float restitution) {
    if (radius <= 0) throw std::invalid_argument("radius must be positive");
    b2CircleShape s; s.m_radius = radius;
    Renderable r; r.type = ShapeType::Circle; r.radius = radius; r.color = c; r.layer = layer;
    return addBody({x, y, 0}, r, dynamic, s, density, friction, restitution);
}

void Engine::destroy(EntityId id) {
    auto e = static_cast<entt::entity>(id);
    if (!reg_.valid(e)) return;
    if (reg_.all_of<Body>(e)) world_.DestroyBody(reg_.get<Body>(e).body);
    reg_.destroy(e);
    --count_;
}

void Engine::setGravity(float gx, float gy) { world_.SetGravity(b2Vec2(gx, gy)); }
void Engine::applyForce(EntityId id, float fx, float fy)   { bodyOf(id)->ApplyForceToCenter({fx, fy}, true); }
void Engine::applyImpulse(EntityId id, float ix, float iy) { bodyOf(id)->ApplyLinearImpulseToCenter({ix, iy}, true); }
void Engine::applyTorque(EntityId id, float t)             { bodyOf(id)->ApplyTorque(t, true); }
void Engine::setVelocity(EntityId id, float vx, float vy)  { bodyOf(id)->SetLinearVelocity({vx, vy}); }
std::pair<float, float> Engine::position(EntityId id) const { auto p = bodyOf(id)->GetPosition(); return {p.x, p.y}; }
std::pair<float, float> Engine::velocity(EntityId id) const { auto v = bodyOf(id)->GetLinearVelocity(); return {v.x, v.y}; }
float Engine::angle(EntityId id) const { return bodyOf(id)->GetAngle(); }

bool Engine::keyDown(const std::string& n) const {
    static const std::unordered_map<std::string, sf::Keyboard::Key> keys = {
        {"left", sf::Keyboard::Left}, {"right", sf::Keyboard::Right}, {"up", sf::Keyboard::Up},
        {"down", sf::Keyboard::Down}, {"space", sf::Keyboard::Space}, {"escape", sf::Keyboard::Escape},
        {"a", sf::Keyboard::A}, {"d", sf::Keyboard::D}, {"w", sf::Keyboard::W}, {"s", sf::Keyboard::S},
        {"r", sf::Keyboard::R}};
    if (!window_ || !window_->hasFocus()) return false;
    auto it = keys.find(n);
    return it != keys.end() && sf::Keyboard::isKeyPressed(it->second);
}

bool Engine::mouseDown(const std::string& b) const {
    if (!window_ || !window_->hasFocus()) return false;
    return sf::Mouse::isButtonPressed(b == "right" ? sf::Mouse::Right : sf::Mouse::Left);
}

std::pair<float, float> Engine::mouseWorld() const {
    if (!window_) return {0, 0};
    auto p = window_->mapPixelToCoords(sf::Mouse::getPosition(*window_), view_);
    return {p.x / PPM, p.y / PPM};
}

void Engine::setCamera(float cx, float cy, float zoom) {
    auto sz = window_ ? sf::Vector2f(window_->getSize()) : view_.getSize();
    view_.setSize(sz.x / zoom, sz.y / zoom);
    view_.setCenter(cx * PPM, cy * PPM);
}

void Engine::setSprite(EntityId id, const std::string& path) {
    auto e = static_cast<entt::entity>(id);
    if (!reg_.valid(e) || !reg_.all_of<Renderable>(e)) throw std::out_of_range("invalid entity id");
    auto it = textures_.find(path);
    if (it == textures_.end()) {
        auto tex = std::make_unique<sf::Texture>();
        if (!tex->loadFromFile(path)) throw std::runtime_error("cannot load texture: " + path);
        it = textures_.emplace(path, std::move(tex)).first;
    }
    reg_.get<Renderable>(e).texture = it->second.get();
}

std::array<int, 3> Engine::pixelAt(int x, int y) {
    if (!window_) throw std::runtime_error("pixel_at needs a window");
    auto sz = window_->getSize();
    if (x < 0 || y < 0 || x >= int(sz.x) || y >= int(sz.y)) throw std::out_of_range("pixel outside window");
    draw();
    sf::Texture t;
    if (!t.create(sz.x, sz.y)) throw std::runtime_error("cannot create capture texture");
    t.update(*window_);
    auto c = t.copyToImage().getPixel(x, y);
    return {c.r, c.g, c.b};
}

void Engine::step(float dt) {
    world_.Step(dt, 8, 3);
    reg_.view<Transform, Body>().each([](Transform& t, const Body& b) {
        auto p = b.body->GetPosition();
        t.x = p.x; t.y = p.y; t.angle = b.body->GetAngle();
    });
    auto begins = std::move(contacts_.begins);
    contacts_.begins.clear();
    if (auto cb = onCollision_)
        for (auto& [a, b] : begins)
            if (reg_.valid(static_cast<entt::entity>(a)) && reg_.valid(static_cast<entt::entity>(b)))
                cb(a, b);
    if (auto up = onUpdate_) up(dt);
}

void Engine::run(std::function<void(float)> onUpdate) {
    if (!window_) throw std::runtime_error("run() needs a window (not headless)");
    if (running_) throw std::runtime_error("run() is already running");
    struct Reset { Engine* e; ~Reset() { e->running_ = false; e->onUpdate_ = nullptr; } } reset{this};
    onUpdate_ = std::move(onUpdate);
    running_ = true;
    sf::Clock clock;
    float acc = 0.f;
    while (running_ && window_->isOpen()) {
        sf::Event ev;
        while (window_->pollEvent(ev))
            if (ev.type == sf::Event::Closed) window_->close();
        acc += std::min(clock.restart().asSeconds(), 0.25f);   // clamp avoids the "spiral of death"
        while (running_ && acc >= DT) { step(DT); acc -= DT; }
        render();
    }
}

void Engine::render() {
    draw();
    window_->display();
}

void Engine::draw() {
    window_->clear(sf::Color(25, 25, 35));
    window_->setView(view_);
    std::vector<std::pair<int, entt::entity>> order;
    for (auto e : reg_.view<Transform, Renderable>()) order.emplace_back(reg_.get<Renderable>(e).layer, e);
    std::stable_sort(order.begin(), order.end(), [](auto& a, auto& b) { return a.first < b.first; });

    for (auto& [layer, e] : order) {
        const auto& t = reg_.get<Transform>(e);
        const auto& r = reg_.get<Renderable>(e);
        const float deg = t.angle * 180.f / 3.14159265f;
        if (r.type == ShapeType::Box) {
            sf::RectangleShape s({r.w * PPM, r.h * PPM});
            s.setOrigin(r.w * PPM / 2, r.h * PPM / 2);
            s.setPosition(t.x * PPM, t.y * PPM); s.setRotation(deg); s.setFillColor(r.color);
            if (r.texture) { s.setTexture(r.texture, true); s.setFillColor(sf::Color::White); }
            window_->draw(s);
        } else {
            sf::CircleShape s(r.radius * PPM);
            s.setOrigin(r.radius * PPM, r.radius * PPM);
            s.setPosition(t.x * PPM, t.y * PPM); s.setRotation(deg); s.setFillColor(r.color);
            if (r.texture) { s.setTexture(r.texture, true); s.setFillColor(sf::Color::White); }
            window_->draw(s);
        }
    }
}
}
