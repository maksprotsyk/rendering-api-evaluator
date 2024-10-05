#pragma once

#include <cmath>
#include <iostream>

#include "Parser.h"


namespace Engine::Utils
{
    class Vector3
	{
	public:
		float x;
		float y;
		float z;

		//constructors
		constexpr Vector3(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}
		explicit constexpr Vector3(float ia) : x(ia), y(ia), z(ia) {}
		constexpr Vector3() : x(0), y(0), z(0) {}

		Vector3 operator-() const { return Vector3(-x, -y, -z); };
		Vector3& operator+=(const Vector3& right);
		Vector3& operator-=(const Vector3& right);
		Vector3& operator*=(const Vector3& right);
		Vector3& operator/=(const Vector3& right);
		Vector3& operator*=(float k);
		Vector3& operator/=(float k);

		//functions
		void normalize();
		Vector3 normalized() const;
		float length() const;
		float lengthSqr() const;
		static float dotProduct(const Vector3& left, const Vector3& right);
		static Vector3 crossProduct(const Vector3& left, const Vector3& right);

		float angleBetweenVectors(const Vector3& otherVector) const;
		void rotateArroundVector(const Vector3& v, float rotation);

		friend Vector3 operator+(const Vector3& left, const Vector3& right);
		friend Vector3 operator-(const Vector3& left, const Vector3& right);
		friend Vector3 operator*(const Vector3& left, const Vector3& right);
		friend Vector3 operator/(const Vector3& left, const Vector3& right);
		friend Vector3 operator*(const Vector3& vec, float k);
		friend Vector3 operator/(const Vector3& vec, float k);

		SERIALIZABLE(
			PROPERTY(Vector3, x),
			PROPERTY(Vector3, y),
			PROPERTY(Vector3, z)
		)
	};

	Vector3 operator+(const Vector3& left, const Vector3& right);
	Vector3 operator-(const Vector3& left, const Vector3& right);
	Vector3 operator*(const Vector3& left, const Vector3& right);
	Vector3 operator/(const Vector3& left, const Vector3& right);
	Vector3 operator*(const Vector3& vec, float k);
	Vector3 operator/(const Vector3& vec, float k);
}
