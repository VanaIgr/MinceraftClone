#pragma once
#include"Unit.h"
#include"Units.h"
#include"Vector.h"

#include<cmath>

namespace pos {
	namespace { template<typename T> constexpr auto cubed(T const t) { return t*t*t; } }
	
	constexpr int blocksInChunkCount = cubed( units::Block(units::Chunk{1}).value() );//blocksInChunkDim*blocksInChunkDim*blocksInChunkDim;
	constexpr int cubesInBlockCount = cubed( units::Cube(units::Block{1}).value() );//cubesInBlockDim*cubesInBlockDim*cubesInBlockDim;
	constexpr int cubesInChunkCount = cubed( units::Cube(units::Chunk{1}).value() );//cubesInChunkDim*cubesInChunkDim*cubesInChunkDim;
			  
	struct UnitsHierarchy;

	//units ID
	struct UnitFractional;
	struct UnitChunk;
	struct UnitBlock;
	struct UnitCube;

	//units
	using Fractional = unit::Unit<UnitsHierarchy, UnitFractional, vec3l>;
	using Chunk      = unit::Unit<UnitsHierarchy, UnitChunk     , vec3i>;
	using Block      = unit::Unit<UnitsHierarchy, UnitBlock     , vec3i>;
	using Cube       = unit::Unit<UnitsHierarchy, UnitCube      , vec3i>;
}

//type conversions
struct pos::UnitsHierarchy {	
	using hierarchy = typename unit::ConstructList<Chunk, Block, Cube, Fractional>::type;
			
	template<typename T> struct UnitInfo;
	template<> struct UnitInfo<Chunk>      { static constexpr Fractional::value_type baseFactor = units::fracInChunkDimAsPow2; };
	template<> struct UnitInfo<Block>      { static constexpr Fractional::value_type baseFactor = units::fracInBlockDimAsPow2; };
	template<> struct UnitInfo<Cube>       { static constexpr Fractional::value_type baseFactor = units::fracInCubeDimAsPow2;  };
	template<> struct UnitInfo<Fractional> { static constexpr Fractional::value_type baseFactor = 0;                           };
};

namespace pos {
	//copies from Units.h, but for positions
	static constexpr inline Fractional posToFrac(vec3d value) { return Fractional::create((value*units::fracInBlockDim).floor()); }
	static constexpr inline Fractional posToFracTrunk(vec3d value) { return Fractional::create((value*units::fracInBlockDim).trunc()); }
	static constexpr inline Fractional posToFracRAway(vec3d value) {  //round away from zero
		auto const val{ (value*units::fracInBlockDim) };
		return Fractional::create( val.abs().ceil().applied([&](auto const coord, auto i) -> double {
			return std::copysign(coord, val[i]);
		}) ); 
	}
	
	static constexpr inline vec3d fracToPos(Fractional value) { return static_cast<vec3d>(value.value()) / units::fracInBlockDim; }
}

/*struct Position {		
	static constexpr inline vec3l posToFrac(vec3d value) { return vec3l{(value*units::fracInBlockDim).floor()}; }
	static constexpr inline vec3l posToFracTrunk(vec3d value) { return vec3l{(value*units::fracInBlockDim).trunc()}; }
	static constexpr inline vec3l posToFracRAway(vec3d value) {  //round away from zero
		auto const val{ (value*units::fracInBlockDim) };
		return vec3l{ val.abs().ceil().applied([&](auto const coord, auto i) -> double {
			return std::copysign(coord, val[i]);
		}) }; 
	}
	template<template<typename> typename Cont>
	static constexpr inline Cont<int64_t> blockToFrac(Cont<int64_t> value) { return Cont<int64_t>(value)*units::fracInBlockDim; }
	template<template<typename> typename Cont>
	static constexpr inline Cont<int64_t> blockCubeToFrac(Cont<int32_t> value) { return Cont<int64_t>(value)*units::fracInCubeDim; }
	static constexpr inline vec3l chunkToFrac(vec3i value) { return vec3l(value) * units::fracInChunkDim; }
	static constexpr inline vec3l cubeToFrac(vec3l value) { return value * units::fracInCubeDim; }
	
	static constexpr inline vec3l chunkToBlock(vec3i value) { return vec3l(value) * units::blocksInChunkDim; }
	
	static constexpr inline vec3d fracToPos(vec3l value) { return static_cast<vec3d>(value) / units::fracInBlockDim; }
	static constexpr inline vec3l fracToBlock(vec3l value) { return value >> units::fracInBlockDimAsPow2; }
	static constexpr inline vec3i fracTochunk(vec3l value) { return vec3i(value >> units::fracInChunkDimAsPow2); }
	static constexpr inline vec3l fracToBlockCube(vec3l value) { return value >> units::fracInCubeDimAsPow2; }
	
	
	vec3l coordinate;
	
	struct Fractional   { vec3l it; explicit Fractional   (vec3l t) : it{t} {} };
	struct Cube         { vec3l it; explicit Cube         (vec3l t) : it{t} {} };
	struct Block        { vec3l it; explicit Block        (vec3l t) : it{t} {} };
	struct BlocksDouble { vec3d it; explicit BlocksDouble (vec3d t) : it{t} {} };
	struct Chunk        { vec3i it; explicit Chunk        (vec3i t) : it{t} {} };
		
	Position() = default;
	Position(vec3l const coord) : coordinate{ coord } {}
	Position(Chunk const chunk) : coordinate{ chunkToFrac(chunk.it) } {}
	Position(vec3i const chunk, Fractional   const coord ) : Position{chunkToFrac(chunk) + coord.it} {}
	Position(vec3i const chunk, BlocksDouble const blocks) : Position{chunk, Fractional{posToFrac(blocks.it)}} {}
	Position(vec3i const chunk, Cube         const cube  ) : Position{chunk, Fractional{cubeToFrac(cube.it )}} {}
	Position(vec3i const chunk, Block        const block ) : Position{chunk, Fractional{blockToFrac(block.it )}} {}

	
	Position(Position const &) = default;
	Position(Position &&) = default;
	
	Position & operator=(Position const &) = default;
	Position & operator=(Position &&) = default;
	
	constexpr inline vec3l coord()  const { return coordinate; } 
	
	constexpr inline vec3l coordInChunk()  const { return coordinate.mod(units::fracInChunkDim); } 
	vec3i chunk() const { return vec3i(coordinate >> units::fracInChunkDimAsPow2); }
	
	constexpr inline vec3l block()        const { return fracToBlock(coord()    ); }
	constexpr inline vec3i blockInChunk() const { return vec3i(fracToBlock(coordInChunk())); }
	
	constexpr inline vec3l cube()        const { return fracToBlockCube(coord()); }
	constexpr inline vec3i cubeInChunk() const { return vec3i(fracToBlockCube(coordInChunk())); }
	
	constexpr inline vec3d position()        const { return fracToPos(coord()    ); }
	constexpr inline vec3d positionInChunk() const { return fracToPos(coordInChunk()); }
	
	
	friend Position operator+(Position const c1, Position const c2) { return Position{c1.coord() + c2.coord()}; }
	friend Position &operator+=(Position    &c1, Position const c2) { return c1 = c1+c2; }
	
	friend Position operator+(Position const c1, Fractional const offset) { return Position{ c1.coord() + offset.it}; } 
	friend Position &operator+=(Position    &c1, Fractional const offset) { return c1 = c1 + offset; }
	
	friend Position operator+(Position const c1, Block const offset) { return c1 + Fractional{blockToFrac(offset.it)}; }
	friend Position &operator+=(Position    &c1, Block const offset) { return c1 = c1 + offset; }
	
	friend Position operator+(Position const c1, Chunk const offset) { return c1 + Fractional{chunkToFrac(offset.it)}; }
	friend Position &operator+=(Position    &c1, Chunk const offset) { return c1 = c1 + offset; }
	
	friend Position operator+(Position const c1, vec3d const offset) { return c1 + Fractional{posToFrac(offset)}; }
	friend Position &operator+=(Position    &c1, vec3d const offset) { return c1 = c1 + offset; }
	
	
	friend Position operator-(Position const c1, Position const c2) { return Position{c1.coord() - c2.coord()}; }
	friend Position &operator-=(Position    &c1, Position const c2) { return c1 = c1-c2; }
	
	friend Position operator-(Position const c1, Fractional const offset) { return Position{ c1.coord() - offset.it}; } 
	friend Position &operator-=(Position    &c1, Fractional const offset) { return c1 = c1 - offset; }
	
	friend Position operator-(Position const c1, Block const offset) { return c1 - Fractional{blockToFrac(offset.it)}; }
	friend Position &operator-=(Position    &c1, Block const offset) { return c1 = c1 - offset; }
	
	friend Position operator-(Position const c1, Chunk const offset) { return c1 - Fractional{chunkToFrac(offset.it)}; }
	friend Position &operator-=(Position    &c1, Chunk const offset) { return c1 = c1 - offset; }
	
	friend Position operator-(Position const c1, vec3d const offset) { return c1 - Fractional{posToFrac(offset)}; }
	friend Position &operator-=(Position    &c1, vec3d const offset) { return c1 = c1 - offset; }
	
	friend std::ostream& operator<<(std::ostream& stream, Position const &v) {
		return stream << "Position{" << v.chunk() << ", " << v.positionInChunk() << '}';
	}
};*/