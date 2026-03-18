import argparse
import json
from pathlib import Path


def get_component_data(entity_or_prefab_data: dict, component_type: str) -> dict | None:
    for component in entity_or_prefab_data.get("components", []):
        if component.get("type") == component_type:
            return component.get("data")
    return None


def get_texture_path(entity_or_prefab_data: dict) -> str:
    texture_data = get_component_data(entity_or_prefab_data, "Components::Texture")
    if not isinstance(texture_data, dict):
        return ""
    path = texture_data.get("path")
    return path if isinstance(path, str) else ""


def load_prefab_metadata(prefabs_dir: Path) -> tuple[dict[str, dict[str, str]], dict[str, dict[str, str]]]:
    prefab_map: dict[str, dict[str, str]] = {}
    texture_map: dict[str, dict[str, str]] = {}
    for prefab_path in sorted(prefabs_dir.glob("*.json")):
        with prefab_path.open("r", encoding="utf-8") as handle:
            payload = json.load(handle)

        if payload.get("dataType") != "Prefab":
            continue

        data = payload.get("data", {})
        name = prefab_path.stem
        prefab_info = {
            "name": name,
            "id": data.get("uuid", ""),
            "filepath": data.get("filepath", f"prefabs/{prefab_path.name}"),
            "texture_path": get_texture_path(data),
        }
        prefab_map[name] = prefab_info

        texture_path = prefab_info["texture_path"]
        if texture_path and texture_path not in texture_map:
            texture_map[texture_path] = prefab_info

    return prefab_map, texture_map


def collect_targets(
    level_payload: dict,
    texture_map: dict[str, dict[str, str]],
    prefab_name_substring: str,
    overwrite_existing: bool,
) -> list[tuple[dict, str, dict[str, str]]]:
    targets: list[tuple[int, str, dict[str, str]]] = []
    entities = level_payload.get("data", {}).get("entities", [])

    for entity in entities:
        if "prefab_reference" in entity and not overwrite_existing:
            continue

        entity_name = entity.get("name")
        if not isinstance(entity_name, str):
            continue

        texture_path = get_texture_path(entity)
        if not texture_path:
            continue

        prefab_info = texture_map.get(texture_path)
        if not prefab_info or not prefab_info.get("id"):
            continue

        if prefab_name_substring.lower() not in prefab_info["name"].lower():
            continue

        targets.append((entity, entity_name, prefab_info))

    return targets


def set_prefab_reference(entity: dict, prefab_info: dict[str, str]) -> None:
    entity["name"] = prefab_info["name"]
    entity["prefab_reference"] = {
        "id": prefab_info["id"],
        "filepath": prefab_info["filepath"],
        "overrides": {
            "Components::Transform2D": ["*"],
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Restore missing prefab bindings for monkey entities in a level JSON file.")
    parser.add_argument("level", type=Path, help="Path to the level JSON file")
    parser.add_argument("--prefabs-dir", type=Path, default=Path("Engine/Assets/prefabs"), help="Path to the prefabs directory")
    parser.add_argument("--prefab-name-substring", default="monkey", help="Only repair entities whose matched prefab name contains this substring")
    parser.add_argument("--overwrite-existing", action="store_true", help="Rewrite existing prefab_reference blocks for matching entities as well")
    parser.add_argument("--dry-run", action="store_true", help="Report what would be changed without writing the file")
    args = parser.parse_args()

    with args.level.open("r", encoding="utf-8") as handle:
        level_payload = json.load(handle)

    _, texture_map = load_prefab_metadata(args.prefabs_dir)
    targets = collect_targets(level_payload, texture_map, args.prefab_name_substring, args.overwrite_existing)

    if not targets:
        print("No matching entities found for repair.")
        return 0

    print(f"Found {len(targets)} matching entities.")
    for entity, entity_name, prefab_info in targets:
        print(
            f"  id={entity.get('id')} "
            f"name={entity_name} texture={prefab_info['texture_path']} -> {prefab_info['name']}"
        )

    if args.dry_run:
        return 0

    for entity, _, prefab_info in targets:
        set_prefab_reference(entity, prefab_info)

    with args.level.open("w", encoding="utf-8", newline="\n") as handle:
        json.dump(level_payload, handle, indent=4)
        handle.write("\n")

    print(f"Repaired {len(targets)} entities.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())