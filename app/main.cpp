// Host executable: C++ owns the Engine and embeds a Python interpreter to run a script.
#include <pybind11/embed.h>
#include <filesystem>
#include <iostream>
#include "bind_engine.hpp"

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(pyengine, m) { bind_engine(m); }

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: engine_host script.py [--headless]\n"; return 2; }
    const bool headless = argc > 2 && std::string(argv[2]) == "--headless";
    int rc = 0;
    try {
        py::scoped_interpreter interpreter;
        {   // Engine must die BEFORE the interpreter (it may hold Python callables)
            eng::Engine engine(1280, 720, "Engine2D (embedded Python)", headless);
            try {
                py::module_::import("pyengine");   // registers the Engine class
                auto dir = std::filesystem::absolute(argv[1]).parent_path().string();
                py::module_::import("sys").attr("path").attr("insert")(0, dir);
                py::dict globals = py::module_::import("__main__").attr("__dict__");
                globals["engine"] = py::cast(&engine, py::return_value_policy::reference);
                py::eval_file(argv[1], globals);
            } catch (const py::error_already_set& e) {
                std::cerr << "Python error:\n" << e.what() << "\n";
                rc = 1;
            }
            engine.clearCallbacks();
        }
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        rc = 1;
    }
    return rc;
}
