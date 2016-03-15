#pragma once

struct Camera
{
	bool WindowFocused = true;
	bool oldFocus;
	bool Init = false;

	// position
	glm::vec3 position = glm::vec3(5, 5, 5);
	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction;
	// Up vector 
	glm::vec3 up;
	// Current view
	glm::mat4x4 view;

	// horizontal angle : toward -Z
	float horizontal_angle = 3.14f;
	// vertical angle : 0, look at the horizon
	float vertical_angle = 0.0f;
	// Initial Field of View
	const float base_fov = 90.0f;
	float fov;
	
	float speed = 4.0f;
	float mouseSpeed = 0.8f;
	double last_time = glfwGetTime();

	void ChangeFov(double offset)
	{
		if (Init && WindowFocused)
		{
			fov = std::fmaxf(10.f, std::fminf(110.f, fov - offset * base_fov * 0.05f));
		}
	}

	void update(GLFWwindow *window)
	{
		double current_time = glfwGetTime();
		float delta_time = float(current_time - last_time);

		if (WindowFocused)
		{
			if (Init)
			{
				// Get mouse position
				double x_pos, y_pos;
				glfwGetCursorPos(window, &x_pos, &y_pos);

				// Reset mouse position for next frame
				glfwSetCursorPos(window, WIDTH * 0.5f, HEIGHT * 0.5f);

				if (oldFocus) // Check if coming from unfocused window
				{
					// Compute new orientation
					horizontal_angle += mouseSpeed * delta_time * float(WIDTH * 0.5f - x_pos);
					vertical_angle += mouseSpeed * delta_time * float(HEIGHT * 0.5f - y_pos);
				}

				direction = glm::vec3(cos(vertical_angle) * sin(horizontal_angle),
					sin(vertical_angle),
					cos(vertical_angle) * cos(horizontal_angle));

				// Right vector
				glm::vec3 right = glm::vec3(sin(horizontal_angle - PI * 0.5f),
					0.f,
					cos(horizontal_angle - PI * 0.5f));

				// Up vector : perpendicular to both direction and right
				up = glm::cross(right, direction);

				float current_speed = speed;

				// Speed up
				if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
				{
					current_speed *= 2.0f;
				}

				// Move forward
				if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				{
					position += direction * delta_time * current_speed;
				}
				// Move backward
				if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				{
					position -= direction * delta_time * current_speed;
				}

				// Strafe right
				if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				{
					position += right * delta_time * current_speed;
				}
				// Strafe left
				if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				{
					position -= right * delta_time * current_speed;
				}

				// Up
				if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
				{
					position += up * delta_time * current_speed;
				}
				// Down
				if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
				{
					position -= up * delta_time * current_speed;
				}
			}
			else
			{
				direction = glm::vec3(0.f);
				up = glm::vec3(0.f, 1.f, 0.f);
				// Reset mouse position for next frame
				glfwSetCursorPos(window, WIDTH * 0.5f, HEIGHT * 0.5f);
				fov = base_fov;
				Init = true;
			}
		}
		
		oldFocus = WindowFocused;
		last_time = current_time;
	}

} cam;