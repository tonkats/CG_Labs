#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	//! \todo Implement this function
	glm::mat2 MinvN = glm::mat2(
		glm::vec2(1, -1),
		glm::vec2(0, 1)
	);
	glm::mat3x2 p_mat = glm::mat3x2(
		glm::vec2(p0.x, p1.x),
		glm::vec2(p0.y, p1.y),
		glm::vec2(p0.z, p1.z)
	);
	glm::vec2 x_vec = glm::vec2(1.0f, x);

	return x_vec * MinvN * p_mat;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	//! \todo Implement this function
	glm::mat4 MinvN = glm::mat4(
		glm::vec4(0, -t, 2 * t, -t),
		glm::vec4(1, 0, t - 3, 2 - t),
		glm::vec4(0, t, 3 - 2 * t, t - 2),
		glm::vec4(0, 0, -t, t)
	);
	glm::mat3x4 p_mat = glm::mat3x4(
		glm::vec4(p0.x, p1.x, p2.x, p3.x),
		glm::vec4(p0.y, p1.y, p2.y, p3.y),
		glm::vec4(p0.z, p1.z, p2.z, p3.z)
	);
	glm::vec4 x_vec = glm::vec4(1.0f, x, pow(x, 2), pow(x, 3));

	return x_vec * MinvN * p_mat;
}
