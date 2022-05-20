#pragma once

#include"Units.h"
#include"Position.h"
#include"Vector.h"

#include<vector>
#include<unordered_map>
#include<stdint.h>
#include<array>
#include<tuple>
#include<utility>
#include<algorithm>


namespace chunk {
	static constexpr bool checkBlockCoordInChunkValid(vec3i const coord) {
		return coord.inMMX(vec3i{0}, vec3i{units::blocksInChunkDim}).all();
	}
	static constexpr bool checkBlockIndexInChunkValid(uint16_t const index) {
		return index < pos::blocksInChunkCount;
	}
	
	//used in main.shader
	inline static constexpr int16_t blockIndex(vec3i const coord) {
		assert(checkBlockCoordInChunkValid(coord));
		return coord.x + coord.y*units::blocksInChunkDim + coord.z*units::blocksInChunkDim*units::blocksInChunkDim;
	}
	inline static constexpr vec3i indexBlock(int16_t index) {
		assert(checkBlockIndexInChunkValid(index));
		return vec3i{ index % units::blocksInChunkDim, (index / units::blocksInChunkDim) % units::blocksInChunkDim, (index / units::blocksInChunkDim / units::blocksInChunkDim) };
	}
	
	static constexpr bool checkCubeCoordInChunkValid(vec3i const coord) {
		return coord.inMMX(vec3i{0}, vec3i{units::cubesInChunkDim}).all();
	}
	static constexpr bool checkCubeIndexInChunkValid(uint32_t const index) {
		return index < pos::cubesInChunkCount;
	}
	
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
	static vec3i cubeCoordInChunk(uint32_t const index) { ///used in main.shader
#pragma clang diagnostic pop
		assert(checkCubeIndexInChunkValid(index));
		return vec3i( index % units::cubesInChunkDim, (index / units::cubesInChunkDim) % units::cubesInChunkDim, (index / units::cubesInChunkDim / units::cubesInChunkDim) );
	}
	static uint32_t cubeIndexInChunk(vec3i const coord) { ///used in main.shader
		assert(checkCubeCoordInChunkValid(coord));
		return coord.x + coord.y*units::cubesInChunkDim + coord.z*units::cubesInChunkDim*units::cubesInChunkDim;
	}
	
	template<typename Chunks>
	struct Chunk_{
	private:
		Chunks *chunks_;
		int chunk_index;
	public:
		Chunk_() = default;
		
		Chunk_(Chunks &chunks, int const chunkIndex) : chunks_{ &chunks }, chunk_index{ chunkIndex } {}	
		
		bool operator==(Chunk_<Chunks> const other) const {
			return chunks_ == other.chunks_ && chunk_index == other.chunk_index;
		}
		
		bool operator!=(Chunk_<Chunks> const other) const {
			return !(*this == other);
		}	
			
		auto &chunks() { return *chunks_; }	
		auto const &chunks() const { return *chunks_; }

		auto &chunkIndex() { return chunk_index; }	
		auto const &chunkIndex() const { return chunk_index; }	
			
		#define gs(name, accessor) decltype(auto) name () { return chunks(). accessor [chunk_index]; } decltype(auto) name () const { return chunks(). accessor [chunk_index]; }	
			gs(position, chunksPos)
			gs(aabb, chunksAABB)
			gs(status, chunksStatus)
			gs(modified, modified)
			gs(data, chunksData)
			gs(neighbours, chunksNeighbours)
			gs(ao, chunksAO)
			gs(skyLighting, chunksSkyLighting)
			gs(blockLighting, chunksBlockLighting)
			gs(emitters, chunksEmitters)
			gs(neighbouringEmitters, chunksNeighbouringEmitters)
		#undef gs
	};
	
	struct Chunks;
	using Chunk = Chunk_<Chunks>;

	
	struct Block { //used in main.shader
		static_assert(pos::cubesInBlockCount <= 8, "cubes state must fit into 8 bits");
		
		static constexpr bool checkCubeCoordValid(vec3i const coord) {
			return coord.inMMX(vec3i{0}, vec3i{units::cubesInBlockDim}).all();
		}
		
		static constexpr bool checkCubeIndexValid(uint8_t const index) {
			return index < pos::cubesInBlockCount;
		}
		
		static constexpr uint8_t cubePosIndex(vec3i const pos) {
			assert(checkCubeCoordValid(pos));
			return pos.x + (pos.y << units::cubesInBlockDimAsPow2) + (pos.z << (units::cubesInBlockDimAsPow2*2));
		}
		
		static constexpr vec3i cubeIndexPos(uint8_t const index) {
			assert(checkCubeIndexValid(index));
			return vec3i(
				 index                                      % units::cubesInBlockDim, 
				(index >>  units::cubesInBlockDimAsPow2   ) % units::cubesInBlockDim, 
				(index >> (units::cubesInBlockDimAsPow2*2)) % units::cubesInBlockDim
			);
		}
		
		static constexpr uint8_t blockCubeMask(uint8_t const index) {
			assert(checkCubeIndexValid(index));
			return 1 << index;
		}
		
		static constexpr uint8_t blockCubeMask(vec3i const coord) {
			const auto index{ cubePosIndex(coord) };
			return 1 << index;
		}
		
		static constexpr bool blockCube(uint8_t const cubes, uint8_t const index) {
			assert(checkCubeIndexValid(index));
			return (cubes >> index) & 1;
		}
		
		static constexpr bool blockCube(uint8_t const cubes, vec3i const coord) {
			const auto index{ cubePosIndex(coord) };
			return (cubes >> index) & 1;
		}
		
		static constexpr Block fullBlock(uint16_t const id) { return Block(id, 0b1111'1111); }
		static constexpr Block emptyBlock() { return Block(0, 0); }
		static constexpr Block noNeighboursBlock(Block const it) { return Block(0, 0, true); }
		static constexpr Block neighboursBlock(Block const it) { return Block(it.id(), it.cubes(), false); }
		
		static constexpr Block idChanged(Block const it, uint16_t const id) { return Block{id, it.cubes()}; }
		static constexpr Block cubesChanged(Block const it, uint8_t const cubes) { return Block{it.id(), cubes}; }
	private:
		uint32_t data_;
	public:
		Block() = default;
		explicit constexpr Block(uint32_t const data__) : data_{ data__ } {}
		constexpr Block(uint16_t const id, uint8_t const cubes) : data_{ uint32_t(id) | (uint32_t(cubes) << 24) } {
			if(id == 0 || cubes == 0) data_ = 0;
		}		
		constexpr Block(uint16_t const id, uint8_t const cubes, bool const noNeighbours) : data_{ uint32_t(id) | (uint32_t(cubes) << 24) | (uint32_t(noNeighbours) << 17) } {
			if(id == 0 || cubes == 0) data_ = (uint32_t(noNeighbours) << 17);
		}
		uint32_t data() const { return data_; }
		uint8_t cubes() const { return uint8_t(data_ >> 24); }
		constexpr uint16_t id() const { return uint16_t(data_ & ((1 << 16) - 1)); }
		constexpr bool hasNoNeighbours() const { return ((data_ >> 17)&1) == 1; }
		
		constexpr bool cube(vec3i const coord) const { return blockCube(cubes(), coord); }
		constexpr bool cube(uint8_t const index) const { return blockCube(cubes(), index); }
		
		explicit operator bool() const { return id() != 0; }
		constexpr bool isEmpty() const { return id() == 0; }
		constexpr bool empty() const { return isEmpty(); }
		
	};
	
	
	struct AABB { //used in main.shader
		static constexpr int64_t cd = units::blocksInChunkDim-1;
		static_assert( cd*cd*cd * cd*cd*cd < (1ll << 32), "two block indices must fit into 32 bits" );
	private:
		uint32_t data;
	public:
		AABB() = default;
		//AABB(int16_t const b1, int16_t const b2) : data{ uint32_t(uint16_t(b1)) | (uint32_t(uint16_t(b2)) << 16) } {}
		AABB(vec3i const start, vec3i const end) : data{ 
			uint32_t(uint16_t(blockIndex(start)))
			| (uint32_t(uint16_t(blockIndex(end))) << 16) 
		} {
			assert(checkBlockCoordInChunkValid(start) && checkBlockCoordInChunkValid(end));
		}
		
		constexpr uint32_t getData() const { return data; } //used in vectex.shader, debug program
		constexpr vec3i start() const { return indexBlock(int16_t(data&0xffff)); } //used in vectex.shader, debug program
		constexpr vec3i end() const { return indexBlock(int16_t(data>>16)); } //used in main vertex shader //used in vectex.shader, debug program
		constexpr vec3i onePastEnd() const { return end() + 1; } //used in main vertex shader //used in vectex.shader, debug program
		constexpr bool empty() const { return (end() < start()).any(); };
	};
	
	
	struct OptionalChunkIndex { //used in main.shader
	// -(chunkIndex) - 1	
	private: int n;
	public:
		OptionalChunkIndex() = default;
		OptionalChunkIndex(int chunkIndex) : n{ -chunkIndex - 1 } {}
		explicit operator int() const { return get(); }
		
		operator bool() const { return is(); }
		bool is() const {
			return n != 0;
		}
		
		int32_t get() const { //return -1 if invalid
			return int32_t(int64_t(n + 1) * -1); //-n - 1 is UB if n is integer min?
		}
	};
	
	
	struct Neighbours {
		static constexpr int neighboursCount{ 6 };
		
		static constexpr bool checkIndexValid(uint8_t const index) {
			return index < neighboursCount;
		}
		
		static constexpr bool checkDirValid(vec3i const dir) {
			return vec3i(dir.notEqual(0)).dot(1) == 1 && dir.abs().equal(1).any();
		}
		
		//used in main.shader
			static constexpr vec3i indexAsDir(uint8_t neighbourIndex) {
				assert(checkIndexValid(neighbourIndex));
				vec3i const dirs[] = { vec3i{-1,0,0},vec3i{1,0,0},vec3i{0,-1,0},vec3i{0,1,0},vec3i{0,0,-1},vec3i{0,0,1} };
				return dirs[neighbourIndex];
			}
			
			static uint8_t dirAsIndex(vec3i const dir) {
				assert(checkDirValid(dir));
				auto const result{ (dir.x+1)/2 + (dir.y+1)/2+abs(dir.y*2) + (dir.z+1)/2+abs(dir.z*4) };
				if(indexAsDir(result) != dir) {
					std::cerr << "err: " << dir << ' ' << result << '\n';
					assert(false);
				}
				return result; 
			}
			static uint8_t mirror(uint8_t index) {
				return dirAsIndex(-indexAsDir(index));
			}
			static uint8_t mirror(vec3i const dir) {
				return dirAsIndex(-dir);
			}
		
		std::array<OptionalChunkIndex, neighboursCount> n;
		
		OptionalChunkIndex &operator[](uint8_t index) { assert(checkIndexValid(index)); return n[index]; }
		OptionalChunkIndex const &operator[](uint8_t index) const { assert(checkIndexValid(index)); return n[index]; }
		
		OptionalChunkIndex &operator[](vec3i dir) { return n[dirAsIndex(dir)]; }
		OptionalChunkIndex const &operator[](vec3i dir) const { return n[dirAsIndex(dir)]; }
	};
	
	
	struct ChunkStatus {
	private:
		uint8_t status : 2;
		
		uint8_t updateBlocks : 1;
		uint8_t updateLightingAdd : 1;
		uint8_t updateNeighbouringEmitters : 1;
		
		uint8_t blocksUpdated : 1;
		uint8_t lightingUpdated : 1;
		uint8_t neighbouringEmittersUpdated : 1;
	public:
		ChunkStatus() = default;
		
		bool isFullyLoadedGPU() const { return (status & 1) != 0; }
		void markFullyLoadedGPU() { resetStatus(); status = 1; }
		
		bool isStubLoadedGPU() const { return (status & 2) != 0; }
		void markStubLoadedGPU() { resetStatus(); status = 2; }
		
		void resetStatus() { status = 0; }
		
		
		bool isInvalidated() const { return blocksUpdated || lightingUpdated || neighbouringEmittersUpdated; }
		bool needsUpdate()   const { return updateBlocks || updateLightingAdd || updateNeighbouringEmitters; }
		
		bool isBlocksUpdated  () const { return blocksUpdated; }
		void setBlocksUpdated  (bool const val) { blocksUpdated = val; } 
		
		bool isLightingUpdated() const { return lightingUpdated; }
		void setLightingUpdated(bool const val) { lightingUpdated = val; }		
		
		bool isNeighbouringEmittersUpdated() const { return neighbouringEmittersUpdated; }
		void setNeighbouringEmittersUpdated(bool const val) { neighbouringEmittersUpdated = val; }
	
		
		void setUpdateBlocks(bool const val) { updateBlocks = val; } 
		bool isUpdateBlocks() const { return updateBlocks; }
		
		bool isUpdateLightingAdd() const { return updateLightingAdd; }
		void setUpdateLightingAdd(bool const val) { updateLightingAdd = val; }
		
		bool isUpdateNeighbouringEmitters() const { return updateNeighbouringEmitters; }
		void setUpdateNeighbouringEmitters(bool const it) { updateNeighbouringEmitters = it; }
		
	};
	
	
	struct ChunkAO {
		static constexpr int size = pos::cubesInChunkCount;
		
		static vec3i dirsForIndex(const int index) { //used in main.shader
			assert(index < 8); //at most 8 cubes share 1 vertex
			const int x = int((index % 2)       != 0); //0, 2, 4, 6 - 0; 1, 3, 5, 7 - 1
			const int y = int(((index / 2) % 2) != 0); //0, 1, 4, 5 - 0; 2, 3, 6, 7 - 1
			const int z = int((index / 4)       != 0); //0, 1, 2, 3 - 0; 4, 5, 6, 7 - 1
			return vec3i{x,y,z} * 2 - 1;
		}
	private:
		std::array<uint8_t, size> vertsBlocks;
	public:
		//ChunkAO() = default;
		
		uint8_t       &operator[](int const index)       { return vertsBlocks[index]; }
		uint8_t const &operator[](int const index) const { return vertsBlocks[index]; }
		
		uint8_t       &operator[](vec3i const cubeCoord)       { return vertsBlocks[cubeIndexInChunk(cubeCoord)]; }
		uint8_t const &operator[](vec3i const cubeCoord) const { return vertsBlocks[cubeIndexInChunk(cubeCoord)]; }
		
		void reset() { vertsBlocks.fill(0); }
	};

	
	struct ChunkLighting {
		static constexpr int size = pos::cubesInChunkCount;
		
		static constexpr int dirsCount{ 6 };
		
		static constexpr bool checkIndexValid(uint8_t const index) {
			return index < dirsCount;
		}
		
		static constexpr bool checkDirValid(vec3i const dir) {
			return vec3i(dir.notEqual(0)).dot(1) == 1 && dir.abs().equal(1).any();
		}
		
		static constexpr vec3i indexAsDir(uint8_t neighbourIndex) {
			assert(checkIndexValid(neighbourIndex));
			vec3i const dirs[] = { vec3i{-1,0,0},vec3i{1,0,0},vec3i{0,-1,0},vec3i{0,1,0},vec3i{0,0,-1},vec3i{0,0,1} };
			return dirs[neighbourIndex];
		}
		static uint8_t dirAsIndex(vec3i const dir) {
			assert(checkDirValid(dir));
			auto const result{ (dir.x+1)/2 + (dir.y+1)/2+abs(dir.y*2) + (dir.z+1)/2+abs(dir.z*4) };
			if(indexAsDir(result) != dir) {
				std::cerr << "err: " << dir << ' ' << result << '\n';
				assert(false);
			}
			return result; 
		}

	private:
		std::array<uint8_t, size> lighting;
	public:
		ChunkLighting() = default;
		ChunkLighting(uint8_t const val) {
			fill(val);
		}
		
		uint8_t       &operator[](int const cubeIndex)       { return lighting[cubeIndex]; }
		uint8_t const &operator[](int const cubeIndex) const { return lighting[cubeIndex]; }		
		
		uint8_t       &operator[](vec3i const cubeCoord)       { return lighting[cubeIndexInChunk(cubeCoord)]; }
		uint8_t const &operator[](vec3i const cubeCoord) const { return lighting[cubeIndexInChunk(cubeCoord)]; }
		
		void fill(uint8_t const val) { lighting.fill(val); }
		void reset() { fill(0); }
	};
	
	
	struct ChunkBlocksList {
		static constexpr int16_t capacity = pos::blocksInChunkCount;
		
		using value_type = int16_t; static_assert(pos::blocksInChunkCount < (1 << 15));
	private:
		std::vector<value_type> list;
	public:
		ChunkBlocksList() = default;
		
		int size() const { return list.size(); }
		bool inRange(int const index) const { return index >= 0 && index < size(); }
		
		value_type       &operator[](int const index)       { assert(inRange(index)); return list[index]; }
		value_type const &operator[](int const index) const { assert(inRange(index)); return list[index]; }	
		
		vec3i operator()(int const index) const { assert(inRange(index)); return indexBlock(list[index]); }
		
		value_type *begin() { return &list[0];      } 
		value_type *end  () { return &list[size()]; } 		
		
		value_type const *cbegin() const { return &(*this)[0]; } 
		value_type const *cend  () const { return &(*this)[size()]; } 
		
		
		
		void add(vec3i const blockCoord) {
			assert(size() != capacity);
			list.push_back(blockIndex(blockCoord));
		}
		
		void clear() {
			list.clear();
		}
	};
	
	struct ChunkData {
		static constexpr int size = pos::blocksInChunkCount;
	private:
		 std::array<Block, size> blocks;
	 public:
		Block &operator[](int const index) { 
			assert(checkBlockIndexInChunkValid(index));
			return blocks[index];
		}
		Block const &operator[](int const index) const { 
			assert(checkBlockIndexInChunkValid(index));
			return blocks[index];
		}
		
		Block &operator[](vec3i const coord) { 
			assert(checkBlockCoordInChunkValid(coord));
			return blocks[blockIndex(coord)];
		}
		
		Block const &operator[](vec3i const coord) const { 
			assert(checkBlockCoordInChunkValid(coord));
			return blocks[blockIndex(coord)];
		}
		
		struct Cube{ Block block; bool isSolid; };
		
		Cube cubeAt(vec3i const cubeCoord) const {
			auto const blockInChunkCoord{ cubeCoord / units::cubesInBlockDim };
			auto const cubeInBlockCoord { cubeCoord % units::cubesInBlockDim };
			
			auto const block{ (*this)[blockInChunkCoord] };
			auto const isCube{ block.cube(cubeInBlockCoord) };
			
			return { block, isCube };
		}
	};
	
	struct Chunk3x3BlocksList {
		static int constexpr sidelength = 3 * units::blocksInChunkDim;
		static_assert((sidelength-1)*(sidelength-1)*(sidelength-1) >= (1<<16), "sadly, we can't store block coords in 27 edjecent chunks in an unsigned short");
		static_assert((sidelength-1)*(sidelength-1)*(sidelength-1) <  (1<<17), "but we can store the coordinate in 17 bits");
		
		static int constexpr capacity = 30;
		
		bool checkCoordValid(vec3i const coord) {
			return coord.in(-units::blocksInChunkDim, units::blocksInChunkDim + units::blocksInChunkDim-1).all();
		}
		
		static int32_t coordToIndex(vec3i const coord) {
			return (coord.x + units::blocksInChunkDim)
				 + (coord.y + units::blocksInChunkDim) * sidelength
				 + (coord.z + units::blocksInChunkDim) * sidelength * sidelength;
		}
		static vec3i indexToCoord(int32_t const index) {
			return vec3i{
				index % sidelength,
				(index / sidelength) % sidelength,
				index / sidelength / sidelength
			} - units::blocksInChunkDim;
		}
	private:

		
		uint32_t bits; /*is list empty, spare bit, 30 bits for the last bit of all the coords*/
		std::array<uint16_t, capacity> coords16;
	public:
		bool isEmpty() const { return bool((bits & 1) == 0); }
		
		int32_t operator[](int const position) const { return int32_t(coords16[position]) | (int32_t((bits >> (position+2)) & 1) << 16); }
		vec3i operator()(int const position) const { return indexToCoord((*this)[position]); }
		
		void clear() { bits = 0; }
		void fillRepeated(std::array<vec3i, 30> const &coords, int const count) {
			assert(count >= 0);
			if(count == 0) { clear(); return; }
			
			uint32_t coordsBits{ 1 };
			for(int i{}; i < capacity; i++) {
				auto const blockIndex{ coordToIndex(coords[i % count]) };
				coords16[i] = uint16_t(blockIndex);
				coordsBits = coordsBits | (((blockIndex >> 16)&1) << i);
			}
			
			bits = 1 | (coordsBits << 2);
		}
	};
	static_assert(sizeof(Chunk3x3BlocksList) == sizeof(uint16_t) * 32);
	
	struct Chunks {	
	private:
		std::vector<int> vacant{};
		std::vector<int> used_{};
		
		struct PosHash { 
			inline std::size_t operator()(vec3i const &it) const noexcept { 
				return  (std::hash<int32_t>{}(it.x) ^ (std::hash<int32_t>{}(it.x) << 1)) ^ (std::hash<int32_t>{}(it.z) << 1);
			} 
		};
		
		std::vector<int> used{};
	public:
		std::vector<vec3i> chunksPos{};
		std::vector<AABB> chunksAABB{};
		std::vector<ChunkStatus> chunksStatus{};
		std::vector<bool> modified{};
		std::vector<ChunkData> chunksData{};
		std::vector<ChunkAO> chunksAO{};
		std::vector<ChunkLighting> chunksSkyLighting{};
		std::vector<ChunkLighting> chunksBlockLighting{};
		std::vector<ChunkBlocksList> chunksEmitters{};
		std::vector<Neighbours> chunksNeighbours{};
		std::vector<Chunk3x3BlocksList> chunksNeighbouringEmitters;
		std::unordered_map<vec3i, int, PosHash> chunksIndex_position{};
	
		std::vector<int>  const &usedChunks() const { return used; }
		
		Chunks() = default;
		Chunks(Chunks const&) = delete; //prevent accidental pass-by-value
		Chunks &operator=(Chunks const&) = delete;
		
		//returns used[] position
		inline int reserve() {
			int index;
			int usedSize = used.size();
	
			if(!vacant.empty()) { 
				index = vacant[vacant.size()-1];
				vacant.pop_back();
			}
			else { //TODO: avoid zero-init, allocate everything at once
				index = usedSize;
				
				chunksPos.resize(index+1);
				chunksAABB.resize(index+1);
				chunksStatus.resize(index+1);
				modified.resize(index+1);
				chunksData.resize(index+1);
				chunksAO.resize(index+1);
				chunksSkyLighting.resize(index+1);
				chunksBlockLighting.resize(index+1);
				chunksEmitters.resize(index+1);
				chunksNeighbours.resize(index+1);
				chunksNeighbouringEmitters.resize(index+1);
			}
			used.push_back(index);
			
			return usedSize;
		}
	
		inline void recycle(int const index) {
			auto chunkIndex = used[index];
			used.erase(used.begin()+index);
			vacant.push_back(chunkIndex);
		}
	
		
		template<typename Action>
		inline void forEachUsed(Action &&action) const {
			for(auto const chunkIndex : used) {
				action(chunkIndex);
			}
		}
		
		template<typename Predicate, typename Free>
		inline void filterUsed(Predicate&& keep, Free &&free) {
			auto const sz = used.size();
			
			for(size_t i = 0; i < sz; ++i) {
				auto const &chunkIndex = used[i];
				if(keep(chunkIndex)) used_.push_back(chunkIndex);
				else { free(chunkIndex); vacant.push_back(chunkIndex); }
			}
			
			used.clear();
			used.swap(used_);
		}
		
		inline Chunk operator[](int chunkIndex) {
			return Chunk{ *this, chunkIndex };
		}
	};
	
	
	struct Move_to_neighbour_Chunk {
	public:
		static bool diagonalNeighbourDirValid(vec3i const dir) {
			return (dir.abs() <= vec3i{1}).all() && dir.abs().equal(1).any();
		}
	private:
		Chunk chunk;
		bool valid;
	public:
		Move_to_neighbour_Chunk() = delete;
		
		Move_to_neighbour_Chunk(Chunks &chunks, OptionalChunkIndex oci) :
			chunk{ chunks[oci.get()] }, /*index -1 may be out of bounds but we need to keep Chunks&*/
			valid{ oci.is() }
		{}
		Move_to_neighbour_Chunk(Chunks &chunks) : chunk{chunks[0]}, valid{ false } {}/*index 0 may be out of bounds but we need to keep Chunks&*/
		Move_to_neighbour_Chunk(Chunk const src) : chunk{src}, valid{ true } {}
			
		Move_to_neighbour_Chunk(Chunks &chunks, vec3i const chunkCoord) {
			auto const chunkIndexP{ chunks.chunksIndex_position.find(chunkCoord) };
			
			valid = chunkIndexP != chunks.chunksIndex_position.end();
			
			if(valid) chunk = chunks[chunkIndexP->second];
			else chunk = chunks[0]; //index 0 may be out of bounds but we need to keep Chunks&
		}
		
		OptionalChunkIndex optChunk() const {
			if(valid) return OptionalChunkIndex{ chunk.chunkIndex() };
			return {};
		}
		
		OptionalChunkIndex move(vec3i const otherChunk) {
			auto const dir{ otherChunk - chunk.position() };
			if(valid && Neighbours::checkDirValid(dir)) return offset(dir);
			if(valid && diagonalNeighbourDirValid(dir)) return offsetDiagonal(dir);
			if(valid && otherChunk == chunk.position()) return optChunk();
			*this = Move_to_neighbour_Chunk(chunk.chunks(), otherChunk);
			return optChunk();
		}
		
		OptionalChunkIndex moveToNeighbour/*offset to neighbour*/(vec3i const neighbour) {
			if(!valid) return {};
			if(Neighbours::checkDirValid(neighbour)) return offset(neighbour);
			if(diagonalNeighbourDirValid(neighbour)) return offsetDiagonal(neighbour);
			if(neighbour == 0) return optChunk();
			*this = Move_to_neighbour_Chunk(chunk.chunks(), chunk.position() + neighbour);
			return optChunk();
		}
		

		OptionalChunkIndex offset/*to immediate neighbour*/(vec3i const dir) {
			if(dir == 0) return { chunk.chunkIndex() };
			if(!valid) return {};
			
			auto const optChunkIndex{ chunk.neighbours()[dir] };
			valid = optChunkIndex.is();
			if(valid) chunk = chunk.chunks()[optChunkIndex.get()];
			return optChunkIndex;
		}
		
		OptionalChunkIndex offsetDiagonal(vec3i const dir) {
			if(dir == 0) return { chunk.chunkIndex() };
			if(!valid) return {};
			assert(diagonalNeighbourDirValid(dir));
			
			OptionalChunkIndex outChunkIndex{ chunk.chunkIndex() };
			auto &chunks{ chunk.chunks() };
			if(dir.x != 0) outChunkIndex = Move_to_neighbour_Chunk{ chunks, outChunkIndex }.offset(vec3i(dir.x,0,0));
			if(dir.y != 0) outChunkIndex = Move_to_neighbour_Chunk{ chunks, outChunkIndex }.offset(vec3i(0,dir.y,0));
			if(dir.z != 0) outChunkIndex = Move_to_neighbour_Chunk{ chunks, outChunkIndex }.offset(vec3i(0,0,dir.z));
			return outChunkIndex;
		}
		
		bool is() const { return valid; }
	};
}