"""Interactive demo. Works both as `python scripts/demo.py` (module mode) and via `engine_host scripts/demo.py`."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts"))
try:
    engine                                   # provided by engine_host (embedded mode)
except NameError:
    sys.path.insert(0, str(ROOT / "build" / "python"))
    import pyengine
    engine = pyengine.Engine(1280, 720, "Engine2D demo")

from level import load_level, LevelError

try:
    objs = load_level(engine, ROOT / "levels" / "level1.json")
except LevelError as e:
    sys.exit(f"Level error: {e}")
player = objs["player"]
hits = 0
frame = 0


def on_collision(a, b):
    global hits
    hits += 1


def update(dt):
    global frame
    frame += 1
    vx, vy = engine.velocity(player)
    if engine.key_down("left"):
        engine.set_velocity(player, -4.0, vy)
    elif engine.key_down("right"):
        engine.set_velocity(player, 4.0, vy)
    if engine.key_down("space") and abs(vy) < 0.05:     # jump only when ~grounded
        engine.apply_impulse(player, 0, -6)
    if engine.mouse_down("left") and frame % 6 == 0 and engine.entity_count < 400:   # spawn a ball at the cursor
        mx, my = engine.mouse_world()
        engine.create_circle(mx, my, 0.25)
    if engine.key_down("escape"):
        engine.quit()


engine.on_collision(on_collision)
engine.run(update)
print("collisions:", hits)
