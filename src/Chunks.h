#pragma once

#include<vector>
#include<stdint.h>
#include<array>
#include <tuple>
#include<utility>

#include"Vector.h"

struct Chunks {
public:
	static constexpr int const chunkDimAsPow2 = 4;
	static constexpr int const chunkDim = 1 << chunkDimAsPow2; //used in vertex.shader
	static constexpr int const chunkSize = chunkDim*chunkDim*chunkDim;
	//static constexpr int const tmpChunkSize = chunkDim*chunkDim*chunkDim/(sizeof(uint16_t) * 8);
	using ChunkData = std::array<uint16_t, chunkSize>;
	//using tmpChunkData = std::array<uint8_t, tmpChunkSize>;
	
	struct AABB {
		static constexpr int64_t cd = chunkDim-1;
		static_assert( cd*cd*cd*cd*cd*cd < (1ll << 32), "two block indices must fit into 32 bits" );
		uint32_t data;
		
		AABB() = default;
		//AABB(int16_t const b1, int16_t const b2) : data{ uint32_t(uint16_t(b1)) | (uint32_t(uint16_t(b2)) << 16) } {}
		AABB(vec3i const start, vec3i const end) : data{ 
			uint32_t(uint16_t(blockIndex(start)))
			| (uint32_t(uint16_t(blockIndex(end))) << 16) 
		} {}
		
		constexpr uint32_t getData() const { return data; } //used in vectex.shader
		constexpr vec3i start() const { return indexBlock(int16_t(data&0xffff)); } //used in vectex.shader
		constexpr vec3i end() const { return indexBlock(int16_t(data>>16)); } //used in main vertex shader //used in vectex.shader
		constexpr vec3i onePastEnd() const { return end() + 1; } //used in main vertex shader //used in vectex.shader
		constexpr bool empty() const { return (end() < start()).any(); };
	};
	
	struct OptionalNeighbour {
	// -(neighbouring chunkIndex) - 1	
	private: int n;
	public:
		OptionalNeighbour() = default;
		OptionalNeighbour(int neighbour) : n{ -neighbour - 1 } {}
		explicit operator int() const { return get(); }
		
		operator bool() const { return is(); }
		bool is() const {
			return n != 0;
		}
		
		int32_t get() const {
			return int32_t(int64_t(n + 1) * -1); //-n - 1 is UB if n is integer min?
		}
	};
	
	using vec2on = vec2<OptionalNeighbour>;
	
	struct Neighbours {
		enum NeighbourDir : uint8_t {
			NEG_X = 0, POS_X, NEG_Y, POS_Y, NEG_Z, POS_z, DIR_COUNT
		};
		
		std::array<OptionalNeighbour, DIR_COUNT> n;
		
		static constexpr vec3i indexAsDir(uint8_t neighbourIndex) {
			/*static*/ constexpr vec3i const dirs[] = { vec3i{-1,0,0},vec3i{1,0,0}, vec3i{0,-1,0},vec3i{0,1,0}, vec3i{0,0,-1},vec3i{0,0,1} };
			assert(neighbourIndex >= 0 && neighbourIndex < 6);
			return dirs[neighbourIndex];
		}
		static constexpr uint8_t dirAsIndex(vec3i dir) {
			vec3i mask( dir.notEqual(0) );
			assert(mask.dot(1) == 1); //one and only one axis
			
			assert(indexAsDir( ((dir + 1)/2 + vec3i{0,2,4}).dot(mask) ) == dir);
			return ((dir + 1)/2 + vec3i{0,2,4}).dot(mask);
		}
		static constexpr uint8_t mirror(uint8_t index) {
			return dirAsIndex( -indexAsDir(index) );
		}
		
		OptionalNeighbour &operator[](uint8_t index) { return n[index]; }
		OptionalNeighbour const &operator[](uint8_t index) const { return n[index]; }
		
		//OptionalNeighbour *begin() { return n.begin(); }
		//OptionalNeighbour *end  () { return n.end  (); }
		
		//OptionalNeighbour const *cbegin() const { return n.cbegin(); }
		//OptionalNeighbour const *cend  () const { return n.cend  (); }
	}; 
	
	
	
	struct Chunk {
	private:
		Chunks *chunks_;
		int chunk_index;
	public:
		Chunk() = default;
		
		Chunk(Chunks &chunks, int const chunkIndex) : chunks_{ &chunks }, chunk_index{ chunkIndex } {}
			
		auto &chunks() { return *chunks_; }	
		auto const &chunks() const { return *chunks_; }

		auto &chunkIndex() { return chunk_index; }	
		auto const &chunkIndex() const { return chunk_index; }	
			
		#define gs(name, accessor) decltype(auto) name () { return chunks(). accessor [chunk_index]; } decltype(auto) name () const { return chunks(). accessor [chunk_index]; }	
			gs(position, chunksPos)
			gs(aabb, chunksAABB)
			gs(gpuPresent, gpuPresent) //is this safe? (gpuPresent returns rvalue reference)
			gs(data, chunksData)
			gs(neighbours, chunksNeighbours)
		#undef gs
	};
private:
	std::vector<int> vacant{};
	std::vector<int> used_{};
public:
	std::vector<int> used{};
	std::vector<vec3i> chunksPos{};
	std::vector<AABB> chunksAABB{};
	std::vector<bool> gpuPresent{};
	std::vector<ChunkData> chunksData{};
	std::vector<Neighbours> chunksNeighbours{};
	
	//returns used[] position 
	//all the chunk data is not initialised
	inline int reserve() {
		int index;
		int usedSize = used.size();

		if(!vacant.empty()) { 
			index = vacant[vacant.size()-1];
			vacant.pop_back();
		}
		else { //TODO: avoid zero-init
			index = usedSize;
			chunksPos.resize(index+1);
			chunksAABB.resize(index+1);
			gpuPresent.resize(index+1);
			chunksData.resize(index+1);
			chunksNeighbours.resize(index+1);
		}
		used.push_back(index);
		
		return usedSize;
	}

	inline void recycle(int const index) {
		auto chunkIndex = used[index];
		chunksNeighbours[chunkIndex] = Neighbours();
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
	
	inline static constexpr int16_t blockIndex(vec3<int32_t> position) {
		return position.x + position.y*Chunks::chunkDim + position.z*Chunks::chunkDim*Chunks::chunkDim;
	}
	
	inline static constexpr vec3i indexBlock(int16_t index) { //used in vectex.shader
		return vec3i{ index % chunkDim, (index / chunkDim) % chunkDim, (index / chunkDim / chunkDim) };
	}
};