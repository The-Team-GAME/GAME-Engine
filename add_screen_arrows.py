import json
import uuid
import copy

with open("Engine/Assets/levels/MainMenu.json", "r") as f:
    data = json.load(f)

entities = data["data"]["entities"]

left_template = next(e for e in entities if e.get("name") == "OptionsBGMLeft")
right_template = next(e for e in entities if e.get("name") == "OptionsBGMRight")

screen_left = copy.deepcopy(left_template)
screen_left["name"] = "OptionsScreenLeft"
screen_left["id"] = max(e.get("id", 0) for e in entities) + 1

for c in screen_left.get("components", []):
    if c["type"] == "Components::Clickable":
        c["data"]["on_click"]["name"] = "main_menu_set_windowed"
    elif c["type"] == "Components::Transform2D":
        c["data"]["position"]["y"] = 0.0 # Will be updated by lua

screen_right = copy.deepcopy(right_template)
screen_right["name"] = "OptionsScreenRight"
screen_right["id"] = max(e.get("id", 0) for e in entities) + 2

for c in screen_right.get("components", []):
    if c["type"] == "Components::Clickable":
        c["data"]["on_click"]["name"] = "main_menu_set_fullscreen"
    elif c["type"] == "Components::Transform2D":
        c["data"]["position"]["y"] = 0.0

# Ensure the new entities are added inside 'entities' array
# Wait, we need to make sure they are appended, but also we need to add their IDs to the root 'child_ids' if any parent has it.
# Check what parent options have.
# If they have a parent, we must add to parent child_ids!

for ent in entities:
    if ent.get("name") == "OptionsScreenLeft":
        # clean up existing one instead of duplicate
        pass
        
parent_id = None
for c in screen_left.get("components", []):
    if c["type"] == "Components::Parent":
        parent_id = c["data"]["parent_id"]

if parent_id is not None:
    parent_ent = next(e for e in entities if e.get("id") == parent_id)
    for c in parent_ent.get("components", []):
        if c["type"] == "Components::Hierarchy":
            c["data"]["child_ids"].append(screen_left["id"])
            c["data"]["child_ids"].append(screen_right["id"])
            
entities.append(screen_left)
entities.append(screen_right)

with open("Engine/Assets/levels/MainMenu.json", "w") as f:
    json.dump(data, f, indent=2)

print("Added screen arrows!")
