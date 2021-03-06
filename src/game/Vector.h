#pragma once

#include"Misc.h"

#include<iostream>
#include<cmath>
#include<stdint.h>
#include<type_traits>
#include<algorithm>
#include<array>

template<typename C>
struct vec2 {
	using value_type = C;
	
	C x, y;

public:
	inline constexpr vec2() : x(0), y(0) {};
	inline constexpr vec2(C value) : x(value), y(value) {};
	inline constexpr vec2(C x_, C y_) : x(x_), y(y_) {};
	
	template<typename C2, typename = std::enable_if_t<std::is_convertible<C, C2>::value>>
	inline constexpr explicit operator vec2<C2>() const {
		return vec2<C2>{
			static_cast<C2>(x),
			static_cast<C2>(y)
		};
	}

	inline constexpr vec2<bool> operator>(const vec2<C> o) const {
		return vec2<bool>(x > o.x, y > o.y);
	}
	inline constexpr vec2<bool> operator>=(const vec2<C> o) const {
		return vec2<bool>(x >= o.x, y >= o.y);
	}
	
	inline constexpr vec2<bool> operator<(const vec2<C> o) const {
		return vec2<bool>(x < o.x, y < o.y);
	}
	inline constexpr vec2<bool> operator<=(const vec2<C> o) const {
		return vec2<bool>(x <= o.x, y <= o.y);
	}
	inline constexpr bool operator==(vec2<C> o) const {
		return x == o.x && y == o.y;
	}	
	inline constexpr bool operator!=(vec2<C> o) const {
		return !(*this == o);
	}
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
	
	inline constexpr vec2<C> operator!() const {
		return vec2(!x, !y);
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

	inline constexpr C lengthSquare() const {
		return this->dot(*this);
	}

	inline constexpr C length() const {
		return sqrt(this->lengthSquare());
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
	
	inline constexpr float cross(vec2<C> o) const {
		return x*o.y - y*o.x;
	}

	inline constexpr vec2<C> normalized() const {
		return *this / this->length();
	}
	
	inline constexpr vec2<C> floor() const {
		return this->applied([](C const &it, size_t const index) -> C { return std::floor(it); });
	}
	
	inline constexpr vec2<C> fract() const {
		return *this - floor();
	}
	
	inline constexpr C all() const {
		return x && y;
	}
	
	inline constexpr vec2<C> clamp(C const b1, C const b2) const {
		return this->applied([&](C const &it, size_t const index) -> C { return misc::clamp<C>(it, b1, b2); });
	}
	
	inline constexpr vec2<C> clamp(vec2<C> const b1, vec2<C>  const b2) const {
		return this->applied([&](C const &it, size_t const index) -> C { return misc::clamp<C>(it, b1[index], b2[index]); });
	}
	
	inline constexpr vec2<C> max(vec2<C> const o) const {
		return this->applied([&](C const &it, size_t const index) -> C { return std::max({it, o[index]}); });
	}
	
	inline constexpr vec2<C> min(vec2<C> const o) const {
		return this->applied([&](C const &it, size_t const index) -> C { return std::min({it, o[index]}); });
	}
	
	inline constexpr auto any() const {
		return x || y;
	}
};

template<typename C>
constexpr inline std::ostream& operator<<(std::ostream& stream, vec2<C>const& v) {
	return stream << '(' << v.x << "; " << v.y << ')';
}

template<typename C>
struct vec3 {	
	using value_type = C;
	
	C x, y, z;
public:
	inline constexpr vec3() : x(0), y(0), z(0) {};
	inline constexpr vec3(C value) : x(value), y(value), z(value) {};
	inline constexpr vec3(C x_, C y_, C z_) : x(x_), y(y_), z(z_) {};
	inline constexpr vec3(vec3<C> const &) = default;
	inline constexpr vec3(vec3<C> &&) = default;
	inline constexpr vec3<C> &operator=(vec3<C> const &) = default;
	inline constexpr vec3<C> &operator=(vec3<C> &&) = default;
	
	template<typename C2, typename = std::enable_if_t<std::is_convertible<C, C2>::value>>
	inline constexpr explicit operator vec3<C2>() const {
		return vec3<C2>{
			static_cast<C2>(x),
			static_cast<C2>(y),
			static_cast<C2>(z)
		};
	}

	inline constexpr vec3<bool> operator<(const vec3<C> o) const {
		return vec3<bool>(x < o.x, y < o.y, z < o.z);
	}
	inline constexpr vec3<bool> operator<=(const vec3<C> o) const {
		return vec3<bool>(x <= o.x, y <= o.y, z <= o.z);
	}
	inline constexpr vec3<bool> operator>(const vec3<C> o) const {
		return vec3<bool>(x > o.x, y > o.y, z > o.z);
	}
	inline constexpr vec3<bool> operator>=(const vec3<C> o) const {
		return vec3<bool>(x >= o.x, y >= o.y, z >= o.z);
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
	inline constexpr vec3<C> operator%(const vec3<C> o) const {
		return vec3(x % o.x, y % o.y, z % o.z);
	}
	
	inline constexpr vec3<C>& operator+=(const vec3<C> o) {
		x += o.x;
		y += o.y;
		z += o.z;
		return *this;
	}

	inline constexpr vec3<C> operator+() const {
		return vec3(+x, +y, +z);
	}
	
	inline constexpr vec3<C> operator-() const {
		return vec3(-x, -y, -z);
	}
	inline constexpr vec3<C> operator!() const {
		return vec3(!x, !y, !z);
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
	
	inline constexpr bool operator==(const vec3<C> o) const {
		return x == o.x && y == o.y && z == o.z;
	}
	
	inline constexpr bool operator!=(const vec3<C> o) const {
		return !(*this == o);
	}
	
	inline constexpr C const &operator[](size_t const index) const {
		return *(&x + index);
	}
	
	inline constexpr C &operator[](size_t const index) {
		return *(&x + index);
	}
	
	inline constexpr vec3<C> operator|(const vec3<C> o) const {
		return vec3<C>{ x | o.x, y | o.y, z | o.z };
	}
	
	inline constexpr vec3<C> operator||(const vec3<C> o) const {
		return vec3<C>{ x || o.x, y || o.y, z || o.z };
	}
	
	inline constexpr vec3<C> operator&(const vec3<C> o) const {
		return vec3<C>{ x & o.x, y & o.y, z & o.z };
	}
	
	inline constexpr vec3<C> operator&&(const vec3<C> o) const {
		return vec3<C>{ x && o.x, y && o.y, z && o.z };
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
	
	
	inline constexpr C dotNonan(vec3<C> o) const {
		return misc::nonan(x * o.x) + misc::nonan(y * o.y) + misc::nonan(z * o.z);
	}

	inline constexpr C lengthSquare() const {
		return this->dot(*this);
	}

	inline constexpr C length() const {
		return sqrt(this->lengthSquare());
	}

	inline constexpr vec3<C> normalized() const {
		return *this / this->length();
	}
	
	inline constexpr vec3<C> normalizedNonan() const {
		return (*this / this->length()).applied([](auto const c, auto i) -> C { 
			if(c==c) return c; else return C(0); 
		});
	}
	
	inline constexpr C distance(vec3<C> const other) const {
		return (*this - other).length();
	}

	template<typename Action, typename C2 = decltype(std::declval<Action>()(std::declval<C>(), 0))>
	inline constexpr vec3<C2> applied(Action&& action) const {
		return vec3<C2>(
			action(this->x, 0),
			action(this->y, 1),
			action(this->z, 2)
		);
	}
	
	template<typename Action>
	inline constexpr vec3<decltype(std::declval<Action>()(std::declval<const C &>()))> 
	appliedFunc(Action&& action) const {
		return vec3<decltype(std::declval<Action>()(std::declval<C>()))>(
			action(this->x),
			action(this->y),
			action(this->z)
		);
	}
	
	inline constexpr vec2<C> xz() const {
		return vec2<C>{ x, z };
	}
	
	inline constexpr vec3<bool> in(vec3<C> const b1, vec3<C> const b2) const {
		return vec3<bool>{misc::in(x, b1.x, b2.x), misc::in(y, b1.y, b2.y), misc::in(z, b1.z, b2.z)};
	}
	
	inline constexpr vec3<bool> inX(vec3<C> const b1, vec3<C> const b2) const {
		return vec3<bool>{misc::inX(x, b1.x, b2.x), misc::inX(y, b1.y, b2.y), misc::inX(z, b1.z, b2.z)};
	}
	
	inline constexpr vec3<bool> inMMX(vec3<C> const min, vec3<C> const maxExcluded) const {
		return applied([&](C const v, auto i) -> bool {
			return min[i] <= v && v < maxExcluded[i];
		});
	}
	
	inline constexpr vec3<int> sign() const {
		return this->appliedFunc<int(*)(C)>(misc::sign);
	}
	
	inline constexpr vec3<C> max(vec3<C> const o) const {
		return this->applied([&](C const &it, size_t const index) -> C { return std::max({it, o[index]}); });
	}
	
	inline constexpr vec3<C> min(vec3<C> const o) const {
		return this->applied([&](C const &it, size_t const index) -> C { return std::min({it, o[index]}); });
	}
	
	inline constexpr vec3<bool> equal(vec3<C> const o) const {
		return this->applied([&](C const &it, size_t const index) -> bool { return it == o[index]; });
	}
	
	inline constexpr vec3<bool> notEqual(vec3<C> const o) const {
		return this->applied([&](C const &it, size_t const index) -> bool { return it != o[index]; });
	}
	
	inline constexpr vec3<C> abs() const {
		return this->applied([](C const &it, size_t const index) -> C { return std::max({it, -it, C(0)}); });
	}
	
	inline constexpr vec3<bool> lnot() const {
		return this->applied([](C const &it, size_t const index) -> bool { return !static_cast<bool>(it); });
	}
	
	inline constexpr vec3<C> floor() const {
		return this->applied([](C const &it, size_t const index) -> C { return std::floor(it); });
	}
	
	inline constexpr vec3<C> ceil() const {
		return this->applied([](C const &it, size_t const index) -> C { return std::ceil(it); });
	}
	
	inline constexpr vec3<C> trunc() const {
		return this->applied([](C const &it, size_t const index) -> C { return std::trunc(it); });
	}
	
	
	inline constexpr vec3<C> clamp(C const b1, C const b2) const {
		return this->applied([&](C const &it, size_t const index) -> C { return misc::clamp<C>(it, b1, b2); });
	}
	
	inline constexpr vec3<C> clamp(vec3<C> const b1, vec3<C>  const b2) const {
		return this->applied([&](C const &it, size_t const index) -> C { return misc::clamp<C>(it, b1[index], b2[index]); });
	}
	
	inline constexpr C all() const {
		return x && y && z;
	}

	inline constexpr C any() const {
		return x || y || z;
	}
	
	inline constexpr C none() const {
		return !x && !y && !z;
	}
	
	inline constexpr C anyNot() const {
		return !x || !y || !z;
	}
	
	inline constexpr vec3<C> nonan() const {
		return applied([](auto const coord, auto i) -> C {
			return misc::nonan(coord);
		});
	}
	
	inline constexpr vec3<C> mod(vec3<C> const it) const {
		return this->applied([it](auto const coord, auto i) -> C {
			return misc::mod(coord, it[i]);
		});
	}
	
	inline constexpr vec3<C> operator>>(vec3<C> const it) const {
		return this->applied([it](auto const coord, auto i) -> C {
			return coord >> it[i];
		});
	}
	
	inline constexpr vec3<C> operator<<(vec3<C> const it) const {
		return this->applied([it](auto const coord, auto i) -> C {
			return coord << it[i];
		});
	}
	
	inline constexpr vec3<C> mix(vec3<C> const from, vec3<C> const to) {
		return misc::mix<vec3<C>>(from, to, *this);
	}	
	
	template<typename O>
	inline constexpr vec3<O> select(vec3<O> const from, vec3<O> const to) {
		return this->applied([&](auto const coord, auto const i) {
			if(bool(coord)) return to[i];
			else return from[i];
		});
	}
	
	inline constexpr vec3<C> atLeast(vec3<C> const other) const {
		return this->max(other);
	}	
	
	inline constexpr vec3<C> atMost(vec3<C> const other) const {
		return this->min(other);
	}
};

template<typename C>
inline constexpr std::ostream& operator<<(std::ostream& stream, vec3<C>const& v) {
	return stream << '(' << v.x << "; " << v.y << "; " << v.z << ')';
}

template<typename El>
inline constexpr vec3<El> vecMult(El const (&m1)[3][3], vec3<El> const m2) {
	static_assert(sizeof(m2) == sizeof(El[3][1]), "");
	vec3<El> o;
	misc::matMult<El, 3, 3>(m1, *(El(*)[3][1])(void*)&m2, (El(*)[3][1])(void*)&o);
	return o;
}
	
template<class C>
inline constexpr vec3<C> vec3lerp(const vec3<C> a, const vec3<C> b, const vec3<C> f) noexcept {
	return vec3<C>(
		misc::lerp(a.x, b.x, f.x), 
		misc::lerp(a.y, b.y, f.y),
		misc::lerp(a.z, b.z, f.z)
	);
}

template<class C>
inline constexpr vec2<C> vec2lerp(const vec2<C> a, const vec2<C> b, const vec2<C> f) noexcept {
	return vec2<C>(misc::lerp(a.x, b.x, f.x), misc::lerp(a.y, b.y, f.y));
}

template<typename C>
struct mat3 {
	std::array<C, 9> values;
	
	mat3(std::array<C, 9> const &values_) : values{ values_ } {}
	mat3(vec3<C> const a, vec3<C> const b, vec3<C> const c) : values{ 
		a.x, b.x, c.x,
		a.y, b.y, c.y,
		a.z, b.z, c.z,
	} {}
	
	vec3<C> operator*(vec3<C> const o) const {
		return vec3<C>{
			values[0] * o.x + values[1] * o.y + values[2] * o.z,
			values[3] * o.x + values[4] * o.y + values[5] * o.z,
			values[6] * o.x + values[7] * o.y + values[8] * o.z,
		};
	}
};
	
using mat3f = mat3<float>;
using mat3d = mat3<double>;
using mat3i = mat3<int32_t>;
using mat3l = mat3<int64_t>;
using mat3b = mat3<bool>;

using vec3f = vec3<float>;
using vec3d = vec3<double>;
using vec3i = vec3<int32_t>;
using vec3l = vec3<int64_t>;
using vec3b = vec3<bool>;

using vec2f = vec2<float>;
using vec2d = vec2<double>;
using vec2i = vec2<int32_t>;
using vec2l = vec2<int64_t>;
using vec2b = vec2<bool>;
