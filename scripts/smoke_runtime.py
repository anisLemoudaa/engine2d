"""Run by CI:  xvfb-run engine_host scripts/smoke_runtime.py
Proves: embedded Python, window, level load, render order (layers), sprite, camera,
Python-created entities, Python moving a body at runtime, collision callbacks, fixed-step loop."""
from pathlib import Path
from level import load_level

ROOT = Path(__file__).resolve().parent.parent
assert engine.has_window()
objs = load_level(engine, ROOT / "levels" / "level1.json")      # JSON level + camera center (8, 4.5)
player = objs["player"]

# --- rendering checks (camera center (8,4.5), 80 px/m  ->  screen (640,360))
engine.create_box(2, 2, 1.5, 1.5, dynamic=False, color=(255, 0, 0), layer=5)   # created FIRST, higher layer
engine.create_box(2, 2, 1.0, 1.0, dynamic=False, color=(0, 0, 255), layer=1)   # created LAST, lower layer
assert engine.pixel_at(160, 160) == [255, 0, 0], "layer ordering broken (red must be on top)"
assert engine.pixel_at(640, 696) == [90, 90, 110], "static ground rectangle not drawn"
crate = engine.create_box(13, 2, 1, 1, dynamic=False)
engine.set_sprite(crate, str(ROOT / "assets" / "crate.png"))
assert engine.pixel_at(1040, 160) == [200, 50, 200], "sprite not drawn"
ball = engine.create_circle(11, 2, 0.5, dynamic=False, color=(10, 220, 10))
assert engine.pixel_at(11 * 80, 160) == [10, 220, 10], "circle not drawn"

# --- error handling surfaces as Python exceptions
for bad in (lambda: engine.position(987654), lambda: engine.create_box(0, 0, -1, 1),
            lambda: engine.set_sprite(crate, "/nonexistent.png")):
    try:
        bad()
        raise AssertionError("expected an exception")
    except (IndexError, ValueError, RuntimeError):
        pass
assert engine.key_down("space") in (True, False) and engine.key_down("nonsense") is False

# --- runtime control from Python
state = {"frames": 0, "hits": 0}
x0 = engine.position(player)[0]
engine.on_collision(lambda a, b: state.__setitem__("hits", state["hits"] + 1))


def update(dt):
    state["frames"] += 1
    engine.set_velocity(player, 3.0, engine.velocity(player)[1])   # Python moves the body while running
    if state["frames"] >= 120:
        engine.quit()


engine.run(update)
assert state["frames"] == 120, state
assert engine.position(player)[0] > x0 + 1.5, "Python did not move the body"
assert state["hits"] > 0, "no collision events"
print("embedded runtime smoke OK")
