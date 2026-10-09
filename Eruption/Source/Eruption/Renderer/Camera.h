#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Eruption
{
	// A perspective camera. Projection uses Vulkan clip space (depth 0..1, via GLM_FORCE_DEPTH_ZERO_TO_ONE);
	// the renderer flips the viewport Y so world space is right-handed, +Y up.
	class Camera
	{
	public:
		void SetPerspective(float fovDegrees, float aspect, float nearClip, float farClip)
		{
			m_Fov    = glm::radians(fovDegrees);
			m_Aspect = aspect;
			m_Near   = nearClip;
			m_Far    = farClip;
			RecalculateProjection();
		}

		void SetViewportSize(uint32_t width, uint32_t height)
		{
			if (height == 0)
				return;
			m_Aspect = static_cast<float>(width) / static_cast<float>(height);
			RecalculateProjection();
		}

		void LookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up = {0.0f, 1.0f, 0.0f})
		{
			m_Position = eye;
			m_View     = glm::lookAt(eye, center, up);
		}

		[[nodiscard]] const glm::mat4& GetProjection() const { return m_Projection; }
		[[nodiscard]] const glm::mat4& GetView() const { return m_View; }
		[[nodiscard]] glm::mat4        GetViewProjection() const { return m_Projection * m_View; }
		[[nodiscard]] const glm::vec3& GetPosition() const { return m_Position; }

	private:
		void RecalculateProjection() { m_Projection = glm::perspective(m_Fov, m_Aspect, m_Near, m_Far); }

	private:
		glm::mat4 m_Projection{1.0f};
		glm::mat4 m_View{1.0f};
		glm::vec3 m_Position{0.0f};

		float m_Fov    = glm::radians(45.0f);
		float m_Aspect = 16.0f / 9.0f;
		float m_Near   = 0.1f;
		float m_Far    = 1000.0f;
	};
}        // namespace Eruption
