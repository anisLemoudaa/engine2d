import pathlib, sys
import pytest

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "build" / "python"))
sys.path.insert(0, str(ROOT / "scripts"))
import pyengine   # must fail loudly if the build is broken (no silent skip)
from level import load_level, LevelError


def make():
    return pyengine.Engine(headless=True)


def test_fall_and_land():
    e = make()
    e.create_box(0, 5, 20, 1, dynamic=False)
    b = e.create_box(0, 0, 1, 1)
    for _ in range(300):
        e.step()
    assert abs(e.position(b)[1] - 4.0) < 0.1


def test_python_moves_body():
    e = make()
    e.set_gravity(0, 0)
    b = e.create_circle(0, 0, 0.5)
    e.set_velocity(b, 2, 0)
    for _ in range(60):
        e.step()
    assert e.position(b)[0] > 1.5


def test_errors():
    e = make()
    with pytest.raises(IndexError):
        e.position(12345)
    with pytest.raises(ValueError):
        e.create_box(0, 0, 0, 1)
    with pytest.raises(ValueError):
        e.create_box(0, 0, 1, 1, color=(999, 0, 0))
    with pytest.raises(RuntimeError):
        e.run(lambda dt: None)          # headless engine has no window


def test_collision_callback_may_destroy():
    e = make()
    e.create_box(0, 5, 20, 1, dynamic=False)
    b = e.create_box(0, 3, 1, 1)
    e.on_collision(lambda x, y: e.destroy(b))
    for _ in range(120):
        e.step()
    assert e.entity_count == 1


def test_level_loads():
    e = make()
    objs = load_level(e, ROOT / "levels" / "level1.json")
    assert "player" in objs and e.entity_count == 8


def test_level_errors(tmp_path):
    e = make()
    bad = tmp_path / "bad.json"
    bad.write_text("{not json")
    with pytest.raises(LevelError):
        load_level(e, bad)
    with pytest.raises(LevelError):
        load_level(e, tmp_path / "missing.json")
    half = tmp_path / "half.json"
    half.write_text('{"objects":[{"type":"box","pos":[0,0],"size":[1,1]},{"type":"triangle","pos":[0,0]}]}')
    with pytest.raises(LevelError):
        load_level(e, half)
    assert e.entity_count == 0          # first object was rolled back
