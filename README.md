# Engine2D — محرك ألعاب ثنائي الأبعاد

C++17 (Box2D + SFML + EnTT) مع سكربتات Python (pybind11). كل المكتبات تُحمَّل تلقائيًا عبر CMake FetchContent.

> **حالة التحقق:** كُتب الكود دون إمكانية ترجمته في بيئة التطوير الأولى، والمرجع الفعلي للنجاح هو **GitHub Actions** (مهمّتان: `build-test` و `sanitizers`). إن فشل أي شيء في أول تشغيل فهذا متوقع ويُصلح من سجل الـ CI.
> منصة CI المُعدّة: Ubuntu 24.04. Windows/macOS لم تُجرَّب (قد تحتاج `SFML_STATIC` وتعديلات بسيطة).

## البناء (Linux)
```bash
sudo apt install cmake ninja-build python3-dev xvfb libx11-dev libxrandr-dev libxcursor-dev \
     libudev-dev libgl1-mesa-dev libfreetype-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure       # اختبارات C++
python3 -m pytest tests -v                        # اختبارات Python + المحمّل
```

## التشغيل — طريقتان
| الوضع | الأمر | من يملك المحرك؟ |
|---|---|---|
| **Python مدمج داخل C++** | `build/engine_host scripts/demo.py` | C++ ينشئ `Engine` ويضعه في السكربت باسم `engine` |
| **وحدة Python** | `python3 scripts/demo.py` | بايثون تستورد `pyengine` وتنشئ `Engine` |

التحكم في الديمو: ←/→ حركة، Space قفز، زر الماوس الأيسر ينشئ كرات، Esc خروج.

## هيكل المشروع
| المسار | الدور |
|---|---|
| `engine/include/engine/Components.hpp` | مكونات ECS (بيانات فقط): `Transform`, `Renderable`, `Body` |
| `engine/include/engine/Engine.hpp`, `engine/src/Engine.cpp` | النواة: حلقة ثابتة، فيزياء، رسم بطبقات، كاميرا، إدخال، Sprites |
| `bindings/bind_engine.hpp`, `bindings/bindings.cpp` | تعريف واجهة Python (مشترك بين الوحدة والمضيف المدمج) |
| `app/main.cpp` | `engine_host`: مفسّر Python مدمج داخل C++ |
| `scripts/level.py` | تحميل المستويات (JSON، أو YAML مع PyYAML) + أخطاء `LevelError` |
| `scripts/demo.py`, `scripts/smoke_runtime.py` | ديمو تفاعلي / اختبار تشغيلي يستخدمه CI |
| `levels/level1.json`, `assets/crate.png` | مستوى وصورة تجريبية |
| `tests/` | `test_physics.cpp` (C++)، `test_python.py` (الوحدة)، `test_level_fake.py` (المحمّل بلا C++) |
| `.github/workflows/ci.yml` | بناء + اختبارات + اختبار تشغيلي تحت xvfb + ASan/UBSan |

## المعمارية
```
Python script ──pybind11──► Engine (C++) ── fixed step 60Hz
                               │
        ┌──────────────────────┼─────────────────────┐
        ▼                      ▼                     ▼
  Box2D World ──sync──► EnTT registry ─────────► draw()
 (يملك الأجسام)      Transform/Renderable     (فرز بالطبقات + View الكاميرا)
        │
  ContactRecorder ──► on_collision(a,b) ► Python (بعد الخطوة، فيجوز حذف الكيانات داخله)
```
- الكيان يصل لبايثون كرقم `uint32` (لا مؤشرات)، ومعرّف خاطئ يعطي `IndexError`.
- الفيزياء بالأمتار، الرسم بالبكسل (`PPM=80`)، محور y لأسفل. الكاميرا الافتراضية: مركزها (8, 4.5) م.
- الحلقة: `accumulator` بخطوة ثابتة مع سقف 0.25 ث. `quit()` يوقف الحلقة فورًا ولو بقيت خطوات متراكمة.
- استثناء داخل `update` يخرج من `run()` ويُعاد ضبط حالة المحرك.
- المحرك يجب أن يُدمَّر قبل المفسّر (يمسك دوال بايثون): `engine_host` يتكفل بذلك.

## واجهة Python
`Engine(width, height, title, headless)` · `create_box / create_circle(..., dynamic, color, layer, density, friction, restitution)` ·
`set_sprite(id, path)` · `destroy` · `set_gravity` · `apply_force/impulse/torque` · `set_velocity` · `position/velocity/angle` ·
`key_down(name)` · `mouse_down` · `mouse_world` · `set_camera(x, y, zoom)` · `on_collision(fn)` · `run(update_fn)` · `step()` · `quit()` · `pixel_at(x, y)` · `entity_count`

## صيغة المستوى (JSON)
```json
{"gravity":[0,10], "camera":{"center":[8,4.5],"zoom":1},
 "objects":[{"name":"player","type":"box","pos":[3,7.5],"size":[0.8,0.8],"color":[80,200,120],"layer":2},
            {"type":"circle","pos":[7.5,0.5],"radius":0.45,"restitution":0.7}]}
```
أي خطأ في الملف يرفع `LevelError` برسالة واضحة ويتراجع عن الكائنات المنشأة.

## ما يغطيه الاختبار (وما لا يغطيه)
- **C++ (`test_physics`)**: سقوط بالجاذبية، استقرار فوق جسم ثابت، الجسم الثابت لا يتحرك، ارتداد (استجابة التصادم)، قوى/عزم، أخطاء الإدخال، حذف مزدوج، الحذف داخل callback.
- **Python**: وحدة المحرك، المحمّل (13 حالة معيبة)، حذف داخل callback.
- **`smoke_runtime.py` تحت xvfb**: Python مدمج، نافذة حقيقية، ترتيب الطبقات (بقراءة ألوان بكسلات)، مستطيل/دائرة/Sprite، تحريك جسم من بايثون أثناء `run()`، أحداث التصادم، 120 خطوة بالضبط.
- **لا يُختبر آليًا**: الضغط الفعلي على لوحة المفاتيح والماوس (يُجرَّب يدويًا بـ `demo.py`).
- **ASan/UBSan** على اختبار C++ للتحقق من التسريبات.

## أخطاء شائعة
Box2D: لا تعدّل العالم داخل callback الفيزياء (لذلك تُرسَل الأحداث بعد الخطوة) · أجسام أصغر من ~0.1م أو أكبر من ~10م تسوء محاكاتها ·
لا تحتفظ بمراجع لمكونات EnTT بعد إنشاء/حذف كيانات · وحدة `.so` يجب أن تطابق إصدار بايثون الذي يستوردها.

## خارطة الطريق
Sprite sheets/أطلس ← Joints و Sensors ← Raycast ← Debug draw للفيزياء ← صوت ← ImGui ← محرر مستويات بـ Python.
