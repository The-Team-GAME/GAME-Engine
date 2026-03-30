import json

with open("Engine/Assets/levels/MainMenu.json", "r") as f:
    data = json.load(f)

bindings = {
    "OptionsWindowed": "main_menu_set_windowed",
    "OptionsFullscreen": "main_menu_set_fullscreen",
    "OptionsBGMLeft": "main_menu_bgm_decrease",
    "OptionsBGMRight": "main_menu_bgm_increase",
    "OptionsSFXLeft": "main_menu_sfx_decrease",
    "OptionsSFXRight": "main_menu_sfx_increase",
}

sizes = {
    "OptionsWindowed": (148.3333333333333, 26.66666666666666),
    "OptionsFullscreen": (160.0, 26.66666666666666),
    "OptionsBGMLeft": (29.16666666666666, 47.5),
    "OptionsBGMRight": (29.16666666666666, 47.5),
    "OptionsSFXLeft": (29.16666666666666, 47.5),
    "OptionsSFXRight": (29.16666666666666, 47.5),
}

for ent in data["data"]["entities"]:
    name = ent.get("name", "")
    if name in ["OptionsBGMBar", "OptionsSFXBar"]:
        ent["components"] = [
            c for c in ent.get("components", [])
            if c["type"] not in ["Components::BoxCollider", "Components::Clickable"]
        ]
    
    if name in bindings:
        comp_list = ent.get("components", [])
        
        clickable = next((c for c in comp_list if c["type"] == "Components::Clickable"), None)
        if not clickable:
            clickable = {
                "type": "Components::Clickable",
                "data": {
                    "on_click": {"name": ""},
                    "on_hover": {"name": ""},
                    "exit_hover": {"name": ""}
                }
            }
            comp_list.append(clickable)
        clickable["data"]["on_click"]["name"] = bindings[name]

        if name in sizes:
            box = next((c for c in comp_list if c["type"] == "Components::BoxCollider"), None)
            if not box:
                box = {
                    "type": "Components::BoxCollider",
                    "data": {
                        "type": "AABB",
                        "width": 0.0, "height": 0.0,
                        "aabb": {"half_width": 0.0, "half_height": 0.0, "pos": {"x": 0.0, "y": 0.0}},
                        "offset": {"x": 0.0, "y": 0.0},
                        "radius": 0.0, "collided": False, "kinetic_friction": 1.0, "resolve_collisions": False,
                        "polygon": {"vertices": [], "position": {"x": 0.0, "y": 0.0}, "rotation": 0.0}
                    }
                }
                comp_list.append(box)
                
            w, h = sizes[name]
            box["data"]["width"] = float(w)
            box["data"]["height"] = float(h)
            box["data"]["aabb"]["half_width"] = float(w) / 2
            box["data"]["aabb"]["half_height"] = float(h) / 2

with open("Engine/Assets/levels/MainMenu.json", "w") as f:
    json.dump(data, f, indent=2)
print("Finished fixing MainMenu.json")
