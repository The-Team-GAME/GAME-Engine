"""
Fix the Bro/BroSleeping split:
  - Add "BroSleeping" entity: Brosleepy3-Sheet.png, 8-frame idle looping
      Start      -> visible, looping animation
      Transition -> instantly hide (Color.a = 0)
      Menu       -> stay hidden
  - Update "Bro" entity (Carriage-Sheet):
      Color.a = 0 initially (hidden)
      Start      -> stay hidden (Color.a prop_anim = 0)
      Transition -> show (Color.a = 1) + play frames 0-8
      Menu       -> stay visible (Color.a = 1), frame 8
"""

import json, os

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

def alpha_prop_anim(a_val, dur=0.001, looping=False):
    return {
        "duration"  : dur,
        "is_looping": looping,
        "tracks": [{
            "easing": "Linear",
            "keyframes": [
                {"time": 0.0,  "value": a_val},
                {"time": dur,  "value": a_val},
            ],
            "target_component" : "Components::Color",
            "target_field_path": "a",
        }],
    }

# ── Carriage-Sheet constants ───────────────────────────────────────
FRAME_COUNT = 9
FRAME_DUR   = 0.125
TOTAL_DUR   = FRAME_COUNT * FRAME_DUR   # 1.125s

def tex_coords_9():
    out = []
    for i in range(FRAME_COUNT):
        u0 = i / FRAME_COUNT
        u1 = (i + 1) / FRAME_COUNT
        out.append([{"x": u0, "y": 1.0}, {"x": u1, "y": 1.0}, {"x": u1, "y": 0.0}, {"x": u0, "y": 0.0}])
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

def carriage_state_anim(anim_data):
    return {
        "animation"    : anim_data,
        "collider_data": None,
        "scale"        : {"x": 1600.0, "y": 900.0},
        "texture"      : {"path": "images/NewMainMenu/Carriage-Sheet.png"},
        "texture_atlas": {"drawIndex": -1, "tex_coords_list": tex_coords_9()},
    }

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 1.  Update "Bro" entity (Carriage-Sheet): hidden in Start, visible in Transition+Menu
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
bro = find_entity("Bro")

# Initial color: hidden
find_comp(bro, "Components::Color")["data"]["a"] = 0.0

sm_bro = find_comp(bro, "Components::StateMachine")["data"]

start_st = find_state(sm_bro, "Start")
start_st["property_animation"] = alpha_prop_anim(0.0)

trans_st = find_state(sm_bro, "Transition")
trans_st["animation"] = carriage_state_anim(make_anim(list(range(9)), False))
# Keep the existing dummy pos prop_anim but add a Color.a=1 track to it
trans_st["property_animation"] = {
    "duration"  : TOTAL_DUR,
    "is_looping": False,
    "tracks": [
        {   # Hold position (prevents instant Transition→Menu)
            "easing": "Linear",
            "keyframes": [{"time": 0.0, "value": 800.0}, {"time": TOTAL_DUR, "value": 800.0}],
            "target_component" : "Components::Transform2D",
            "target_field_path": "position.x",
        },
        {   # Show immediately
            "easing": "Linear",
            "keyframes": [{"time": 0.0, "value": 1.0}, {"time": TOTAL_DUR, "value": 1.0}],
            "target_component" : "Components::Color",
            "target_field_path": "a",
        },
    ],
}

menu_st = find_state(sm_bro, "Menu")
menu_st["animation"] = carriage_state_anim(make_anim([8], True))
menu_st["property_animation"] = alpha_prop_anim(1.0)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 2.  Add "BroSleeping" entity: looping idle, hidden in Transition + Menu
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

# Brosleepy3-Sheet.png: 5120x640 = 8 frames of 640x640
SLEEP_FRAMES = 8
SLEEP_DUR    = 0.1

def sleep_tex_coords():
    out = []
    for i in range(SLEEP_FRAMES):
        u0 = i / SLEEP_FRAMES
        u1 = (i + 1) / SLEEP_FRAMES
        out.append([{"x": u0, "y": 1.0}, {"x": u1, "y": 1.0}, {"x": u1, "y": 0.0}, {"x": u0, "y": 0.0}])
    return out

def sleep_anim_override():
    n = SLEEP_FRAMES
    anim = {
        "atlas_indices" : list(range(n)),
        "before_times"  : [i * SLEEP_DUR for i in range(n)],
        "durations"     : [SLEEP_DUR] * n,
        "is_looping"    : True,
        "startTime"     : 0.0,
        "totalDuration" : n * SLEEP_DUR,
    }
    return {
        "animation"    : anim,
        "collider_data": None,
        "scale"        : {"x": 450.0, "y": 450.0},
        "texture"      : {"path": "images/UI/MainMenu (Start)/Brosleepy3-Sheet.png"},
        "texture_atlas": {"drawIndex": -1, "tex_coords_list": sleep_tex_coords()},
    }

used_ids = {e["id"] for e in entities}
bro_sleeping_id = max(used_ids) + 1

# Insert right before "Bro" entity so it renders behind it in z-order
bro_idx = next(i for i, e in enumerate(entities) if e["name"] == "Bro")

# Whole train id for parenting
whole_train = find_entity("Whole train")
wt_id = whole_train["id"]
find_comp(whole_train, "Components::Children")["data"]["child_ids"].append(bro_sleeping_id)

bro_sleeping_entity = {
    "id"    : bro_sleeping_id,
    "name"  : "BroSleeping",
    "layers": ["default", "main_menu_4"],
    "components": [
        {
            "type": "Components::Animation",
            "data": {
                "atlas_indices" : list(range(SLEEP_FRAMES)),
                "before_times"  : [i * SLEEP_DUR for i in range(SLEEP_FRAMES)],
                "durations"     : [SLEEP_DUR] * SLEEP_FRAMES,
                "is_looping"    : True,
                "startTime"     : 0.0,
                "totalDuration" : SLEEP_FRAMES * SLEEP_DUR,
            }
        },
        {"type": "Components::Children", "data": {"child_ids": []}},
        {"type": "Components::Color",    "data": {"a": 1.0, "b": 1.0, "g": 1.0, "r": 1.0}},
        {
            "type": "Components::Parent",
            "data": {
                "local_offset"      : {"x": 1354.7939453125, "y": 774.271728515625},
                "offset_initialized": True,
                "parent_id"         : wt_id,
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
                        "first": "Start",
                        "second": {
                            "animation"          : sleep_anim_override(),
                            "audio"              : None,
                            "name"               : "Start",
                            "on_enter_callback"  : {"name": "main_menu_start_on_enter"},
                            "on_exit_callback"   : {"name": "main_menu_start_on_exit"},
                            "on_update_callback" : {"name": "main_menu_start_on_update"},
                            "property_animation" : None,
                        }
                    },
                    {
                        "first": "Transition",
                        "second": {
                            "animation"          : None,
                            "audio"              : None,
                            "name"               : "Transition",
                            "on_enter_callback"  : {"name": "main_menu_transition_on_enter"},
                            "on_exit_callback"   : {"name": "main_menu_transition_on_exit"},
                            "on_update_callback" : {"name": "main_menu_transition_on_update"},
                            "property_animation" : alpha_prop_anim(0.0),
                        }
                    },
                    {
                        "first": "Menu",
                        "second": {
                            "animation"          : None,
                            "audio"              : None,
                            "name"               : "Menu",
                            "on_enter_callback"  : {"name": "main_menu_menu_on_enter"},
                            "on_exit_callback"   : {"name": "main_menu_menu_on_exit"},
                            "on_update_callback" : {"name": "main_menu_menu_on_update"},
                            "property_animation" : alpha_prop_anim(0.0),
                        }
                    },
                ]
            }
        },
        {
            "type": "Components::Texture",
            "data": {"path": "images/UI/MainMenu (Start)/Brosleepy3-Sheet.png"}
        },
        {
            "type": "Components::TextureAtlas",
            "data": {"drawIndex": 0, "tex_coords_list": sleep_tex_coords()}
        },
        {
            "type": "Components::Transform2D",
            "data": {
                "position": {"x": 1354.7939453125, "y": 774.271728515625},
                "rotation": 0.0,
                "scale"   : {"x": 450.0, "y": 450.0},
            }
        },
    ]
}

entities.insert(bro_idx, bro_sleeping_entity)

with open(JSON_PATH, "w") as f:
    json.dump(data, f, indent=4)

print(f"Done. BroSleeping (id={bro_sleeping_id}) added before Bro. Bro now hidden in Start state.")
