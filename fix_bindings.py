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
    "OptionsScreenLeft": "main_menu_set_windowed",
    "OptionsScreenRight": "main_menu_set_fullscreen",
}

for ent in data["data"]["entities"]:
    name = ent.get("name", "")
    if name in bindings:
        comp_list = ent.get("components", [])
        clickable = next((c for c in comp_list if c["type"] == "Components::Clickable"), None)
        if clickable:
            clickable["data"]["on_click"]["name"] = bindings[name]

with open("Engine/Assets/levels/MainMenu.json", "w") as f:
    json.dump(data, f, indent=2)

print("Fixed bindings!")
