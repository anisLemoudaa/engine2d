#pragma once
#include <box2d/box2d.h>
#include <SFML/Graphics/Color.hpp>

namespace sf { class Texture; }

namespace eng {
enum class ShapeType { Box, Circle };

struct Transform  { float x = 0, y = 0, angle = 0; };            // meters / radians
struct Renderable { ShapeType type = ShapeType::Box;
                    float w = 1, h = 1, radius = 0.5f;
                    sf::Color color{255, 255, 255};
                    int layer = 0;
                    const sf::Texture* texture = nullptr; };   // non-owning; Engine owns textures
struct Body       { b2Body* body = nullptr; };                    // owned by b2World
}
