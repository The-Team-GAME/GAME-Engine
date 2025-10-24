# Settings Documentation

## How Settings Work

Settings are automatically saved to JSON files using reflection. 

When you add a new member to a settings struct (like `EngineSettings` or `UserSettings`), the new field will automatically be added to the json the next time you run the Engine!! So no need to add it into the json manually.

To add a new setting:
1. Add the member to the struct
2. Add it to the `REFLECT_FIELDS` macro at the bottom of the file

The `SettingsManager` handles loading, merging defaults, and saving automatically.
