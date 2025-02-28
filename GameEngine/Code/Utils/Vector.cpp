#include "Vector.h"
#include "Quaternion.h"

namespace Engine::Utils
{
	//////////////////////////////////////////////////////////////////////////

	Vector3 operator+(const Vector3& left, const Vector3& right)
	{
		return Vector3(left.x + right.x, left.y + right.y, left.z + right.z);
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3 operator-(const Vector3& left, const Vector3& right)
	{
		return Vector3(left.x - right.x, left.y - right.y, left.z - right.z);
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3 operator*(const Vector3& left, const Vector3& right)
	{
		return Vector3(left.x * right.x, left.y * right.y, left.z * right.z);
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3 operator/(const Vector3& left, const Vector3& right)
	{
		return Vector3(left.x / right.x, left.y / right.y, left.z / right.z);
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3 operator*(const Vector3& vec, float k)
	{
		return Vector3(vec.x * k, vec.y * k, vec.z * k);
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3 operator/(const Vector3& vec, float k)
	{
		return Vector3(vec.x / k, vec.y / k, vec.z / k);
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3& Vector3::operator+=(const Vector3& right)
	{
		x += right.x;
		y += right.y;
		z += right.z;
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3& Vector3::operator-=(const Vector3& right)
	{
		x -= right.x;
		y -= right.y;
		z -= right.z;
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3& Vector3::operator*=(const Vector3& right)
	{
		x *= right.x;
		y *= right.y;
		z *= right.z;
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3& Vector3::operator/=(const Vector3& right)
	{
		x /= right.x;
		y /= right.y;
		z /= right.z;
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3& Vector3::operator*=(float k)
	{
		x *= k;
		y *= k;
		z *= k;
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3& Vector3::operator/=(float k)
	{
		x /= k;
		y /= k;
		z /= k;
		return *this;
	}

	//////////////////////////////////////////////////////////////////////////

	void Vector3::normalize()
	{	
		float lengthInverse = 1.0f / length();
		x *= lengthInverse;
		y *= lengthInverse;
		z *= lengthInverse;
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3 Vector3::normalized() const
	{
		float lengthInverse = 1.0f / length();
		return Vector3(x * lengthInverse, y * lengthInverse, z * lengthInverse);
	}

	//////////////////////////////////////////////////////////////////////////

	float Vector3::length() const
	{
		return std::sqrtf(lengthSqr());
	}

	//////////////////////////////////////////////////////////////////////////

	float Vector3::lengthSqr() const
	{
		return x * x + y * y + z * z;
	}

	//////////////////////////////////////////////////////////////////////////

	float Vector3::dotProduct(const Vector3& left, const Vector3& right)
	{
		return left.x * right.x + left.y * right.y + left.z * right.z;
	}

	//////////////////////////////////////////////////////////////////////////

	Vector3 Vector3::crossProduct(const Vector3& left, const Vector3& right)
	{
		return Vector3(
			left.y * right.z - left.z * right.y,
			left.z * right.x - left.x * right.z,
			left.x * right.y - left.y * right.x);
	}

	//////////////////////////////////////////////////////////////////////////

	float Vector3::angleBetweenVectors(const Vector3& otherVector) const
	{
		return std::acos(dotProduct(*this, otherVector) / std::sqrtf(lengthSqr() * otherVector.lengthSqr())
		);
	}

	//////////////////////////////////////////////////////////////////////////

	void Vector3::rotateArroundVector(const Vector3& v, float rotation)
	{
		const Quaternion rotationInQuaternionSpace(v, rotation);
		// Apply rotation
		*this = (rotationInQuaternionSpace * *this * rotationInQuaternionSpace.getConjugate()).toVector();
	}

	//////////////////////////////////////////////////////////////////////////
}
