#pragma once
#include "SDL/SDL.h"
#include "SDL/SDL_image.h"

#include <SDL/SDL_mixer.h>
#include <SDL/SDL_audio.h>

#include <algorithm>
#include <vector>

struct Vector2
{
	float x;
	float y;
};

class Game
{
public:
	Game();
	bool Initialize();
	void RunLoop();
	void Shutdown();
private:
	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();

	bool mIsRunning;
	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	Uint32 mTicksCount;

	SDL_Color highlightColor;
	int highlightColorChangeDirection;
	int highlightThickness;

	// Sounds
	Mix_Chunk* mSoundtrack;
	Mix_Chunk* mJump;
};
