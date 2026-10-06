import json, pathlib, sys
import pytest
ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "scripts"))
from level import load_level, LevelError


class Fake:
    def __init__(self): self.ents, self.n, self.gravity, self.cam = {}, 0, None, None
    def set_gravity(self, x, y): self.gravity = (x, y)
    def set_camera(self, x, y, z): self.cam = (x, y, z)
    def _add(self, kind, **kw):
        if kw.get("w", 1) <= 0 or kw.get("h", 1) <= 0: raise ValueError("size")
        self.n += 1; self.ents[self.n] = kind; return self.n
    def create_box(self, x, y, w, h, **kw): return self._add("box", w=w, h=h)
    def create_circle(self, x, y, radius, **kw): return self._add("circle")
    def destroy(self, i): self.ents.pop(i, None)


def test_real_level():
    f = Fake(); objs = load_level(f, ROOT / "levels" / "level1.json")
    assert len(f.ents) == 8 and "player" in objs and f.gravity == (0, 10) and f.cam == (8, 4.5, 1.0)


@pytest.mark.parametrize("content", ["{oops", "[]", '{"objects": 5}', '{"objects":[1]}',
    '{"objects":[{"type":"box","pos":[0],"size":[1,1]}]}',
    '{"objects":[{"type":"circle","pos":[0,0]}]}',
    '{"objects":[{"type":"box","pos":[0,0],"size":[-1,1]}]}',
    '{"objects":[{"type":"box","pos":[0,0],"size":[1,1],"layer":"top"}]}',
    '{"objects":[],"camera":5}', '{"objects":[],"camera":{"center":[1]}}',
    '{"objects":[{"type":"box","pos":[0,0],"size":[1,1],"color":[1,2]}]}'])
def test_bad_levels(tmp_path, content):
    p = tmp_path / "l.json"; p.write_text(content)
    with pytest.raises(LevelError): load_level(Fake(), p)


def test_rollback(tmp_path):
    p = tmp_path / "l.json"
    p.write_text('{"objects":[{"type":"box","pos":[0,0],"size":[1,1]},{"type":"nope","pos":[0,0]}]}')
    f = Fake()
    with pytest.raises(LevelError): load_level(f, p)
    assert f.ents == {}
