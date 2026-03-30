"""
Split "Carriage" entity into:
  - Carriage (main_menu_3): static Carriage.png, no atlas/animation
  - Bro     (main_menu_4): Carriage-Sheet.png 9-frame atlas animation
               Start      -> frame 0, looping
               Transition -> frames 0-8, non-looping + prop-anim timer
               Menu       -> frame 8, looping
"""

import json, copy, os

JSON_PATH = os.path.join(os.path.dirname(__file__), "..", "Engine", "Assets", "levels", "MainMenu.json")

with open(JSON_PATH) as f:
    data = json.load(f)

entities = data["data"]["entities"]

def find_entity(name):
    for e in entities:
        if e["name"] == name:
            return e
    raise KeyError(name)

def find_comp(ent, t):
    for c in ent["components"]:
        if c["type"] == t:
            return c
    return None

def find_state(sm, sname):
    for s in sm["states"]:
        if s["first"] == sname:
            return s["second"]
    return None

# ── helpers ────────────────────────────────────────────────────────────
FRAME_COUNT = 9
FRAME_DUR   = 0.125          # 8 fps
TOTAL_DUR   = FRAME_COUNT * FRAME_DUR  # 1.125 s

def tex_coords_9():
    out = []
    for i in range(FRAME_COUNT):
        u0 = i / FRAME_COUNT
        u1 = (i + 1) / FRAME_COUNT
        out.append([
            {"x": u0, "y": 1.0},
            {"x": u1, "y": 1.0},
            {"x": u1, "y": 0.0},
            {"x": u0, "y": 0.0},
        ])
    return out

def make_anim(indices, looping):
    n = len(indices)
    return {
        "atlas_indices" : indices,
        "before_times"  : [i * FRAME_DUR for i in range(n)],
        "durations"     : [FRAME_DUR] * n,
        "is_looping"    : looping,
        "startTime"     : 0.0,
        "totalDuration" : n * FRAME_DUR,
    }

def state_anim_override(anim_data):
    return {
        "animation"    : anim_data,
        "collider_data": None,
        "scale"        : {"x": 1600.0, "y": 900.0},
        "texture"      : {"path": "images/NewMainMenu/Carriage-Sheet.png"},
        "texture_atlas": {
            "drawIndex"      : -1,
            "tex_coords_list": tex_coords_9(),
        },
    }

TC9 = tex_coords_9()

# ══════════════════════════════════════════════════════════════════════
# 1.  Carriage entity → static Carriage.png
# ══════════════════════════════════════════════════════════════════════
carriage = find_entity("Carriage")

# Remove TextureAtlas and Animation components
carriage["components"] = [
    c for c in carriage["components"]
    if c["type"] not in ("Components::TextureAtlas", "Components::Animation")
]

# Fix texture
find_comp(carriage, "Components::Texture")["data"]["path"] = "images/NewMainMenu/Carriage.png"

# Clear state animation overrides so state machine doesn't swap the texture
sm_c = find_comp(carriage, "Components::StateMachine")["data"]
for s in sm_c["states"]:
    s["second"]["animation"] = None

# ══════════════════════════════════════════════════════════════════════
# 2.  Find insertion point and next entity id
# ══════════════════════════════════════════════════════════════════════
carriage_idx = next(i for i, e in enumerate(entities) if e["name"] == "Carriage")
# pick an id that doesn't collide
used_ids = {e["id"] for e in entities}
new_id = max(used_ids) + 1

# ══════════════════════════════════════════════════════════════════════
# 3.  Build "Bro" entity
# ══════════════════════════════════════════════════════════════════════
#  Parent is the same as Carriage (Whole train, id=1963)
carriage_parent = find_comp(carriage, "Components::Parent")["data"]
parent_id = carriage_parent["parent_id"]

bro_entity = {
    "id"    : new_id,
    "name"  : "Bro",
    "layers": ["default", "main_menu_4"],
    "components": [
        {
            "type": "Components::Animation",
            "data": make_anim([0], True)   # default: frame 0 looping
        },
        {
            "type": "Components::Children",
            "data": {"child_ids": []}
        },
        {
            "type": "Components::Color",
            "data": {"a": 1.0, "b": 1.0, "g": 1.0, "r": 1.0}
        },
        {
            "type": "Components::Parent",
            "data": {
                "local_offset"      : {"x": 800.0, "y": 450.0},
                "offset_initialized": True,
                "parent_id"         : parent_id,
                "prev_parent_pos"   : {"x": 0.0, "y": 0.0},
            }
        },
        {
            "type": "Components::StateMachine",
            "data": {
                "blueprint_id"         : "MainMenu",
                "current_state_name"   : "Start",
                "property_anim_playing": False,
                "states": [
                    {
                        "first": "Menu",
                        "second": {
                            "animation"           : state_anim_override(make_anim([8], True)),
                            "audio"               : None,
                            "name"                : "Menu",
                            "on_enter_callback"   : {"name": "main_menu_menu_on_enter"},
                            "on_exit_callback"    : {"name": "main_menu_menu_on_exit"},
                            "on_update_callback"  : {"name": "main_menu_menu_on_update"},
                            "property_animation"  : None,
                        }
                    },
                    {
                        "first": "Transition",
                        "second": {
                            "animation"           : state_anim_override(make_anim(list(range(9)), False)),
                            "audio"               : None,
                            "name"                : "Transition",
                            "on_enter_callback"   : {"name": "main_menu_transition_on_enter"},
                            "on_exit_callback"    : {"name": "main_menu_transition_on_exit"},
                            "on_update_callback"  : {"name": "main_menu_transition_on_update"},
                            # Dummy prop-anim keeps is_property_anim_playing() == true
                            # until the 9-frame animation finishes.
                            "property_animation"  : {
                                "duration"  : TOTAL_DUR,
                                "is_looping": False,
                                "tracks": [{
                                    "easing": "Linear",
                                    "keyframes": [
                                        {"time": 0.0,       "value": 800.0},
                                        {"time": TOTAL_DUR, "value": 800.0},
                                    ],
                                    "target_component" : "Components::Transform2D",
                                    "target_field_path": "position.x",
                                }],
                            },
                        }
                    },
                    {
                        "first": "Start",
                        "second": {
                            "animation"           : state_anim_override(make_anim([0], True)),
                            "audio"               : None,
                            "name"                : "Start",
                            "on_enter_callback"   : {"name": "main_menu_start_on_enter"},
                            "on_exit_callback"    : {"name": "main_menu_start_on_exit"},
                            "on_update_callback"  : {"name": "main_menu_start_on_update"},
                            "property_animation"  : None,
                        }
                    },
                ]
            }
        },
        {
            "type": "Components::Texture",
            "data": {"path": "images/NewMainMenu/Carriage-Sheet.png"}
        },
        {
            "type": "Components::TextureAtlas",
            "data": {
                "drawIndex"      : 0,
                "tex_coords_list": TC9,
            }
        },
        {
            "type": "Components::Transform2D",
            "data": {
                "position": {"x": 800.0, "y": 450.0},
                "rotation": 0.0,
                "scale"   : {"x": 1600.0, "y": 900.0},
            }
        },
    ]
}

# Register id in parent's Children component (Whole train)
whole_train = find_entity("Whole train")
wt_children = find_comp(whole_train, "Components::Children")
if wt_children:
    wt_children["data"]["child_ids"].append(new_id)

# Also register in root level entity's children
root = find_entity("Entity 2125")
# Bro's parent is Whole train (1963), which is already a child of root — no change needed.

# Insert Bro right after Carriage
entities.insert(carriage_idx + 1, bro_entity)

with open(JSON_PATH, "w") as f:
    json.dump(data, f, indent=4)

print(f"Done. Bro entity (id={new_id}) added on main_menu_4, Carriage now uses Carriage.png.")
