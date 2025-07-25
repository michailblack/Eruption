#pragma once
#include "Eruption/Core/KeyCodes.h"

#include <map>

namespace Eruption
{
	struct KeyData
	{
		KeyCode  Key;
		KeyState State    = KeyState::None;
		KeyState OldState = KeyState::None;
	};

	struct ButtonData
	{
		MouseButton Button;
		KeyState    State    = KeyState::None;
		KeyState    OldState = KeyState::None;
	};

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode keyCode);
		static bool IsKeyHeld(KeyCode keyCode);
		static bool IsKeyDown(KeyCode keyCode);
		static bool IsKeyReleased(KeyCode keyCode);

		static bool  IsMouseButtonPressed(MouseButton button);
		static bool  IsMouseButtonHeld(MouseButton button);
		static bool  IsMouseButtonDown(MouseButton button);
		static bool  IsMouseButtonReleased(MouseButton button);
		static float GetMouseX();
		static float GetMouseY();
		static auto  GetMousePosition() -> std::pair<float, float>;

		// Internal use only...
		static void TransitionPressedKeys();
		static void TransitionPressedButtons();
		static void UpdateKeyState(KeyCode key, KeyState newState);
		static void UpdateButtonState(MouseButton button, KeyState newState);
		static void ClearReleasedKeys();

	private:
		inline static std::map<KeyCode, KeyData>        s_KeyData;
		inline static std::map<MouseButton, ButtonData> s_MouseData;
	};
}        // namespace Eruption
