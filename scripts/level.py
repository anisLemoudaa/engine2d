"""Level loader: JSON (built in) or YAML (if PyYAML is installed). Raises LevelError on bad input."""
import json
from pathlib import Path


class LevelError(Exception):
    pass


def _read(path):
    p = Path(path)
    try:
        text = p.read_text(encoding="utf-8")
    except OSError as e:
        raise LevelError(f"cannot read level '{path}': {e}") from e
    try:
        if p.suffix in (".yml", ".yaml"):
            import yaml  # pip install pyyaml
            return yaml.safe_load(text)
        return json.loads(text)
    except ImportError as e:
        raise LevelError("YAML levels need: pip install pyyaml") from e
    except Exception as e:  # json.JSONDecodeError / yaml.YAMLError
        raise LevelError(f"'{path}' is not valid {p.suffix.lstrip('.')}: {e}") from e


def _nums(obj, key, n, idx):
    v = obj.get(key)
    if not (isinstance(v, (list, tuple)) and len(v) == n and all(isinstance(x, (int, float)) for x in v)):
        raise LevelError(f"object #{idx}: '{key}' must be a list of {n} numbers")
    return v


def _build_object(engine, obj, i):
    if not isinstance(obj, dict):
        raise LevelError(f"object #{i}: must be an object")
    x, y = _nums(obj, "pos", 2, i)
    common = dict(dynamic=not obj.get("static", False),
                  color=tuple(_nums(obj, "color", 3, i)) if "color" in obj else (255, 255, 255),
                  layer=int(obj.get("layer", 0)),
                  friction=obj.get("friction", 0.3), restitution=obj.get("restitution", 0.2))
    kind = obj.get("type")
    if kind == "box":
        w, h = _nums(obj, "size", 2, i)
        return engine.create_box(x, y, w, h, **common)
    if kind == "circle":
        r = obj.get("radius")
        if not isinstance(r, (int, float)):
            raise LevelError(f"object #{i}: 'radius' must be a number")
        return engine.create_circle(x, y, r, **common)
    raise LevelError(f"object #{i}: unknown type {kind!r}")


def load_level(engine, path):
    """Builds the level and returns {name: entity_id} for named objects.
    Any malformed input raises LevelError and leaves no half-built level behind."""
    data = _read(path)
    if not isinstance(data, dict) or not isinstance(data.get("objects"), list):
        raise LevelError("level must be an object with an 'objects' list")
    cam = data.get("camera")
    if cam is not None and not isinstance(cam, dict):
        raise LevelError("'camera' must be an object")
    gx, gy = _nums(data, "gravity", 2, "level") if "gravity" in data else (0, 10)

    created, named = [], {}
    try:
        engine.set_gravity(gx, gy)
        if cam:
            cx, cy = _nums(cam, "center", 2, "camera")
            engine.set_camera(cx, cy, cam.get("zoom", 1.0))
        for i, obj in enumerate(data["objects"]):
            try:
                ent = _build_object(engine, obj, i)
            except (ValueError, TypeError, AttributeError) as e:   # bad types / engine-side validation
                raise LevelError(f"object #{i}: {e}") from e
            created.append(ent)
            if "name" in obj:
                named[obj["name"]] = ent
    except Exception:
        for ent in created:
            try:
                engine.destroy(ent)
            except Exception:
                pass
        raise
    return named
