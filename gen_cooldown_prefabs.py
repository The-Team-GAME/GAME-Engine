"""
gen_cooldown_prefabs.py
Generate SkillDash.json, SkillSmash.json, SkillUppercut.json prefabs.

Sprite sheet: 2304x64, 36 frames of 64x64 each.
Frame layout:
  0..8   : CD countdown (0=9s, 8=1s)  -> states CD_9..CD_1
  9..34  : Transition animation        -> state CD_TRANSITION
  35     : Ready idle (looping)        -> state READY
"""

import json, os, math

TOTAL_FRAMES = 36
FRAME_W = 64 / 2304          # 1/36

def uv(i):
    """Return the 4-corner UV quad for frame i (bottom-left winding)."""
    x0 = i * FRAME_W
    x1 = x0 + FRAME_W
    return [
        {"x": x0, "y": 1.0},
        {"x": x1, "y": 1.0},
        {"x": x1, "y": 0.0},
        {"x": x0, "y": 0.0},
    ]

TEX_COORDS_LIST = [uv(i) for i in range(TOTAL_FRAMES)]

# ---- Animation helpers -------------------------------------------------------

FRAME_DURATION = 0.08          # seconds per frame

def anim(indices, looping):
    n = len(indices)
    durations   = [FRAME_DURATION] * n
    before_times = [i * FRAME_DURATION for i in range(n)]
    return {
        "atlas_indices":  indices,
        "before_times":   before_times,
        "durations":      durations,
        "is_looping":     looping,
        "startTime":      0.0,
        "totalDuration":  n * FRAME_DURATION,
    }

# ---- State builder -----------------------------------------------------------

def make_state(state_name, atlas_indices, looping,
               on_enter, on_update, on_exit, texture_path):
    return {
        "first":  state_name,
        "second": {
            "animation": {
                "animation": anim(atlas_indices, looping),
                "collider_data": None,
                "scale": {"x": 64.0, "y": 64.0},
                "texture": {"path": texture_path},
                "texture_atlas": {
                    "drawIndex": -1,
                    "tex_coords_list": TEX_COORDS_LIST,
                },
            },
            "audio": None,
            "fmod_audio": None,
            "name": state_name,
            "on_enter_callback": {"name": on_enter},
            "on_exit_callback":  {"name": on_exit},
            "on_update_callback": {"name": on_update},
        },
    }

# ---- Per-skill state list ----------------------------------------------------

def make_states(skill, texture_path):
    states = []

    cd_update         = f"{skill}_cd_update"
    ready_update      = f"{skill}_ready_update"
    transition_update = f"{skill}_transition_update"

    for sec in range(9, 0, -1):          # 9, 8, 7, ..., 1
        frame_idx = 9 - sec              # sec=9 -> frame 0, sec=1 -> frame 8
        states.append(make_state(
            f"CD_{sec}",
            [frame_idx],
            False,
            "",
            cd_update,
            "",
            texture_path,
        ))

    # CD_TRANSITION  frames 9..34
    states.append(make_state(
        "CD_TRANSITION",
        list(range(9, 35)),
        False,
        "",
        transition_update,
        "",
        texture_path,
    ))

    # READY  frame 35, looping — uses _ready_update (stays silent when cd==0)
    states.append(make_state(
        "READY",
        [35],
        True,
        "",
        ready_update,
        "",
        texture_path,
    ))

    return states

# ---- Top-level component list -----------------------------------------------

def make_prefab(blueprint_id, texture_path):
    states = make_states(blueprint_id.lower().replace("cooldown", "").strip("_"),
                         texture_path)

    state_machine = {
        "data": {
            "blueprint_id":       blueprint_id,
            "current_state_name": "READY",
            "states":             states,
        },
        "type": "Components::StateMachine",
    }

    components = [
        {"data": {},                                     "type": "Components::UI"},
        {"data": {"a":1.0,"b":1.0,"g":1.0,"r":1.0},    "type": "Components::Color"},
        {"data": {"path": texture_path},                 "type": "Components::Texture"},
        state_machine,
        {
            "data": {
                "cols": TOTAL_FRAMES,
                "drawIndex": -1,
                "rows": 1,
                "tex_coords_list": TEX_COORDS_LIST,
            },
            "type": "Components::TextureAtlas",
        },
        {
            "data": {
                "atlas_indices": [35],
                "before_times":  [0.0],
                "durations":     [FRAME_DURATION],
                "is_looping": True,
                "startTime": 0.0,
                "totalDuration": FRAME_DURATION,
            },
            "type": "Components::Animation",
        },
        {
            "data": {
                "position": {"x": 0.0, "y": 0.0},
                "rotation": 0.0,
                "scale":    {"x": 64.0, "y": 64.0},
            },
            "type": "Components::Transform2D",
        },
    ]

    return {
        "data": {
            "components": components,
            "layers": ["default"],
        },
        "dataType": "Prefab",
    }

# ---- Skills ------------------------------------------------------------------

SKILLS = [
    # (output filename, blueprint_id, texture path relative to images/)
    ("SkillDash",     "DashCooldown",     "images/skill_dash.png"),
    ("SkillSmash",    "SmashCooldown",    "images/skill_smash.png"),
    ("SkillUppercut", "UppercutCooldown", "images/skill_uppercut.png"),
]

# ---- Write files -------------------------------------------------------------

OUT_DIR = os.path.join(os.path.dirname(__file__), "Engine", "Assets", "prefabs")
os.makedirs(OUT_DIR, exist_ok=True)

for (filename, blueprint_id, texture_path) in SKILLS:
    # The skill name for callbacks is the part before "Cooldown" lowercased
    # e.g.  DashCooldown -> "dash"
    prefab = make_prefab(blueprint_id, texture_path)
    out_path = os.path.join(OUT_DIR, f"{filename}.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(prefab, f, indent=4)
    print(f"Written: {out_path}")

print("Done.")
