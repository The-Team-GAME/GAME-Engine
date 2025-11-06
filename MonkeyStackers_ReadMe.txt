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

Press F1 to turn off the ImGui windows
Press F2 to toggle between normal viewport mode or ImGui viewport mode
Press P toggle pause/play
Press ] to go to the next step when paused
Ctrl + click drag to move the camera
Ctrl + scroll to zoom the camera

Demo states can be accessed under the ImGui window "State Chooser"
===========================================================================================================



===========================================================================================================
Demo states
-----------------------------------------------------------------------------------------------------------

ShowcaseState:

Instructions:
Player can move around using WASD
Press E to attack
Press C to collect banana

Note: Collision is not implemented in this state, but will be demonstrated in the PhysicsTestState.h

This state demonstrates:
Banana and LazyMonkey game objects loaded from a JSON into an ObjectPrefab using the ResourceManager
AttackEvent and CollectEvent are sent when you attack or collect banana
Sound plays when the player walks
Textures, Fonts, Audio, and ObjectPrefabs are loaded using the Resource Manager (Resources.cpp)
ECS use is demonstrated in this state
Animation is done using Sprite Sheets
Animation changes when the player walks or punches
Multiple text shown
Texture background is displayed

-----------------------------------------------------------------------------------------------------------

Rubrics2500ParticlesState.h:

This state demonstrates:
2500 particles running above 60 FPS

Note: you can see the FPS in the titlebar OR in the ImGui window Performance Viewer
Note: you can zoom out by [ctrl+scrolling down], or use the ImGui Window Camera Control to ensure that all
      the particles are in the viewport

-----------------------------------------------------------------------------------------------------------

PhysicsTestState:

Instructions:
Player can move around using W and D to go left and right
Player can jump using Space

This state demonstrates:

Physics working with Player (gravity pulling the player down when you jump)
Collision working and response working (player stays above the platform)

-----------------------------------------------------------------------------------------------------------

LevelLoaderTestState:

This state demonstrates:

Level is loaded from the JSON file Assets/levels/test_level.json
Level files contains the Object Prefab JSON file paths, and position of the objects.

-----------------------------------------------------------------------------------------------------------
===========================================================================================================



===========================================================================================================
Things that works in any state:
-----------------------------------------------------------------------------------------------------------

Step by Step Physics mode:

P key to enable
] key to go to next step

NOTE: Recommened to use in PhysicsTestState
NOTE: Tick rate can be adjusted using the slider in the Performance Viewer ImGui window

-----------------------------------------------------------------------------------------------------------

Entity Editor (ImGui window):

- Entities can be cloned, select an entity in the editor, then click Clone This Entity. (Modify the
  Position using the Entity Editor to see the newly created entity.)
- Can use Entity Editor to scale entities (with Scale component added)
- Can use Entity Editor to rotate entities (with Rotate component added)

Entities can be created
Steps to create a Rect entity:
- Go to ShowcaseState using the State Chooser ImGui panel
- Under the ImGui Entity Editor, press Create New Entity
- With the new entity selected, add:
      Position
      Size
      Color

-----------------------------------------------------------------------------------------------------------

Performance Viewer (ImGui window):
Tick rate can be adjusted using the slider
You can see the system time consumption info of the systems of our engine

-----------------------------------------------------------------------------------------------------------

Camera Control (ImGui window):
Can be used to zoom, rotate and translate the camera.
You can also ctrl click drag to move and ctrl scroll to zoom the camera.

-----------------------------------------------------------------------------------------------------------

Simulate a crash:

Press F11 to throw a runtime error

crash.log will be created in the same directory as the executable with the crash info 
In this case the log should contain: CRASH>> Unhandled exception in main: F11 was pressed

-----------------------------------------------------------------------------------------------------------
===========================================================================================================