#pragma once

#include <reimu/graphics/matrix.h>
#include <reimu/graphics/vector.h>

#include <mutex>

namespace reimu {

class Transform2D {
public:
	Transform2D(const Vector2f& position = { 0.f, 0.f }, const Vector2f& scale = { 1.f, 1.f }, float rotationDegrees = 0);
	Transform2D(const Transform2D&);

	Transform2D& operator=(const Transform2D&);

	Transform2D& set_position(const Vector2f& position);
	Transform2D& set_position(float x, float y);
	Transform2D& set_z_index(float z);
	Transform2D& set_scale(const Vector2f& scale);
	Transform2D& set_scale(float scaleX, float scaleY);
	Transform2D& set_rotation(float degrees);

	Transform2D& translate(const Vector2f& delta);

	inline const Vector2f& get_position() const { return m_position; }
	inline float get_z_index() const { return m_z_index; }
	inline const Vector2f& get_scale() const { return m_scale; }

	const Matrix4& matrix();

private:
	std::mutex m_matrixLock;

	Vector2f m_position;
	float m_z_index = 0;
	Vector2f m_scale = {1.f, 1.f};
	float m_rotation = 0; // Radians

	bool m_matrix_dirty = true; // Only update matrix when necessary, no need to update after every transformation
	Matrix4 m_matrix;
};

} // namespace reimu
