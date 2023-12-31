// One block = 50 pixel

#include "Game.h"

const int thickness = 15;

struct Player {
	Vector2 mPos;
	int mWidth;
	int mHeight;
	float mVelY;
	float mVelX;
	bool isCrouching;
	bool isOnGround;
	bool facingRight;	

	// Animation fields
	SDL_Texture* spriteSheet;
	int frameWidth;
	int frameHeight;
	int currentFrame;
	float frameTime;
};


struct Block {
	bool isActive;
	SDL_Color color;
};

struct Inventory {
	std::vector<Block> blocks;
	int maxCapacity = 64;
	int selectedIndex = 0;
};

struct BlockPickup {
	Vector2 position;
	Block block;
	bool isActive;
};

struct Cloud {
	Vector2 position;
	int width;
	int height;
	SDL_Texture* texture;
	float speed;  // Speed of the cloud
};

Player mPlayer;
SDL_Rect playerRect;
SDL_Rect groundRect;
Inventory mInventory;
std::vector<BlockPickup> mBlockPickups;
std::vector<Cloud> clouds;

const SDL_Color blockColors[9] = {
	{255, 255, 255, 255}, // White
	{192, 192, 192, 255}, // Light Gray
	{128, 128, 128, 255}, // Gray
	{64, 64, 64, 255},    // Dark Gray
	{255, 0, 0, 255},     // Red
	{0, 255, 0, 255},     // Green
	{0, 0, 255, 255},     // Blue
	{255, 255, 0, 255},   // Yellow
	{0, 0, 0, 255}        // Black
};



// Grid variables
bool mShowGrid = true;
int mGridSize = 50; // Grid cell size

// Block variables
const int gridWidth = 1024 / 50; // Assuming grid size of 50
const int gridHeight = 768 / 50;
std::vector<std::vector<Block>> gridBlocks(gridWidth, std::vector<Block>(gridHeight, { false, {128, 0, 128, 255} })); // Initialize all blocks as inactive

// Inventory variables
const int invGridSize = 50; // Size of each inventory grid cell
const int invGridWidth = 1024 / invGridSize; // Width of inventory grid
const int invGridHeight = 1; // Height of inventory grid (1 row)
const int invGridYPos = 768 - invGridSize; // Y position of inventory grid

// World variables
const int groundHeight = 168; // You can adjust this value as needed

// Collision Check, Check if two rectangles intersect
bool CheckCollision(const SDL_Rect& a, const SDL_Rect& b) {
	// Check if two rectangles intersect
	return (a.x < b.x + b.w) &&
		(a.x + a.w > b.x) &&
		(a.y < b.y + b.h) &&
		(a.y + a.h > b.y);
}


Game::Game()
{
	mWindow = nullptr;
	mRenderer = nullptr;
	mTicksCount = 0;
	mIsRunning = true;

	highlightColor = { 255, 255, 255, 255 };
	highlightColorChangeDirection = 1;
	highlightThickness = 2;
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

	// Initialize sounds
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2400);
	mSoundtrack = Mix_LoadWAV("Chiptune.wav");
	mJump = Mix_LoadWAV("se_jump_003.wav");


	// Initialize player sprite
	SDL_Surface* tempSurface = IMG_Load("Idle.png");
	mPlayer.spriteSheet = SDL_CreateTextureFromSurface(mRenderer, tempSurface);
	SDL_FreeSurface(tempSurface);

	mPlayer.frameWidth = 128; // Width of each frame
	mPlayer.frameHeight = 128; // Height of each frame
	mPlayer.currentFrame = 0;
	mPlayer.frameTime = 0.0f;

	// Initialize player
	mPlayer.mPos.x = 100.0f;
	mPlayer.mPos.y = 768.0f - thickness - 100.0f;
	mPlayer.mWidth = 50;
	mPlayer.mHeight = 100;
	mPlayer.mVelY = 0.0f;
	mPlayer.isCrouching = false;
	mPlayer.isOnGround = true; // Initially, the player is on the ground
	mPlayer.facingRight = true; // Initially facing 

	// Initialize inventory
	for (int i = 0; i < 9; ++i) {
		mInventory.blocks.push_back({ true, blockColors[i] });
	}
	mInventory.selectedIndex = 0; // Start with the first block selected
	
	// Play Soundtrack
	Mix_PlayChannel(-1, mSoundtrack, 0);

	// Load cloud texture
	SDL_Surface* cloudSurface = IMG_Load("Clouds.png");
	SDL_Texture* cloudTexture = SDL_CreateTextureFromSurface(mRenderer, cloudSurface);
	SDL_FreeSurface(cloudSurface);

	// Initialize clouds
	for (int i = 0; i < 5; ++i) {
		Cloud cloud;
		cloud.position.x = static_cast<float>(rand() % 1024);
		cloud.position.y = static_cast<float>(rand() % 200);
		cloud.speed = 0.0f + static_cast<float>(rand() % 100);
		cloud.texture = cloudTexture;
		SDL_QueryTexture(cloud.texture, NULL, NULL, &cloud.width, &cloud.height); // Get width and height from texture
		clouds.push_back(cloud);
	}

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
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			mIsRunning = false;
			break;
		case SDL_KEYDOWN:
			if (!event.key.repeat) {
				if (event.key.keysym.scancode == SDL_SCANCODE_G) {
					mShowGrid = !mShowGrid;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_EQUALS) {
			#ifndef NDEBUG
					mGridSize += 10; // Increase grid size
					mGridSize = std::min(mGridSize, 200); // Optional: Max grid size limit
			#endif
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_MINUS) {
			#ifndef NDEBUG
					mGridSize = std::max(10, mGridSize - 10); // Decrease grid size, minimum 10
			#endif
				}

				// Handle inventory selection with number keys
				// (I know I am big brained lmao uwu. I just came out with an idea at 2am LOL)
				if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9) {
					int selectedBlock = event.key.keysym.sym - SDLK_1;
					if (selectedBlock < mInventory.blocks.size()) {
						mInventory.selectedIndex = selectedBlock;
					}
				}

			}
			break;
		}

		if (event.type == SDL_MOUSEBUTTONDOWN) {
			int x, y;
			SDL_GetMouseState(&x, &y);
			int gridX = x / mGridSize; // Calculate gridX based on mouse x-coordinate
			int gridY = y / mGridSize; // Calculate gridY based on mouse y-coordinate

			if (gridX < gridWidth && gridY < gridHeight) {
				if (event.button.button == SDL_BUTTON_LEFT) {
					gridBlocks[gridX][gridY].isActive = false; // Remove block
				}
				else if (event.button.button == SDL_BUTTON_RIGHT) {
					if (mInventory.selectedIndex < mInventory.blocks.size()) {
						gridBlocks[gridX][gridY] = mInventory.blocks[mInventory.selectedIndex]; // Place selected block
						gridBlocks[gridX][gridY].isActive = true;
					}
				}
			}
		}

	}

	// Handle block pickup
	for (auto& pickup : mBlockPickups) {
		if (pickup.isActive) {
			SDL_Rect pickupRect = { static_cast<int>(pickup.position.x), static_cast<int>(pickup.position.y), mGridSize / 2, mGridSize / 2 };
			if (CheckCollision(playerRect, pickupRect)) {
				if (mInventory.blocks.size() < mInventory.maxCapacity) {
					mInventory.blocks.push_back(pickup.block);
					pickup.isActive = false; // Deactivate the pickup
				}
			}
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
		if (mPlayer.isCrouching) {
			mPlayer.mVelX = -100.0f; // Move slower while crouching
			mPlayer.facingRight = false; // Facing left
		}
		else {
			mPlayer.mVelX = -300.0f; // Move left
			mPlayer.facingRight = false; // Facing left
		}
	}
	else if (state[SDL_SCANCODE_D]) {
		if (mPlayer.isCrouching) {
			mPlayer.mVelX = 100.0f; // Move slower while crouching
			mPlayer.facingRight = true; // Facing right
		}
		else {
			mPlayer.mVelX = 300.0f; // Move right
			mPlayer.facingRight = true; // Facing right
		}
	}
	else {
		mPlayer.mVelX = 0.0f; // Stop moving horizontally
	}
	if (state[SDL_SCANCODE_W] && mPlayer.isOnGround) {
		mPlayer.mVelY = -350.0f; // Set a negative velocity to move up
		mPlayer.isOnGround = false;
		Mix_PlayChannel(-1, mJump, 0); // Play Jump sound effect
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
	// Handle inventory selection
	if (state[SDL_SCANCODE_LEFT]) {
		mInventory.selectedIndex = std::max(0, mInventory.selectedIndex - 1);
	}
	if (state[SDL_SCANCODE_RIGHT]) {
		mInventory.selectedIndex = std::min(static_cast<int>(mInventory.blocks.size()) - 1, mInventory.selectedIndex + 1);
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


	// Animation logic
	const float frameDuration = 0.25f; // Duration of each frame in seconds

	mPlayer.frameTime += deltaTime;
	if (mPlayer.frameTime >= frameDuration) {
		mPlayer.currentFrame = (mPlayer.currentFrame + 1) % 4; // 4 frames
		mPlayer.frameTime = 0.0f;
	}


	// Apply gravity to vertical velocity
	if (!mPlayer.isOnGround) {
		mPlayer.mVelY += 500.0f * deltaTime; // Gravity effect
	}

	// Update player's position
	mPlayer.mPos.y += mPlayer.mVelY * deltaTime;
	mPlayer.mPos.x += mPlayer.mVelX * deltaTime;

	// Reset ground collision flag
	mPlayer.isOnGround = false;

	// Collision detection and response (Player Hitbox)
	playerRect = {
		static_cast<int>(mPlayer.mPos.x),
		static_cast<int>(mPlayer.mPos.y),
		mPlayer.mWidth,
		mPlayer.mHeight
	};

	for (int x = 0; x < gridWidth; ++x) {
		for (int y = 0; y < gridHeight; ++y) {
			if (gridBlocks[x][y].isActive) {
				SDL_Rect blockRect = { x * mGridSize, y * mGridSize, mGridSize, mGridSize };
				if (CheckCollision(playerRect, blockRect)) {
					// Calculate overlap on each axis
					float overlapX = std::min(playerRect.x + playerRect.w, blockRect.x + blockRect.w) - std::max(playerRect.x, blockRect.x);
					float overlapY = std::min(playerRect.y + playerRect.h, blockRect.y + blockRect.h) - std::max(playerRect.y, blockRect.y);

					// Resolve collision based on the axis of least overlap
					if (overlapX < overlapY) {
						// Horizontal collision
						if (mPlayer.mVelX > 0) { // Moving right
							mPlayer.mPos.x -= overlapX;
						}
						else if (mPlayer.mVelX < 0) { // Moving left
							mPlayer.mPos.x += overlapX;
						}
						mPlayer.mVelX = 0;
					}
					else {
						// Vertical collision
						if (mPlayer.mVelY > 0) { // Falling down
							mPlayer.mPos.y -= overlapY;
							mPlayer.isOnGround = true;
						}
						else if (mPlayer.mVelY < 0) { // Going up
							mPlayer.mPos.y += overlapY;
						}
						mPlayer.mVelY = 0;
					}
				}
			}
		}
	}

	// Example of collision detection with ground
	if (CheckCollision(playerRect, groundRect)) {
		mPlayer.mPos.y = groundRect.y - mPlayer.mHeight;
		mPlayer.isOnGround = true;
		mPlayer.mVelY = 0.0f;
	}

	// Check if the player has landed on the ground
	if (mPlayer.mPos.y >= 768.0f - thickness - mPlayer.mHeight) {
		mPlayer.mPos.y = 768.0f - thickness - mPlayer.mHeight;
		mPlayer.isOnGround = true;
		mPlayer.mVelY = 0.0f;
	}

	// Update highlight color for selection
	const int colorChangeSpeed = 5; // Adjust speed of color change
	if (highlightColorChangeDirection == 1) {
		highlightColor.r = std::min(255, highlightColor.r + colorChangeSpeed);
		highlightColor.g = std::min(255, highlightColor.g + colorChangeSpeed);
		highlightColor.b = std::min(255, highlightColor.b + colorChangeSpeed);
		if (highlightColor.r == 255 && highlightColor.g == 255 && highlightColor.b == 255) {
			highlightColorChangeDirection = -1;
		}
	}
	else {
		highlightColor.r = std::max(0, highlightColor.r - colorChangeSpeed);
		highlightColor.g = std::max(0, highlightColor.g - colorChangeSpeed);
		highlightColor.b = std::max(0, highlightColor.b - colorChangeSpeed);
		if (highlightColor.r == 0 && highlightColor.g == 0 && highlightColor.b == 0) {
			highlightColorChangeDirection = 1;
		}
	}

	// Update clouds
	for (auto& cloud : clouds) {
		cloud.position.x += cloud.speed * deltaTime;
		if (cloud.position.x > 1024) { // If cloud moves off-screen
			cloud.position.x = -static_cast<float>(cloud.width); // Reset to left side
		}
	}

	// Update tick counts (for next frame)
	mTicksCount = SDL_GetTicks();
}

void Game::GenerateOutput() {
    // Set background to blue
    SDL_SetRenderDrawColor(mRenderer, 0, 191, 255, 255);
    SDL_RenderClear(mRenderer);

	// Draw clouds
	for (const auto& cloud : clouds) {
		// Define scale factor (e.g., 0.5 for half size)
		float scaleFactor = 0.3f;

		SDL_Rect cloudRect = {
			static_cast<int>(cloud.position.x),
			static_cast<int>(cloud.position.y),
			static_cast<int>(cloud.width * scaleFactor),  // Scale width
			static_cast<int>(cloud.height * scaleFactor)  // Scale height
		};
		SDL_RenderCopy(mRenderer, cloud.texture, NULL, &cloudRect);
	}

	// Draw ground
	SDL_SetRenderDrawColor(mRenderer, 139, 69, 19, 255); // Brown color for ground
	groundRect = { 0, 768 - groundHeight, 1024, groundHeight };
	SDL_RenderFillRect(mRenderer, &groundRect);

	SDL_Rect srcRect = {
		mPlayer.frameWidth * mPlayer.currentFrame, // X position based on current frame
		0, // Y position (top of the sprite sheet)
		mPlayer.frameWidth,
		mPlayer.frameHeight
	};

	int adjustedHeight = mPlayer.mHeight;
	int yOffset = 0;

	// Adjust height and Y-offset if the player is crouching
	if (mPlayer.isCrouching) {
		adjustedHeight /= 2; // Example: Reduce height by half
		yOffset = adjustedHeight; // Move down to keep feet at the same position
	}

	SDL_RendererFlip flipType = mPlayer.facingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;

	SDL_Rect destRect = {
		static_cast<int>(mPlayer.mPos.x),
		static_cast<int>(mPlayer.mPos.y) + yOffset,
		mPlayer.frameWidth,
		adjustedHeight
	};

	SDL_RenderCopyEx(mRenderer, mPlayer.spriteSheet, &srcRect, &destRect, 0.0, NULL, flipType);


	// Get current mouse position
	int mouseX, mouseY;
	SDL_GetMouseState(&mouseX, &mouseY);

	// Set the range around the mouse to display the grid
	int gridRange = 3; // This is the number of grid cells around the mouse to display

	// Calculate the top-left corner for the grid rendering
	int startX = mouseX - (mouseX % mGridSize) - (gridRange * mGridSize);
	int startY = mouseY - (mouseY % mGridSize) - (gridRange * mGridSize);

	// Draw grid keybind
	if (mShowGrid) {
		SDL_SetRenderDrawColor(mRenderer, 255, 255, 255, 255); // White color for grid

		// Draw vertical lines within range
		for (int x = startX; x <= startX + 2 * gridRange * mGridSize; x += mGridSize) {
			SDL_RenderDrawLine(mRenderer, x, startY, x, startY + 2 * gridRange * mGridSize);
		}

		// Draw horizontal lines within range
		for (int y = startY; y <= startY + 2 * gridRange * mGridSize; y += mGridSize) {
			SDL_RenderDrawLine(mRenderer, startX, y, startX + 2 * gridRange * mGridSize, y);
		}
	}

	// Draw blocks
	for (int x = 0; x < gridWidth; ++x) {
		for (int y = 0; y < gridHeight; ++y) {
			if (gridBlocks[x][y].isActive) {
				SDL_Rect blockRect = { x * mGridSize, y * mGridSize, mGridSize, mGridSize };
				SDL_SetRenderDrawColor(mRenderer, gridBlocks[x][y].color.r, gridBlocks[x][y].color.g, gridBlocks[x][y].color.b, gridBlocks[x][y].color.a);
				SDL_RenderFillRect(mRenderer, &blockRect);
			}
		}
	}

	// Draw inventory grid
	for (size_t i = 0; i < mInventory.blocks.size(); ++i) {
		SDL_Rect invRect = { static_cast<int>(i * mGridSize), 768 - mGridSize, mGridSize, mGridSize };
		SDL_SetRenderDrawColor(mRenderer, mInventory.blocks[i].color.r, mInventory.blocks[i].color.g, mInventory.blocks[i].color.b, mInventory.blocks[i].color.a);
		SDL_RenderFillRect(mRenderer, &invRect);
	}

	// Draw block pickups
	for (const auto& pickup : mBlockPickups) {
		if (pickup.isActive) {
			SDL_Rect pickupRect = { static_cast<int>(pickup.position.x), static_cast<int>(pickup.position.y), mGridSize / 2, mGridSize / 2 };
			SDL_SetRenderDrawColor(mRenderer, pickup.block.color.r, pickup.block.color.g, pickup.block.color.b, pickup.block.color.a);
			SDL_RenderFillRect(mRenderer, &pickupRect);
		}
	}

	// Draw inventory grid background
	SDL_SetRenderDrawColor(mRenderer, 50, 50, 50, 255); // Dark gray color for inventory background
	SDL_Rect invBackgroundRect = { 0, invGridYPos, 1024, invGridSize };
	SDL_RenderFillRect(mRenderer, &invBackgroundRect);

	// Calculate starting position for inventory blocks
	int invStartX = 512 - (mInventory.blocks.size() * invGridSize) / 2;

	// Draw inventory blocks
	for (size_t i = 0; i < mInventory.blocks.size(); ++i) {
		SDL_Rect invBlockRect = { invStartX + static_cast<int>(i * invGridSize), invGridYPos, invGridSize, invGridSize };
		SDL_SetRenderDrawColor(mRenderer, mInventory.blocks[i].color.r, mInventory.blocks[i].color.g, mInventory.blocks[i].color.b, mInventory.blocks[i].color.a);
		SDL_RenderFillRect(mRenderer, &invBlockRect);
	}

	// Highlight selected block in inventory
	
	int selectedX = invStartX + mInventory.selectedIndex * invGridSize;
	SDL_Rect selectedRect = { selectedX, invGridYPos, invGridSize, invGridSize };

	// Set the color for the highlight
	SDL_SetRenderDrawColor(mRenderer, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);

	// Draw multiple rectangles for a thicker border
	for (int i = 0; i < highlightThickness; ++i) {
		SDL_Rect highlightRect = {
			selectedRect.x - i, selectedRect.y - i,
			selectedRect.w + 2 * i, selectedRect.h + 2 * i
		};
		SDL_RenderDrawRect(mRenderer, &highlightRect);
	}

	SDL_SetRenderDrawColor(mRenderer, highlightColor.r, highlightColor.g, highlightColor.b, highlightColor.a);
	SDL_RenderDrawRect(mRenderer, &selectedRect);

	for (int x = 0; x < gridWidth; ++x) {
		for (int y = 0; y < gridHeight; ++y) {
			if (gridBlocks[x][y].isActive) {
				SDL_Rect blockRect = { x * mGridSize, y * mGridSize, mGridSize, mGridSize };
				SDL_SetRenderDrawColor(mRenderer, gridBlocks[x][y].color.r, gridBlocks[x][y].color.g, gridBlocks[x][y].color.b, gridBlocks[x][y].color.a);
				SDL_RenderFillRect(mRenderer, &blockRect);
			}
		}
	}


    SDL_RenderPresent(mRenderer);
}

void Game::Shutdown()
{
	SDL_DestroyTexture(mPlayer.spriteSheet);
	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
	SDL_Quit();
}