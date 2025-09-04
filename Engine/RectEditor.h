#pragma once


#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>


namespace Engine
{
	struct Rect
	{
		float x{ 0.0f };
		float y{ 0.0f };
		float w{ 0.0f };
		float h{ 0.0f };
		glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
	};


	class RectEditor
	{
	public:
		static void Update(); // Mouse interactions
		static void DrawPanel(); // ImGui UI


		static const std::vector<Rect>& GetRects();
		static bool GetPreviewRect(Rect& outRect); // true if currently drawing


		static void Clear();


	private:
		static void finalizeCurrentDrag();
		static Rect makeRectFromPoints(const glm::vec2& a, const glm::vec2& b, const glm::vec4& color);


	private:
		static std::vector<Rect> s_rects;


		static bool s_showPanel;
		static bool s_editMode;
		static bool s_dragging;


		static int s_selectedIndex; // -1 if none


		static glm::vec2 s_start;
		static glm::vec2 s_curr;
		static glm::vec4 s_newRectColor; // default for new rects
	};
}