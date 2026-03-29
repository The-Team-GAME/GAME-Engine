"""
One-shot script to update MainMenu.json for the new sprites in images/NewMainMenu/.

Changes:
  1. BackSky: new texture, wider scale (8000), updated scroll animation keyframes
  2. BackCarriage (Window): new texture path
  3. Train Overlay → Carriage-Sheet.png with 9-frame TextureAtlas + per-state animation
  4. Hide unused entities: player_sleeping, Red, Any key, Monkey, Bro (clear layers)
  5. MainMenuImpact + all buttons: texture paths → NewMainMenu/Buttons/
"""

import json, sys, os, copy

JSON_PATH = os.path.join(os.path.dirname(__file__), "..", "Engine", "Assets", "levels", "MainMenu.json")

with open(JSON_PATH, "r") as f:
    data = json.load(f)

entities = data["data"]["entities"]

# ---------- helpers ----------
def find_entity(name):
    for e in entities:
        if e["name"] == name:
            return e
    raise KeyError(f"Entity '{name}' not found")

def find_component(entity, comp_type):
    for c in entity["components"]:
        if c["type"] == comp_type:
            return c["data"]
    return None

def add_component(entity, comp_type, comp_data):
    entity["components"].append({"type": comp_type, "data": comp_data})

def find_state(sm_data, state_name):
    for entry in sm_data["states"]:
        if entry["first"] == state_name:
            return entry["second"]
    return None

def make_tex_coords_9():
    """Generate UV coordinates for a 9-frame horizontal sprite sheet."""
    coords = []
    for i in range(9):
        u0 = i / 9.0
        u1 = (i + 1) / 9.0
        coords.append([
            {"x": u0, "y": 1.0},
            {"x": u1, "y": 1.0},
            {"x": u1, "y": 0.0},
            {"x": u0, "y": 0.0},
        ])
    return coords

def make_animation_data(indices, dur_per_frame, looping):
    n = len(indices)
    total = n * dur_per_frame
    before = [i * dur_per_frame for i in range(n)]
    durations = [dur_per_frame] * n
    return {
        "atlas_indices": indices,
        "before_times": before,
        "durations": durations,
        "is_looping": looping,
        "startTime": 0.0,
        "totalDuration": total,
    }

def make_state_animation_override(texture_path, tex_coords, anim_data, scale_x, scale_y):
    """Build the 'animation' block used in a state machine state."""
    return {
        "animation": anim_data,
        "collider_data": None,
        "scale": {"x": scale_x, "y": scale_y},
        "texture": {"path": texture_path},
        "texture_atlas": {
            "drawIndex": -1,
            "tex_coords_list": tex_coords,
        },
    }

# ================================================================
# 1. BackSky  (entity "Back sky", id 1875)
# ================================================================
sky = find_entity("Back sky")
# Texture
find_component(sky, "Components::Texture")["path"] = "images/NewMainMenu/BackSky.png"
# Scale: old 4800 → new 8000 (proportional to 9600/5760)
t2d = find_component(sky, "Components::Transform2D")
t2d["scale"]["x"] = 8000.0
# Update initial position to centre the wider image
t2d["position"]["x"] = 4000.0

sm = find_component(sky, "Components::StateMachine")
# Start state property animation: scroll from 4000 → -2400 over 60s
start_state = find_state(sm, "Start")
start_state["property_animation"]["duration"] = 60.0
start_state["property_animation"]["is_looping"] = True
kf = start_state["property_animation"]["tracks"][0]["keyframes"]
kf[0]["value"] = 4000.0;  kf[0]["time"] = 0.0
kf[1]["value"] = -2400.0; kf[1]["time"] = 60.0

# Menu state property animation: same range, faster (25.6s)
menu_state = find_state(sm, "Menu")
menu_state["property_animation"]["duration"] = 25.6
menu_state["property_animation"]["is_looping"] = True
kf2 = menu_state["property_animation"]["tracks"][0]["keyframes"]
kf2[0]["value"] = 4000.0;  kf2[0]["time"] = 0.0
kf2[1]["value"] = -2400.0; kf2[1]["time"] = 25.6

# ================================================================
# 2. Window / BackCarriage  (entity "Window", id 1863)
# ================================================================
window = find_entity("Window")
find_component(window, "Components::Texture")["path"] = "images/NewMainMenu/BackCarriage.png"

# ================================================================
# 3. Train Overlay → Carriage-Sheet  (entity "Train Overlay", id 1864)
# ================================================================
train = find_entity("Train Overlay")
find_component(train, "Components::Texture")["path"] = "images/NewMainMenu/Carriage-Sheet.png"

# Add TextureAtlas component (9 frames)
tex_coords_9 = make_tex_coords_9()
add_component(train, "Components::TextureAtlas", {
    "drawIndex": 0,
    "tex_coords_list": tex_coords_9,
})

# Add Animation component (start with frame 0 only, looping — will be overridden by states)
add_component(train, "Components::Animation", make_animation_data([0], 0.15, True))

# Update state machine states
sm_train = find_component(train, "Components::StateMachine")
FRAME_DUR = 0.15
TOTAL_DUR = 9 * FRAME_DUR  # 1.35s

# --- Start state: frame 0 only (looping), no texture swap ---
start_st = find_state(sm_train, "Start")
start_st["animation"] = make_state_animation_override(
    "images/NewMainMenu/Carriage-Sheet.png",
    tex_coords_9,
    make_animation_data([0], FRAME_DUR, True),
    1600.0, 900.0,
)

# --- Transition state: play frames 0-8, NOT looping ---
trans_st = find_state(sm_train, "Transition")
trans_st["animation"] = make_state_animation_override(
    "images/NewMainMenu/Carriage-Sheet.png",
    tex_coords_9,
    make_animation_data(list(range(9)), FRAME_DUR, False),
    1600.0, 900.0,
)
# Add a dummy property animation of the same duration so
# is_property_anim_playing() keeps the entity in Transition state.
trans_st["property_animation"] = {
    "duration": TOTAL_DUR,
    "is_looping": False,
    "tracks": [{
        "easing": "Linear",
        "keyframes": [
            {"time": 0.0,       "value": 800.0},
            {"time": TOTAL_DUR, "value": 800.0},
        ],
        "target_component": "Components::Transform2D",
        "target_field_path": "position.x",
    }],
}

# --- Menu state: last frame (8) only, looping ---
menu_st = find_state(sm_train, "Menu")
menu_st["animation"] = make_state_animation_override(
    "images/NewMainMenu/Carriage-Sheet.png",
    tex_coords_9,
    make_animation_data([8], FRAME_DUR, True),
    1600.0, 900.0,
)

# ================================================================
# 4. Hide entities with no NewMainMenu replacement
# ================================================================
for name in ["player_sleeping", "Red", "Any key", "Monkey", "Bro"]:
    ent = find_entity(name)
    ent["layers"] = []

# ================================================================
# 5. Button / UI texture path updates
# ================================================================
tex_remap = {
    "images/UI/MainMenu/UI/MainMenuImpact.png": "images/NewMainMenu/Buttons/MainMenuImpact.png",
    "images/UI/MainMenu/UI/PlayButton.png":      "images/NewMainMenu/Buttons/PlayButton.png",
    "images/UI/MainMenu/UI/OptionsButton.png":   "images/NewMainMenu/Buttons/OptionsButton.png",
    "images/UI/MainMenu/UI/CreditsButton.png":   "images/NewMainMenu/Buttons/CreditsButton.png",
    "images/UI/MainMenu/UI/ExitButton.png":      "images/NewMainMenu/Buttons/ExitButton.png",
}

for ent in entities:
    tex = find_component(ent, "Components::Texture")
    if tex and tex.get("path") in tex_remap:
        tex["path"] = tex_remap[tex["path"]]

# ================================================================
# Write back
# ================================================================
with open(JSON_PATH, "w") as f:
    json.dump(data, f, indent=4)

print("MainMenu.json updated successfully.")
