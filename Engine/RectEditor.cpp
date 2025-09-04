#include "RectEditor.h"
#include "InputManager.h" // Engine::Input
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>


using namespace Engine;


std::vector<Rect> RectEditor::s_rects = {};
bool RectEditor::s_showPanel = true;
bool RectEditor::s_editMode = true;
bool RectEditor::s_dragging = false;
int RectEditor::s_selectedIndex = -1;
glm::vec2 RectEditor::s_start = { 0.0f, 0.0f };
glm::vec2 RectEditor::s_curr = { 0.0f, 0.0f };
glm::vec4 RectEditor::s_newRectColor = { 1.0f, 1.0f, 1.0f, 1.0f };


static inline bool want_capture_mouse()
{
	// Avoid beginning a drag when user is interacting with UI.
	return ImGui::GetIO().WantCaptureMouse;
}


void RectEditor::Update()
{
	// Edge detection using Held state only.
	const bool leftDown = Engine::Input::IsMouseButtonHeld(GLFW_MOUSE_BUTTON_LEFT);
	const glm::vec2 mouse = Engine::Input::GetMousePosition();


	if (!s_editMode)
	{
		// Keep preview position up-to-date so panel can show coords.
		s_curr = mouse;
		s_dragging = false;
		return;
	}


	if (!s_dragging)
	{
		if (leftDown && !want_capture_mouse())
		{
			s_dragging = true;
			s_start = s_curr = mouse;
		}
	}
	else // dragging
	{
		s_curr = mouse;


		if (!leftDown)
		{
			finalizeCurrentDrag();
			s_dragging = false;
		}
	}
}

void RectEditor::finalizeCurrentDrag()
{
	Rect r = makeRectFromPoints(s_start, s_curr, s_newRectColor);
	// Ignore tiny or degenerate rectangles to avoid accidental clicks.
	if (r.w >= 1.0f && r.h >= 1.0f)
	{
		s_rects.push_back(r);
		s_selectedIndex = static_cast<int>(s_rects.size()) - 1;
	}
}


Rect RectEditor::makeRectFromPoints(const glm::vec2& a, const glm::vec2& b, const glm::vec4& color)
{
	const float x = std::min(a.x, b.x);
	const float y = std::min(a.y, b.y);
	const float w = std::fabs(b.x - a.x);
	const float h = std::fabs(b.y - a.y);
	return Rect{ x, y, w, h, color };
}


const std::vector<Rect>& RectEditor::GetRects()
{
	return s_rects;
}


bool RectEditor::GetPreviewRect(Rect& outRect)
{
	if (s_editMode && s_dragging)
	{
		outRect = makeRectFromPoints(s_start, s_curr, glm::vec4(1.0f, 1.0f, 1.0f, 0.35f));
		return true;
	}
	return false;
}


void RectEditor::Clear()
{
	s_rects.clear();
	s_selectedIndex = -1;
}

void RectEditor::DrawPanel()
{
	if (!s_showPanel) return;


	if (ImGui::Begin("Rect Editor", &s_showPanel))
	{
		ImGui::Checkbox("Edit mode (hold LMB to draw)", &s_editMode);


		ImGui::Separator();
		ImGui::Text("New Rect Color");
		ImGui::ColorEdit4("##newrectcolor", &s_newRectColor.x, ImGuiColorEditFlags_NoInputs);


		ImGui::Separator();
		ImGui::Text("Mouse: (%.1f, %.1f)", s_curr.x, s_curr.y);
		if (s_editMode && s_dragging)
		{
			ImGui::Text("Dragging from (%.1f, %.1f)", s_start.x, s_start.y);
		}


		ImGui::Separator();
		ImGui::Text("Rectangles: %d", (int)s_rects.size());


		// List + selection
		if (ImGui::BeginListBox("##rectlist", ImVec2(-FLT_MIN, 180)))
		{
			for (int i = 0; i < (int)s_rects.size(); ++i)
			{
				const Rect& r = s_rects[i];
				char label[128];
				snprintf(label, sizeof(label), "[%d] x=%.0f y=%.0f w=%.0f h=%.0f", i, r.x, r.y, r.w, r.h);
				const bool selected = (s_selectedIndex == i);
				if (ImGui::Selectable(label, selected))
				{
					s_selectedIndex = i;
				}
			}
			ImGui::EndListBox();
		}


		// Selected rect inspector
		if (s_selectedIndex >= 0 && s_selectedIndex < (int)s_rects.size())
		{
			Rect& r = s_rects[s_selectedIndex];
			ImGui::Separator();
			ImGui::Text("Selected Rect");
			ImGui::DragFloat("x", &r.x, 1.0f, 0.0f, 10000.0f);
			ImGui::DragFloat("y", &r.y, 1.0f, 0.0f, 10000.0f);
			ImGui::DragFloat("w", &r.w, 1.0f, 0.0f, 10000.0f);
			ImGui::DragFloat("h", &r.h, 1.0f, 0.0f, 10000.0f);
			ImGui::ColorEdit4("color", &r.color.x);
			if (ImGui::Button("Delete Selected"))
			{
				s_rects.erase(s_rects.begin() + s_selectedIndex);
				if (s_selectedIndex >= (int)s_rects.size()) s_selectedIndex = (int)s_rects.size() - 1;
			}
		}
	

ImGui::Separator();
if (ImGui::Button("Clear All"))
{
Clear();
}
ImGui::SameLine();
if (ImGui::Button("Save (TODO)"))
{
// TODO: Serialize s_rects to a file (JSON or binary) when serialization system is ready.
}
ImGui::SameLine();
if (ImGui::Button("Load (TODO)"))
{
// TODO: Load from file into s_rects.
}
}
ImGui::End();
}