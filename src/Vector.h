#pragma once

#include<iostream>
#include<cmath>
#include<stdint.h>
#include<type_traits>
#include"Misc.h"

template<typename C>
struct vec2
{
	C x, y;

public:
	constexpr vec2() : x(0), y(0) {};
	constexpr vec2(C value) : x(value), y(value) {};
	constexpr vec2(C x_, C y_) : x(x_), y(y_) {};

	inline constexpr vec2<C> operator+(const vec2<C>& o) const {
		return vec2(x + o.x, y + o.y);
	}
	inline constexpr vec2<C> operator-(const vec2<C>& o) const {
		return vec2(x - o.x, y - o.y);
	}
	inline constexpr vec2<C> operator/(const vec2<C>& o) const {
		return vec2(x / o.x, y / o.y);
	}
	inline constexpr vec2<C> operator*(const vec2<C>& o) const {
		return vec2(x * o.x, y * o.y);
	}
	inline constexpr vec2<C>& operator+=(const vec2<C>& o) {
		x += o.x;
		y += o.y;
		return *this;
	}

	inline constexpr vec2<C> operator-() const {
		return vec2(-x, -y);
	}

	inline constexpr vec2<C>& operator-=(const vec2<C>& o) {
		x -= o.x;
		y -= o.y;
		return *this;
	}
	inline constexpr vec2<C>& operator*=(const vec2<C>& o) {
		x *= o.x;
		y *= o.y;
		return *this;
	}

	inline constexpr vec2<C>& operator/=(const vec2<C>& o) {
		x /= o.x;
		y /= o.y;
		return *this;
	}

	template<typename C2>
	inline constexpr vec2<C2> convertedTo() const {
		return vec2<C2>(static_cast<C2>(x), static_cast<C2>(y));
	}

	inline constexpr C dot(vec2<C> o) const {
		return x * o.x + y * o.y;
	}

	inline constexpr C lengthSuqare() const {
		return this->dot(*this);
	}

	inline constexpr C length() const {
		return sqrt(this->lengthSuqare());
	}

	inline constexpr C const &operator[](size_t const index) const {
		return *(&x + index);
	}

	inline constexpr C &operator[](size_t const index) {
		return *(&x + index);
	}

	template<typename Action>
	inline constexpr vec2<C> applied(Action&& action) const {
		return vec2<C>(
			action(this->x, 0),
			action(this->y, 1)
		);
	}
	
	template<typename Action>
	inline constexpr vec2<C> appliedFunc(Action&& action) const {
		return vec2<C>(
			action(this->x),
			action(this->y)
		);
	}
};

template<typename C>
struct vec3
{
	C x, y, z;

public:
	constexpr vec3() : x(0), y(0), z(0) {};
	constexpr vec3(C value) : x(value), y(value), z(value) {};
	constexpr vec3(C x_, C y_, C z_) : x(x_), y(y_), z(z_) {};
	
	template<typename C2, typename = std::enable_if_t<std::is_convertible<C, C2>::value>>
	inline constexpr operator vec3<C2>() const {
		return vec3<C2>{
			static_cast<C2>(x),
			static_cast<C2>(y),
			static_cast<C2>(z)
		};
	}

	inline constexpr vec3<C> operator+(const vec3<C> o) const {
		return vec3(x + o.x, y + o.y, z + o.z);
	}
	inline constexpr vec3<C> operator-(const vec3<C> o) const {
		return vec3(x - o.x, y - o.y, z - o.z);
	}
	inline constexpr vec3<C> operator/(const vec3<C> o) const {
		return vec3(x / o.x, y / o.y, z / o.z);
	}
	inline constexpr vec3<C> operator*(const vec3<C> o) const {
		return vec3(x * o.x, y * o.y, z * o.z);
	}
	
	inline constexpr vec3<C>& operator+=(const vec3<C> o) {
		x += o.x;
		y += o.y;
		z += o.z;
		return *this;
	}

	inline constexpr vec3<C> operator-() const {
		return vec3(-x, -y, -z);
	}

	inline constexpr vec3<C>& operator-=(const vec3<C> o) {
		x -= o.x;
		y -= o.y;
		z -= o.z;
		return *this;
	}
	inline constexpr vec3<C>& operator*=(const vec3<C> o) {
		x *= o.x;
		y *= o.y;
		z *= o.z;
		return *this;
	}

	inline constexpr vec3<C>& operator/=(const vec3<C> o) {
		x /= o.x;
		y /= o.y;
		z /= o.z;
		return *this;
	}

	template<typename C2>
	inline constexpr vec3<C2> convertedTo() const {
		return vec3<C2>(static_cast<C2>(x), static_cast<C2>(y), static_cast<C2>(z));
	}

	inline constexpr vec3<C> cross(vec3<C> o) const {
		return vec3<C>{
			y* o.z - z * o.y,
				z* o.x - x * o.z,
				x* o.y - y * o.x
		};
	}

	inline constexpr C dot(vec3<C> o) const {
		return x * o.x + y * o.y + z * o.z;
	}

	inline constexpr C lengthSuqare() const {
		return this->dot(*this);
	}

	inline constexpr C length() const {
		return sqrt(this->lengthSuqare());
	}

	inline constexpr vec3<C> normalized() const {
		return *this / this->length();
	}

	template<typename Action>
	inline constexpr vec3<C> applied(Action&& action) const {
		return vec3<C>(
			action(this->x, 0),
			action(this->y, 1),
			action(this->z, 2)
		);
	}
	
	template<typename Action>
	inline constexpr vec3<C> appliedFunc(Action&& action) const {
		return vec3<C>(
			action(this->x),
			action(this->y),
			action(this->z)
		);
	}
	
	inline constexpr bool in(vec3<C> const b1, vec3<C> const b2) const {
		auto const in = [](C const v, C const b1, C const b2) {
			return (b1 < b2) ? (b1 <= v && v <= b2) : (b2 <= v && v <= b1);
		};
		return in(x, b1.x, b2.x) && in(y, b1.y, b2.y) && in(z, b1.z, b2.z);
	}
};

template<typename C>
constexpr inline std::ostream& operator<<(std::ostream& stream, vec3<C>const& v) {
	return stream << '(' << v.x << "; " << v.y << "; " << v.z << ')';
}

template<typename El>
	inline constexpr vec3<El> vecMult(El const (&m1)[3][3], vec3<El> const &m2) {
		static_assert(sizeof(m2) == sizeof(El[3][1]), "");
		vec3<El> o;
		misc::matMult<El, 3, 3>(m1, *(El(*)[3][1])(void*)&m2, (El(*)[3][1])(void*)&o);
		return o;
	}
	
	template<class C>
    static inline constexpr vec3<C> vec3lerp(const vec3<C> a, const vec3<C> b, const C f) noexcept {
        return vec3<C>(
			misc::lerp(a.x, b.x, f), 
			misc::lerp(a.y, b.y, f),
			misc::lerp(a.z, b.z, f)
		);
    }


template<class C>
    static inline constexpr vec2<C> vec2lerp(const vec2<C> a, const vec2<C> b, const C f) noexcept {
        return vec2<C>(misc::lerp(a.x, b.x, f), misc::lerp(a.y, b.y, f));
    }
	
using vec3f = vec3<float>;