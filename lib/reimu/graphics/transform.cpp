#include <reimu/graphics/transform.h>

#include <assert.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

namespace reimu {

Transform2D::Transform2D(const Vector2f& position, const Vector2f& scale, float rotationDegrees)
	: m_position(position), m_scale(scale) {
	std::lock_guard acq(m_matrixLock);
	m_rotation = rotationDegrees * (M_PI / 180.f);

	m_matrix_dirty = true;
}

Transform2D::Transform2D(const Transform2D& other){
	m_position = other.m_position;
	m_scale = other.m_scale;
	m_rotation = other.m_rotation;
	m_z_index = other.m_z_index;

	m_matrix_dirty = true;
}

Transform2D& Transform2D::operator=(const Transform2D& other){
	m_position = other.m_position;
	m_scale = other.m_scale;
	m_rotation = other.m_rotation;
	m_z_index = other.m_z_index;

	std::unique_lock acq(m_matrixLock);
	m_matrix_dirty = true;

	return *this;
}

Transform2D& Transform2D::set_position(const Vector2f& position) {
	std::unique_lock acq(m_matrixLock);
	m_position = position;

	m_matrix_dirty = true;

	return *this;
}

Transform2D& Transform2D::set_z_index(float z) {
	std::unique_lock acq(m_matrixLock);
	
	m_z_index = z;

	m_matrix_dirty = true;

	return *this;
}

Transform2D& Transform2D::set_position(float x, float y) {
	return set_position({x, y});
}
Transform2D& Transform2D::set_scale(const Vector2f& scale) {
	std::lock_guard acq(m_matrixLock);
	m_scale = scale;

	m_matrix_dirty = true;

	return *this;
}
Transform2D& Transform2D::set_scale(float scaleX, float scaleY) {
	return set_scale({scaleX, scaleY});
}
Transform2D& Transform2D::set_rotation(float degrees) {
	m_rotation = degrees * (M_PI / 180.f);

	m_matrix_dirty = true;

	return *this;
}

Transform2D& Transform2D::translate(const Vector2f& delta) {
	m_position += delta;

	m_matrix_dirty = true;

	return *this;
}

const Matrix4& Transform2D::matrix() {
	std::lock_guard acq(m_matrixLock);

	if(m_matrix_dirty){
		float sin = sinf(m_rotation);
		float cos = cosf(m_rotation);

		m_matrix = Matrix4(
						cos * m_scale.x, -sin * m_scale.y, 0, m_position.x,
						sin * m_scale.x, cos * m_scale.y, 0, m_position.y,
						0,    0,   1, m_z_index,
						0,    0,   0, 1);

		m_matrix_dirty = false;
	}

	return m_matrix;
}

}; // namespace reimu
