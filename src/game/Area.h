#pragma once

#include"Vector.h"
#include"Units.h"

#include<type_traits>
#include<array>

template<typename C, typename Action> 
inline std::enable_if_t<std::is_same_v< decltype(std::declval<Action>() ( std::declval<vec3<C>>() )), void >>
iterateArea(vec3<C> const begin, vec3<C> const end, Action &&action) {
	for(C z{begin.z}; z <= end.z; ++z)
	for(C y{begin.y}; y <= end.y; ++y)
	for(C x{begin.x}; x <= end.x; ++x) {
		action(vec3<C>{x, y, z});
	}
}

struct Area {
public:
	using value_type = vec3i;
	static Area empty() {
		return Area{1, 0};
	}
public:
	value_type first; 
	value_type last; 
public:
	Area() = default;
	Area(value_type const it) : first{it}, last{it} {}
	Area(value_type const first_, value_type const last_) : first{first_}, last{last_} {}
	
	bool isEmpty() const {
		return (last < first).any();
	}
	
	bool contains(value_type const value) const {
		if(isEmpty()) return false;
		else return value.clamp(first, last) == value;
	}
	
	Area &operator*=(Area const a2) { return *this = *this * a2; }
	Area &operator+=(Area const a2) { return *this = *this + a2; }
	
	friend Area operator*(Area const a1, Area const a2) {
		using T = value_type::value_type;
		
		auto const i = [](T const v1, T const v2, T const u1, T const u2) -> std::array<T, 2> {
			auto const vi{ misc::min(v1, v2) };
			auto const va{ misc::max(v1, v2) };
			auto const ui{ misc::min(u1, u2) };
			auto const ua{ misc::max(u1, u2) };
			
			if(va < ui || ua < vi) return {0, -1};
			else return { misc::clamp(v1, u1, u2), misc::clamp(v2, u1, u2) };
		};
		
		if(a1.isEmpty()) return a1;
		if(a2.isEmpty()) return a2;
		
		std::array<T, 2> const is[] = { 
			i(a1.first.x, a1.last.x, a2.first.x, a2.last.x),
			i(a1.first.y, a1.last.y, a2.first.y, a2.last.y),
			i(a1.first.z, a1.last.z, a2.first.z, a2.last.z)
		};
		
		return {
			value_type{ is[0][0], is[1][0], is[2][0] },
			value_type{ is[0][1], is[1][1], is[2][1] }
		};
	}
	
	friend Area operator+(Area const a1, Area const a2) {
		if(a2.isEmpty()) return a1;
		if(a1.isEmpty()) return a2;
		
		return {
			a1.first.min(a2.first),
			a1.last .max(a2.last )
		};
	}
	
	friend bool operator==(Area const a1, Area const a2) {
		if(a1.isEmpty() && a2.isEmpty()) return true;
		else if(a1.isEmpty() ^ a2.isEmpty()) return false;
		else return a1.first == a2.first && a1.last == a2.last;
	}
	
	friend bool operator!=(Area const a1, Area const a2) {
		return !(a1 == a2);
	}
};

template<typename Action> 
inline std::enable_if_t<std::is_same_v< decltype(std::declval<Action>() ( std::declval<Area::value_type>() )), void >>
iterateArea(Area const area, Action &&action) {
	using C = Area::value_type::value_type;
	for(C z{area.first.z}; z <= area.last.z; ++z)
	for(C y{area.first.y}; y <= area.last.y; ++y)
	for(C x{area.first.x}; x <= area.last.x; ++x) {
		action(vec3<C>{x, y, z});
	}
}

inline auto intersectAreas3(Area const a1, Area const a2) { return a1 * a2; }
inline auto intersectAreas3i(Area const a1, Area const a2) { return intersectAreas3(a1, a2); }

inline constexpr int index3FromDir(vec3i const dir) {
	assert((dir >= -1).all() && (dir <= 1).all());
	return (dir.x+1) + (dir.y+1)*3 + (dir.z+1) * 9;
}

inline constexpr vec3i dirFromIndex3(int const i) {
	assert(i >= 0 && i < 27);
	return { (i%3)-1, ((i/3)%3)-1, ((i/9)%3)-1 };
}

template<typename T> inline void iterate3by3Volume(T &&action) {
	for(int i{}; i < 27; i++) {
		vec3i const dir{ dirFromIndex3(i) };
		
		if constexpr(std::is_convertible_v<decltype(action( vec3i{}, int(0) )), bool>) {
			if(action(dir, i)) break;	
		}
		else {
			action(dir, i);
		}
	}
}

