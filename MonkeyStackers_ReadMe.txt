===========================================================================================================
Team: Monkey Stackers
Game: Monkey Stole My GPA
===========================================================================================================


===========================================================================================================
Members:

CHEW Shee Yang | Product Manager | Programmer | Graphics Champion
CHENG Ming Xian | Technical Lead | Programmer | Engine Champion
ANG Ryan An Kiat | Programmer | Programmer | Resources Champion
HOW Hong Chin Ryan | Programmer | Programmer | Architecture Champion
ONG Zi Ming | Programmer | Programmer | Tools Champion
===========================================================================================================


===========================================================================================================
Global controls:

Press F1 to toggle ImGui windows on/off
Press F2 to toggle between normal viewport mode and ImGui viewport mode
Press F3 to toggle debug layer visibility
Press F10 to toggle FPS overlay
Press F11 to toggle fullscreen
Press P to toggle pause/play
Press ] to step forward one frame when paused
MiddleMouse Click and drag to enter free camera mode (detach from active camera)
Scroll to zoom the camera in/out

Demo states can be accessed under the ImGui window "State Chooser"
===========================================================================================================



===========================================================================================================
Demo states
-----------------------------------------------------------------------------------------------------------

EmptyState:

This is a blank state used for testing and creating entities from scratch.

Recommended for:
- Testing tilemap editor
- Testing prefab instantiation
- Creating entities without interference from other scene objects

-----------------------------------------------------------------------------------------------------------

MainMenuState:

This state demonstrates:
- START button - navigates to ShowcaseState
- EXIT button - closes the game
- UI button system using Clickable component and ClickableSystem
- Layer system (buttons on "ui" layer, background on "background" layer)

-----------------------------------------------------------------------------------------------------------

ShowcaseState:

Instructions:
Player can move around using A and D
Press Space to jump
Press E to attack

This state demonstrates:
- Camera follows the player when moving with A or D
- Player movement with A and D, banana rotates by itself
- Player sprite animation (idle, running, punching) changes based on state
- Monkey idle animation
- Sound plays when player moves, jumps, or attacks (3 different sounds)
- Background music plays throughout the scene
- Multiple text shown using two different fonts (PlayfairDisplay and Roboto)
- Textures, Fonts, Audio, and ObjectPrefabs loaded using the Resource Manager (Resources.cpp)
- ECS use demonstrated
- Animation done using Sprite Sheets
- Texture background displayed
- UI system with clickable components

-----------------------------------------------------------------------------------------------------------

RectPainterState:

This state demonstrates:
- Basic rectangle rendering and painting functionality
- Only works in normal viewport mode for currently (press F2 to toggle viewport mode)

-----------------------------------------------------------------------------------------------------------

Rubric2500ParticlesState:

This state demonstrates:
- 2500 particles running above 60 FPS
- Quad tree implementation for collision optimization
- Each particle has rotation, color, and physics properties
- Rotating colored particles demonstrating ECS performance

Note: You can see the FPS in the titlebar OR in the ImGui window "Performance Viewer"
Note: You can zoom out by [ctrl+scrolling down] or use the ImGui "Camera Control" panel to ensure all
      particles are in the viewport
Note: Green grid lines in this state show quad tree nodes with entities, grey/white show empty nodes

-----------------------------------------------------------------------------------------------------------

StressTestState:

Instructions:
Press Space to switch between monkey and player sprites

This state demonstrates:
- 1000 animated sprites of monkey walking
- Can switch to player running sprite by pressing Space
- Batched rendering performance
- Sprite animation at scale

-----------------------------------------------------------------------------------------------------------

BasicState:

This state demonstrates:
- Basic ECS entity creation and component usage
- As well as non-entity based graphics rendering to demonstrate how to use the graphics library without
  creating entities

-----------------------------------------------------------------------------------------------------------

PrefabsState:

This state demonstrates:
- Prefab loading and instantiation

-----------------------------------------------------------------------------------------------------------

PhysicsTestState:

Instructions:
Player can move using A and D to go left and right
Player can jump using Space

This state demonstrates:
- Physics system with gravity (pulling player down when jumping)
- Forces acting on rigidbody components (vertical acceleration for jumping/falling)
- Static collision detection and response using Minkowski difference
- Collision resolution (player stays above platform, can't move through walls)
- Layer-based collision (entities only collide if on the same layer)

Note: You can click on the player rect int he viewport, then in Entity Editor, under layers, pick another 
      layer, then uncheck the default layer. You should see the player fall through the floor since now 
      they are in different layers.

-----------------------------------------------------------------------------------------------------------

TimelineState:

This state demonstrates:
- Timeline editor for sequencing animations and entity visibility
- Entity visibility toggling based on timeline playback
- Frame-based animation control
- Multiple entities on different timeline layers

Note: Work in progress - full timeline functionality under development

-----------------------------------------------------------------------------------------------------------

DistortionEffectState:

This state demonstrates:
- Custom shader effects
- Default shader for normal rendering (left monkey)
- Distortion shader for VHS-style glitch effect (right monkey)
- Two identical animated sprites rendered with different shaders

-----------------------------------------------------------------------------------------------------------

LevelTestState:

This state demonstrates:
- Complete level loaded from JSON file
- All entities, positions, and components loaded from JSON
- Level serialization and deserialization

-----------------------------------------------------------------------------------------------------------
 ===========================================================================================================
 Things that works in any state:
-----------------------------------------------------------------------------------------------------------

Entity Editor (ImGui window):

- Entities can be created: click "Create New Entity" button
- Entities can be cloned: select an entity, then click "Clone This Entity" or right-click and select 
  "Clone Entity". The cloned entity will be selected automatically
- Entities can be deleted: select an entity and click "Delete Selected" or right-click and select 
  "Delete Entity"
- Entities can be selected by clicking on them in the viewport
- Selected entities are highlighted in the viewport
- Can use Entity Editor to edit Transform2D components (position, rotation, scale) in real-time
- Entities can be viewed in Flat View (simple list) or Hierarchy View (grouped by layers)
- Entities can be searched/filtered using the search bar
- Components can be added to selected entities from the "Add Component" dropdown (only reflectable 
  components shown, buttons disabled if component already present)
- Components can be edited in the "Components" section with fields auto-generated via reflection
- All component changes apply in real-time
- Components can be removed by clicking the "Remove" button under each component
- Entities can be assigned to multiple layers in the "Layers" section with checkboxes
- Layers can be enabled and disabled in Layer Manager
- Prefabs can be created from entities: click "Create Prefab from Entity", enter filename, save to 
  prefabs directory
- Entities can be exported to JSON: click "Export to JSON" to copy JSON representation to clipboard
- Can change entity textures: select entity with texture, go to Components->Components::Texture, 
  click "Select Texture..." to pick any loaded texture
- Drag and drop texture files into editor to load and use new textures

-----------------------------------------------------------------------------------------------------------

Prefab Editor (ImGui window):

- Prefabs can be created, edited, saved, and instantiated
- Can instantiate prefabs by selecting from list and clicking "Instantiate"
- Prefabs are stored as JSON files in Assets/prefabs/
- Can drag and drop .json prefab files into the editor to load and instantiate
- Example: Select "Rect.json" and press "Instantiate" to create a white rectangle

Steps to create a white Rect entity using prefabs:
1. Open Prefab Editor panel
2. Select Rect.json from the prefab list
3. Click "Instantiate" button
4. A white rectangle appears at screen center

Note: If rect doesn't appear, it may be covered by entities in higher layers. In Entity Editor, 
      under Layers, select "debug" layer to view it. Try in EmptyState if still not visible.

-----------------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------------

Selective Level Save (ImGui window):

- Allows saving a level with only selected components from entities
- Useful for creating prefabs or partial level data
- Filter components to include/exclude in the save file

-----------------------------------------------------------------------------------------------------------

Level Manager (ImGui window):

- Complete level saving: saves all entities in current scene to JSON file
- Complete level loading: loads all entities, components, and positions from JSON
- Levels stored in Assets/levels/
- Can save/load entire game states including entity hierarchy and component data

-----------------------------------------------------------------------------------------------------------

Tilemap Editor (ImGui window):

Features:
- Enable/disable Tilemap Mode
- Enable/disable Painting mode
- Adjustable Tile Size
- Layer selection for painting
- Save and load tilemaps from JSON files
- Select tiles from texture atlas
- Visual feedback: tiles not on selected layer are darkened
- Grid overlay shows tile boundaries
- Erase mode for removing tiles
- Move all tiles from one layer to another

Usage:
1. Enable Tilemap Mode
2. Enable Painting
3. Set Tile Size
4. Choose layer to paint on
5. Select tile from atlas
6. Click in viewport to place tiles
7. Save tilemap when done

Example: Load "tilemap_save.json" in EmptyState to see a pre-made tilemap

Note: Disable Tilemap Mode to see your level without tinting effects

-----------------------------------------------------------------------------------------------------------

Performance Viewer (ImGui window):

- Shows real-time performance metrics for core systems
- Displays system execution time for:
  * Audio System
  * Collision System
  * ECS Draw System
  * ECS Update System
  * Graphics Flush
  * Input System
  * Physics Update
- Adjustable tick rate using slider
- FPS display in window title bar and panel

-----------------------------------------------------------------------------------------------------------

Camera Control (ImGui window):

- Manual camera zoom, rotation, and translation controls
- Can select between multiple cameras
- Toggle camera active state
- Free camera mode: Ctrl+Click and drag in viewport to detach from active camera
- Ctrl+Scroll to zoom camera
- Reactivate camera by selecting it in Camera Control panel and ticking "Active" checkbox

-----------------------------------------------------------------------------------------------------------

Resource Manager (ImGui window):

- Displays all loaded resources (Textures, Sounds, Fonts, Prefabs)
- Supports drag-and-drop for:
  * Image files (Textures)
  * Audio files (Sounds)
  * Font files (.ttf)
  * Prefab files (.json)
- Automatically loads resources when files are dragged into editor
- Resource preview and management

-----------------------------------------------------------------------------------------------------------

State Chooser (ImGui window):

- Switch between all available game states/scenes
- Live scene switching without restarting application
- Lists all registered states

-----------------------------------------------------------------------------------------------------------

Viewport Panel (ImGui window):

- Main game rendering view
- Click entities to select them
- Drag selected entities with mouse to reposition
- Entity selection synchronized with Entity Editor
- Gizmo system for entity manipulation (select and drag entities when game is not paused)

Note: Full gizmo functionality (scale/rotate handles) under development

-----------------------------------------------------------------------------------------------------------

Toolbar:

- Play button: Start game simulation
- Pause button: Pause game simulation (or press P key)
- Stop button: Stop and reset game simulation

-----------------------------------------------------------------------------------------------------------

Game Loop:

- Fixed timestep game loop using accumulator
- Ensures consistent physics and game logic regardless of frame rate
- Decoupled rendering from update logic

-----------------------------------------------------------------------------------------------------------

Event System:

Implementation: EventManager (Engine/EventManager/)

Features:
- Decoupled publisher-subscriber pattern
- Multiple subscribers can listen to same event
- Events are queued and dispatched in update_events()
- Type-safe event handling

Examples:
1. WindowResizeEvent: Updates renderer when window size changes
2. FileDropEvent: Adds resources to Resource Manager when files are dragged into editor
3. SelectEntityEvent: Synchronizes entity selection between viewport clicks and Entity Editor panel

-----------------------------------------------------------------------------------------------------------

Settings System:

Implementation: SettingsManager (Engine/FileIO/)

- All settings loaded from JSON files
- Settings types defined in Settings.h
- JSON values take priority over struct defaults
- Struct defaults only used when creating JSON files for first time
- Persistent settings across sessions

-----------------------------------------------------------------------------------------------------------

Batched Rendering:

Implementation: Renderer.cpp

- All draw calls batched before sending to GPU
- Stores vertices in BatchState until end_batch() is called
- Single GPU call for multiple entities
- Works for all rendered types: rectangles, textures, text, sprites
- Significant performance improvement for large entity counts

-----------------------------------------------------------------------------------------------------------

Layer System:

Implementation: WorldData.h, World.cpp, System.cpp, CollisionSystem.cpp

- Entities can belong to multiple layers simultaneously
- Rendering order based on layer priority (higher layers drawn last)
- Collision only occurs between entities on same layer
- Layers: "background", "tilemap_background", "tilemap_foreground", "gameplay", "ui", "effects", "debug"
- Visual layer management in Layer Manager and Entity Editor

Examples:
- MainMenuState: Buttons on "ui" layer (higher), background on "background" layer (lower)
- Change entity layers in Entity Editor to control render order and collision behavior

-----------------------------------------------------------------------------------------------------------

Animation State System:

Implementation: GraphicsComponents.h, GraphicsSystem.h

- AnimationState component for managing multiple animation states per entity
- AnimationStateSystem handles state transitions
- Example in ShowcaseState: Player switches between idle, running, and punching animations

-----------------------------------------------------------------------------------------------------------

Shader System:

Implementation: Renderer.cpp, Assets/shaders/

Available shaders:
- default.frag/vert: Standard rendering for sprites, textures, and text
- distortion.frag/vert: VHS-style glitch distortion effect

Example: DistortionEffectState shows two identical sprites with different shaders

-----------------------------------------------------------------------------------------------------------

JSON Serialization:

Supported for:
- Settings (complete engine configuration)
- Levels (all entities and components)
- Prefabs (entity templates)
- Tilemaps (tile placement and atlas data)
- Any reflectable component type

Uses RapidJSON library for fast parsing and serialization

-----------------------------------------------------------------------------------------------------------

Quad Tree Optimization:

Implementation: CollisionSystem.cpp, DebugDrawSystem.cpp

- Spatial partitioning for collision detection
- Reduces collision checks from O(n²) to O(n log n)
- Visual debug view: green nodes contain entities, grey/white nodes are empty
- Best demonstrated in Rubric2500ParticlesState with 2500 entities

-----------------------------------------------------------------------------------------------------------

Additional Technical Features:

Vector Library:
- Custom math vector library implementation (as in M1)
- Matrix operations use GLM library for graphics optimization
- Vector math used throughout engine for transforms, physics, and collision

Audio System:
- Background music support (looping)
- Sound effects (one-shot playback)
- Example: GameplayState plays background music and 6 different sound effects
- Powered by FMOD audio library

Font Rendering:
- Multiple font support (.ttf files)
- Text rendering with custom fonts
- Example: ShowcaseState uses PlayfairDisplay and Roboto fonts
- Powered by FreeType library

Sprite Sheet Animation:
- Texture atlas support for sprite sheets
- Configurable frame count and animation speed
- Per-entity animation playback
- Frame-based and time-based animation modes

Component Reflection:
- Runtime component introspection
- Automatic ImGui editor generation for components
- Type-safe component serialization to JSON
- Used throughout Entity Editor for dynamic component editing

Resource Management:
- Centralized resource loading and caching
- Supports 4 resource types: Textures, Sounds, Fonts, Prefabs
- Automatic resource deduplication
- Hot-reloading support via drag-and-drop

Physics System:
- Rigidbody dynamics with mass and velocity
- Force application (gravity, impulses)
- Angular velocity and rotation
- Vertical acceleration for jumping/falling
- Linear horizontal movement for smooth player control

Collision System:
- Minkowski difference for collision detection
- Penetration vector calculation
- Collision response (separation)
- Box colliders
- Layer-based collision filtering
- Static and dynamic object support

-----------------------------------------------------------------------------------------------------------

Scene Management:
- Scene Stack architecture (push/pop scenes)
- Allows overlay states (e.g., Pause Menu) on top of gameplay
- Non-destructive scene switching

Build System:
- WinMain entry point for Game Build
- Hides console window in release builds for a professional look

UI System Optimization:
- Direct texture rendering for UI elements (Health Bar)
- Reduces ECS overhead for static UI components

-----------------------------------------------------------------------------------------------------------
===========================================================================================================



===========================================================================================================
QUICK REFERENCE: Feature Locations
-----------------------------------------------------------------------------------------------------------

Want to see...                          | Check this state/panel
---------------------------------------------------------------------------------------------------------
Object Factory                          | ShowcaseState, PrefabEditor panel, EntityEditor panel
JSON Loading (Levels)                   | LevelTestState, Level Manager panel
JSON Loading (Settings)                 | Settings.h, SettingsManager
Audio (Music + SFX)                     | ShowcaseState
Mouse picking/selection                 | Any state - click entities in viewport
UI Buttons                              | MainMenuState
Multiple Fonts                          | ShowcaseState (uses 2 different fonts)
Camera following                        | ShowcaseState (follows player)
Transform editing                       | Any state - Entity Editor panel
Sprite sheet animation                  | ShowcaseState (player & monkey animations)
Resource loading                        | Resources.cpp, Resource Manager panel
Static collision                        | PhysicsTestState
Performance metrics                     | Performance Viewer panel (any state)
Custom shaders                          | DistortionEffectState
Batched rendering                       | Renderer.cpp, demonstrated in all states
Animation state system                  | ShowcaseState (player state changes)
Event system                            | EventManager - WindowResizeEvent, SelectEntityEvent
Tilemap editor                          | Tilemap Editor panel (try in EmptyState)
Level saving/loading                    | Level Manager panel
Prefab system                           | Prefab Editor panel
Quad tree visualization                 | Rubric2500ParticlesState (with debug drawing)
Layer system                            | Entity Editor panel - Layers section
Fixed timestep game loop                | Core.cpp
Physics forces                          | PhysicsTestState, PhysicsSystem.cpp
Drag & drop support                     | Drag files into editor (textures, sounds, fonts, prefabs)
Entity hierarchy                        | Entity Editor - toggle Hierarchy View
Component reflection                    | Entity Editor - Components section
Timeline/sequencing                     | TimelineState (work in progress)
Free camera                             | MiddleMouse+Click drag in any state
Stress test (1000 sprites)              | StressTestState

-----------------------------------------------------------------------------------------------------------
===========================================================================================================
