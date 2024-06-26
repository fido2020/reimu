#pragma once

#include <reimu/graphics/vector.h>

namespace reimu {

class Matrix4 {
public:
    // Default constructor, iniitalize with identity matrix
    Matrix4();
    // Initialize with matrix, where matrix is an array of 16 float matrices, stored column-major.
    Matrix4(const float* matrix);
    // Iniitialize using the following matrices, row major
    Matrix4(float v00, float v10, float v20, float v30,
                               float v01, float v11, float v21, float v31,
                               float v02, float v12, float v22, float v32,
                               float v03, float v13, float v23, float v33);

    Vector2f apply(const Vector2f& vector) const;

    // 2D translation
    void translate(float x, float y);
    inline void translate(const Vector2f& vector) { return translate(vector.x, vector.y); }

    // 2D scale
    void scale(float x, float y);
    inline void scale(const Vector2f& vector) { return scale(vector.x, vector.y); }

    // 2D rotation, we only have one axis to rotate around
    void rotate(float angle);

    inline const float* matrix() const { return m_matrix; }
    inline float* mut_matrix() { return m_matrix; }

    Matrix4& operator*=(const Matrix4& r);
    Matrix4& operator*=(const float* matrix);
    
    static const float s_identity_matrix[16]; // Identity matrix (no transformation)
    
private:
    float m_matrix[16]; // column-major 4x4 matrix
};

} // namespace reimu
