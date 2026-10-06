#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>
#include "engine/Engine.hpp"

namespace bind_detail {
namespace py = pybind11;
using eng::Engine;
using Color = std::tuple<int, int, int>;
inline sf::Color col(const Color& c) {
    auto ok = [](int v) { if (v < 0 || v > 255) throw std::invalid_argument("color channel must be 0..255"); return (sf::Uint8)v; };
    return sf::Color(ok(std::get<0>(c)), ok(std::get<1>(c)), ok(std::get<2>(c)));
}

inline void bind_engine(py::module_& m) {
    m.doc() = "2D engine: C++ core (Box2D + SFML + EnTT), scripted from Python";
    py::class_<Engine>(m, "Engine")
        .def(py::init<int, int, std::string, bool>(),
             py::arg("width") = 1280, py::arg("height") = 720,
             py::arg("title") = "Engine2D", py::arg("headless") = false)
        .def("create_box",
             [](Engine& e, float x, float y, float w, float h, bool dynamic, Color c, int layer,
                float density, float friction, float restitution) {
                 return e.createBox(x, y, w, h, dynamic, col(c), layer, density, friction, restitution);
             },
             py::arg("x"), py::arg("y"), py::arg("w"), py::arg("h"), py::arg("dynamic") = true,
             py::arg("color") = Color(255, 255, 255), py::arg("layer") = 0,
             py::arg("density") = 1.f, py::arg("friction") = 0.3f, py::arg("restitution") = 0.2f)
        .def("create_circle",
             [](Engine& e, float x, float y, float r, bool dynamic, Color c, int layer,
                float density, float friction, float restitution) {
                 return e.createCircle(x, y, r, dynamic, col(c), layer, density, friction, restitution);
             },
             py::arg("x"), py::arg("y"), py::arg("radius"), py::arg("dynamic") = true,
             py::arg("color") = Color(255, 255, 255), py::arg("layer") = 0,
             py::arg("density") = 1.f, py::arg("friction") = 0.3f, py::arg("restitution") = 0.4f)
        .def("destroy", &Engine::destroy)
        .def("set_gravity", &Engine::setGravity)
        .def("apply_force", &Engine::applyForce)
        .def("apply_impulse", &Engine::applyImpulse)
        .def("apply_torque", &Engine::applyTorque)
        .def("set_velocity", &Engine::setVelocity)
        .def("position", &Engine::position)
        .def("velocity", &Engine::velocity)
        .def("angle", &Engine::angle)
        .def("key_down", &Engine::keyDown)
        .def("mouse_down", &Engine::mouseDown, py::arg("button") = "left")
        .def("mouse_world", &Engine::mouseWorld)
        .def("set_camera", &Engine::setCamera, py::arg("x"), py::arg("y"), py::arg("zoom") = 1.f)
        .def("set_sprite", &Engine::setSprite)
        .def("pixel_at", &Engine::pixelAt)
        .def("has_window", &Engine::hasWindow)
        .def("on_collision", &Engine::setCollisionCallback)
        .def("step", &Engine::step, py::arg("dt") = Engine::DT)
        .def("run", &Engine::run)
        .def("quit", &Engine::quit)
        .def_property_readonly("entity_count", &Engine::entityCount);
}
}  // namespace bind_detail
using bind_detail::bind_engine;
