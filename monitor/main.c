#define SDL_DISABLE_IMMINTRIN_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>

#define bool int
#define true 1
#define false 0

#define float64 double
#define KILO (1024)
#define MEGA (1024 * KILO)
#define GIGA (1024 * MEGA)

#define SCREEN_WIDTH (1080)
#define SCREEN_HEIGHT (720)
#define CHAR_WIDTH (1223.0f/90.0f)
#define CHAR_HEIGHT (28.0f)
#define SMALL_CHAR_WIDTH (571.0f/90.0f)
#define SMALL_CHAR_HEIGHT (14.0f)

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

typedef struct {
    int r;
    int g;
    int b;
} Color;

typedef struct {
    char date[11];
    char time[9];
    int days;
    int seconds;
    double temperature;
    double pressure;
} Data;

Color
RGB(int r, int g, int b) {
    Color color;
    color.r = r;
    color.g = g;
    color.b = b;
    return color;
}

void
ExitSequence(SDL_Window* window, SDL_Renderer* renderer, Data* data) {
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

    if (data) {
	free(data);
	data = NULL;
    }
    
    SDL_Quit();
    printf("Complete.\n");

    exit(1);
}

void
BlitChar(SDL_Renderer* renderer, SDL_Texture *atlas, char c, int x, int y, int charWidth) {
    SDL_Rect dest;
    dest.x = x;
    dest.y = y;
    dest.w = charWidth;
    SDL_QueryTexture(atlas, NULL, NULL, NULL, &dest.h);
    
    SDL_Rect src;
    src.x = ((int)c - 32) * charWidth; /* the first 32 ascii codes aren't on the atlas */
    src.y = 0;
    src.w = (int)charWidth;
    src.h = 31;
    SDL_RenderCopy(renderer, atlas, &src, &dest);
}

void
BlitChars(SDL_Renderer* renderer, SDL_Texture* atlas, char* s, int count, int x, int y, int charWidth) {
    for (int i = 0; i < count; ++i) {
	BlitChar(renderer, atlas, s[i], x, y, charWidth);
	x += charWidth;
    }
}


void DataExpand(Data *data, int* dataCap) {
    Data* new = (Data*)malloc((*dataCap + MEGA) * sizeof(Data));
    memccpy(new, data, *dataCap, sizeof(Data));
    free(data);
    data = new;
    *dataCap += MEGA;
    printf("Data expanded to %dMB\n", *dataCap/MEGA);
}

void SDL_SetRenderDrawColorRGB(SDL_Renderer* renderer, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
}

double LinearMap(double value, double fromLow, double fromHigh, double toLow, double toHigh) {
    return (value - fromLow) / (fromHigh - fromLow) * (toHigh - toLow) + toLow;
}

void
DrawDottedLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, float shift) {
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    float length = sqrt(dx*dx + dy*dy);
    float segmentLength = 3.0f;
    int segmentCount = (int)(length / segmentLength);
    float dxs = dx / segmentCount;
    float dys = dy / segmentCount;
    shift = shift - floorf(shift);
    int x = x1 + shift * 2 * dxs;
    int y = y1 + shift * 2 * dys;
    if (shift > 0.5f) {
	SDL_RenderDrawLine(renderer, x1, y1, x - shift*dxs, y - shift*dys);
    }
    for (int i = 0; i < segmentCount - 2; i += 2) {
	SDL_RenderDrawLine(renderer, x, y, x + dxs, y + dys);
	x += dxs * 2;
	y += dys * 2;
    }
    SDL_RenderDrawLine(renderer, x, y, x1 + segmentCount * dxs, y2 + segmentCount * dys);
}

int
main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
	printf("Init SDL failed\n");
	return 0;
    }

    SDL_Window* window = SDL_CreateWindow("Weather Data Monitoring",
					  SDL_WINDOWPOS_UNDEFINED,
					  SDL_WINDOWPOS_UNDEFINED,
					  SCREEN_WIDTH, SCREEN_HEIGHT,
					  0);

    if (!window) {
	printf("create window failed");
	ExitSequence(window, NULL, NULL);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
						SDL_RENDERER_ACCELERATED);
    if (!renderer) {
	printf("create renderer failed");
	ExitSequence(window, renderer, NULL);
    }

    /* Init IMG */
    if (IMG_Init(IMG_INIT_PNG) < 0) {
	printf("IMG_Init failed\n");
	ExitSequence(window, renderer, NULL);
    }
    
    /* load font atlas png */
    SDL_Texture* fontAtlas;
    char* fontFile = "font.png";
    printf("Loading %s\n", fontFile);
    fontAtlas = IMG_LoadTexture(renderer, fontFile);
    if (!fontAtlas) {
	printf("Failed to load %s\n", fontFile);
    }
    
    SDL_Texture* smallFontAtlas;
    char* smallFontFile = "smallfont.png";
    printf("Loading %s\n", smallFontFile);
    smallFontAtlas = IMG_LoadTexture(renderer, smallFontFile);
    if (!smallFontAtlas) {
	printf("Failed to load %s\n", smallFontFile);
    }

    printf("Init Successful\n");

    Data* data = (Data*)malloc(MEGA * sizeof(Data));
    struct stat file_info;
    int dataCap = MEGA;
    int dataIndex = 0;
    int visibleIndex = 0;
    int targetVisibleIndex = 0;
    
    /* read data file */
    char* filename = "../data.txt";
    if (argc == 2) filename = argv[1];
    FILE* file = fopen(filename, "r");
    printf("Opening file %s\n", filename);
    if (file == NULL) {
	printf("Can't open file\n");
	ExitSequence(window, renderer, data);
    } else {
	// Load data file
	if (stat(filename, &file_info) != 0) {
	    printf("Failed to retrieve file edit time\n");
	} else {
	    printf("File size: %lld bytes\n", (long long)file_info.st_size);	    
	    Data inData;
	    while (fscanf(file, "%s %s %lf %lf", inData.date, inData.time, &inData.temperature, &inData.pressure) == 4) {
		if (dataIndex == dataCap) {
		    DataExpand(data, &dataCap);
		}
		int year, month, day;
		int hour, minute, second;
		sscanf(inData.date, "%d-%d-%d", &year, &month, &day);
		inData.days = 365*year + 30*month + day;
		sscanf(inData.time, "%d-%d-%d", &hour, &minute, &second);
		inData.seconds = 3600*hour + 60*minute + second;
		data[dataIndex] = inData;
		++dataIndex;
	    }
	}

	fclose(file);
	printf("Read %d rows from file\n", dataIndex);
    }

    /* main loop */
    bool running = true;
    int targetFPS = 60;
    float64 frameTimeBudget = 1000.0/(float64)(targetFPS); // in ms
    clock_t lastTick = clock();
    float64 frameTime = 0;
    float delta = (float)frameTimeBudget/1000.0f;
    Uint32 sleepTime;

    const Color backgroundColor = RGB(54,51,95);
    const Color graphFrameColor = RGB(255,255,255);
    const Color tempDataColor = RGB(219,217,247);
    const Color pressLineColor = RGB(80,220,162);
    const Color dottedLineColor = RGB(255,255,255);
    const Color tempDefinitionLineColor = RGB(82,11,16);
    const Color pressDefinitionLineColor = RGB(21,53,18);
    const Color tempGraphBG = RGB(166,33,88);
    const Color pressGraphBG = RGB(77,90,54);

    while (running) {
	/* event handling */
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0) {
	    switch (event.type) {
	    case SDL_QUIT: { running = false; } break;
	    case SDL_KEYDOWN: {
		switch (event.key.keysym.sym) {
		case SDLK_ESCAPE: { running = false; } break;
		}
	    } break;
	    case SDL_MOUSEWHEEL: {
		int step = dataIndex / 20;
		targetVisibleIndex -= event.wheel.y * step ;
		targetVisibleIndex = max(0, min((dataIndex - 100), targetVisibleIndex));
	    } break;
	    }
	}

	float catchUpSpeed = (float)(targetVisibleIndex - visibleIndex) / 50.0f;
	visibleIndex += catchUpSpeed;
	visibleIndex = max(0, min(dataIndex, visibleIndex));
	/* check if file updated */
	struct stat new_info;
	if (stat(filename, &new_info) != 0) {
	    printf("Failed to retrieve file edit time\n");
	} else if (file_info.st_size != new_info.st_size) {
	    printf("File size went from %ld to %ld, reading new entries\n", file_info.st_size, new_info.st_size);
	    file_info = new_info;
	    
	    FILE* file = fopen(filename, "r");
	    if (file == NULL) {
		printf("Can't open file\n");
	    } else {
		// Load data file
		int row = 0;
		char buffer[50];
		while (row < dataIndex && fgets(buffer, sizeof(buffer), file) != NULL) {
		    ++row;
		}
		Data inData;
		int newRows = 0;
		while (fscanf(file, "%s %s %lf %lf", inData.date, inData.time, &inData.temperature, &inData.pressure) == 4) {
		    if (dataIndex == dataCap) {
			DataExpand(data, &dataCap);
		    }
		    data[dataIndex] = inData;
		    ++dataIndex;
		    ++newRows;
		}
		fclose(file);
		printf("Read %d new rows from file\n", newRows);
	    }
	}	
	SDL_SetRenderDrawColorRGB(renderer,backgroundColor);
	SDL_RenderClear(renderer);

	/* find max and min */
	double tempMin = 10000;
	double tempMax = -10000;
	double pressMin = 1000000;
	double pressMax = 0;

	for (int i = visibleIndex; i < dataIndex; ++i) {
	    Data item = data[i];
	    tempMin = min(tempMin, item.temperature);
	    tempMax = max(tempMax, item.temperature);
	    pressMin = min(pressMin, item.pressure);
	    pressMax = max(pressMax, item.pressure);
	}
	
	/* draw stuff */
	/* prepare some numbers */
	int graphY = 100;
	int graphW = 400;
	int graphH = 300;
	int graphY2 = graphY + graphH;

	int tempGraphX = 100;
	int tempGraphX2 = tempGraphX + graphW * 0.85f;
	float tempDisplayRangeLow = (tempMax - tempMin) * -0.2 + tempMin;
	float tempDisplayRangeHigh = tempMax + (tempMax - tempMin) * 0.2;
	
	int pressGraphX = SCREEN_WIDTH - 100 - graphW;
	int pressGraphX2 = pressGraphX + graphW * 0.85f;
	float pressDisplayRangeLow = (pressMax - pressMin) * -0.2 + pressMin;
	float pressDisplayRangeHigh = pressMax + (pressMax - pressMin) * 0.2;

	/* graph titles */
	BlitChars(renderer, fontAtlas, "Temperature (Celsius)", 22, tempGraphX, graphY - CHAR_HEIGHT, CHAR_WIDTH);
	BlitChars(renderer, fontAtlas, "Atomospheric Pressure (Millibar)", 33, pressGraphX, graphY - CHAR_HEIGHT, CHAR_WIDTH);
	
	/* graph background */
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_Rect tempRect;
	tempRect.x = tempGraphX;
	tempRect.y = graphY;
	tempRect.w = graphW;
	tempRect.h = graphH;
	SDL_SetRenderDrawColorRGB(renderer, tempGraphBG);
	SDL_RenderFillRect(renderer, &tempRect);

	SDL_Rect pressRect;
	pressRect.x = pressGraphX;
	pressRect.y = graphY;
	pressRect.w = graphW;
	pressRect.h = graphH;
	SDL_SetRenderDrawColorRGB(renderer, pressGraphBG);
	SDL_RenderFillRect(renderer, &pressRect);
	
	/* Temperature */
	/* Horizontal whole number lines */
	float tempDefinitionLevels[6] = { 5,2,1,0.5,0.2,0.1};
	float tempDisplayDiff = (tempDisplayRangeHigh - tempDisplayRangeLow) / 5.0f;
	float tempDefinitionLevel = tempDefinitionLevels[0];
	for (int i = 0; i < 6; ++i) {
	    if (tempDisplayDiff < tempDefinitionLevels[i]) {
		tempDefinitionLevel = tempDefinitionLevels[i];
	    } else {
		break;
	    }
	}
	float lineValue = 0;
	while (lineValue < tempDisplayRangeLow) {
	    lineValue += tempDefinitionLevel;
	}
	SDL_SetRenderDrawColorRGB(renderer, tempDefinitionLineColor);
	while (lineValue < tempDisplayRangeHigh) {
	    int y = (int)(LinearMap(lineValue, tempDisplayRangeLow, tempDisplayRangeHigh, graphY2, graphY));
	    SDL_RenderDrawLine(renderer, tempGraphX, y, tempGraphX + graphW, y);
	    char num[6];
	    sprintf(num, "%.2f", lineValue);
	    BlitChars(renderer, smallFontAtlas, num,6,tempGraphX + SMALL_CHAR_WIDTH, y, SMALL_CHAR_WIDTH);
	    lineValue += tempDefinitionLevel;
	}

	/* Pressure */
	/* Horizontal whole number lines */
	float pressDefinitionLevels[8] = { 20.0f, 10.0f, 5,2,1,0.5,0.2,0.1 };
	float pressDisplayDiff = (pressDisplayRangeHigh - pressDisplayRangeLow) / 5.0f;
	float pressDefinitionLevel = pressDefinitionLevels[0];
	for (int i = 0; i < 8; ++i) {
	    if (pressDisplayDiff <= pressDefinitionLevels[i]) {
		pressDefinitionLevel = pressDefinitionLevels[i];
	    } else {
		break;
	    }
	}
	lineValue = 0;
	while (lineValue < pressDisplayRangeLow) {
	    lineValue += pressDefinitionLevel;
	}
	SDL_SetRenderDrawColorRGB(renderer, pressDefinitionLineColor);
	while (lineValue < pressDisplayRangeHigh) {
	    int y = (int)(LinearMap(lineValue, pressDisplayRangeLow, pressDisplayRangeHigh, graphY2, graphY));
	    SDL_RenderDrawLine(renderer, pressGraphX, y, pressGraphX + graphW, y);
	    char num[6];
	    sprintf(num, "%.2f", lineValue);
	    BlitChars(renderer, smallFontAtlas, num, 6, pressGraphX+SMALL_CHAR_WIDTH, y, SMALL_CHAR_WIDTH);
	    lineValue += pressDefinitionLevel;
	}
	
	
	/* Data points */
	/* Temperature */
	bool drewMin = false;
	bool drewMax = false;
	static float shift = 0;
	shift += delta * 3;
	SDL_SetRenderDrawColorRGB(renderer, tempDataColor);
	bool drawRect = dataIndex - visibleIndex < 1000;
	for (int i = visibleIndex; i < dataIndex; ++i) {
	    Data item = data[i];
	    int x = ((int)LinearMap(i, visibleIndex, dataIndex, tempGraphX, tempGraphX2));
	    int y = ((int)LinearMap(item.temperature, tempDisplayRangeLow, tempDisplayRangeHigh, graphY2, graphY));
	    if (drawRect) {
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = 2;
		rect.h = 2;
		SDL_RenderDrawRect(renderer, &rect);
	    } else {
		SDL_RenderDrawPoint(renderer, x, y);
	    }
	    if ((item.temperature == tempMin && !drewMin) || (item.temperature == tempMax && !drewMax)) {
		/* horizontal dotted line */
		char s[6];
		sprintf(s, "%.2f", item.temperature);
		BlitChars(renderer, fontAtlas, s,4, tempGraphX - 4.5 * CHAR_WIDTH, y - CHAR_HEIGHT/2, CHAR_WIDTH);
		SDL_SetRenderDrawColorRGB(renderer, dottedLineColor);
		DrawDottedLine(renderer, tempGraphX - CHAR_WIDTH/2, y, x, y, shift);
		SDL_SetRenderDrawColorRGB(renderer, tempDataColor);
	    }
	    if (item.temperature == tempMin) drewMin = true;
	    if (item.temperature == tempMax) drewMax = true;
	}
	
	/* Pressure */
	drewMin = false;
	drewMax = false;
	SDL_SetRenderDrawColorRGB(renderer, pressLineColor);
	for (int i = visibleIndex; i < dataIndex; ++i) {
	    Data item = data[i];
	    int x = ((int)LinearMap(i, visibleIndex, dataIndex, pressGraphX, pressGraphX2));
	    int y = ((int)LinearMap(item.pressure, pressDisplayRangeLow, pressDisplayRangeHigh, graphY2, graphY));
	    if (drawRect) {
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = 2;
		rect.h = 2;
		SDL_RenderDrawRect(renderer, &rect);
	    } else {
		SDL_RenderDrawPoint(renderer, x, y);
	    }
	    SDL_RenderDrawPoint(renderer, x, y);
	    if ((item.pressure == pressMin && !drewMin) || (item.pressure == pressMax && !drewMax)) {
		/* horizontal dotted line */
		char s[6];
		sprintf(s, "%.2f", item.pressure);
		BlitChars(renderer, fontAtlas, s,6, pressGraphX + graphW + CHAR_WIDTH/2, y - CHAR_HEIGHT/2, CHAR_WIDTH);
		SDL_SetRenderDrawColorRGB(renderer, dottedLineColor);
		DrawDottedLine(renderer, x, y, pressGraphX + graphW + CHAR_WIDTH/2, y, shift);
		SDL_SetRenderDrawColorRGB(renderer, pressLineColor);
	    }
	    if (item.pressure == pressMin) drewMin = true;
	    if (item.pressure == pressMax) drewMax = true;
	}


	/* frame */
	SDL_SetRenderDrawColorRGB(renderer,graphFrameColor);
	SDL_RenderDrawRect(renderer, &tempRect);
	SDL_SetRenderDrawColorRGB(renderer,graphFrameColor);
	SDL_RenderDrawRect(renderer, &pressRect);
        SDL_RenderPresent(renderer);

	/* Timing and sleeping */
	/* Time is divided into three parts: */
	/* process time, sleep time, misc undil process */
	/* we start timing right after sleep, stop at the end of process time, their sum is true frame time */
	/* frame time budget minus that is how much we should sleep */
	frameTime = 1000.0 * (double)(clock() - lastTick) / (double)(CLOCKS_PER_SEC);
	if (frameTime > frameTimeBudget) {
	    printf("WARNING: Missed frame, frame time was %.3fms\n", frameTime);
	    delta = (float)frameTime/1000.0f;
	} else {
	    sleepTime = (Uint32)(frameTimeBudget - frameTime);
	    SDL_Delay(sleepTime);
	    delta = frameTimeBudget/1000.0f;
	}
	lastTick = clock();
    }

    ExitSequence(window, renderer, data);
    return 0;
}
