/**
 * \file math/matrix.cpp
 **/
#include "math/matrix.hpp"

#include "core/logger.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace other {

  glm::vec3 scale(const glm::vec3& v, float desired_len) {
    float mag = glm::length(v);
    if (glm::epsilonEqual(mag, 0.0f, glm::epsilon<float>()))
      return glm::vec3(0.0f);

    return v * desired_len / mag;
  }

  void decompose_mat4(const glm::mat4& mat, glm::vec3& out_translation, glm::quat& out_rotation, glm::vec3& out_scale) {
    using namespace glm;
    using T = float;

    out_translation = glm::vec3(0);
    out_rotation = glm::quat(1, 0, 0, 0);
    out_scale = glm::vec3(1);

    mat4 local_mat(mat);
    if (epsilonEqual(local_mat[3][3], static_cast<T>(0), epsilon<T>())) {
      return;
    }

    // Assume matrix is already normalized
    OTHER_ASSERT(epsilonEqual(local_mat[3][3], static_cast<T>(1), static_cast<T>(0.00001)), "Matrix decomposition requires normalized matrix.");
    // for (length_t i = 0; i < 4; ++i)
    //	for (length_t j = 0; j < 4; ++j)
    //		local_mat[i][j] /= local_mat[3][3];

    // Ignore perspective
    OTHER_ASSERT(
      epsilonEqual(local_mat[0][3], static_cast<T>(0), epsilon<T>()) &&
        epsilonEqual(local_mat[1][3], static_cast<T>(0), epsilon<T>()) &&
        epsilonEqual(local_mat[2][3], static_cast<T>(0), epsilon<T>()),
      "Matrix decomposition requires no perspective component."
    );
    //// perspectiveMatrix is used to solve for perspective, but it also provides
    //// an easy way to test for singularity of the upper 3x3 component.
    // mat<4, 4, T, Q> PerspectiveMatrix(local_mat);
    //
    // for (length_t i = 0; i < 3; i++)
    //	PerspectiveMatrix[i][3] = static_cast<T>(0);
    // PerspectiveMatrix[3][3] = static_cast<T>(1);
    //
    ///// TODO: Fixme!
    // if (epsilonEqual(determinant(PerspectiveMatrix), static_cast<T>(0), epsilon<T>()))
    //	return false;
    //
    //// First, isolate perspective.  This is the messiest.
    // if (
    //	epsilonNotEqual(local_mat[0][3], static_cast<T>(0), epsilon<T>()) ||
    //	epsilonNotEqual(local_mat[1][3], static_cast<T>(0), epsilon<T>()) ||
    //	epsilonNotEqual(local_mat[2][3], static_cast<T>(0), epsilon<T>()))
    //{
    //	// rightHandSide is the right hand side of the equation.
    //	vec<4, T, Q> RightHandSide;
    //	RightHandSide[0] = local_mat[0][3];
    //	RightHandSide[1] = local_mat[1][3];
    //	RightHandSide[2] = local_mat[2][3];
    //	RightHandSide[3] = local_mat[3][3];
    //
    //	// Solve the equation by inverting PerspectiveMatrix and multiplying
    //	// rightHandSide by the inverse.  (This is the easiest way, not
    //	// necessarily the best.)
    //	mat<4, 4, T, Q> InversePerspectiveMatrix = glm::inverse(PerspectiveMatrix);//   inverse(PerspectiveMatrix, inversePerspectiveMatrix);
    //	mat<4, 4, T, Q> TransposedInversePerspectiveMatrix = glm::transpose(InversePerspectiveMatrix);//   transposeMatrix4(inversePerspectiveMatrix, transposedInversePerspectiveMatrix);
    //
    //	Perspective = TransposedInversePerspectiveMatrix * RightHandSide;
    //	//  v4MulPointByMatrix(rightHandSide, transposedInversePerspectiveMatrix, perspectivePoint);
    //
    //	// Clear the perspective partition
    //	local_mat[0][3] = local_mat[1][3] = local_mat[2][3] = static_cast<T>(0);
    //	local_mat[3][3] = static_cast<T>(1);
    // }
    // else
    //{
    //	// No perspective.
    //	Perspective = vec<4, T, Q>(0, 0, 0, 1);
    // }

    // Next take care of translation (easy).
    out_translation = vec3(local_mat[3]);
    local_mat[3] = vec4(0, 0, 0, local_mat[3].w);

    vec3 row[3];

    // Now get scale and shear.
    for (length_t i = 0; i < 3; ++i)
      for (length_t j = 0; j < 3; ++j)
        row[i][j] = local_mat[i][j];

    // Compute X scale factor and normalize first row.
    out_scale.x = length(row[0]);
    row[0] = scale(row[0], static_cast<T>(1));

    // Ignore shear
    //// Compute XY shear factor and make 2nd row orthogonal to 1st.
    // Skew.z = dot(row[0], row[1]);
    // row[1] = detail::combine(row[1], row[0], static_cast<T>(1), -Skew.z);

    // Now, compute Y scale and normalize 2nd row.
    out_scale.y = length(row[1]);
    row[1] = scale(row[1], static_cast<T>(1));
    // Skew.z /= Scale.y;

    //// Compute XZ and YZ shears, orthogonalize 3rd row.
    // Skew.y = glm::dot(row[0], row[2]);
    // row[2] = detail::combine(row[2], row[0], static_cast<T>(1), -Skew.y);
    // Skew.x = glm::dot(row[1], row[2]);
    // row[2] = detail::combine(row[2], row[1], static_cast<T>(1), -Skew.x);

    // Next, get Z scale and normalize 3rd row.
    out_scale.z = length(row[2]);
    row[2] = scale(row[2], static_cast<T>(1));
    // Skew.y /= Scale.z;
    // Skew.x /= Scale.z;

#if _DEBUG
    // At this point, the matrix (in rows[]) is orthonormal.
    // Check for a coordinate system flip.  If the determinant
    // is -1, then negate the matrix and the scaling factors.
    vec3 Pdum3 = cross(row[1], row[2]);  // v3Cross(row[1], row[2], Pdum3);
    OTHER_ASSERT(dot(row[0], Pdum3) >= static_cast<T>(0), "Matrix decomposition requires a right-handed coordinate system.");
#endif
    // if (dot(row[0], Pdum3) < 0)
    //{
    //	for (length_t i = 0; i < 3; i++)
    //	{
    //		scale[i] *= static_cast<T>(-1);
    //		row[i] *= static_cast<T>(-1);
    //	}
    // }

    // Rotation as XYZ Euler angles
    // rotation.y = asin(-row[0][2]);
    // if (cos(rotation.y) != 0.f)
    //{
    //	rotation.x = atan2(row[1][2], row[2][2]);
    //	rotation.z = atan2(row[0][1], row[0][0]);
    //}
    // else
    //{
    //	rotation.x = atan2(-row[2][0], row[1][1]);
    //	rotation.z = 0;
    //}

    // Rotation as quaternion
    int i, j, k = 0;
    T root, trace = row[0].x + row[1].y + row[2].z;
    if (trace > static_cast<T>(0)) {
      root = sqrt(trace + static_cast<T>(1));
      out_rotation.w = static_cast<T>(0.5) * root;
      root = static_cast<T>(0.5) / root;
      out_rotation.x = root * (row[1].z - row[2].y);
      out_rotation.y = root * (row[2].x - row[0].z);
      out_rotation.z = root * (row[0].y - row[1].x);
    }  // End if > 0
    else {
      static int Next[3] = { 1, 2, 0 };
      i = 0;
      if (row[1].y > row[0].x) i = 1;
      if (row[2].z > row[i][i]) i = 2;
      j = Next[i];
      k = Next[j];

      root = sqrt(row[i][i] - row[j][j] - row[k][k] + static_cast<T>(1.0));

      out_rotation[i] = static_cast<T>(0.5) * root;
      root = static_cast<T>(0.5) / root;
      out_rotation[j] = root * (row[i][j] + row[j][i]);
      out_rotation[k] = root * (row[i][k] + row[k][i]);
      out_rotation.w = root * (row[j][k] - row[k][j]);
    }  // End if <= 0
  }

}  // namespace other