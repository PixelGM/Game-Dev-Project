
#include "Game.h"

const int thickness = 15;

struct Player {
	Vector2 mPos;
	int mWidth;
	int mHeight;
	float mVelY; // Vertical velocity
	bool isCrouching;
	bool isOnGround;
};

Player mPlayer;


Game::Game()
{
	mWindow = nullptr;
	mRenderer = nullptr;
	mTicksCount = 0;
	mIsRunning = true;
}

bool Game::Initialize()
{
	// Initialize SDL
	int sdlResult = SDL_Init(SDL_INIT_VIDEO);
	if (sdlResult != 0)
	{
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return false;
	}
	
	// Create an SDL Window
	mWindow = SDL_CreateWindow(
		"CMPT 1267", // Window title
		100,	// Top left x-coordinate of window
		100,	// Top left y-coordinate of window
		1024,	// Width of window
		768,	// Height of window
		0		// Flags (0 for no flags set)
	);

	if (!mWindow)
	{
		SDL_Log("Failed to create window: %s", SDL_GetError());
		return false;
	}
	
	//// Create SDL renderer
	mRenderer = SDL_CreateRenderer(
		mWindow, // Window to create renderer for
		-1,		 // Usually -1
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
	);

	if (!mRenderer)
	{
		SDL_Log("Failed to create renderer: %s", SDL_GetError());
		return false;
	}

	// Initialize player
	mPlayer.mPos.x = 100.0f;
	mPlayer.mPos.y = 768.0f - thickness - 100.0f;
	mPlayer.mWidth = 50;
	mPlayer.mHeight = 100;
	mPlayer.mVelY = 0.0f;
	mPlayer.isCrouching = false;
	mPlayer.isOnGround = true; // Initially, the player is on the ground

	return true;
}

void Game::RunLoop()
{
	while (mIsRunning)
	{
		ProcessInput();
		UpdateGame();
		GenerateOutput();
	}
}

void Game::ProcessInput()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			// If we get an SDL_QUIT event, end loop
			case SDL_QUIT:
				mIsRunning = false;
				break;
		}
	}
	
	// Get state of keyboard
	const Uint8* state = SDL_GetKeyboardState(NULL);
	// If escape is pressed, also end loop
	if (state[SDL_SCANCODE_ESCAPE])
	{
		mIsRunning = false;
	}


	// Player movement logic
	if (state[SDL_SCANCODE_A]) {
		mPlayer.mPos.x -= 5.0f; // Move left
	}
	if (state[SDL_SCANCODE_D]) {
		mPlayer.mPos.x += 5.0f; // Move right
	}
	if (state[SDL_SCANCODE_W] && mPlayer.isOnGround) {
		mPlayer.mVelY = -350.0f; // Set a negative velocity to move up
		mPlayer.isOnGround = false;
	}
	if (state[SDL_SCANCODE_S]) {
		if (!mPlayer.isCrouching) {
			mPlayer.isCrouching = true;
			mPlayer.mHeight = 60; // Crouch Height
			mPlayer.mPos.y += 40; // Adjust position to stay on ground (100 - 60)
		}
	}
	else {
		if (mPlayer.isCrouching) {
			mPlayer.isCrouching = false;
			mPlayer.mHeight = 100; // Stand up
			mPlayer.mPos.y -= 40; // Adjust position back to standing (100 - 60)
		}
	}
}

void Game::UpdateGame()
{
	// Wait until 16ms has elapsed since last frame
	while (!SDL_TICKS_PASSED(SDL_GetTicks(), mTicksCount + 16))
		;

	// Delta time is the difference in ticks from last frame
	// (converted to seconds)
	float deltaTime = (SDL_GetTicks() - mTicksCount) / 1000.0f;
	
	// Clamp maximum delta time value
	if (deltaTime > 0.05f)
	{
		deltaTime = 0.05f;
	}

	// Apply gravity to vertical velocity
	if (!mPlayer.isOnGround) {
		mPlayer.mVelY += 500.0f * deltaTime; // Gravity effect
	}

	// Update player's vertical position
	mPlayer.mPos.y += mPlayer.mVelY * deltaTime;

	// Check if the player has landed on the ground
	if (mPlayer.mPos.y >= 768.0f - thickness - mPlayer.mHeight) {
		mPlayer.mPos.y = 768.0f - thickness - mPlayer.mHeight;
		mPlayer.isOnGround = true;
		mPlayer.mVelY = 0.0f;
	}

	// Update tick counts (for next frame)
	mTicksCount = SDL_GetTicks();
}

void Game::GenerateOutput() {
    // Set background to blue
    SDL_SetRenderDrawColor(mRenderer, 0, 0, 255, 255);
    SDL_RenderClear(mRenderer);

    // Draw ground
    SDL_SetRenderDrawColor(mRenderer, 139, 69, 19, 255); // Brown color
    SDL_Rect groundRect = {0, 768 - thickness, 1024, thickness};
    SDL_RenderFillRect(mRenderer, &groundRect);

    // Draw player
    SDL_SetRenderDrawColor(mRenderer, 255, 255, 255, 255); // White color for the player
    SDL_Rect playerRect = {
        static_cast<int>(mPlayer.mPos.x),
        static_cast<int>(mPlayer.mPos.y),
        mPlayer.mWidth,
        mPlayer.mHeight
    };
    SDL_RenderFillRect(mRenderer, &playerRect);

    // ... [Rest of the rendering code]
    SDL_RenderPresent(mRenderer);
}

void Game::Shutdown()
{
	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
	SDL_Quit();
}