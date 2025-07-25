#include "Input.h"

#include "Eruption/Core/Application.h"

namespace Eruption
{
	bool Input::IsKeyPressed(KeyCode keyCode)
	{
		return s_KeyData.contains(keyCode) && s_KeyData[keyCode].State == KeyState::Pressed;
	}

	bool Input::IsKeyHeld(KeyCode keyCode)
	{
		return s_KeyData.contains(keyCode) && s_KeyData[keyCode].State == KeyState::Held;
	}

	bool Input::IsKeyDown(KeyCode keyCode)
	{
		return s_KeyData.contains(keyCode) && s_KeyData[keyCode].State == KeyState::Pressed &&
		       s_KeyData[keyCode].OldState == KeyState::Pressed;
	}

	bool Input::IsKeyReleased(KeyCode key)
	{
		return s_KeyData.contains(key) && s_KeyData[key].State == KeyState::Released;
	}

	bool Input::IsMouseButtonPressed(MouseButton button)
	{
		return s_MouseData.contains(button) && s_MouseData[button].State == KeyState::Pressed;
	}

	bool Input::IsMouseButtonHeld(MouseButton button)
	{
		return s_MouseData.contains(button) && s_MouseData[button].State == KeyState::Held;
	}

	bool Input::IsMouseButtonDown(MouseButton button)
	{
		return s_MouseData.contains(button) && s_MouseData[button].State == KeyState::Pressed &&
		       s_MouseData[button].OldState == KeyState::Pressed;
	}

	bool Input::IsMouseButtonReleased(MouseButton button)
	{
		return s_MouseData.contains(button) && s_MouseData[button].State == KeyState::Released;
	}

	float Input::GetMouseX()
	{
		auto [x, y] = GetMousePosition();
		return x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		return y;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		const auto& window = static_cast<Window&>(Application::Get().GetWindow());

		double x, y;
		glfwGetCursorPos(static_cast<GLFWwindow*>(window.GetNativeWindow()), &x, &y);
		return {static_cast<float>(x), static_cast<float>(y)};
	}

	void Input::TransitionPressedKeys()
	{
		for (const auto& [key, keyData] : s_KeyData)
		{
			if (keyData.State == KeyState::Pressed)
				UpdateKeyState(key, KeyState::Held);
		}
	}

	void Input::TransitionPressedButtons()
	{
		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Pressed)
				UpdateButtonState(button, KeyState::Held);
		}
	}

	void Input::UpdateKeyState(KeyCode key, KeyState newState)
	{
		auto& keyData    = s_KeyData[key];
		keyData.Key      = key;
		keyData.OldState = keyData.State;
		keyData.State    = newState;
	}

	void Input::UpdateButtonState(MouseButton button, KeyState newState)
	{
		auto& mouseData    = s_MouseData[button];
		mouseData.Button   = button;
		mouseData.OldState = mouseData.State;
		mouseData.State    = newState;
	}

	void Input::ClearReleasedKeys()
	{
		for (const auto& [key, keyData] : s_KeyData)
		{
			if (keyData.State == KeyState::Released)
				UpdateKeyState(key, KeyState::None);
		}

		for (const auto& [button, buttonData] : s_MouseData)
		{
			if (buttonData.State == KeyState::Released)
				UpdateButtonState(button, KeyState::None);
		}
	}
}        // namespace Eruption