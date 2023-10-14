#define SDL_DISABLE_IMMINTRIN_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <time.h>

#define bool int
#define true 1
#define false 0
#define assert(expr) if(!(expr)) *(char*)0 = 0;

#define float64 double
#define KILO (1024)
#define MEGA (1024 * KILO)
#define GIGA (1024 * MEGA)


#define SCREEN_WIDTH (1080)
#define SCREEN_HEIGHT (720)
#define PER_CHAR_WIDTH (1256.0f / 95.0f)
#define PER_CHAR_HEIGHT 28
#define CHAR_PER_ROW ((SCREEN_WIDTH)/(PER_CHAR_WIDTH))

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

typedef struct {
    int r;
    int g;
    int b;
} Color;

Color
RGB(int r, int g, int b) {
    Color color;
    color.r = r;
    color.g = g;
    color.b = b;
    return color;
}

typedef struct {
    char* buffer;
    int size;
    int gapIncrement;
    int gapLeft;
    int gapRight;
} GapBuffer;

int
GapBuffer_FindLastVisualRow(const GapBuffer* gapBuffer, const int charPerRow, const int start) {
    int steps = 0;
    while (start - steps >= 0 && gapBuffer->buffer[start - steps] != '\n') ++steps;
    return start - (steps % charPerRow);
}

void
GapBuffer_FindVisualRowsAround(const GapBuffer* gapBuffer,
			       const int charPerRow,
			       const int lStart, const int rStart,
			       int* left, int* right) {
    int _left = GapBuffer_FindLastVisualRow(gapBuffer, charPerRow, lStart);
    if (left) *left = _left;

    if (right) {
	int leftDist = lStart - _left;
	int steps;
	for (steps = 0; rStart + steps < gapBuffer->size && gapBuffer->buffer[rStart + steps] != '\n'; ++steps);
	*right = rStart + min(charPerRow - leftDist, steps);
    }
}

void
GapBuffer_MoveGap(GapBuffer* gapBuffer, int movement) {
    int gapSize = gapBuffer->gapRight - gapBuffer->gapLeft;
    int nonGapSize = gapBuffer->size - gapSize;
    int newLeft = max(0, min(nonGapSize, gapBuffer->gapLeft + movement));
    int actualMove = newLeft - gapBuffer->gapLeft;
    int newRight = gapBuffer->gapRight + actualMove;
    int source, destination;
    int dir = actualMove/abs(actualMove);

    if (actualMove < 0) { //copy [newleft, left] into [newright, right] 
	source = gapBuffer->gapLeft - 1;
	destination = gapBuffer->gapRight - 1;
    } else { // copy [right, newright] into [left, newleft]
	source = gapBuffer->gapRight ;
	destination = gapBuffer->gapLeft;
    }
	
    for (int i = 0; i < abs(actualMove); ++i) {
	gapBuffer->buffer[destination+i*dir] = gapBuffer->buffer[source+i*dir];
	gapBuffer->buffer[source+i*dir] = '?';
    }
    gapBuffer->gapLeft = newLeft;
    gapBuffer->gapRight = newRight;
}

void
GapBuffer_Expand(GapBuffer* gapBuffer) {
    int newSize = gapBuffer->size + gapBuffer->gapIncrement;
    int newRight = gapBuffer->gapRight + gapBuffer->gapIncrement;
    char* newBuffer = malloc(newSize * sizeof(char));
    for (int i = 0; i < gapBuffer->gapLeft; ++i) {
	newBuffer[i] = gapBuffer->buffer[i];
    }
    for (int i = gapBuffer->gapLeft; i < newRight; ++i) {
	newBuffer[i] = '\0';
    }
    for (int i = gapBuffer->gapRight; i < gapBuffer->size; ++i) {
	newBuffer[i + gapBuffer->gapIncrement] = gapBuffer->buffer[i];
    }
    free(gapBuffer->buffer);
    gapBuffer->buffer = newBuffer;
    gapBuffer->size = newSize;
    gapBuffer->gapRight = newRight;
    printf("DOMAIN EXPANSION\n");
}

void
GapBuffer_PutChar(GapBuffer* gapBuffer, char c) {
    gapBuffer->buffer[gapBuffer->gapLeft] = c;
    ++gapBuffer->gapLeft;
    if (gapBuffer->gapLeft == gapBuffer->gapRight) {
	GapBuffer_Expand(gapBuffer);
    }
}

void
GapBuffer_DeleteChar(GapBuffer* gapBuffer) {
    if (gapBuffer->gapLeft > 0) {
	--gapBuffer->gapLeft;
	gapBuffer->buffer[gapBuffer->gapLeft] = '\0';
    }
}

bool
GapBuffer_SaveFile(const GapBuffer* gapBuffer, char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION,
		       SDL_LOG_PRIORITY_ERROR,
		       "Couldn't save %s\n", filename);
	return  false;
    }

    int bytesWrote = 0;
    for (int i = 0; i < gapBuffer->size; ++i) {
	if (i == gapBuffer->gapLeft) {
	    i = gapBuffer->gapRight - 1;
	} else {
	    char c = gapBuffer->buffer[i];
	    ++bytesWrote;
	    if (c == '\0') break;
	    fputc(c, file);
	}
    }
    printf("Wrote %d bytes to file %s\n", bytesWrote, filename);

    fclose(file);
    return true;
}

void
ExitSequence(SDL_Window* window, SDL_Renderer* renderer) {
    printf("Exit...");
   
    IMG_Quit();

    if (window) {
	SDL_DestroyWindow(window);
	window = NULL;
    }
    if (renderer) {
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
    }

    SDL_Quit();
    printf("Complete.\n");
}

void
BlitText(SDL_Renderer* renderer, SDL_Texture *atlas, char c, int x, int y) {
    float width = PER_CHAR_WIDTH;

    SDL_Rect dest;
    dest.x = x;
    dest.y = y;
    dest.w = width;
    SDL_QueryTexture(atlas, NULL, NULL, NULL, &dest.h);
    
    SDL_Rect src;
    src.x = ((int)c - 32) * width; /* the first 32 ascii codes aren't on the atlas */
    src.y = 0;
    src.w = (int)width;
    src.h = 31;
    SDL_RenderCopy(renderer, atlas, &src, &dest);
}

int
main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	printf("Init SDL failed\n");
	return 0;
    }

    SDL_Window* window = SDL_CreateWindow("texteditor",
					  SDL_WINDOWPOS_UNDEFINED,
					  SDL_WINDOWPOS_UNDEFINED,
					  SCREEN_WIDTH, SCREEN_HEIGHT,
					  0);

    if (!window) {
	printf("create window failed");
	ExitSequence(window, NULL);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
						SDL_RENDERER_ACCELERATED);
    if (!renderer) {
	printf("create renderer failed");
	ExitSequence(window, renderer);
    }

    /* Init IMG */
    if (IMG_Init(IMG_INIT_PNG) < 0) {
	printf("IMG_Init failed\n");
	ExitSequence(window, renderer);
    }

    /* load font atlas png */
    SDL_Texture* fontAtlas;
    char* fontFile = "firamono.png";
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Loading %s\n", fontFile);
    fontAtlas = IMG_LoadTexture(renderer, fontFile);
    if (!fontAtlas) {
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, "Failed to load %s\n", fontFile);
    }
    
    printf("Init Successful\n");

    /* read default file */
    GapBuffer gapBuffer;
    gapBuffer.buffer = NULL;
    char* filename = "test";
    if (argc == 2) filename = argv[1];
    FILE* file = fopen(filename, "r");
    printf("Opening file %s\n", filename);
    if (file == NULL) {
	printf("Error opening file\n");
	ExitSequence(window, renderer);
    } else {
	fseek(file, 0, SEEK_END);
	long long fileSize = ftell(file);

	if (fileSize == -1) {
	    printf("Error getting file size\n");
	} else {
	    printf("File size: %lld bytes\n", fileSize);
	    fseek(file, 0, SEEK_SET);
	    
	    gapBuffer.gapIncrement = 128;
	    gapBuffer.size = fileSize + gapBuffer.gapIncrement;
	    gapBuffer.buffer = malloc(gapBuffer.size * sizeof(char));
	    gapBuffer.gapLeft = 0;
	    gapBuffer.gapRight = gapBuffer.gapLeft + gapBuffer.gapIncrement;
	    
	    fread(&gapBuffer.buffer[gapBuffer.gapRight], 1, fileSize, file);
	}	    

	fclose(file);
    }

    /* main loop */
    bool running = true;
    int targetFPS = 60;
    float64 frameTimeBudget = 1000.0/(float64)(targetFPS); // in ms
    clock_t lastTick = clock();
    float64 frameTime = 0;
    float delta = (float)frameTimeBudget;
    Uint32 sleepTime;
    int tabSpaceRatio = 4;

    bool initModState = SDL_GetModState();
    bool shiftDown = initModState & KMOD_SHIFT;
    bool ctrlDown  = initModState & KMOD_CTRL;
    bool altDown   = initModState & KMOD_ALT;
    bool capslock  = initModState & KMOD_CAPS;

    const Color backgroundColor = RGB(97,85,52);
    const Color gapBufferVisualColor = RGB(255,255,0);
    const Color renderRangeColor = RGB(0,255,0);
    const Color renderHeadColor = RGB(222,208,164);

    float visualDrawRow = 0;
    
    while (running) {
	/* event handling */
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0) {
	    switch (event.type) {
	    case SDL_QUIT: { running = false; } break;
	    case SDL_KEYDOWN: {
		switch (event.key.keysym.sym) {
		case SDLK_LSHIFT: case SDLK_RSHIFT: { shiftDown = true; } break;
		case SDLK_LCTRL:  case SDLK_RCTRL:  {  ctrlDown = true; } break;
		case SDLK_LALT:   case SDLK_RALT:   {   altDown = true; } break;
		case SDLK_s: {
		    if (ctrlDown) {
			printf("Save command received\n");
			GapBuffer_SaveFile(&gapBuffer, filename);
		    }
		} break;
		case SDLK_f: {
		    if (ctrlDown) {
			printf("Open file command received\n");
		    }
		} break;
		case SDLK_LEFT:  { GapBuffer_MoveGap(&gapBuffer, -1); } break;
		case SDLK_RIGHT: { GapBuffer_MoveGap(&gapBuffer,  1); } break;
		case SDLK_UP: {
		    int lastRow = GapBuffer_FindLastVisualRow(&gapBuffer, CHAR_PER_ROW, gapBuffer.gapLeft);
		    if (lastRow == -1) {
			GapBuffer_MoveGap(&gapBuffer, 0 - gapBuffer.gapLeft);
		    } else {
			const int lastRowDist = gapBuffer.gapLeft - lastRow;
			int lastLastRow = GapBuffer_FindLastVisualRow(&gapBuffer, CHAR_PER_ROW, lastRow - 1);
			const int last2RowDist = lastRow - lastLastRow;
			int newLeft = lastLastRow + min(lastRowDist, last2RowDist);
			GapBuffer_MoveGap(&gapBuffer, newLeft - gapBuffer.gapLeft);
		    }
		} break;
		case SDLK_DOWN: {
		    int lastRow, nextRow;
		    GapBuffer_FindVisualRowsAround(&gapBuffer,CHAR_PER_ROW,
						   gapBuffer.gapLeft,gapBuffer.gapRight,
						   &lastRow,&nextRow);
		    if (nextRow == gapBuffer.size) {
			GapBuffer_MoveGap(&gapBuffer, gapBuffer.size - gapBuffer.gapRight - gapBuffer.gapLeft);
		    } else {
			int next2RowDist;
			for (next2RowDist = 1; next2RowDist < CHAR_PER_ROW; ++next2RowDist) {
			    if (nextRow + next2RowDist == gapBuffer.size ||
				gapBuffer.buffer[nextRow + next2RowDist] == '\n') {
				break;
			    }
			}
			int movement = (nextRow - gapBuffer.gapRight) + min(gapBuffer.gapLeft - lastRow, next2RowDist);
			GapBuffer_MoveGap(&gapBuffer, movement);
		    }
		} break;
		case SDLK_ESCAPE: { running = false; } break;
		case SDLK_BACKSPACE: { GapBuffer_DeleteChar(&gapBuffer); } break;
		case SDLK_RETURN: { GapBuffer_PutChar(&gapBuffer, '\n'); } break;
		case SDLK_TAB: {
		    for (int i = 0; i < tabSpaceRatio; ++i) {
			GapBuffer_PutChar(&gapBuffer, ' ');
		    }
		} break;
		}
		// inputs that should be put into the buffer
		if (!ctrlDown &&
		     32 <= event.key.keysym.sym &&
		    126 >= event.key.keysym.sym) {
		    char inputChar = event.key.keysym.sym;

		    // capitalize alphabet keys
		    if ((shiftDown || capslock) &&
			 97 <= event.key.keysym.sym &&
			122 >= event.key.keysym.sym) {
			inputChar -= 32;
		    }

		    // keys that turn into other keys when shift is down
		    if (shiftDown) {
			switch (inputChar) {
			case '0': { inputChar = ')'; } break;
			case '1': { inputChar = '!'; } break;
			case '2': { inputChar = '@'; } break;
			case '3': { inputChar = '#'; } break;
			case '4': { inputChar = '$'; } break;
			case '5': { inputChar = '%'; } break;
			case '6': { inputChar = '^'; } break;
			case '7': { inputChar = '&'; } break;
			case '8': { inputChar = '*'; } break;
			case '9': { inputChar = '('; } break;
			case ',': { inputChar = '<'; } break;
			case '.': { inputChar = '>'; } break;
			case '/': { inputChar = '?'; } break;
			case '`': { inputChar = '~'; } break;
			case '[': { inputChar = '{'; } break;
			case ']': { inputChar = '}'; } break;
			case '-': { inputChar = '_'; } break;
			case '\\':{ inputChar = '|'; } break;
			case ';': { inputChar = ':'; } break;
			case '\'':{ inputChar = '"'; } break;
			case '=': { inputChar = '+'; } break;
			}
		    }
		    printf("input:%d(%c)\n",(int)inputChar, inputChar);
		    GapBuffer_PutChar(&gapBuffer, inputChar);
		}
	    } break;
	    case SDL_KEYUP: {
		switch (event.key.keysym.sym) {
		case SDLK_LSHIFT: case SDLK_RSHIFT: { shiftDown = false; } break;
		case SDLK_LCTRL:  case SDLK_RCTRL:  {  ctrlDown = false; } break;
		case SDLK_LALT:   case SDLK_RALT:   {   altDown = false; } break;
		case SDLK_CAPSLOCK: { capslock = !capslock;} break;
		}
	    } break;
	    }
	}

	SDL_SetRenderDrawColor(renderer,
			       backgroundColor.r,
			       backgroundColor.g,
			       backgroundColor.b,
			       255);
	SDL_RenderClear(renderer);
	
	/* find render start by back trace from gapleft for five newlines/linelengths */
	const int rowsHalfScreen = SCREEN_HEIGHT / PER_CHAR_HEIGHT / 2;
	const int charPerRow = SCREEN_WIDTH / PER_CHAR_WIDTH;
	int targetDrawHead = gapBuffer.gapLeft;
	for (int i = 0; i < rowsHalfScreen;++i) {
	    targetDrawHead = GapBuffer_FindLastVisualRow(&gapBuffer, CHAR_PER_ROW, targetDrawHead - 1);
	    if (targetDrawHead < 0) break;
	}
	++targetDrawHead;
	int renderEnd = gapBuffer.size;

	/* render text */
	float x = 0;
	float y = 0;
	int textIndex = targetDrawHead;
	while (textIndex < gapBuffer.size) {
	    if (x > SCREEN_WIDTH - PER_CHAR_WIDTH) {
		y += PER_CHAR_HEIGHT;
		x = 0;
	    }
	    if (textIndex == gapBuffer.gapLeft) {
		textIndex = gapBuffer.gapRight;
		/* draw point */
		SDL_Rect pointRect;
		pointRect.x = x;
		pointRect.y = y;
		pointRect.w = PER_CHAR_WIDTH;
		pointRect.h = PER_CHAR_HEIGHT;
		SDL_SetRenderDrawColor(renderer,
				       renderHeadColor.r,
				       renderHeadColor.g,
				       renderHeadColor.b,
				       255);
		SDL_RenderDrawRect(renderer, &pointRect);
	    }
	    char c = gapBuffer.buffer[textIndex];
	    if (c == '\0' || y > SCREEN_HEIGHT + PER_CHAR_HEIGHT) {
		renderEnd = textIndex;
		break;
	    }
	    if (c == '\n') {
		y += PER_CHAR_HEIGHT;
		x = 0;
		++textIndex;
	    } else {
		BlitText(renderer, fontAtlas, c, x, y);
		if (c == '\t') {
		    x += tabSpaceRatio * PER_CHAR_WIDTH;
		} else {
		    x += PER_CHAR_WIDTH;
		}
		++textIndex;
	    }
	}

	/* render gap buffer visualization */
	SDL_SetRenderDrawColor(renderer,255,0,0,255);
        float unitWidth = (float)SCREEN_WIDTH / gapBuffer.size;
	float unitHeight = 20;

	SDL_Rect gapRect;
	/* pre-left */
	gapRect.x = 0;
	gapRect.y = SCREEN_HEIGHT-unitHeight;
	gapRect.w = gapBuffer.gapLeft * unitWidth;
	gapRect.h = unitHeight;
	
        SDL_RenderFillRect(renderer, &gapRect);

	/* gap */
	gapRect.x = gapBuffer.gapLeft * unitWidth;
	gapRect.w = (gapBuffer.gapRight - gapBuffer.gapLeft) * unitWidth;
	
	SDL_RenderDrawRect(renderer, &gapRect);

	/* post-right */
	gapRect.x += gapRect.w;
	gapRect.w = (gapBuffer.size - gapBuffer.gapRight) * unitWidth;

	SDL_RenderFillRect(renderer, &gapRect);

	/* rendered region */
	SDL_SetRenderDrawColor(renderer,
			       renderRangeColor.r,
			       renderRangeColor.g,
			       renderRangeColor.b,
			       255);
	gapRect.y += 1;
	gapRect.x = targetDrawHead * unitWidth;
	gapRect.w = (renderEnd - targetDrawHead) * unitWidth;
	gapRect.h -= 2;
	SDL_RenderDrawRect(renderer, &gapRect);
	
        SDL_RenderPresent(renderer);

	/* Timing and sleeping */
	/* Time is divided into three parts: */
	/* process time, sleep time, misc undil process */
	/* we start timing right after sleep, stop at the end of process time, their sum is true frame time */
	/* frame time budget minus that is how much we should sleep */
	frameTime = 1000.0 * (double)(clock() - lastTick) / (double)(CLOCKS_PER_SEC);
	if (frameTime > frameTimeBudget) {
	    printf("WARNING: Missed frame, frame time was %.3fms\n", frameTime);
	    delta = (float)frameTime;
	} else {
	    sleepTime = (Uint32)(frameTimeBudget - frameTime);
	    SDL_Delay(sleepTime);
	    delta = (float)sleepTime + (float)frameTime;
	}
	lastTick = clock();
    }

    free(gapBuffer.buffer);
    ExitSequence(window, renderer);
    return 0;
}
