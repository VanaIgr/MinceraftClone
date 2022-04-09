﻿#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
	#include <GLEW/glew.h>
#pragma clang diagnostic pop

#include <GLFW/glfw3.h>

#include"Vector.h"
#include"Chunks.h"
#include"ChunkCoord.h"
#include"Viewport.h"

#include"Font.h"
#include"ShaderLoader.h"
#include"Read.h"
#include"Misc.h"
#include"PerlinNoise.h"
#include"MeanCounter.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include<iostream>
#include<chrono>
#include<optional>
#include<vector>
#include<array>
#include<limits>
#include<tuple>
#include<sstream>
#include<fstream>

//#define FULLSCREEN

#ifdef FULLSCREEN
static const vec2<uint32_t> windowSize{ 1920, 1080 };
#else
static const vec2<uint32_t> windowSize{ 1280, 720 };
#endif // FULLSCREEN

static constexpr bool loadChunks = false, saveChunks = false;

GLFWwindow* window;

static int viewDistance = 3;

static const vec2<double> windowSize_d{ windowSize.convertedTo<double>() };

static vec2<double> mousePos(0, 0), pmousePos(0, 0);

static bool shift{ false }, ctrl{ false };

static bool isPan{ false };

static bool debug{ false };

static double const height{ 1.95 };
static double const width{ 0.6 };

static int64_t const width_i{ ChunkCoord::posToFracRAway(width).x }; 
static int64_t const height_i{ ChunkCoord::posToFracRAway(height).x };

static double       deltaTime{ 16.0/1000.0 };
static double const fixedDeltaTime{ 16.0/1000.0 };

static double speedModifier = 2.5;
static double playerSpeed{ 2.7 };
static double spectatorSpeed{ 0.2 };

static double const aspect{ windowSize_d.y / windowSize_d.x };

static bool isOnGround{false};

static vec3d playerForce{};
static vec3l const playerCameraOffset{ 0, ChunkCoord::posToFrac(height*0.85).x, 0 };
static ChunkCoord playerCoord{ vec3i{0,0,0}, ChunkCoord::Position{vec3d{0.01,12.001,0.01}} };
static ChunkCoord playerCamPos{playerCoord + playerCameraOffset};
static Viewport playerViewport{ 
	vec2d{ misc::pi / 2.0, 0 }
};

static ChunkCoord spectatorCoord{ playerCoord };

static Camera playerCamera {
	aspect,
	90.0 / 180.0 * misc::pi,
	0.001,
	800
};

static bool breakFullBlock{ false };

static Viewport viewportDesired{ playerViewport };

static bool isSpectator{ false };

static bool isSmoothCamera{ false };
static double zoom = 3;

static double currentZoom() {
	if(isSmoothCamera) return zoom;
	return 1;
}

static Camera &currentCamera() {
	return playerCamera;
}

static Viewport &viewport_current() {
	return playerViewport;
}

static ChunkCoord &currentCoord() {
	if(isSpectator) return spectatorCoord;
	return playerCoord;
}

static ChunkCoord currentCameraPos() {
	if(isSpectator) return spectatorCoord;
	return playerCamPos;
}

struct Input {
	vec3i movement;
	bool jump;
};

static Input playerInput, spectatorInput;
static vec2d deltaRotation{ 0 };

Input &currentInput() {
	if(isSpectator) return spectatorInput;
	else return playerInput;
}

static void reloadShaders();

static bool testInfo = false;
static bool debugBtn0 = false, debugBtn1 = false, debugBtn2 = false;


bool mouseCentered = true;

enum class BlockAction {
	NONE = 0,
	PLACE,
	BREAK
} static blockAction{ BlockAction::NONE };

static double const blockActionCD{ 300.0 / 1000.0 };

static int blockPlaceId = 1;

static void quit();

enum class Key : uint8_t { RELEASE = GLFW_RELEASE, PRESS = GLFW_PRESS, REPEAT = GLFW_REPEAT, NOT_PRESSED };
static_assert(GLFW_RELEASE >= 0 && GLFW_RELEASE < 256 && GLFW_PRESS >= 0 && GLFW_PRESS < 256 && GLFW_REPEAT >= 0 && GLFW_REPEAT < 256);
static Key keys[GLFW_KEY_LAST+1];

void handleKey(int const key) {
	auto const action{ misc::to_underlying(keys[key]) };
	
	bool isPress = !(action == GLFW_RELEASE);
	if(key == GLFW_KEY_GRAVE_ACCENT && !isPress) {
		mouseCentered = !mouseCentered;
		if(mouseCentered) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	if(key == GLFW_KEY_ESCAPE && !isPress) {
		quit();
	}
	if(key == GLFW_KEY_W)
		currentInput().movement.z = 1 * isPress;
	else if(key == GLFW_KEY_S)
		currentInput().movement.z = -1 * isPress;
	
	if(isSpectator) {
		if (key == GLFW_KEY_Q)
			currentInput().movement.y = 1 * isPress;
		else if(key == GLFW_KEY_E)
			currentInput().movement.y = -1 * isPress;
	}
	else if(key == GLFW_KEY_SPACE) {
		currentInput().jump = isPress;
	}
	
	if(key == GLFW_KEY_Z && !isPress) {
		breakFullBlock = !breakFullBlock;
	}
	
	if(key == '-' && isPress) zoom = 1 + (zoom - 1) * 0.95;
	else if(key == '=' && isPress) zoom = 1 + (zoom - 1) * (1/0.95);
		
	     if(key == GLFW_KEY_KP_0 && !isPress) debugBtn0 = !debugBtn0;
	else if(key == GLFW_KEY_KP_1 && !isPress) debugBtn1 = !debugBtn1;	
	else if(key == GLFW_KEY_KP_2 && !isPress) debugBtn2 = !debugBtn2;

	if(key == GLFW_KEY_D)
		currentInput().movement.x = 1 * isPress;
	else if(key == GLFW_KEY_A)
		currentInput().movement.x = -1 * isPress;
	
	if(key == GLFW_KEY_F5 && action == GLFW_PRESS)
		reloadShaders();
	else if(key == GLFW_KEY_F4 && action == GLFW_PRESS)
		debug = !debug;
	else if(key == GLFW_KEY_F3 && action == GLFW_RELEASE) { 
		if(!isSpectator) {
			spectatorCoord = currentCameraPos();
		}
		isSpectator = !isSpectator;
	}
	else if(key == GLFW_KEY_TAB && action == GLFW_PRESS) testInfo = true;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(key == GLFW_KEY_UNKNOWN) return;
	if(action == GLFW_REPEAT) return;
	
	if(action == GLFW_PRESS) keys[key] = Key::PRESS;
	else if(action == GLFW_RELEASE) keys[key] = Key::RELEASE;
	
	handleKey(key);
	
	shift = (mods & GLFW_MOD_SHIFT) != 0;
    ctrl = (mods & GLFW_MOD_CONTROL) != 0;
}

static void cursor_position_callback(GLFWwindow* window, double mousex, double mousey) {
	static vec2d pMouse{0};
    static vec2<double> relativeTo{ 0, 0 };
    vec2<double> mousePos_{ mousex,  mousey };
    
	if(mouseCentered) {
		relativeTo += mousePos_ - pMouse;
	} else {
		relativeTo += mousePos_;
		mousePos_.x = misc::modf(mousePos_.x, windowSize_d.x);
		mousePos_.y = misc::modf(mousePos_.y, windowSize_d.y);
		relativeTo -= mousePos_;
		glfwSetCursorPos(window, mousePos_.x, mousePos_.y);
	}
	
	pMouse = mousePos_;
    mousePos = vec2<double>(relativeTo.x + mousePos_.x, relativeTo.y + mousePos_.y);
	
	if(isPan || mouseCentered) {
		deltaRotation += (mousePos - pmousePos) / windowSize_d;
	}
	
	pmousePos = mousePos;
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
		if(mouseCentered) blockAction = action == GLFW_PRESS ? BlockAction::BREAK : BlockAction::NONE;
		else isPan = action != GLFW_RELEASE;
	}
	else if(button == GLFW_MOUSE_BUTTON_RIGHT) {
		if(mouseCentered) blockAction = action == GLFW_PRESS ? BlockAction::PLACE : BlockAction::NONE;
	}
	else if(button == GLFW_MOUSE_BUTTON_MIDDLE) {
		isSmoothCamera = action != GLFW_RELEASE;
		if(!isSmoothCamera) {
			viewportDesired.rotation = viewport_current().rotation; //may reset changes from cursor_position_callback
		}
	}
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	blockPlaceId = 1+misc::mod(blockPlaceId-1 + int(yoffset), 12);
}

static const Font font{ ".\\assets\\font.txt" };

enum Textures : GLuint {
	atlas_it = 0,
	font_it,
	noise_it,
	texturesCount
};

static GLuint textures[texturesCount];
static GLuint &atlas_t = textures[atlas_it], 
			  &font_t = textures[font_it], 
			  &noise_t = textures[noise_it];

static GLuint framebuffer;

static GLuint mainProgram = 0;
  static GLuint rightDir_u;
  static GLuint topDir_u;
  static GLuint near_u, far_u;
  static GLuint fov_u;
  static GLuint playerRelativePosition_u, drawPlayer_u;
  static GLuint startChunkIndex_u;
  static GLuint time_u;
  static GLuint mouseX_u, mouseY_u;
  static GLuint projection_u, toLocal_matrix_u;
  static GLuint playerChunk_u, playerInChunk_u, chunksPostions_ssbo, chunksBounds_ssbo, chunksNeighbours_ssbo;

static GLuint mapChunks_p;

static GLuint fontProgram;

static GLuint testProgram;
	static GLuint tt_projection_u, tt_toLocal_u;
	
static GLuint debugProgram;
	static GLuint db_projection_u, db_toLocal_u, db_isInChunk_u;
	static GLuint db_playerChunk_u, db_playerInChunk_u;
	
static GLuint currentBlockProgram;
  static GLuint cb_blockIndex_u;

static GLuint blockHitbox_p;
  static GLuint blockHitboxProjection_u, blockHitboxModelMatrix_u;

static GLuint chunkIndex_u;
static GLuint blockSides_u;

static int32_t gpuChunksCount = 0;
Chunks chunks{};


void resizeBuffer() {
	//assert(newGpuChunksCount >= 0);
	gpuChunksCount = chunks.used.size();
	auto &it = chunks.gpuChunksStatus;
	
	it.assign(it.size(), Chunks::GPUChunkStatus{});
	
	static_assert(sizeof(Chunks::ChunkData{}) == 16384);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkIndex_u);
	glBufferData(GL_SHADER_STORAGE_BUFFER, gpuChunksCount * sizeof(Chunks::ChunkData{}), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, chunkIndex_u);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);	
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunksPostions_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, gpuChunksCount * sizeof(vec3i), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, chunksPostions_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunksBounds_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, gpuChunksCount * sizeof(uint32_t), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, chunksBounds_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunksNeighbours_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, gpuChunksCount * Chunks::Neighbours::neighboursCount * sizeof(int32_t), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, chunksNeighbours_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

static void reloadShaders() {
	glDeleteTextures(texturesCount, &textures[0]);
	glGenTextures(texturesCount, &textures[0]);
	
	/*{
		//color
		glActiveTexture(GL_TEXTURE0 + framebufferColor_it);
		glBindTexture(GL_TEXTURE_2D, framebufferColor_t);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowSize.x, windowSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		//depth
		glActiveTexture(GL_TEXTURE0 + framebufferDepth_it);
		glBindTexture(GL_TEXTURE_2D, framebufferDepth_t);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, windowSize.x, windowSize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		
		glDeleteFramebuffers(1, &framebuffer);
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferColor_t, 0);
		  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebufferDepth_t, 0);
		  
		  GLenum status;
		  if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
		  	fprintf(stderr, "framebuffer: error %u", status);
		  }
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}*/
	
	{//noise texture
		Image image{};
		ImageLoad("assets/noise.bmp", &image);
		glActiveTexture(GL_TEXTURE0 + noise_it);
		glBindTexture(GL_TEXTURE_2D, noise_t);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.sizeX, image.sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data);
	}
	
	{ //main program
		glDeleteProgram(mainProgram);
		mainProgram = glCreateProgram();
		ShaderLoader sl{};

		//sl.addShaderFromProjectFileName("shaders/vertex.shader",GL_VERTEX_SHADER,"main vertex");
		//https://gist.github.com/rikusalminen/9393151
		//... usage: glDrawArrays(GL_TRIANGLES, 0, 36), disable all vertex arrays
		sl.addShaderFromProjectFileName(
			"shaders/vertex.shader",
			GL_VERTEX_SHADER,
			"main vertex"
		);
		sl.addShaderFromProjectFileName("shaders/main.shader", GL_FRAGMENT_SHADER, "Main shader");
	
		sl.attachShaders(mainProgram);
	
		glLinkProgram(mainProgram);
		glValidateProgram(mainProgram);
	
		sl.deleteShaders();
	
		glUseProgram(mainProgram);
		
		glUniform2ui(glGetUniformLocation(mainProgram, "windowSize"), windowSize.x, windowSize.y);
		
		time_u   = glGetUniformLocation(mainProgram, "time");
		mouseX_u = glGetUniformLocation(mainProgram, "mouseX");
		mouseY_u = glGetUniformLocation(mainProgram, "mouseY");
	
	
		rightDir_u = glGetUniformLocation(mainProgram, "rightDir");
		topDir_u = glGetUniformLocation(mainProgram, "topDir");
		near_u = glGetUniformLocation(mainProgram, "near");
		far_u = glGetUniformLocation(mainProgram, "far");
		projection_u = glGetUniformLocation(mainProgram, "projection");
		toLocal_matrix_u = glGetUniformLocation(mainProgram, "toLocal");
		//chunk_u = glGetUniformLocation(mainProgram, "chunk");
	
		playerChunk_u = glGetUniformLocation(mainProgram, "playerChunk");
		playerInChunk_u = glGetUniformLocation(mainProgram, "playerInChunk");
		startChunkIndex_u = glGetUniformLocation(mainProgram, "startChunkIndex");
		
		playerRelativePosition_u = glGetUniformLocation(mainProgram, "playerRelativePosition");
		drawPlayer_u = glGetUniformLocation(mainProgram, "drawPlayer");
	
		Image image;
		ImageLoad("assets/atlas.bmp", &image);
	
		glActiveTexture(GL_TEXTURE0 + atlas_it);
		glBindTexture(GL_TEXTURE_2D, atlas_t);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.sizeX, image.sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data);
		glUniform2f(glGetUniformLocation(mainProgram, "atlasTileCount"), 512 / 16, 512 / 16); //in current block program, in block hitbox program
		
		glUniform1i(glGetUniformLocation(mainProgram, "atlas"), atlas_it);
		
		glUniform1i(glGetUniformLocation(mainProgram, "noise"), noise_it);
		
		{
			auto const c = [](int16_t const x, int16_t const y) -> int32_t {
				return int32_t( uint32_t(uint16_t(x)) | (uint32_t(uint16_t(y)) << 16) );
			}; //pack coord
			int32_t const sides[] = { //side, top, bot;
				c(0, 0), c(0, 0), c(0, 0), //0
				c(1, 0), c(2, 0), c(3, 0), //grass
				c(3, 0), c(3, 0), c(3, 0), //dirt
				c(4, 0), c(4, 0), c(4, 0), //planks
				c(5, 0), c(6, 0), c(6, 0), //wood
				c(7, 0), c(7, 0), c(7, 0), //leaves
				c(8, 0), c(8, 0), c(8, 0), //stone
				c(9, 0), c(9, 0), c(9, 0), //glass
				c(11, 0), c(11, 0), c(11, 0), //diamond
				c(12, 0), c(12, 0), c(12, 0), //obsidian
				c(13, 0), c(13, 0), c(13, 0), //rainbow?
				c(14, 0), c(14, 0), c(14, 0), //brick
				c(15, 0), c(15, 0), c(15, 0), //stone brick
			};
			glGenBuffers(1, &blockSides_u);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, blockSides_u);
			glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(sides), &sides, GL_STATIC_DRAW);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, blockSides_u);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		}
		/*GLuint ssbo;
		glGenBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(field), field, GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);*/
		
		/*GLuint chunkIndex_b = glGetUniformBlockIndex(mainProgram, "Chunk");
		glGenBuffers(1, &chunkUBO);
		glBindBuffer(GL_UNIFORM_BUFFER, chunkUBO);
		glUniformBlockBinding(mainProgram, chunkIndex_b, 1);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, chunkUBO);
		glBufferData(GL_UNIFORM_BUFFER, 12, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);*/
		
		//relativeChunkPos_u = glGetUniformLocation(mainProgram, "relativeChunkPos");
		
		glDeleteBuffers(1, &chunkIndex_u);
		glGenBuffers(1, &chunkIndex_u);
		glDeleteBuffers(1, &chunksPostions_ssbo);
		glGenBuffers(1, &chunksPostions_ssbo);
		
		glDeleteBuffers(1, &chunksBounds_ssbo);
		glGenBuffers(1, &chunksBounds_ssbo);
		glDeleteBuffers(1, &chunksNeighbours_ssbo);
		glGenBuffers(1, &chunksNeighbours_ssbo);

		resizeBuffer();
	}
	
	{ //font program
		glDeleteProgram(fontProgram);
		fontProgram = glCreateProgram();
		ShaderLoader sl{};
		
		sl.addShaderFromCode(
		R"(#version 300 es
			precision mediump float;
			
			layout(location = 0) in vec2 pos_s;
			layout(location = 1) in vec2 pos_e;
			layout(location = 2) in vec2 uv_s;
			layout(location = 3) in vec2 uv_e;
			
			//out vec2 uv;
			
			out vec2 startPos;
			out vec2 endPos;
			
			out vec2 startUV;
			out vec2 endUV;
			void main(void){
				vec2 interp = vec2(gl_VertexID % 2, gl_VertexID / 2);
				gl_Position = vec4(mix(pos_s, pos_e, interp), 0, 1);
				//uv = mix(uv_s, uv_e, interp);
				startPos = pos_s;
				endPos   = pos_e;
				startUV  = uv_s ;
				endUV    = uv_e ;
			}
		)", GL_VERTEX_SHADER,"font vertex");
		
		sl.addShaderFromCode(
		R"(#version 420
			in vec4 gl_FragCoord;
			//in vec2 uv;
			
			in vec2 startPos;
			in vec2 endPos;
			
			in vec2 startUV;
			in vec2 endUV;
			
			uniform sampler2D font;
			uniform vec2 screenSize;
			
			out vec4 color;
			
			float col(vec2 coord) {
				const vec2 pos = (coord / screenSize) * 2 - 1;
				const vec2 uv = startUV + (pos - startPos) / (endPos - startPos) * (endUV - startUV);
				
				return texture2D(font, clamp(uv, startUV, endUV)).r;
			}
			
			float rand(const vec2 co) {
				return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
			}
			
			float sampleN(const vec2 coord, const uint n, const vec2 startRand) {
				const vec2 pixelCoord = floor(coord);
				const float fn = float(n);
			
				float result = 0;
				for (uint i = 0; i < n; i++) {
					for (uint j = 0; j < n; j++) {
						const vec2 curCoord = pixelCoord + vec2(i / fn, j / fn);
						const vec2 offset = vec2(rand(startRand + curCoord.xy), rand(startRand + curCoord.xy + i+1)) / fn;
						const vec2 offsetedCoord = curCoord + offset;
			
						const float sampl = col(offsetedCoord);
						result += sampl;
					}
				}
			
				return result / (fn * fn);
			}

			void main() {
				const float col = sampleN(gl_FragCoord.xy, 4, startUV);
				
				color = vec4(vec3(0), 1-col);
			}
		)",
		GL_FRAGMENT_SHADER,
		"font shader");
		
		sl.attachShaders(fontProgram);
	
		glLinkProgram(fontProgram);
		glValidateProgram(fontProgram);
	
		sl.deleteShaders();
	
		glUseProgram(fontProgram);
		
		Image image;
		ImageLoad("assets/font.bmp", &image);
	
		glActiveTexture(GL_TEXTURE0 + font_it);
		glBindTexture(GL_TEXTURE_2D, font_t);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.sizeX, image.sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data);
        //glBindTexture(GL_TEXTURE_2D, 0);
		
		GLuint const fontTex_u = glGetUniformLocation(fontProgram, "font");
		glUniform1i(fontTex_u, font_it);
		
		glUniform2f(glGetUniformLocation(fontProgram, "screenSize"), windowSize_d.x, windowSize_d.y);
	}
	
	{ //test program
		glDeleteProgram(testProgram);
		testProgram = glCreateProgram();
		ShaderLoader sl{};
		
		sl.addShaderFromCode(
		R"(#version 420
			precision mediump float;
			uniform mat4 projection;
			uniform mat4 toLocal;
			
			layout(location = 0) in vec3 relativePos;
			layout(location = 1) in vec3 color_;
			
			out vec3 col;
			void main(void){
				//const mat4 translation = {
				//	vec4(1,0,0,0),
				//	vec4(0,1,0,0),
				//	vec4(0,0,1,0),
				//	vec4(relativePos, 1)
				//};			
		
				const mat4 model_matrix = toLocal;// * translation;
				
				gl_Position = projection * (model_matrix * vec4(relativePos, 1.0));
				col = color_;
			}
		)", GL_VERTEX_SHADER,"test vertex");
		
		sl.addShaderFromCode(
		R"(#version 420			
			out vec4 color;
			
			in vec3 col;
			void main() {
				color = vec4(col, 1);
			}
		)",
		GL_FRAGMENT_SHADER,
		"test shader");
		
		sl.attachShaders(testProgram);
	
		glLinkProgram(testProgram);
		glValidateProgram(testProgram);
	
		sl.deleteShaders();
		
		tt_toLocal_u = glGetUniformLocation(testProgram, "toLocal");
		tt_projection_u = glGetUniformLocation(testProgram, "projection");
	}
	
	{ //debug program
		glDeleteProgram(debugProgram);
		debugProgram = glCreateProgram();
		ShaderLoader dbsl{};
		
		//sl.addShaderFromProjectFileName("shaders/vertex.shader",GL_VERTEX_SHADER,"main vertex");
		//https://gist.github.com/rikusalminen/9393151
		//... usage: glDrawArrays(GL_TRIANGLES, 0, 36), disable all vertex arrays
		dbsl.addShaderFromCode(R"(#version 430
			uniform mat4 projection;
			uniform mat4 toLocal;
			
			uniform ivec3 playerChunk;
			uniform  vec3 playerInChunk;
			
			in layout(location = 0) uint chunkIndex_;
			
			layout(binding = 3) restrict readonly buffer ChunksPoistions {
				int positions[];
			} ps;

			layout(binding = 4) restrict readonly buffer ChunksBounds {
				uint bounds[];
			} bs;
			
			ivec3 chunkPosition(const uint chunkIndex) {
				const uint index = chunkIndex * 3;
				return ivec3(
					ps.positions[index+0],
					ps.positions[index+1],
					ps.positions[index+2]
				);
			}
			
			uint chunkBounds(const uint chunkIndex) {
				return bs.bounds[chunkIndex];
			}
			
			//copied from Chunks.h
			#define chunkDim 16u

			vec3 indexBlock(const uint index) { //copied from Chunks.h
				return vec3( index % chunkDim, (index / chunkDim) % chunkDim, (index / chunkDim / chunkDim) );
			}
			vec3 start(const uint data) { return indexBlock(data&65535u); } //copied from Chunks.h
			vec3 end(const uint data) { return indexBlock((data>>16)&65535u); } //copied from Chunks.h
			vec3 onePastEnd(const uint data) { return end(data) + 1; } //copied from Chunks.h
			
			vec3 relativeChunkPosition(const uint chunkIndex) {
				return vec3( (chunkPosition(chunkIndex) - playerChunk) * chunkDim ) - playerInChunk;
			}
			
			void main()
			{
				int tri = gl_VertexID / 3;
				int idx = gl_VertexID % 3;
				int face = tri / 2;
				int top = tri % 2;
			
				int dir = face % 3;
				int pos = face / 3;
			
				int nz = dir >> 1;
				int ny = dir & 1;
				int nx = 1 ^ (ny | nz);
			
				vec3 d = vec3(nx, ny, nz);
				float flip = 1 - 2 * pos;
			
				vec3 n = flip * d;
				vec3 u = -d.yzx;
				vec3 v = flip * d.zxy;
			
				float mirror = -1 + 2 * top;
				vec3 xyz = n + mirror*(1-2*(idx&1))*u + mirror*(1-2*(idx>>1))*v;
				xyz = (xyz + 1) / 2;
				
				const vec3 relativeChunkPos_ = relativeChunkPosition(chunkIndex_);
				const uint positions_ = chunkBounds(chunkIndex_);
				
				const mat4 translation = {
					vec4(1,0,0,0),
					vec4(0,1,0,0),
					vec4(0,0,1,0),
					vec4(relativeChunkPos_, 1)
				};			
		
				const mat4 model_matrix = toLocal * translation;
				
				const vec3 startPos = start(positions_);
				const vec3 endPos = onePastEnd(positions_);
				const vec3 position = mix(startPos, endPos, xyz);
			
				gl_Position = projection * (model_matrix * vec4(position, 1.0));
			}
		)", GL_VERTEX_SHADER, "debug vertex");
		dbsl.addShaderFromCode(
			R"(#version 420
			uniform bool isInChunk;
			out vec4 color;
			void main() {
				color = vec4(float(!isInChunk), 0, float(isInChunk), 1);
			})",
			GL_FRAGMENT_SHADER, "debug shader"
		);
	
		dbsl.attachShaders(debugProgram);
	
		glLinkProgram(debugProgram);
		glValidateProgram(debugProgram);
	
		dbsl.deleteShaders();
	
		glUseProgram(debugProgram);
		
		db_toLocal_u = glGetUniformLocation(debugProgram, "toLocal");
		db_projection_u = glGetUniformLocation(debugProgram, "projection");
		db_isInChunk_u = glGetUniformLocation(debugProgram, "isInChunk");
		
		db_playerChunk_u = glGetUniformLocation(debugProgram, "playerChunk");
		db_playerInChunk_u = glGetUniformLocation(debugProgram, "playerInChunk");
	}
	
	{ //block hitbox program
		glDeleteProgram(blockHitbox_p);
		blockHitbox_p = glCreateProgram();
		ShaderLoader sl{};

		sl.addShaderFromCode(
			R"(#version 420
			uniform mat4 projection;
			uniform mat4 modelMatrix;
			
			out vec2 uv;
			void main()
			{
				int tri = gl_VertexID / 3;
				int idx = gl_VertexID % 3;
				int face = tri / 2;
				int top = tri % 2;
			
				int dir = face % 3;
				int pos = face / 3;
			
				int nz = dir >> 1;
				int ny = dir & 1;
				int nx = 1 ^ (ny | nz);
			
				vec3 d = vec3(nx, ny, nz);
				float flip = 1 - 2 * pos;
			
				vec3 n = flip * d;
				vec3 u = -d.yzx;
				vec3 v = flip * d.zxy;
			
				float mirror = -1 + 2 * top;
				vec3 xyz = n + mirror*(1-2*(idx&1))*u + mirror*(1-2*(idx>>1))*v;
				xyz = (xyz + 1) / 2;
			
				gl_Position = projection * (modelMatrix * vec4(xyz, 1.0));
				uv = (vec2(mirror*(1-2*(idx&1)), mirror*(1-2*(idx>>1)))+1) / 2;
			}
			)", GL_VERTEX_SHADER, "Block hitbox vertex"
		);
		
		sl.addShaderFromCode(
			R"(#version 420
			in vec2 uv;
			out vec4 color;
			
			uniform vec2 atlasTileCount;
			uniform sampler2D atlas;
			
			vec3 sampleAtlas(const vec2 offset, const vec2 coord) {
				vec2 uv = vec2(
					coord.x + offset.x,
					coord.y + atlasTileCount.y - (offset.y + 1)
				) / atlasTileCount;
				return texture(atlas, uv).rgb;
			}

			void main() {
				const vec3 value = sampleAtlas(vec2(31), uv);
				if(dot(value, vec3(1)) / 3 > 0.9) discard;
				color = vec4(value, 0.8);
			}
			)", 
			GL_FRAGMENT_SHADER,  
			"Block hitbox fragment"
		);
	
		sl.attachShaders(blockHitbox_p);
	
		glLinkProgram(blockHitbox_p);
		glValidateProgram(blockHitbox_p);
	
		sl.deleteShaders();
	
		glUseProgram(blockHitbox_p);
		
		glUniform1i(glGetUniformLocation(blockHitbox_p, "atlas"), atlas_it);
		glUniform2f(glGetUniformLocation(blockHitbox_p, "atlasTileCount"), 512 / 16, 512 / 16);
		
		blockHitboxModelMatrix_u = glGetUniformLocation(blockHitbox_p, "modelMatrix");
		blockHitboxProjection_u  = glGetUniformLocation(blockHitbox_p, "projection");
	}
	
	{ //current block program
		glDeleteProgram(currentBlockProgram);
		currentBlockProgram = glCreateProgram();
		ShaderLoader sl{};
		
		sl.addShaderFromCode(
		R"(#version 430		
			uniform vec2 startPos;
			uniform vec2 endPos;
			
			out vec2 uv;
			void main(void){
				const vec2 verts[] = {
					vec2(0),
					vec2(1, 0),
					vec2(0, 1),
					vec2(1)
				};
				
				gl_Position = vec4( mix(startPos, endPos, verts[gl_VertexID]), 0.0, 1 );
				//gl_Position = vec4(  verts[gl_VertexID], 0.0, 1 );
				uv = verts[gl_VertexID];
			}
		)", GL_VERTEX_SHADER,"current block vertex");
		
		sl.addShaderFromCode(
		R"(#version 430
			uniform uint block;
			uniform vec2 atlasTileCount;
			uniform sampler2D atlas;
			
			layout(binding = 2) restrict readonly buffer AtlasDescription {
				int positions[]; //16bit xSide, 16bit ySide; 16bit xTop, 16bit yTop; 16bit xBot, 16bit yBot 
			};
			
			vec3 sampleAtlas(const vec2 offset, const vec2 coord) {
				vec2 uv = vec2(
					coord.x + offset.x,
					coord.y + atlasTileCount.y - (offset.y + 1)
				) / atlasTileCount;
				return pow(texture2D(atlas, uv).rgb, vec3(2.2));
			}
			
			vec2 atlasAt(const uint id, const ivec3 side) {
				const int offset = int(side.y == 1) + int(side.y == -1) * 2;
				const int index = (int(id) * 3 + offset);
				const int pos = positions[index];
				const int bit16 = 65535;
				return vec2( pos&bit16, (pos>>16)&bit16 );
			}

			in vec2 uv;
			
			out vec4 color;
			void main() {
				const vec2 offset = atlasAt(block, ivec3(1,0,0));
				color = vec4(sampleAtlas(offset, uv), 1);
			}
		)",
		GL_FRAGMENT_SHADER,
		"current block shader");
		
		sl.attachShaders(currentBlockProgram);
	
		glLinkProgram(currentBlockProgram);
		glValidateProgram(currentBlockProgram);
	
		sl.deleteShaders();
	
		glUseProgram(currentBlockProgram);
		
		glUniform1i(glGetUniformLocation(currentBlockProgram, "atlas"), atlas_it);
		cb_blockIndex_u = glGetUniformLocation(currentBlockProgram, "block");
		
		double const size{ 0.12 };
		vec2d const end{ 1 - 0.04*aspect, 1-0.04 };
		vec2d const start{ end - vec2d{aspect,1}*size };

		glUniform2f(glGetUniformLocation(currentBlockProgram, "startPos"), start.x, start.y);
		glUniform2f(glGetUniformLocation(currentBlockProgram, "endPos"), end.x, end.y);
		glUniform2f(glGetUniformLocation(currentBlockProgram, "atlasTileCount"), 512 / 16, 512 / 16);
	}
}

template<typename T, typename L>
inline void apply(size_t size, T &t, L&& l) {
	for(size_t i = 0; i < size; ++i) 
		if(l(t[i], i) == true) break;
}

template<typename T, int32_t maxSize, typename = std::enable_if<(maxSize>0)>>
struct CircularArray {
private:
	T arr[maxSize];
	uint32_t curIndex;
public:
	CircularArray() : curIndex{0} {};
	
	T *begin() { return arr[0]; }
	T *end() { return arr[size()]; }
	T const *cbegin() const { return arr[0]; }
	T const *cend() const { return arr[size()]; }
	
	template<typename T2>
	void push(T2 &&el) {
		auto ind = curIndex & ~(0x80000000);
		auto const flag = curIndex & (0x80000000);
		arr[ ind ] = std::forward<T2>(el);
		
		curIndex = (ind+1 == maxSize) ? 0x80000000 : (ind+1 | flag);
	}
	
	int32_t size() const {
		bool const max{ (curIndex & 0x80000000) != 0 };
		return max ? maxSize : curIndex;
	}
	T &operator[](int32_t index) {
		return arr[index];
	}
};

double heightAt(vec2i const flatChunk, vec2i const block) {
	static siv::PerlinNoise perlin{ (uint32_t)rand() };
	auto const value = perlin.octave2D(
		(flatChunk.x * 1.0 * Chunks::chunkDim + block.x) / 20.0, 
		(flatChunk.y * 1.0 * Chunks::chunkDim + block.y) / 20.0, 
		3
	);
						
	return misc::map<double>(misc::clamp<double>(value,-1,1), -1, 1, 5, 15);
}

vec3i getTreeBlock(vec2i const flatChunk) {
	static auto const random = [](vec2i const co) -> double {
		auto const fract = [](auto it) -> auto { return it - std::floor(it); };
		return fract(sin( vec2d(co).dot(vec2d(12.9898, 78.233)) ) * 43758.5453);
	};
	
	vec2i const it( 
		(vec2d(random(vec2i{flatChunk.x+1, flatChunk.y}), random(vec2i{flatChunk.x*3, flatChunk.y*5})) 
		 * Chunks::chunkDim)
		.floor().clamp(0, 15)
	);
	
	auto const height{ heightAt(flatChunk,it) };
	
	assert(height >= 0 && height < 16);
	return vec3i{ it.x, int32_t(std::floor(height))+1, it.y };
}

std::string chunkFilename(Chunks::Chunk const &chunk) {
	auto const &pos{ chunk.position() };
	std::stringstream ss{};
	ss << "./save/" << pos << ".cnk";
	return ss.str();
}

std::string chunkNewFilename(Chunks::Chunk const &chunk) {
	auto const &pos{ chunk.position() };
	std::stringstream ss{};
	ss << "./save2/" << pos << ".cnk2";
	return ss.str();
}

void writeChunk(Chunks::Chunk &chunk) {
	if(!saveChunks) return;
	else (std::cout << "not implemented\n"), (assert(false));
	auto const &data{ chunk.data() };
	
	std::ofstream chunkFileOut{ chunkNewFilename(chunk), std::ios::binary };
	
	for(int x{}; x < Chunks::chunkDim; x++) 
	for(int y{}; y < Chunks::chunkDim; y++) 
	for(int z{}; z < Chunks::chunkDim; z++) {
		vec3i const blockCoord{x,y,z};
		auto const blockData = data[Chunks::blockIndex(blockCoord)].data();
		
		uint8_t const blk[] = { 
			(unsigned char)((blockData >> 0) & 0xff), 
			(unsigned char)((blockData >> 8) & 0xff),
			(unsigned char)((blockData >> 16) & 0xff),
			(unsigned char)((blockData >> 24) & 0xff),
		};
		chunkFileOut.write(reinterpret_cast<char const *>(&blk[0]), 4);
	}
}

bool tryReadChunk(Chunks::Chunk &chunk, vec3i &start, vec3i &end) {
	std::cout << "not implemented\n";
	assert(false);
	return false;
	/*auto &data{ chunk.data() };
	
	auto const filename2{ chunkNewFilename(chunk) };
	std::ifstream chunkFileIn2{ filename2, std::ios::binary };	
	if(!chunkFileIn2.fail()) {
		for(int x{}; x < Chunks::chunkDim; x++) 
		for(int y{}; y < Chunks::chunkDim; y++) 
		for(int z{}; z < Chunks::chunkDim; z++) 
		{
			vec3i const blockCoord{x,y,z};
			//uint16_t &block = data[Chunks::blockIndex(blockCoord)];
			uint8_t blk[4];
			
			chunkFileIn2.read( reinterpret_cast<char *>(&blk[0]), 4 );
			
			uint32_t const block( 
				  (uint32_t(blk[0]) << 0 )
				| (uint32_t(blk[1]) << 8 )
				| (uint32_t(blk[2]) << 16)
				| (uint32_t(blk[3]) << 24)					
			);
			
			data[Chunks::blockIndex(blockCoord)] = Chunks::Block(block);
			if(block != 0) {
				start = start.min(blockCoord);
				end   = end  .max(blockCoord);
			}
		}

		return true;
	}
	chunkFileIn2.close();
	
	auto const filename{ chunkFilename(chunk) };
	std::ifstream chunkFileIn{ filename, std::ios::binary };
	if(!chunkFileIn.fail()) {
		for(int x{}; x < Chunks::chunkDim; x++) 
		for(int y{}; y < Chunks::chunkDim; y++) 
		for(int z{}; z < Chunks::chunkDim; z++) {
			vec3i const blockCoord{x,y,z};
			//uint16_t &block = data[Chunks::blockIndex(blockCoord)];
			uint8_t blk[2];
			
			chunkFileIn.read( reinterpret_cast<char *>(&blk[0]), 2 );
			
			uint16_t const block( blk[0] | (uint16_t(blk[1]) << 8) );
			
			data[Chunks::blockIndex(blockCoord)] = Chunks::Block::fullBlock(block);
			if(block != 0) {
				start = start.min(blockCoord);
				end   = end  .max(blockCoord);
			}
		}

		return true;
	}
	
	
	return false;*/
}

void genTrees(vec3i const chunk, Chunks::ChunkData &data, vec3i &start, vec3i &end) {	
	for(int32_t cx{-1}; cx <= 1; cx ++) {
		for(int32_t cz{-1}; cz <= 1; cz ++) {
			vec3i const chunkOffset{ cx, -chunk.y, cz };
			auto const curChunk{ chunk + chunkOffset };
			
			auto const treeBlock{ getTreeBlock(vec2i{curChunk.x, curChunk.z}) };
			
			for(int32_t x{-2}; x <= 2; x++) {
				for(int32_t y{0}; y < 6; y++) {
					for(int32_t z{-2}; z <= 2; z++) {
						vec3i tl{ x,y,z };// tree-local block
						auto const blk{ chunkOffset * Chunks::chunkDim + treeBlock + tl };
						auto const index{ Chunks::blockIndex(blk) };
						Chunks::Block &curBlock{ data[index] };
						
						if(blk.inMMX(vec3i{0}, vec3i{Chunks::chunkDim}).all() && curBlock.id() == 0) {
							bool is = false;
							if((is = tl.x == 0 && tl.z == 0 && tl.y <= 4)) curBlock = Chunks::Block::fullBlock(4);
							else if((is = 
									 (tl.y >= 2 && tl.y <= 3
									  && !( (abs(x) == abs(z))&&(abs(x)==2) )
								     ) || 
								     (tl.in(vec3i{-1, 4, -1}, vec3i{1, 5, 1}).all()
									  && !( (abs(x) == abs(z))&&(abs(x)==1) &&(tl.y==5 || (treeBlock.x*(x+1)/2+treeBlock.z*(z+1)/2)%2==0) )
									 )
							)) curBlock = Chunks::Block::fullBlock(5);
							
							if(is) {
								start = start.min(blk);
								end   = end  .max(blk);
							}
						}
					}
				}
			}
		}
	}
}

void genChunkData(double const (&heights)[Chunks::chunkDim * Chunks::chunkDim], Chunks::Chunk chunk, vec3i &start, vec3i &end) {
	auto const &pos{ chunk.position() };
	auto &data{ chunk.data() };
	
	for(int z = 0; z < Chunks::chunkDim; ++z)
		for(int y = 0; y < Chunks::chunkDim; ++y) 
			for(int x = 0; x < Chunks::chunkDim; ++x) {
				vec3i const blockCoord{ x, y, z };
				
				auto const height{ heights[z * Chunks::chunkDim + x] };
				auto const index{ Chunks::blockIndex(blockCoord) };
				//if(misc::mod(int32_t(height), 9) == misc::mod((pos.y * Chunks::chunkDim + y + 1), 9)) { //repeated floor
				double const diff{ height - double(pos.y * Chunks::chunkDim + y) };
				if(diff >= 0) {
					uint16_t block = 0;
					if(diff < 1) block = 1; //grass
					else if(diff < 5) block = 2; //dirt
					else block = 6; //stone
					data[index] = Chunks::Block::fullBlock(block);
					
					start = start.min(blockCoord);
					end   = end  .max(blockCoord);
				}
				else {
					data[index] = Chunks::Block::emptyBlock();
				}
			}
		
	genTrees(pos, data, *&start, *&end);	
}

void fillChunkData(double const (&heights)[Chunks::chunkDim * Chunks::chunkDim], Chunks::Chunk chunk) {
	auto const &index{ chunk.chunkIndex() };
	auto const &pos{ chunk.position() };
	auto &chunks{ chunk.chunks() };
	auto &data{ chunk.data() };
	auto &aabb{ chunk.aabb() };
	
	vec3i start{15};
	vec3i end  {0 };
	
	if(loadChunks) (std::cout << "loading chunks is not supported yet\n"), exit(-1);// && tryReadChunk(chunk, start, end)) chunk.modified() = false;
	else { genChunkData(heights, chunk, start, end); chunk.modified() = true; }

	aabb = Chunks::AABB(start, end);
}

static void genChunksColumnAt(vec2i const columnPosition) {
	double heights[Chunks::chunkDim * Chunks::chunkDim];
	for(int z = 0; z < Chunks::chunkDim; z++) 
	for(int x = 0; x < Chunks::chunkDim; x++) {
		heights[z* Chunks::chunkDim + x] = heightAt(vec2i{columnPosition.x,columnPosition.y}, vec2i{x,z});
	}
	
	vec3i const neighbourDirs[] = { 
		vec3i{-1,0,0}, vec3i{+1,0,0}, vec3i{0,0,-1}, vec3i{0,0,+1}
	};
	
	Chunks::Move_to_neighbour_Chunk neighbourChunks[] = {              
		{chunks, vec3i{columnPosition.x, -16, columnPosition.y} + neighbourDirs[0]},
		{chunks, vec3i{columnPosition.x, -16, columnPosition.y} + neighbourDirs[1]},
		{chunks, vec3i{columnPosition.x, -16, columnPosition.y} + neighbourDirs[2]},
		{chunks, vec3i{columnPosition.x, -16, columnPosition.y} + neighbourDirs[3]}
	};
	Chunks::OptionalChunkIndex botNeighbourIndex{};
	
	for(int32_t y = -16; y < 16; y++) {
		int32_t const usedIndex{ chunks.reserve() };
		auto const chunkIndex{ chunks.used[usedIndex] };
		vec3i chunkPosition{ vec3i{columnPosition.x, y, columnPosition.y} };
		
		chunks.chunksPos[chunkIndex] = chunkPosition;
		chunks.chunksIndex_position[chunkPosition] = chunkIndex;
		chunks.gpuChunksStatus[chunkIndex].reset();
		
		auto chunk{ chunks[chunkIndex] };
		auto &neighbours_{ chunk.neighbours() };
		Chunks::Neighbours neighbours{};
		
		for(int j{}; j < 4; j++) {
			vec3i const offset{ neighbourDirs[j] };
			
			auto const neighbourIndex{ neighbourChunks[j].optChunk().get() };
			
			if(neighbourIndex >= 0) {
				neighbours[offset] = Chunks::OptionalNeighbour(neighbourIndex);
				chunks[neighbourIndex].neighbours()[Chunks::Neighbours::mirror(offset)] = chunkIndex;
				chunks[neighbourIndex].gpuChunkStatus().setNeedsUpdate();
			}
			else neighbours[offset] = Chunks::OptionalNeighbour();
		}
		
		{
			vec3i const offset{ 0, -1, 0 };
			auto const neighbourIndex{ botNeighbourIndex.get() };
			
			if(neighbourIndex >= 0) {
				neighbours[offset] = Chunks::OptionalNeighbour(neighbourIndex);
				chunks[neighbourIndex].neighbours()[Chunks::Neighbours::mirror(offset)] = chunkIndex;
			}
			else neighbours[offset] = Chunks::OptionalNeighbour();
		}
		
		neighbours_ = neighbours;
		
		fillChunkData(heights, Chunks::Chunk{ chunks, chunkIndex });
		
		for(int i{}; i < 4; i ++) {
			neighbourChunks[i].offset(vec3i{0,1,0});
		}
		botNeighbourIndex = Chunks::OptionalChunkIndex{ chunkIndex };
	}
}

static void updateChunks() {
	if(debugBtn1) return;
	static std::vector<bool> chunksPresent{};
	auto const viewWidth = (viewDistance*2+1);
	chunksPresent.resize(viewWidth*viewWidth);
	chunksPresent.assign(chunksPresent.size(), false);
	
	vec3i const playerChunk{ currentCoord().chunk() };
	
	chunks.filterUsed([&](int chunkIndex) -> bool { //keep
			auto const relativeChunkPos = chunks.chunksPos[chunkIndex] - vec3i{ playerChunk.x, 0, playerChunk.z };
			auto const chunkInBounds{ relativeChunkPos.in(vec3i{-viewDistance, -16, -viewDistance}, vec3i{viewDistance, 15, viewDistance}).all() };
			auto const relativeChunkPosPositive = relativeChunkPos + vec3i{viewDistance, 16, viewDistance};
			auto const index{ relativeChunkPosPositive.x + relativeChunkPosPositive.z * viewWidth };
			if(chunkInBounds) {
				chunksPresent[index] = true;	
				return true;
			} 
			return false;
		}, 
		[&](int chunkIndex) -> void { //free chunk
			auto chunk{ chunks[chunkIndex] };
			chunks.chunksIndex_position.erase( chunk.position() );
			if(chunk.modified()) {
				writeChunk(chunk);
				chunk.modified() = false;
			}
			auto const &neighbours{ chunk.neighbours() };
			for(int i{}; i < Chunks::Neighbours::neighboursCount; i++) {
				auto const &optNeighbour{ neighbours[i] };
				if(optNeighbour) {
					auto const neighbourIndex{ optNeighbour.get() };
					chunks[neighbourIndex].neighbours()[Chunks::Neighbours::mirror(i)] = Chunks::OptionalNeighbour();
					chunks[neighbourIndex].gpuChunkStatus().reset();
				}
			}
		}
	);
	
	for(int i = 0; i < viewWidth; i ++) {
		for(int k = 0; k < viewWidth; k ++) {
			auto const index{ chunksPresent[i+k*viewWidth] };
			if(index == false) {//generate chunk
				auto const relativeChunksPos{ vec2i{i, k} - vec2i{viewDistance} };
				auto const chunksPos{ playerChunk.xz() + relativeChunksPos };
			
				genChunksColumnAt(chunksPos);
			}
		}	
	}
}

struct PosDir {
	vec3l start;
	vec3l end;
	
	vec3i direction;
	vec3i chunk;
	
	PosDir(ChunkCoord const coord, vec3l const line): 
		start{ coord.coordInChunk() },
		end{ start + line },
		
		direction{ line.sign() },
		chunk{ coord.chunk() }
	{} 
	
	constexpr int64_t atCoord(vec3i const inAxis_, int64_t coord, vec3i const outAxis_) const {
		vec3l const inAxis( inAxis_ );
		vec3l const outAxis( outAxis_ );
		auto const ist = start.dot(inAxis);
		auto const ind = end.dot(inAxis); 
		auto const ost = start.dot(outAxis); 
		auto const ond = end.dot(outAxis); 
		return ist == ind ? ost : ( ost + (ond - ost)*(coord-ist) / (ind - ist) );		
	};
	
	int difference(vec3l const p1, vec3l const p2) const {
		vec3l const diff{p1 - p2};
		return (diff * vec3l(direction)).sign().dot(1);
	};
	
	constexpr vec3l part_at(vec3i inAxis, int64_t const coord) const { 
		return vec3l{
			atCoord(inAxis, coord, vec3i{1,0,0}),
			atCoord(inAxis, coord, vec3i{0,1,0}),
			atCoord(inAxis, coord, vec3i{0,0,1})
		};
	}
	ChunkCoord at(vec3i inAxis, int64_t const coord) const { return ChunkCoord{ chunk, ChunkCoord::Fractional{part_at(inAxis, coord)} }; }
	
	friend std::ostream &operator<<(std::ostream &o, PosDir const v) {
		return o << "PosDir{" << v.start << ", " << v.end << ", " << v.direction << ", " << v.chunk << "}";
	}
};

struct DDA {
private:
	//const
	PosDir posDir;
	
	//mutable
	vec3l current;
	bool end;
public:
	DDA(PosDir const pd_) : 
	    posDir{ pd_ },
		current{ pd_.start },
		end{ false }
	{}
	
	vec3b next() {
		if(end) return {};

		struct Candidate { vec3l coord; vec3b side; };
		
		vec3l const nextC{ nextCoords(current, posDir.direction) };
		
		Candidate const candidates[] = {
			Candidate{ posDir.part_at(vec3i{1,0,0}, nextC.x), vec3b{1,0,0} },
			Candidate{ posDir.part_at(vec3i{0,1,0}, nextC.y), vec3b{0,1,0} },
			Candidate{ posDir.part_at(vec3i{0,0,1}, nextC.z), vec3b{0,0,1} },
			Candidate{ posDir.end, 0 }//index == 3
		};
		
		int minI{ 3 };
		Candidate minCand{ candidates[minI] };
		
		for(int i{}; i < 4; i++) {
			auto const cand{ candidates[i] };
			auto const diff{ posDir.difference(cand.coord, minCand.coord) };
			if(posDir.difference(cand.coord, current) > 0 && diff <= 0) {
				minI = i;
				if(diff == 0) {
					minCand = Candidate{ 
						minCand.coord * vec3l(!vec3b(cand.side)) + cand.coord * vec3l(cand.side),
						minCand.side || cand.side
					};
				}
				else {
					minCand = cand;
				}
			}
		}
		
		end = minI == 3;
		current = minCand.coord;
		
		return minCand.side;
	}
	
	static constexpr vec3l nextCoords(vec3l const current, vec3i const dir) {
		return current.applied([&](auto const coord, auto const i) -> int64_t {
			//return ((coord / ChunkCoord::fracBlockDim) + std::max(dir[i], 0)) << ChunkCoord::fracBlockDimAsPow2;
			
			if(dir[i] >= 0) //round down
				return ((coord >> ChunkCoord::fracCubeDimAsPow2) + 1) << ChunkCoord::fracCubeDimAsPow2;
			else //round up
				return (-((-coord) >> ChunkCoord::fracCubeDimAsPow2) - 1) << ChunkCoord::fracCubeDimAsPow2;
		});
	};
	
	#define G(name) auto get_##name() const { return name ; }
	G(posDir)
	G(current)
	G(end)
	#undef G
};

static bool checkIntersection(Chunks &chunks, ChunkCoord const coord1, ChunkCoord const coord2) {
	ChunkCoord const c1{ coord1.coord().min(coord2.coord()) };
	ChunkCoord const c2{ coord1.coord().max(coord2.coord()) };
	Chunks::Move_to_neighbour_Chunk chunk{chunks, c1.chunk() };
	
	vec3l const c1c{ c1.cube() };
	vec3l const c2c{ c2.cube() };
	
	for(int64_t x = c1c.x; x <= c2c.x; x++)
	for(int64_t y = c1c.y; y <= c2c.y; y++)
	for(int64_t z = c1c.z; z <= c2c.z; z++) {
		vec3l const cubeCoord{x, y, z};
		
		auto const cubeLocalCoord{ 
			cubeCoord.applied([](auto const coord, auto i) -> bool {
				return bool(misc::mod(coord, 2ll));
			})
		};
		
		ChunkCoord const coord{ ChunkCoord::cubeToFrac(cubeCoord) };
				
		vec3i const blockChunk = coord.chunk();
		
		auto const chunkIndex{ chunk.move(blockChunk).get() };
		if(chunkIndex == -1) { std::cout << "checkIntersection " << coord1 << ' ' << coord2 << ": add chunk gen!" << coord << '\n'; return true; }
		
		auto const chunkData{ chunks.chunksData[chunkIndex] };
		auto const index{ Chunks::blockIndex(coord.blockInChunk()) };
		auto const block{ chunkData[index] };
		
		if(block.id() != 0 && block.cube(cubeLocalCoord)) return true;
	}
	
	return false;
}

static void updateCollision(ChunkCoord &player, vec3d &playerForce, bool &isOnGround) {	
	//auto const playerChunk{ player.chunk() };
	
	vec3l playerPos{ player.coord() };
	vec3l force{ ChunkCoord::posToFracTrunk(playerForce) };
	
	vec3l maxPlayerPos{};
	
	vec3i dir{};
	vec3b positive_{};
	vec3b negative_{};
	
	vec3l playerMin{};
	vec3l playerMax{};
	
	vec3l min{};
	vec3l max{};
	
	Chunks::Move_to_neighbour_Chunk chunk{ chunks, player.chunk() };
	
	auto const updateBounds = [&]() {			
		dir = force.sign();
		positive_ = dir > vec3i(0);
		negative_ = dir < vec3i(0);
		
		maxPlayerPos = playerPos + force;
		
		/*static_*/assert(width_i % 2 == 0);
		playerMin = vec3l{ playerPos.x-width_i/2, playerPos.y         , playerPos.z-width_i/2 };
		playerMax = vec3l{ playerPos.x+width_i/2, playerPos.y+height_i, playerPos.z+width_i/2 };
		
		min = ChunkCoord::fracToBlockCube(playerPos - vec3l{width_i/2,0,width_i/2});
		max = ChunkCoord::fracToBlockCube(playerPos + vec3l{width_i/2,height_i,width_i/2} - 1);
	};
	
	struct MovementResult {
		ChunkCoord coord;
		bool isCollision;
		bool continueMovement;
	};
	
	auto const moveAlong = [&](vec3b const axis, int64_t const axisPlayerOffset, vec3b const otherAxis1, vec3b const otherAxis2, bool const upStep) -> MovementResult {
		if(!( (otherAxis1 || otherAxis2).equal(!axis).all() )) {
			std::cout << "Error, axis not valid: " << axis << " - " << otherAxis1 << ' ' << otherAxis2 << '\n';
			assert(false);
		}
		
		auto const otherAxis{ otherAxis1 || otherAxis2 };
		
		auto const axisPositive{ positive_.dot(axis) };
		auto const axisNegative{ negative_.dot(axis) };
		auto const axisDir{ dir.dot(vec3i(axis)) };
		
		auto const axisPlayerMax{ playerMax.dot(vec3l(axis)) };
		auto const axisPlayerMin{ playerMax.dot(vec3l(axis)) };
		
		auto const axisPlayerPos{ playerPos.dot(vec3l(axis)) };
		auto const axisPlayerMaxPos{ maxPlayerPos.dot(vec3l(axis)) };
		
		auto const start{ misc::divFloor( axisPositive ? axisPlayerMax : axisPlayerMin, ChunkCoord::fracCubeDim)-int64_t(axisNegative) };
		auto const end{ misc::divFloor(axisPlayerMaxPos + axisPlayerOffset, ChunkCoord::fracCubeDim) };
		auto const count{ (end - start) * axisDir };
		
		
		for(int64_t a{}; a <= count; a++) {
			auto const axisCurCubeCoord{ start + a * axisDir };
			
			auto const axisNewCoord{ ChunkCoord::blockCubeToFrac(vec3i(axisCurCubeCoord + int64_t(axisNegative))).x - axisPlayerOffset }; 
			ChunkCoord const newCoord{ vec3l(axisNewCoord) * vec3l(axis) + vec3l(playerPos) * vec3l(otherAxis) };
				
			vec3l const upStepOffset{ vec3l{0, ChunkCoord::fracCubeDim, 0} + vec3l(axis) * vec3l(axisDir) };
			ChunkCoord const upStepCoord{newCoord + ChunkCoord::Fractional{upStepOffset}};
			
			auto const upStepMin = (upStepCoord.coord() - vec3l{width_i/2,0,width_i/2});
			auto const upStepMax = (upStepCoord.coord() + vec3l{width_i/2,height_i,width_i/2} - 1);
			
			auto const upStepPossible{ upStep && !checkIntersection(chunks, upStepMin, upStepMax)};
			
			for(int64_t o1{ min.dot(vec3l(otherAxis1)) }; o1 <= max.dot(vec3l(otherAxis1)); o1++)
			for(int64_t o2{ min.dot(vec3l(otherAxis2)) }; o2 <= max.dot(vec3l(otherAxis2)); o2++) {
				vec3l const cubeCoord{
					vec3l(axis      ) * axisCurCubeCoord
					+ vec3l(otherAxis1) * o1
					+ vec3l(otherAxis2) * o2
				};
				
				auto const cubeLocalCoord{ 
					cubeCoord.applied([](auto const coord, auto i) -> bool {
						return bool(misc::mod(coord, 2ll));
					})
				};
				
				ChunkCoord const coord{ ChunkCoord::cubeToFrac(cubeCoord) };
						
				vec3i const blockChunk = coord.chunk();
				
				auto const chunkIndex{ chunk.move(blockChunk).get() };
				if(chunkIndex == -1) { std::cout << "collision " << vec3i(axis) << ": add chunk gen!" << coord << '\n'; return { playerPos, false, false }; }
				
				auto const chunkData{ chunks.chunksData[chunkIndex] };
				auto const index{ Chunks::blockIndex(coord.blockInChunk()) };
				auto const block{ chunkData[index] };
				
				if(block.id() != 0 && block.cube(cubeLocalCoord)) {
					if(axisPositive ? 
						(axisNewCoord >= axisPlayerPos)
					: (axisNewCoord <= axisPlayerPos)
					) {
						auto const diff{ playerPos.y - coord.coord().y };
						if(upStepPossible && diff <= ChunkCoord::fracCubeDim && diff >= 0) return { upStepCoord, false, true };
						else return { newCoord, true, false };
					}
				}
			}
		}
		
		return { vec3l(axisPlayerMaxPos) * vec3l(axis) + vec3l(playerPos) * vec3l(otherAxis), false, false };
	};
	
	updateBounds();
		
	if(dir.y != 0) {
		MovementResult result;
		do { 
			result = moveAlong(vec3b{0,1,0}, (positive_.y ? height_i : 0), vec3b{0,0,1},vec3b{1,0,0}, false);
			
			if(result.continueMovement) {
				force -= (result.coord.coord() - playerPos) * vec3l{0,1,0};
			}
			else if(result.isCollision) {
				if(negative_.y) isOnGround = true;
				force = ChunkCoord::posToFracTrunk(ChunkCoord::fracToPos(force) * 0.8);
				force.y = 0;
			}
			
			playerPos = result.coord.coord();
			updateBounds();
		} while(result.continueMovement);
	}

	if(dir.x != 0) {
		MovementResult result;
		do { 
			result = moveAlong(vec3b{1,0,0}, width_i/2*dir.x, vec3b{0,0,1},vec3b{0,1,0}, isOnGround);
			
			if(result.continueMovement) {
				force -= (result.coord.coord() - playerPos) * vec3l{1,0,0};
			}
			else if(result.isCollision) { force.x = 0; }
			
			playerPos = result.coord.coord();
			updateBounds();
		} while(result.continueMovement);
	}
	
	if(dir.z != 0) {
		MovementResult result;
		do { 
			result = moveAlong(vec3b{0,0,1}, width_i/2*dir.z, vec3b{0,1,0},vec3b{1,0,0}, isOnGround);
			
			if(result.continueMovement) {
				force -= (result.coord.coord() - playerPos) * vec3l{0,0,1};
			}
			else if(result.isCollision) { force.z = 0; }
			
			playerPos = result.coord.coord();
			updateBounds();
		} while(result.continueMovement);
	}
	
	player = ChunkCoord{ playerPos };
	playerForce = ChunkCoord::fracToPos(force) * (isOnGround ? vec3d{0.8,1,0.8} : vec3d{1});
}

bool checkCanPlaceBlock(vec3i const blockChunk, vec3i const blockCoord) {
	ChunkCoord const relativeBlockCoord{ ChunkCoord{ blockChunk, ChunkCoord::Block{vec3l(blockCoord)} } - currentCoord() };
	vec3l const blockStartF{ relativeBlockCoord.coord() };
	vec3l const blockEndF{ blockStartF + ChunkCoord::fracBlockDim };
	
	/*static_*/assert(width_i % 2 == 0);
	
	return !(
		misc::intersectsX(0ll       , height_i ,  blockStartF.y, blockEndF.y) &&
		misc::intersectsX(-width_i/2, width_i/2,  blockStartF.x, blockEndF.x) &&
		misc::intersectsX(-width_i/2, width_i/2,  blockStartF.z, blockEndF.z)
	);
}

struct BlockIntersection {
	Chunks::Chunk chunk;
	int16_t blockIndex;
	uint8_t cubeIndex;
};

static std::optional<BlockIntersection> trace(Chunks &chunks, PosDir const pd) {
	vec3i const dirSign{ pd.direction };
	DDA checkBlock{ pd };
		
	Chunks::Move_to_neighbour_Chunk chunk{ chunks, pd.chunk };
		
	vec3l intersectionAxis{0};
	for(int i = 0;; i++) {
		vec3l const intersection{ checkBlock.get_current() };
		
		vec3l const cubeCoord{ 
			ChunkCoord::fracToBlockCube(intersection)
			  + vec3l(pd.direction.min(0)) * intersectionAxis
		};
		
		auto const cubeLocalCoord{ 
			cubeCoord.applied([](auto const coord, auto i) -> bool {
				return bool(misc::mod(coord, 2ll));
			})
		};
		
		ChunkCoord const blockAt { 
			vec3i{},
			ChunkCoord::Fractional{ 
				(cubeCoord - vec3l(cubeLocalCoord)) * ChunkCoord::fracCubeDim
			}  
		};
		ChunkCoord const coord{ 
			pd.chunk,
			ChunkCoord::Fractional{ blockAt.coord() }
		};
		
		vec3i const blockCoord = coord.blockInChunk();
		vec3i const blockChunk = coord.chunk();
		
		int chunkIndex{ chunk.move(blockChunk).get() };
		
		if(chunkIndex == -1) {
			std::cout << __FILE__ << ':' << __LINE__ << " error: add chunk gen!" << coord << '\n'; 
			break;
		}
		
		auto const chunk{ chunks[chunkIndex] };
		auto const chunkData{ chunk.data() };
		
		auto const blockIndex{ Chunks::blockIndex(blockCoord) };
		auto &block{ chunkData[blockIndex] };
		auto const blockId{ block.id() };
		
		if(blockId != 0) {
			if(block.cube(cubeLocalCoord)) return {BlockIntersection{ chunk, blockIndex, Chunks::Block::cubePosIndex(cubeLocalCoord) }};
		}
		
		if(i >= 10000) {
			std::cout << __FILE__ << ':' << __LINE__ << " error: to many iterations!" << pd << '\n'; 
			break;
		}
		
		if(checkBlock.get_end()) {
			break;
		}
		
		intersectionAxis = vec3l(checkBlock.next());
	}

	return {};
}

static void update() {	
	static std::chrono::time_point<std::chrono::steady_clock> lastPhysicsUpdate{std::chrono::steady_clock::now()};
	static std::chrono::time_point<std::chrono::steady_clock> lastBlockUpdate{std::chrono::steady_clock::now()};
	auto const now{std::chrono::steady_clock::now()};

	auto const diffBlockMs{ std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBlockUpdate).count() };
	auto const diffPhysicsMs{ std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPhysicsUpdate).count() };
	
	
	for(size_t i{}; i < sizeof(keys)/sizeof(keys[0]); ++i) {
		auto &key{ keys[i] };
		auto const action{ misc::to_underlying(key) };
		if(action == GLFW_PRESS) {
			key = Key::REPEAT;
		}
		else if(action == GLFW_REPEAT) {
			handleKey(i);
		}
		else if(action == GLFW_RELEASE) {
			key = Key::NOT_PRESSED;
		}
	}

	if(diffBlockMs >= blockActionCD * 1000 && blockAction != BlockAction::NONE && !isSpectator) {
		bool isAction = false;

		ChunkCoord const viewport{ currentCameraPos() };
		PosDir const pd{ PosDir(viewport, ChunkCoord::posToFracTrunk(viewport_current().forwardDir() * 7)) };
		vec3i const dirSign{ pd.direction };
		DDA checkBlock{ pd };
		
		Chunks::Move_to_neighbour_Chunk chunk{ chunks, pd.chunk };
	
		if(blockAction == BlockAction::BREAK) {
			auto optionalResult{ trace(chunks, pd) };
			
			if(optionalResult) {
				auto result{ *optionalResult };
				
				auto chunk{ result.chunk };
				auto const &chunkData{ chunk.data() };
				auto const chunkIndex{ chunk.chunkIndex() };
				auto &block{ chunk.data()[result.blockIndex] };
				
				if(breakFullBlock) block = Chunks::Block::emptyBlock();
				else block = Chunks::Block{ block.id(), uint8_t( block.cubes() & (~Chunks::Block::blockCubeMask(result.cubeIndex)) ) };
				
				chunks.gpuChunksStatus[chunkIndex].reset();
				chunks.modified[chunkIndex] = true;
				isAction = true;
				
				auto &aabb{ chunks.chunksAABB[chunkIndex] };
				vec3i const start_{ aabb.start() };
				vec3i const end_  { aabb.end  () };
				
				vec3i start{ 16 };
				vec3i end  { 0 };
				for(int32_t x = start_.x; x <= end_.x; x++)
				for(int32_t y = start_.y; y <= end_.y; y++)
				for(int32_t z = start_.z; z <= end_.z; z++) {
					vec3i const blk{x, y, z};
					if(chunkData[Chunks::blockIndex(blk)].id() != 0) {
						start = start.min(blk);
						end   = end  .max(blk);
					}
				}
				
				aabb = Chunks::AABB(start, end);
			}
		}
		else {
			for(int i = 0;; i++) {
				vec3b const intersectionAxis{ checkBlock.next() };
				vec3l const intersection{ checkBlock.get_current() };
				
				if(intersectionAxis == 0) break;
				
				ChunkCoord const coord{ 
					pd.chunk,
					ChunkCoord::Cube{ 
						  ChunkCoord::fracToBlockCube(intersection)
						+ vec3l(pd.direction.min(0)) * vec3l(intersectionAxis)
					} 
				};
				
				vec3i const blockCoord = coord.blockInChunk();
				vec3i const blockChunk = coord.chunk();
				
				int chunkIndex{ chunk.move(blockChunk).get() }; 
				
				if(chunkIndex == -1) { 
					//auto const usedIndex{ genChunkAt(blockChunk) }; //generates every frame
					//chunkIndex = chunks.usedChunks()[usedIndex];
					std::cout << "block pl: add chunk gen!" << coord << '\n'; 
					break;
				}
				
				auto const chunkData{ chunks.chunksData[chunkIndex] };
				auto const index{ Chunks::blockIndex(coord.blockInChunk()) };
				auto const blockId{ chunkData[index].id() };
				
				if(blockId != 0) {
					ChunkCoord const bc{ coord - ChunkCoord::Block{ vec3l(dirSign) * vec3l(intersectionAxis) } };
					vec3i const blockCoord = bc.blockInChunk();
					vec3i const blockChunk = bc.chunk();
			
					int chunkIndex{ chunk.move(blockChunk).get() };
				
					auto const index{ Chunks::blockIndex(blockCoord) };
					auto &block{ chunks.chunksData[chunkIndex][index] };
					
					if(checkCanPlaceBlock(blockChunk, blockCoord) && block.id() != 0) { std::cout << "!\n"; } 
					if(checkCanPlaceBlock(blockChunk, blockCoord) && block.id() == 0) {
						block = Chunks::Block::fullBlock(blockPlaceId);
						chunks.gpuChunksStatus[chunkIndex].reset();
						chunks.modified[chunkIndex] = true;
						isAction = true;
						
						auto &aabb{ chunks.chunksAABB[chunkIndex] };
						vec3i start{ aabb.start() };
						vec3i end  { aabb.end  () };
						
						start = start.min(blockCoord);
						end   = end  .max(blockCoord);
					
						aabb = Chunks::AABB(start, end);
					}
					break;
				}
				else if(checkBlock.get_end() || i >= 100) break;
			}
		}
	
		if(isAction) {
			lastBlockUpdate = now;
		}
	}
	
	auto const curZoom{ currentZoom() };
		
	{
		double projection[3][3];
		viewport_current().localToGlobalSpace(&projection);
				
		auto const movement{ 
			vecMult( projection, vec3d(spectatorInput.movement).normalizedNonan() ) 
			* spectatorSpeed / curZoom
			* (shift ? 1.0*speedModifier : 1)
			* (ctrl  ? 1.0/speedModifier : 1)
		};
		
		spectatorCoord += movement;
		spectatorInput = Input();
	}
	
	{
		static vec3d playerMovement{};
		
		playerMovement += ( 
			(
				  playerViewport.flatForwardDir()*playerInput.movement.z
				+ playerViewport.flatTopDir()    *playerInput.movement.y
				+ playerViewport.flatRightDir()  *playerInput.movement.x
			).normalizedNonan()
		    * playerSpeed
			* (shift ? 1.0*speedModifier : 1)
			* (ctrl  ? 1.0/speedModifier : 1)
		) * deltaTime; 
			
		if(diffPhysicsMs > fixedDeltaTime * 1000) {
			lastPhysicsUpdate += std::chrono::milliseconds(static_cast<long long>(fixedDeltaTime*1000.0));

			if(!debugBtn0) {
				playerForce += vec3d{0,-1,0} * fixedDeltaTime; 
				if(isOnGround) {
					playerForce += (
						vec3d{0,1,0}*14*double(playerInput.jump)	
					) * fixedDeltaTime + playerMovement;
				}
				else {
					auto const movement{ playerMovement * 0.5 * fixedDeltaTime };
					playerForce = playerForce.applied([&](double const coord, auto const index) -> double { 
						return misc::clamp(coord + movement[index], fmin(coord, movement[index]), fmax(coord, movement[index]));
					});
				}

				isOnGround = false;
				updateCollision(playerCoord, playerForce, isOnGround);
			}
			
			playerMovement = 0;
		}
		
		playerInput = Input();
	}
	
	auto &currentViewport{ viewport_current() };
	
	viewportDesired.rotation += deltaRotation * (2 * misc::pi) * (vec2d{ 0.8, -0.8 } / curZoom);
	viewportDesired.rotation.y = misc::clamp(viewportDesired.rotation.y, -misc::pi / 2 + 0.001, misc::pi / 2 - 0.001);
	deltaRotation = 0;
	
	if(isSmoothCamera) {
		currentViewport.rotation = vec2lerp( currentViewport.rotation, viewportDesired.rotation, vec2d(0.05) );
	}
	else {
		currentViewport.rotation = viewportDesired.rotation;
	}
	
	playerCamera.fov = misc::lerp( playerCamera.fov, 90.0 / 180 * misc::pi / curZoom, 0.1 );
	
	updateChunks();
	
	auto const diff{(playerCoord+playerCameraOffset - playerCamPos).position()};
	playerCamPos = playerCamPos + vec3lerp(vec3d{}, vec3d(diff), vec3d(0.4));
}
	
int main(void) {	
    if (!glfwInit()) return -1;
	
	updateChunks();

    GLFWmonitor* monitor;
#ifdef FULLSCREEN
    monitor = glfwGetPrimaryMonitor();
#else
    monitor = NULL;
#endif // !FULLSCREEN

    window = glfwCreateWindow(windowSize.x, windowSize.y, "VMC", monitor, NULL);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
	glfwSwapInterval( 0 );
	if(mouseCentered) glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        glfwTerminate();
        return -1;
    }
	
    fprintf(stdout, "Using GLEW %s\n", glewGetString(GLEW_VERSION));

    //callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPos(window, 0, 0);
	cursor_position_callback(window, 0, 0);
	
	//load shaders
	reloadShaders();
	
	
	GLuint fontVB;
	glGenBuffers(1, &fontVB);
	glBindBuffer(GL_ARRAY_BUFFER, fontVB);
	glBufferData(GL_ARRAY_BUFFER, sizeof(std::array<std::array<vec2f, 4>, 15>{}), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	GLuint fontVA;
	glGenVertexArrays(1, &fontVA);
	glBindVertexArray(fontVA);
		glBindBuffer(GL_ARRAY_BUFFER, fontVB);
		
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
	
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), NULL); //startPos
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)( 2*sizeof(float) )); //endPos
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)( 4*sizeof(float) )); //startUV
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)( 6*sizeof(float) )); //endUV
	
		glVertexAttribDivisor(0, 1);
		glVertexAttribDivisor(1, 1);
		glVertexAttribDivisor(2, 1);
		glVertexAttribDivisor(3, 1);
	glBindVertexArray(0); 
	
	
	GLuint testVB;
	glGenBuffers(1, &testVB);
	
	GLuint testVA;
	glGenVertexArrays(1, &testVA);
	glBindVertexArray(testVA);
		glBindBuffer(GL_ARRAY_BUFFER, testVB);
		
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), NULL); //relativePos
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)( 3*sizeof(float) )); //relativePos
		
		
	glBindVertexArray(0); 
	
	
	while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cout << err << std::endl;
    }

    auto start = std::chrono::steady_clock::now();
	
	MeanCounter<150> mc{};
	int _i_ = 50;

    while (!glfwWindowShouldClose(window))
    {
		auto startFrame = std::chrono::steady_clock::now();
		
		//auto &player{ playerCoord() };
		auto &currentViewport{ viewport_current() };
		//auto const position{ player.position()+viewportOffset() };
        auto const rightDir{ currentViewport.rightDir() };
        auto const topDir{ currentViewport.topDir() };
        auto const forwardDir{ currentViewport.forwardDir() };
		
		float toLoc[3][3];
		float toGlob[3][3];
		currentViewport.localToGlobalSpace(&toGlob);
		currentViewport.globalToLocalSpace(&toLoc);
		ChunkCoord const cameraCoord{ currentCameraPos() };
		auto const cameraChunk{ cameraCoord.chunk() };
		auto const cameraPosInChunk{ cameraCoord.positionInChunk() };
		
		float const toLoc4[4][4] = {
			{ toLoc[0][0], toLoc[0][1], toLoc[0][2], 0 },
			{ toLoc[1][0], toLoc[1][1], toLoc[1][2], 0 },
			{ toLoc[2][0], toLoc[2][1], toLoc[2][2], 0 },
			{ 0          , 0          , 0          , 1 },
		};
		
		float projection[4][4];
		currentCamera().projectionMatrix(&projection);
		
		glUseProgram(debugProgram);
		glUniformMatrix4fv(db_projection_u, 1, GL_TRUE, &projection[0][0]);	
		
		glUseProgram(mainProgram);
		glUniformMatrix4fv(projection_u, 1, GL_TRUE, &projection[0][0]);
		
		glUseProgram(testProgram);
		glUniformMatrix4fv(tt_projection_u, 1, GL_TRUE, &projection[0][0]);
		
		glProgramUniformMatrix4fv(blockHitbox_p, blockHitboxProjection_u, 1, GL_TRUE, &projection[0][0]);
		
		//glClear(GL_COLOR_BUFFER_BIT);
		
		glEnable(GL_FRAMEBUFFER_SRGB); 
		glDisable(GL_DEPTH_TEST); 
		glDisable(GL_CULL_FACE); 
		
		glUseProgram(mainProgram);
		
        glUniform3f(rightDir_u, rightDir.x, rightDir.y, rightDir.z);
        glUniform3f(topDir_u, topDir.x, topDir.y, topDir.z);
		
		auto const &currentCam{ currentCamera() };
		glUniform1f(near_u, currentCam.near);
        glUniform1f(far_u , currentCam.far );

        glUniform1f(mouseX_u, mousePos.x / windowSize_d.x);
        glUniform1f(mouseY_u, mousePos.y / windowSize_d.y);

		static double lastTime{};
		double curTime{ std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() / 1000.0 };
		
		static double offset{};
		if(debugBtn2) offset += curTime - lastTime;
		lastTime = curTime;
		glUniform1f(time_u, offset);
		
		if(testInfo) {
			std::cout.precision(17);
			std::cout << currentCoord().positionInChunk() << '\n';
			std::cout.precision(2);
			std::cout << currentCoord().blockInChunk() << '\n';
			std::cout << currentCoord().chunk() << '\n';
			std::cout << "----------------------------\n";
		}
		
		glUniformMatrix4fv(toLocal_matrix_u, 1, GL_TRUE, &toLoc4[0][0]);
		glProgramUniformMatrix4fv(debugProgram, db_toLocal_u, 1, GL_TRUE, &toLoc4[0][0]);
		
		glUniform3i(playerChunk_u, cameraChunk.x, cameraChunk.y, cameraChunk.z);
		glUniform3f(playerInChunk_u, cameraPosInChunk.x, cameraPosInChunk.y, cameraPosInChunk.z);
		
		glProgramUniform3i(debugProgram, db_playerChunk_u, cameraChunk.x, cameraChunk.y, cameraChunk.z);
		glProgramUniform3f(debugProgram, db_playerInChunk_u, cameraPosInChunk.x, cameraPosInChunk.y, cameraPosInChunk.z);
		
		auto const playerChunkCand{ Chunks::Move_to_neighbour_Chunk(chunks, cameraChunk).optChunk().get() };
		if(playerChunkCand != -1) {
			int playerChunkIndex = playerChunkCand;
			glUniform1i(startChunkIndex_u, playerChunkIndex);
			
			auto const playerRelativePos{ vec3f((playerCoord - ChunkCoord{ChunkCoord::Chunk{cameraChunk}}).position()) + vec3f{0, float(height/2.0), 0} };
			glUniform3f(playerRelativePosition_u, playerRelativePos.x, playerRelativePos.y, playerRelativePos.z);
			
			glUniform1i(drawPlayer_u, isSpectator);
			
			int sentCount{};
			for(auto const chunkIndex : chunks.used) {
				Chunks::ChunkData &chunkData{ chunks.chunksData[chunkIndex] };
				vec3i const chunkPosition{ chunks[chunkIndex].position() };
				auto &gpuStatus = chunks.gpuChunksStatus[chunkIndex];
				
				if(gpuStatus.needsUpdate() || !gpuStatus.isFullyLoaded()) {
					if(chunkIndex >= gpuChunksCount) {
						std::cout << "Error: gpu buffer was not properly resized. size=" << gpuChunksCount << " expected=" << (chunkIndex+1);
						exit(-1);
					}
					if(playerChunkIndex != chunkIndex && ((sentCount > 100) || (chunkPosition - cameraChunk).in(vec3i{-viewDistance}, vec3i{viewDistance}).anyNot())) {
						if(gpuStatus.isStubLoaded()) continue;
						if(gpuStatus.isFullyLoaded() && gpuStatus.needsUpdate()) continue;
						
						Chunks::Neighbours const neighbours{};
						static_assert(sizeof(neighbours) == sizeof(int32_t) * Chunks::Neighbours::neighboursCount);
						glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunksNeighbours_ssbo); 
						glBufferSubData(
							GL_SHADER_STORAGE_BUFFER, 
							sizeof(int32_t) * Chunks::Neighbours::neighboursCount * chunkIndex, 
							Chunks::Neighbours::neighboursCount * sizeof(uint32_t), 
							&neighbours
						);
						glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
						
						gpuStatus.markStubLoaded();
						continue;
					}
					
					uint32_t const aabbData{ chunks[chunkIndex].aabb().getData() };
					auto const &neighbours{ chunks[chunkIndex].neighbours() };
					
					gpuStatus.markFullyLoaded();
					gpuStatus.resetNeedsUpdate();
					
					static_assert(sizeof chunkData == 16384);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkIndex_u); 
					glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(chunkData) * chunkIndex, sizeof(chunkData), &chunkData);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
						
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunksBounds_ssbo); 
					glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * chunkIndex, sizeof(uint32_t), &aabbData);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
					
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunksPostions_ssbo); 
					glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(vec3i) * chunkIndex, sizeof(vec3i), &chunkPosition);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

					static_assert(sizeof(neighbours) == sizeof(int32_t) * Chunks::Neighbours::neighboursCount);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunksNeighbours_ssbo); 
					glBufferSubData(
						GL_SHADER_STORAGE_BUFFER, 
						sizeof(int32_t) * Chunks::Neighbours::neighboursCount * chunkIndex, 
						Chunks::Neighbours::neighboursCount * sizeof(uint32_t), 
						&neighbours
					);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
					
					sentCount ++;
				} 
			}

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		else glClear(GL_COLOR_BUFFER_BIT);
		
		{
			glUseProgram(blockHitbox_p);
			
			PosDir const pd{ PosDir(cameraCoord, ChunkCoord::posToFracTrunk(viewport_current().forwardDir() * 7)) };
			auto const optionalResult{ trace(chunks, pd) };
				
			if(optionalResult) {
				auto const result{ *optionalResult };
				auto const chunk { result.chunk };
				
				ChunkCoord const blockCoord{ chunk.position(), ChunkCoord::Block{vec3l(Chunks::indexBlock(result.blockIndex))} };
				
				auto const blockRelativePos{ 
					vec3f((blockCoord - cameraCoord).position()) +  
					(breakFullBlock ? vec3f{0} : (vec3f{Chunks::Block::cubeIndexPos(result.cubeIndex)} * 0.5))
				};
				float const size{ breakFullBlock ? 1.0f : 0.5f };
				float const translation4[4][4] = {
						{ size, 0, 0, blockRelativePos.x },
						{ 0, size, 0, blockRelativePos.y },
						{ 0, 0, size, blockRelativePos.z },
						{ 0, 0, 0, 1                     },
				};			
				float playerToLocal[4][4];
				misc::matMult(toLoc4, translation4, &playerToLocal);
				
				glUniformMatrix4fv(blockHitboxModelMatrix_u, 1, GL_TRUE, &playerToLocal[0][0]);
				
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glClear(GL_DEPTH_BUFFER_BIT);	
				
				glDrawArrays(GL_TRIANGLES, 0, 36);
				
				glDisable(GL_BLEND);
				glDisable(GL_DEPTH_TEST);
			}
		}
		
		{
			glUseProgram(currentBlockProgram);
			glUniform1ui(cb_blockIndex_u, blockPlaceId);
			  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		
		glDisable(GL_FRAMEBUFFER_SRGB); 
		
		/*{
			glUseProgram(testProgram);
			glUniformMatrix4fv(tt_toLocal_u, 1, GL_TRUE, &toLoc4[0][0]);
			
			glBindBuffer(GL_ARRAY_BUFFER, testVB);
			glBufferData(GL_ARRAY_BUFFER, size * 2*sizeof(vec3f), &[0], GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			
			glPointSize(10);
			glBindVertexArray(testVA);
			glDrawArrays(GL_POINTS, 0, relativePositions.size());
			glBindVertexArray(0);
		}*/
	
		
		
		{ 
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			vec2f const textPos(10, 10);
			    
			std::stringstream ss{};
			ss.precision(1);
			ss << std::fixed << (1000000.0 / mc.mean()) << '(' << (1000000.0 / mc.max()) << ')' << "FPS";
			std::string const text{ ss.str() };
	
			auto const textCount{ std::min(text.size(), 15ull) };
			
			std::array<std::array<vec2f, 4>, 15> data;
			
			vec2f currentPoint( textPos.x / windowSize_d.x * 2 - 1, 1 - textPos.y / windowSize_d.y * 2 );
			
			auto const lineH = font.base();
			float const scale = 5;
			
			for(uint64_t i{}; i != textCount; i++) {
				auto const &fc{ font[text[i]] };
				
				data[i] = {
					//pos
					currentPoint + vec2f(0, 0 - lineH) / scale,
					currentPoint + vec2f(fc.width*aspect, fc.height - lineH) / scale, 
					//uv
					vec2f{fc.x, 1-fc.y-fc.height},
					vec2f{fc.x+fc.width,1-fc.y},
				};
				
				currentPoint += vec2f(fc.xAdvance*aspect, 0) / scale;
			}

			glUseProgram(fontProgram);
			
			glBindBuffer(GL_ARRAY_BUFFER, fontVB);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(data[0]) * textCount, &data[0]);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			
			glBindVertexArray(fontVA);
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, textCount);
			glBindVertexArray(0);
			
			glDisable(GL_BLEND);
		}
		
		
		testInfo = false; 
		glfwSwapBuffers(window);
		glfwPollEvents();
		
		static GLenum lastError;
        while ((err = glGetError()) != GL_NO_ERROR) {
			if(lastError == err) {
				
			}
            else { 
				std::cout << "glError: " << err << std::endl;
				lastError = err;
			}
		}
		
        update();
		
		auto const dTime{ std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startFrame).count() };
		mc.add(dTime);
		deltaTime = double(dTime) / 1000000.0;
    }

    glfwTerminate();
	
    return 0;
}

static void quit() {
	for(auto const chunkIndex : chunks.used) {
		auto chunk{ chunks[chunkIndex] };
		
		if(chunk.modified()) {
			writeChunk(chunk);
			//chunk.modified() = false;
		}
	}
	
	glfwSetWindowShouldClose(window, GL_TRUE);
}
