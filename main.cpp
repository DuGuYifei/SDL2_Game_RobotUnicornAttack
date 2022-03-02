#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>			//just use the srand to show map randomly, never use others
#include <stdlib.h>

extern "C"
{
#include "./SDL2-2.0.10/include/SDL.h"
#include "./SDL2-2.0.10/include/SDL_main.h"
#include "./SDL2-2.0.10/include/SDL_timer.h"
}

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define Max_Frame 23		//max frames of animation
#define Max_Life 3			//max lives
#define w_TimeFont 15		//width of time display
#define h_TimeFont 20		//heighth of time display 
#define Num_Map 20			//how mant maps will used in "zx" mode


typedef struct
{
	SDL_Rect UnicornRect;
	bool Unicorn_Down;
	bool Unicorn_Up;
	bool Collision;
	int jump[11] = { 35, 20, 10, 5, 0, -3, -7, -14, -30, -40, -5 };
	int jumpindex;
	int jump_time;		
	int jump_max;		//used to check double jump or jump more
	int Frame_Run;
	int UnicornSpeed;
	int dash;
}UnicornType;

typedef struct
{
	SDL_Rect MapRect;
	SDL_Rect MapRect2;
	SDL_Rect Obstacle;
	SDL_Rect Obstacle2;
	int Map1Index;
	int Map2Index;
}MapType;

// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface *screen, int x, int y, int w, int h, const char *text,
				SDL_Surface *charset)
{
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = w;
	d.h = h;
	while (*text)
	{
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitScaled(charset, &s, screen, &d);
		x += w;
		text++;
	};
};

void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y, int w, int h) {
	SDL_Rect Rect;
	Rect.x = x;
	Rect.y = y;
	Rect.w = w;
	Rect.h = h;
	SDL_BlitSurface(sprite, NULL, screen, &Rect);
};

// make the surface show in the window
void ApplySurface(SDL_Surface* screen, int delay, SDL_Renderer* renderer, SDL_Texture* scrtex)
{
	SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, scrtex, NULL, NULL);
	SDL_RenderPresent(renderer);
	if (delay != 0)
	{
		SDL_Delay(delay);
	}
}

// make animation
void animation(int Num_Frame, SDL_Surface* sprite[Max_Frame], int x, int y, int w, int h, int delay, SDL_Surface* screen, SDL_Renderer* renderer, SDL_Texture* scrtex)
{
	for (int i = 0; i < Num_Frame; i++)
	{
		DrawSurface(screen, sprite[i], x, y, w, h);	
		ApplySurface(screen, delay, renderer, scrtex);
	}
}

// draw a single pixel
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color)
{
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32 *)p = color;
};

// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color)
{
	for (int i = 0; i < l; i++)
	{
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k,
				   Uint32 outlineColor, Uint32 fillColor)
{
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
	DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

// set transparent
void SetColorKey(SDL_Surface* sprite, int r, int g, int b)
{
	SDL_SetColorKey(sprite, 1, SDL_MapRGB(sprite->format, r, g, b));
}

//get the pixels info of rgb from a SDL2 surface.
Uint8* GetSurfaceRGB(
	SDL_Surface* surface,  // surface contain pixels we want to test
	Uint32 pixelFormat,    // usually SDL_GetWindowPixelFormat(window)
	SDL_Renderer* renderer,// main SDL2 renderer
	int* width,            // stores result width
	int* height,           // stores result height
	int* pitch)            // stores result pitch
{
	Uint8* pixels = 0;
	SDL_Surface* tmpSurface = 0;
	SDL_Texture* texture = 0;
	int sizeInBytes = 0;
	void* tmpPixels = 0;
	int tmpPitch = 0;

	tmpSurface = SDL_ConvertSurfaceFormat(surface, pixelFormat, 0);
	if (tmpSurface) 
		texture = SDL_CreateTexture(renderer, pixelFormat, SDL_TEXTUREACCESS_STREAMING, tmpSurface->w, tmpSurface->h);

	if (texture) {
		if (width) {
			*width = tmpSurface->w;
		}
		if (height) {
			*height = tmpSurface->h;
		}
		if (pitch) {
			*pitch = tmpSurface->pitch;
		}
		sizeInBytes = tmpSurface->pitch * tmpSurface->h;
		pixels = (Uint8*)malloc(sizeInBytes);
		SDL_LockTexture(texture, 0, &tmpPixels, &tmpPitch);
		memcpy(pixels, tmpSurface->pixels, sizeInBytes);
		SDL_UnlockTexture(texture);
	}

	// Cleanup
	if (texture) {
		SDL_DestroyTexture(texture);
	}
	if (tmpSurface) {
		SDL_FreeSurface(tmpSurface);
	}

	return pixels;
}

// get rgba of (x,y) 
void GetPixelRGB(SDL_Surface* sprite, SDL_Window* window, SDL_Renderer* renderer, int x, int y, int rgb[4])
{
	Uint32 pf = SDL_GetWindowPixelFormat(window);
	int w = 0, h = 0, p = 0;	//whp to store width,height,pitch
	Uint8* pixels = GetSurfaceRGB(sprite, pf, renderer, &w, &h, &p);
	if (pixels)
	{
		//BGRA format
		rgb[2] = pixels[4 * (y * w + x) + 0]; // Blue
		rgb[1] = pixels[4 * (y * w + x) + 1]; // Green
		rgb[0] = pixels[4 * (y * w + x) + 2]; // Red
		rgb[3] = pixels[4 * (y * w + x) + 3]; // Alpha
	}
	free(pixels);
}

bool CheckLeg(int x, int y, SDL_Rect Map,SDL_Rect Unicorn, SDL_Surface* sprite, SDL_Window* window, SDL_Renderer* renderer)
{
	bool test = true;
	if (y >= 0 && y < Map.h)	// bottom between the map bmp
	{
		int rgb[4];
		GetPixelRGB(sprite, window, renderer, x, y, rgb);
		if (rgb[0] != 70 || rgb[1] != 32 || rgb[2] != 96) //if it is the colour of map
		{
			test = true;
		}
		else
		{
			test = false;
		}
	}
	else if (y >= Map.h || y < 0 && Unicorn.y < SCREEN_HEIGHT)	//if unicorn is below the map or above the map, but still in the screen
	{
		test = true;
	}
	return test;
}

//check Unicorn down`
void CheckUnicornDownUp(SDL_Surface* sprite, SDL_Surface* sprite2, SDL_Window* window, SDL_Renderer* renderer, SDL_Rect Unicorn, SDL_Rect Map,SDL_Rect Map2, bool* Unicorn_Down, bool* Unicorn_Up)
{
	int x1;											// back leg of unicorn
	int x2;											// front leg
	int y;											// the bottom
	int y_up;										// the bottom to test up
	SDL_Surface* tmpsprite;							//test back leg
	SDL_Surface* tmpsprite2;						//test front leg
	SDL_Rect tmpMapRect;							//the leg in which map rect
	bool up_back = false;							//test unicorn up
	bool up_front = false;

	//back leg
	if ((Unicorn.x + 2 >= Map.x && Unicorn.x + 2 <= Map.x + Map.w)||(Unicorn.x + 2 >= Map2.x && Unicorn.x + 2 <= Map2.x + Map2.w))
	{
		if (Unicorn.x + 2 > Map.x && Unicorn.x + 2 < Map.x + Map.w)
		{
			x1 = Unicorn.x + 2 - Map.x;
			y = (Unicorn.y + Unicorn.h - 1) - Map.y;
			y_up = y - 3;
			tmpsprite = sprite;
			tmpMapRect = Map;
		}
		else if (Unicorn.x + 2 > Map2.x && Unicorn.x + 2 < Map2.x + Map2.w)
		{
			x1 = Unicorn.x + 2 - Map2.x;
			y = (Unicorn.y + Unicorn.h - 1) - Map2.y;
			y_up = y - 3;
			tmpsprite = sprite2;
			tmpMapRect = Map2;
		}
		*Unicorn_Down = CheckLeg(x1,y,tmpMapRect,Unicorn,tmpsprite,window,renderer);		// while not in the screen, it should stop down
		up_back= CheckLeg(x1, y_up, tmpMapRect, Unicorn, tmpsprite, window, renderer);
	}
	else //the back leg is in the gap between two maps
		*Unicorn_Down = true;

	//front leg
	if (*Unicorn_Down == true)
	{
		if ((Unicorn.x + Unicorn.w - 2 >= Map.x && Unicorn.x + Unicorn.w - 2 <= Map.x + Map.w) || (Unicorn.x + Unicorn.w - 2 >= Map2.x && Unicorn.x + Unicorn.w - 2 <= Map2.x + Map2.w))
		{
			if (Unicorn.x + Unicorn.w - 2 > Map.x && Unicorn.x + Unicorn.w - 2 < Map.x + Map.w)
			{
				x2 = Unicorn.x + Unicorn.w - 2 - Map.x;
				y = (Unicorn.y + Unicorn.h - 1) - Map.y;
				y_up = y - 3;
				tmpsprite = sprite;
				tmpMapRect = Map;
			}
			else if (Unicorn.x + Unicorn.w - 2 > Map2.x && Unicorn.x + Unicorn.w - 2 < Map2.x + Map2.w)
			{
				x2 = Unicorn.x + Unicorn.w - 2 - Map2.x;
				y = (Unicorn.y + Unicorn.h - 1) - Map2.y;
				y_up = y - 3;
				tmpsprite = sprite2;
				tmpMapRect = Map2;
			}
			*Unicorn_Down = CheckLeg(x2, y, tmpMapRect, Unicorn, tmpsprite, window, renderer);		// while not in the screen, it should stop down
			up_front = CheckLeg(x2, y_up, tmpMapRect, Unicorn, tmpsprite, window, renderer);
		}
		else //the back leg is in the gap between two maps
			*Unicorn_Down = true;
	}

	//test up
	if (*Unicorn_Down == false)	//if now drop, test up
	{
		if (up_back == true || up_front == true)
		{
			*Unicorn_Up = true;
		}
	}
	else
		*Unicorn_Up = false;
}

//check up collision
bool MapCollision(int xunicorn, int yunicorn, SDL_Rect Map, SDL_Rect Unicorn, SDL_Surface* mapbg, SDL_Window* window, SDL_Renderer* renderer)
{
	bool collision = false;
	if (xunicorn > Map.x && xunicorn <Map.x + Map.w && yunicorn<Map.y + Map.h && yunicorn > Map.y)
	{
		int x = xunicorn - Map.x;
		int y = yunicorn - Map.y;
		int rgb[4];
		GetPixelRGB(mapbg, window, renderer, x, y, rgb);
		if (rgb[0] == 70 && rgb[1] == 32 && rgb[2] == 96)
		{
			collision = true;
		}
	}
	return collision;
}

//check collision with the map
void CheckCollison(SDL_Surface* Map1, SDL_Surface* Map2, SDL_Surface* Obstacle1, SDL_Surface* Obstacle2, SDL_Window* window, SDL_Renderer* renderer, SDL_Rect Unicorn, SDL_Rect MapRect1, SDL_Rect MapRect2, SDL_Rect ObstacleRect1, SDL_Rect ObstacleRect2,bool* Collision)
{
	*Collision = false;
	int y_up = Unicorn.y + 6;
	//up of unicorn collision with obstacle
	for (int x_up = Unicorn.x + 5; x_up < Unicorn.x + Unicorn.w - 3; x_up++)
	{
		*Collision = MapCollision(x_up, y_up, ObstacleRect1, Unicorn, Obstacle1, window, renderer);
		if (*Collision == true)
			return;
		*Collision = MapCollision(x_up, y_up, ObstacleRect2, Unicorn, Obstacle2, window, renderer);
		if (*Collision == true)
			return;
		*Collision = MapCollision(x_up, y_up, MapRect2, Unicorn, Map2, window, renderer);
		if (*Collision == true)
			return;
		*Collision = MapCollision(x_up, y_up, MapRect1, Unicorn, Map1, window, renderer);
		if (*Collision == true)
			return;
	}

	int x_head = Unicorn.x + Unicorn.w - 4;
	//head of unicorn collision with obstacle
	for (int y_head = Unicorn.y + 6; y_head < Unicorn.y + Unicorn.h - 10; y_head++)
	{
		*Collision = MapCollision(x_head, y_head, ObstacleRect1, Unicorn, Obstacle1, window, renderer);
		if (*Collision == true)
			return;
		*Collision = MapCollision(x_head, y_head, ObstacleRect2, Unicorn, Obstacle2, window, renderer);
		if (*Collision == true)
			return;
		*Collision = MapCollision(x_head, y_head, MapRect2, Unicorn, Map2, window, renderer);
		if (*Collision == true)
			return;
		*Collision = MapCollision(x_head, y_head, MapRect1, Unicorn, Map1, window, renderer);
		if (*Collision == true)
			return;
	}
}


// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv)
{
	int quit, rc, t0, t1, gap;
	double t;
	SDL_Event event;
	SDL_Surface* screen, * charset, * menu, * bg, * life[Max_Life], * result;
	SDL_Surface* start[Max_Frame], * mapbg[Max_Frame], * map[Max_Frame], * run[Max_Frame], * explosion[Max_Frame], * dolphins[Max_Frame], * dash[10];
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
	UnicornType Unicorn;
	MapType Map;
	//TTF_Font* font=TTF_OpenFont("./fonts/1.ttf",FontSize);
	SDL_Color fontcolour = { 0,0,0 };

	printf("welcom unicorn attack\n");
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	// fullscreen mode
	//rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP,
	//	                                 &window, &renderer);
	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0,
									 &window, &renderer);

	if (rc != 0)
	{
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_SetWindowTitle(window, "Robot Unicorn Attack - Yifei Liu s188026");

	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
								  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
							   SDL_TEXTUREACCESS_STREAMING,
							   SCREEN_WIDTH, SCREEN_HEIGHT);
	

	//cursor disable
	SDL_ShowCursor(SDL_DISABLE);

	// wczytanie obrazka cs8x8.bmp
	charset = SDL_LoadBMP("./img/cs8x8.bmp");
	if (charset == NULL)
	{
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);
	//load the menu
	menu = SDL_LoadBMP("./img/menu.bmp");
	if (menu == NULL)
	{
		printf("SDL_LoadBMP(menu.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};

	start[0] = SDL_LoadBMP("./img/start/1.bmp");		//load the cutscenes
	start[1] = SDL_LoadBMP("./img/start/2.bmp"); 	
	start[2] = SDL_LoadBMP("./img/start/3.bmp"); 	
	start[3] = SDL_LoadBMP("./img/start/4.bmp"); 	
	start[4] = SDL_LoadBMP("./img/start/5.bmp"); 	
	start[5] = SDL_LoadBMP("./img/start/6.bmp"); 	
	start[6] = SDL_LoadBMP("./img/start/7.bmp"); 	
	start[7] = SDL_LoadBMP("./img/start/8.bmp"); 	
	start[8] = SDL_LoadBMP("./img/start/9.bmp"); 	
	start[9] = SDL_LoadBMP("./img/start/10.bmp");
	start[10] = SDL_LoadBMP("./img/start/11.bmp");
	start[11] = SDL_LoadBMP("./img/start/12.bmp");
	start[12] = SDL_LoadBMP("./img/start/13.bmp");
	start[13] = SDL_LoadBMP("./img/start/14.bmp");
	start[14] = SDL_LoadBMP("./img/start/15.bmp");
	start[15] = SDL_LoadBMP("./img/start/16.bmp");
	start[16] = SDL_LoadBMP("./img/start/17.bmp");
	start[17] = SDL_LoadBMP("./img/start/18.bmp");
	start[18] = SDL_LoadBMP("./img/start/19.bmp");
	start[19] = SDL_LoadBMP("./img/start/20.bmp");
	start[20] = SDL_LoadBMP("./img/start/21.bmp");
	bg = SDL_LoadBMP("./img/bg.bmp");					//background
	mapbg[0] = SDL_LoadBMP("./img/mapbg/1.bmp");		//map background
	mapbg[1] = SDL_LoadBMP("./img/mapbg/2.bmp");
	mapbg[2] = SDL_LoadBMP("./img/mapbg/3.bmp");
	mapbg[3] = SDL_LoadBMP("./img/mapbg/4.bmp");
	mapbg[4] = SDL_LoadBMP("./img/mapbg/5.bmp");
	mapbg[5] = SDL_LoadBMP("./img/mapbg/6.bmp");
	mapbg[6] = SDL_LoadBMP("./img/mapbg/7.bmp");
	mapbg[7] = SDL_LoadBMP("./img/mapbg/8.bmp");
	mapbg[8] = SDL_LoadBMP("./img/mapbg/9.bmp");
	mapbg[9] = SDL_LoadBMP("./img/mapbg/10.bmp");
	mapbg[10] = SDL_LoadBMP("./img/mapbg/11.bmp");
	mapbg[11] = SDL_LoadBMP("./img/mapbg/12.bmp");
	mapbg[12] = SDL_LoadBMP("./img/mapbg/13.bmp");
	mapbg[13] = SDL_LoadBMP("./img/mapbg/14.bmp");
	mapbg[14] = SDL_LoadBMP("./img/mapbg/15.bmp");
	mapbg[15] = SDL_LoadBMP("./img/mapbg/16.bmp");
	mapbg[16] = SDL_LoadBMP("./img/mapbg/17.bmp");
	mapbg[17] = SDL_LoadBMP("./img/mapbg/18.bmp");
	mapbg[18] = SDL_LoadBMP("./img/mapbg/19.bmp");
	mapbg[19] = SDL_LoadBMP("./img/mapbg/20.bmp");
	mapbg[20] = SDL_LoadBMP("./img/mapbg/21.bmp");
	map[0] = SDL_LoadBMP("./img/map/1.bmp");			//map
	map[1] = SDL_LoadBMP("./img/map/2.bmp");
	map[2] = SDL_LoadBMP("./img/map/3.bmp");
	map[3] = SDL_LoadBMP("./img/map/4.bmp");
	map[4] = SDL_LoadBMP("./img/map/5.bmp");
	map[5] = SDL_LoadBMP("./img/map/6.bmp");
	map[6] = SDL_LoadBMP("./img/map/7.bmp");
	map[7] = SDL_LoadBMP("./img/map/8.bmp");
	map[8] = SDL_LoadBMP("./img/map/9.bmp");
	map[9] = SDL_LoadBMP("./img/map/10.bmp");
	map[10] = SDL_LoadBMP("./img/map/11.bmp");
	map[11] = SDL_LoadBMP("./img/map/12.bmp");
	map[12] = SDL_LoadBMP("./img/map/13.bmp");
	map[13] = SDL_LoadBMP("./img/map/14.bmp");
	map[14] = SDL_LoadBMP("./img/map/15.bmp");
	map[15] = SDL_LoadBMP("./img/map/16.bmp");
	map[16] = SDL_LoadBMP("./img/map/17.bmp");
	map[17] = SDL_LoadBMP("./img/map/18.bmp");
	map[18] = SDL_LoadBMP("./img/map/19.bmp");
	map[19] = SDL_LoadBMP("./img/map/20.bmp");
	map[20] = SDL_LoadBMP("./img/map/21.bmp");
	run[0] = SDL_LoadBMP("./img/run/1.bmp");			//unicorn animation of running
	run[1] = SDL_LoadBMP("./img/run/2.bmp");
	run[2] = SDL_LoadBMP("./img/run/3.bmp");
	run[3] = SDL_LoadBMP("./img/run/4.bmp");
	run[4] = SDL_LoadBMP("./img/run/5.bmp");
	run[5] = SDL_LoadBMP("./img/run/6.bmp");
	run[6] = SDL_LoadBMP("./img/run/7.bmp");
	run[7] = SDL_LoadBMP("./img/run/8.bmp");
	run[8] = SDL_LoadBMP("./img/run/9.bmp");
	explosion[0] = SDL_LoadBMP("./img/explosion/1.bmp");			//explosion
	explosion[1] = SDL_LoadBMP("./img/explosion/2.bmp");
	explosion[2] = SDL_LoadBMP("./img/explosion/3.bmp");
	explosion[3] = SDL_LoadBMP("./img/explosion/4.bmp");
	explosion[4] = SDL_LoadBMP("./img/explosion/5.bmp");
	explosion[5] = SDL_LoadBMP("./img/explosion/6.bmp");
	explosion[6] = SDL_LoadBMP("./img/explosion/7.bmp");
	explosion[7] = SDL_LoadBMP("./img/explosion/8.bmp");
	explosion[8] = SDL_LoadBMP("./img/explosion/9.bmp");
	explosion[9] = SDL_LoadBMP("./img/explosion/10.bmp");
	explosion[10] = SDL_LoadBMP("./img/explosion/11.bmp");
	explosion[11] = SDL_LoadBMP("./img/explosion/12.bmp");
	explosion[12] = SDL_LoadBMP("./img/explosion/13.bmp");
	explosion[13] = SDL_LoadBMP("./img/explosion/14.bmp");
	explosion[14] = SDL_LoadBMP("./img/explosion/15.bmp");
	explosion[15] = SDL_LoadBMP("./img/explosion/16.bmp");
	explosion[16] = SDL_LoadBMP("./img/explosion/17.bmp");
	life[0] = SDL_LoadBMP("./img/life/1.bmp");				//life
	life[1] = SDL_LoadBMP("./img/life/2.bmp");
	life[2] = SDL_LoadBMP("./img/life/3.bmp");
	dolphins[0] = SDL_LoadBMP("./img/dolphins/1.bmp");		//dolphins
	dolphins[1] = SDL_LoadBMP("./img/dolphins/2.bmp");
	dolphins[2] = SDL_LoadBMP("./img/dolphins/3.bmp");
	dolphins[3] = SDL_LoadBMP("./img/dolphins/4.bmp");
	dolphins[4] = SDL_LoadBMP("./img/dolphins/5.bmp");
	dolphins[5] = SDL_LoadBMP("./img/dolphins/6.bmp");
	dolphins[6] = SDL_LoadBMP("./img/dolphins/7.bmp");
	dolphins[7] = SDL_LoadBMP("./img/dolphins/8.bmp");
	dolphins[8] = SDL_LoadBMP("./img/dolphins/9.bmp");
	dolphins[9] = SDL_LoadBMP("./img/dolphins/10.bmp");
	dolphins[10] = SDL_LoadBMP("./img/dolphins/11.bmp");
	dolphins[11] = SDL_LoadBMP("./img/dolphins/12.bmp");
	dolphins[12] = SDL_LoadBMP("./img/dolphins/13.bmp");
	dolphins[13] = SDL_LoadBMP("./img/dolphins/14.bmp");
	dolphins[14] = SDL_LoadBMP("./img/dolphins/15.bmp");
	dolphins[15] = SDL_LoadBMP("./img/dolphins/16.bmp");
	dolphins[16] = SDL_LoadBMP("./img/dolphins/17.bmp");
	dolphins[17] = SDL_LoadBMP("./img/dolphins/18.bmp");
	dolphins[18] = SDL_LoadBMP("./img/dolphins/19.bmp");
	dolphins[19] = SDL_LoadBMP("./img/dolphins/20.bmp");
	dolphins[20] = SDL_LoadBMP("./img/dolphins/21.bmp");
	dolphins[21] = SDL_LoadBMP("./img/dolphins/22.bmp");
	dolphins[22] = SDL_LoadBMP("./img/dolphins/23.bmp");
	result = SDL_LoadBMP("./img/result.bmp");
	dash[0] = SDL_LoadBMP("./img/dash/1.bmp");			//dash
	dash[1] = SDL_LoadBMP("./img/dash/2.bmp");
	dash[2] = SDL_LoadBMP("./img/dash/3.bmp");
	dash[3] = SDL_LoadBMP("./img/dash/4.bmp");
	dash[4] = SDL_LoadBMP("./img/dash/5.bmp");
	dash[5] = SDL_LoadBMP("./img/dash/6.bmp");
	dash[6] = SDL_LoadBMP("./img/dash/7.bmp");
	dash[7] = SDL_LoadBMP("./img/dash/8.bmp");
	dash[8] = SDL_LoadBMP("./img/dash/9.bmp");
	dash[9] = SDL_LoadBMP("./img/dash/10.bmp");
	SetColorKey(mapbg[0], 197, 219, 252);				//make bg of bmp be transparent
	SetColorKey(mapbg[1], 197, 219, 252);
	SetColorKey(mapbg[2], 197, 219, 252);
	SetColorKey(mapbg[3], 197, 219, 252);
	SetColorKey(mapbg[4], 197, 219, 252);
	SetColorKey(mapbg[5], 197, 219, 252);
	SetColorKey(mapbg[6], 197, 219, 252);
	SetColorKey(mapbg[7], 197, 219, 252);
	SetColorKey(mapbg[8], 197, 219, 252);
	SetColorKey(mapbg[9], 197, 219, 252);
	SetColorKey(mapbg[10], 197, 219, 252);
	SetColorKey(mapbg[11], 197, 219, 252);
	SetColorKey(mapbg[12], 197, 219, 252);
	SetColorKey(mapbg[13], 197, 219, 252);
	SetColorKey(mapbg[14], 197, 219, 252);
	SetColorKey(mapbg[15], 197, 219, 252);
	SetColorKey(mapbg[16], 197, 219, 252);
	SetColorKey(mapbg[17], 197, 219, 252);
	SetColorKey(mapbg[18], 197, 219, 252);
	SetColorKey(mapbg[19], 197, 219, 252);
	SetColorKey(mapbg[20], 197, 219, 252);
	SetColorKey(map[0], 197, 219, 252);				
	SetColorKey(map[1], 197, 219, 252);
	SetColorKey(map[2], 197, 219, 252);
	SetColorKey(map[3], 197, 219, 252);
	SetColorKey(map[4], 197, 219, 252);
	SetColorKey(map[5], 197, 219, 252);
	SetColorKey(map[6], 197, 219, 252);
	SetColorKey(map[7], 197, 219, 252);
	SetColorKey(map[8], 197, 219, 252);
	SetColorKey(map[9], 197, 219, 252);
	SetColorKey(map[10], 197, 219, 252);
	SetColorKey(map[11], 197, 219, 252);
	SetColorKey(map[12], 197, 219, 252);
	SetColorKey(map[13], 197, 219, 252);
	SetColorKey(map[14], 197, 219, 252);
	SetColorKey(map[15], 197, 219, 252);
	SetColorKey(map[16], 197, 219, 252);
	SetColorKey(map[17], 197, 219, 252);
	SetColorKey(map[18], 197, 219, 252);
	SetColorKey(map[19], 197, 219, 252);
	SetColorKey(map[20], 197, 219, 252);
	SetColorKey(run[0], 197, 219, 252);
	SetColorKey(run[1], 197, 219, 252);
	SetColorKey(run[2], 197, 219, 252);
	SetColorKey(run[3], 197, 219, 252);
	SetColorKey(run[4], 197, 219, 252);
	SetColorKey(run[5], 197, 219, 252);
	SetColorKey(run[6], 197, 219, 252);
	SetColorKey(run[7], 197, 219, 252);
	SetColorKey(run[8], 197, 219, 252);
	SetColorKey(explosion[0], 197, 219, 252);				
	SetColorKey(explosion[1], 197, 219, 252);
	SetColorKey(explosion[2], 197, 219, 252);
	SetColorKey(explosion[3], 197, 219, 252);
	SetColorKey(explosion[4], 197, 219, 252);
	SetColorKey(explosion[5], 197, 219, 252);
	SetColorKey(explosion[6], 197, 219, 252);
	SetColorKey(explosion[7], 197, 219, 252);
	SetColorKey(explosion[8], 197, 219, 252);
	SetColorKey(explosion[9], 197, 219, 252);
	SetColorKey(explosion[10], 197, 219, 252);
	SetColorKey(explosion[11], 197, 219, 252);
	SetColorKey(explosion[12], 197, 219, 252);
	SetColorKey(explosion[13], 197, 219, 252);
	SetColorKey(explosion[14], 197, 219, 252);
	SetColorKey(explosion[15], 197, 219, 252);
	SetColorKey(explosion[16], 197, 219, 252);
	SetColorKey(dolphins[0], 197, 219, 252);
	SetColorKey(dolphins[1], 197, 219, 252);
	SetColorKey(dolphins[2], 197, 219, 252);
	SetColorKey(dolphins[3], 197, 219, 252);
	SetColorKey(dolphins[4], 197, 219, 252);
	SetColorKey(dolphins[5], 197, 219, 252);
	SetColorKey(dolphins[6], 197, 219, 252);
	SetColorKey(dolphins[7], 197, 219, 252);
	SetColorKey(dolphins[8], 197, 219, 252);
	SetColorKey(dolphins[9], 197, 219, 252);
	SetColorKey(dolphins[10], 197, 219, 252);
	SetColorKey(dolphins[11], 197, 219, 252);
	SetColorKey(dolphins[12], 197, 219, 252);
	SetColorKey(dolphins[13], 197, 219, 252);
	SetColorKey(dolphins[14], 197, 219, 252);
	SetColorKey(dolphins[15], 197, 219, 252);
	SetColorKey(dolphins[16], 197, 219, 252);
	SetColorKey(dolphins[17], 197, 219, 252);
	SetColorKey(dolphins[18], 197, 219, 252);
	SetColorKey(dolphins[19], 197, 219, 252);
	SetColorKey(dolphins[20], 197, 219, 252);
	SetColorKey(dolphins[21], 197, 219, 252);
	SetColorKey(dolphins[22], 197, 219, 252);
	SetColorKey(dash[0], 177, 221, 255);
	SetColorKey(dash[1], 177, 221, 255);
	SetColorKey(dash[2], 177, 221, 255);
	SetColorKey(dash[3], 177, 221, 255);
	SetColorKey(dash[4], 177, 221, 255);
	SetColorKey(dash[5], 177, 221, 255);
	SetColorKey(dash[6], 177, 221, 255);
	SetColorKey(dash[7], 177, 221, 255);
	SetColorKey(dash[8], 177, 221, 255);
	SetColorKey(dash[9], 177, 221, 255);

	char text[128];
	int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	quit = 0;
	while (quit == 0)
	{
		SDL_BlitSurface(menu, NULL, screen, NULL);
		SDL_UpdateWindowSurface(window);

		// info text
		DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, czerwony, niebieski);
		sprintf(text, "Press N to start game, ESC to leave");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 18, 8, 8, text, charset);
		ApplySurface(screen, 0, renderer, scrtex);

		// handling of events (if there were any)
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					SDL_FreeSurface(charset);
					SDL_FreeSurface(screen);
					SDL_DestroyTexture(scrtex);
					SDL_DestroyRenderer(renderer);
					SDL_DestroyWindow(window);
					SDL_Quit();
					return 0;
				}
				else if (event.key.keysym.sym == SDLK_n)	//press n to start game
				{
					int Newgame = 0;	//cost all lives
					int newgame = 0;	//cost one life
					while (Newgame < Max_Life)
					{
						animation(21, start, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 50, screen, renderer, scrtex);										//load the Cutscenes
						//start new game (with arrow keys)
						DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);																	//load the first platform
						Map.MapRect.x = 0;
						Map.MapRect.y = 300;
						Map.MapRect.w = mapbg[0]->w;
						Map.MapRect.h = mapbg[0]->h;
						Map.MapRect2.x = Map.MapRect.w + 1;
						Map.MapRect2.y = 300;
						Map.MapRect2.w = mapbg[0]->w;
						Map.MapRect2.h = mapbg[0]->h;
						Map.Obstacle.x = Map.MapRect.x + 300;
						Map.Obstacle.y = Map.MapRect.y - mapbg[20]->h - run[0]->h - 30;
						Map.Obstacle.w = mapbg[20]->w;
						Map.Obstacle.h = mapbg[20]->h;
						Map.Obstacle2.x = Map.MapRect2.x + 300;
						Map.Obstacle2.y = Map.MapRect2.y - mapbg[20]->h - run[0]->h - 30;
						Map.Obstacle2.w = mapbg[20]->w;
						Map.Obstacle2.h = mapbg[20]->h;
						DrawSurface(screen, mapbg[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);									//load the first/second platform
						DrawSurface(screen, mapbg[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
						DrawSurface(screen, map[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
						DrawSurface(screen, map[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
						DrawSurface(screen, mapbg[20], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
						DrawSurface(screen, mapbg[20], Map.Obstacle2.x, Map.Obstacle2.y, Map.Obstacle2.w, Map.Obstacle2.h);
						DrawSurface(screen, map[20], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
						DrawSurface(screen, map[20], Map.Obstacle2.x, Map.Obstacle2.y, Map.Obstacle2.w, Map.Obstacle2.h);
						Unicorn.UnicornRect.x = 0;
						Unicorn.UnicornRect.y = 220;
						Unicorn.UnicornRect.w = run[8]->w;
						Unicorn.UnicornRect.h = run[8]->h;
						DrawSurface(screen, run[8], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);	//load the unicorn drop down .bmp
						ApplySurface(screen, 0, renderer, scrtex);

						//if the unicorn is in the sky, it will drop down to the platform
						Unicorn.Unicorn_Down = true;
						Unicorn.Unicorn_Up = false;
						Unicorn.Collision = false;
						while (Unicorn.Unicorn_Down == true)
						{
							Unicorn.UnicornRect.y++;
							DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);																	//load the first platform
							DrawSurface(screen, mapbg[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);									//load the first platform
							DrawSurface(screen, run[8], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);	//load the unicorn drop down .bmp
							ApplySurface(screen, 10, renderer, scrtex);
							CheckUnicornDownUp(mapbg[0], mapbg[0], window, renderer, Unicorn.UnicornRect, Map.MapRect, Map.MapRect2, &Unicorn.Unicorn_Down, &Unicorn.Unicorn_Up);
						}

						DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);																		//load the bg
						DrawSurface(screen, life[Newgame], 0, 0, life[Newgame]->w, life[Newgame]->h);													//load the life
						DrawSurface(screen, mapbg[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);										//load the first platform
						DrawSurface(screen, map[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);										//load the first platform
						DrawSurface(screen, mapbg[20], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
						DrawSurface(screen, map[20], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
						DrawSurface(screen, run[0], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);		//load the unicorn drop down .bmp
						ApplySurface(screen, 0, renderer, scrtex);

						Unicorn.Frame_Run = 0;
						Unicorn.jumpindex = -1;
						const Uint8* state = SDL_GetKeyboardState(NULL);	//used to press two or more keys at the same time
						t0 = SDL_GetTicks();
						while (newgame == Newgame)
						{
							//time
							t1 = SDL_GetTicks();
							t = (t1 - t0) * 0.001;
							DrawRectangle(screen, SCREEN_WIDTH - 11 * w_TimeFont - 4, 1, 11 * w_TimeFont + 4, 22, czerwony, czarny);
							sprintf(text, "TIME:%.1lf", t);
							DrawString(screen, SCREEN_WIDTH - 11 * w_TimeFont - 2, 2, w_TimeFont, h_TimeFont, text, charset);
							ApplySurface(screen, 0, renderer, scrtex);

							while (SDL_PollEvent(&event))
							{
								switch (event.type)
								{
								case SDL_KEYDOWN:
									if (event.key.keysym.sym == SDLK_ESCAPE)
									{
										SDL_FreeSurface(charset);
										SDL_FreeSurface(screen);
										SDL_DestroyTexture(scrtex);
										SDL_DestroyRenderer(renderer);
										SDL_DestroyWindow(window);
										SDL_Quit();
										return 0;
									}
									else if (event.key.keysym.sym == SDLK_UP)
										Unicorn.jumpindex = 10;
									else if (event.key.keysym.sym == SDLK_RIGHT)	//press n to start game
									{
										if (newgame != Newgame)
											break;
										//check collision
										CheckCollison(mapbg[0], mapbg[0], mapbg[20], mapbg[20], window, renderer, Unicorn.UnicornRect, Map.MapRect, Map.MapRect2, Map.Obstacle, Map.Obstacle2, &Unicorn.Collision);
										if (Unicorn.Collision == true)
										{
											newgame++;
											animation(17, explosion, Unicorn.UnicornRect.x + Unicorn.UnicornRect.w / 3, Unicorn.UnicornRect.y - 30, explosion[0]->w, explosion[0]->h, 15, screen, renderer, scrtex);
											break;
										}
										//check the unicorn whether on the plat form
										CheckUnicornDownUp(mapbg[0], mapbg[0], window, renderer, Unicorn.UnicornRect, Map.MapRect, Map.MapRect2, &Unicorn.Unicorn_Down, &Unicorn.Unicorn_Up);
										if (Unicorn.Unicorn_Down == true)
											Unicorn.UnicornRect.y += 2;
										if (Unicorn.Unicorn_Up == true)
											Unicorn.UnicornRect.y -= 2;

										//loop the map
										if (Map.MapRect.x + Map.MapRect.w <= 0)
										{
											Map.MapRect.x = Map.MapRect2.w + 1;
											Map.Obstacle.x = Map.MapRect.x + 300;
										}
										if (Map.MapRect2.x + Map.MapRect2.w <= 0)
										{
											Map.MapRect2.x = Map.MapRect.w + 1;
											Map.Obstacle2.x = Map.MapRect2.x + 300;
										}
										Map.MapRect.x -= 20;
										Map.MapRect2.x -= 20;
										Map.Obstacle.x -= 20;
										Map.Obstacle2.x -= 20;

										//jump
										if (state[SDL_SCANCODE_RIGHT] && state[SDL_SCANCODE_UP])
											Unicorn.jumpindex = 10;
										if (Unicorn.jumpindex > -1)
										{
											Unicorn.UnicornRect.y += Unicorn.jump[Unicorn.jumpindex];
											Unicorn.jumpindex--;
										}

										DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
										//time
										t1 = SDL_GetTicks();
										t = (t1 - t0) * 0.001;
										DrawRectangle(screen, SCREEN_WIDTH - 11 * w_TimeFont - 4, 1, 11 * w_TimeFont + 4, 22, czerwony, czarny);
										sprintf(text, "TIME:%.1lf", t);
										DrawString(screen, SCREEN_WIDTH - 11 * w_TimeFont - 2, 2, w_TimeFont, h_TimeFont, text, charset);
										DrawSurface(screen, mapbg[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
										DrawSurface(screen, mapbg[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
										DrawSurface(screen, map[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
										DrawSurface(screen, map[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
										DrawSurface(screen, mapbg[20], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
										DrawSurface(screen, mapbg[20], Map.Obstacle2.x, Map.Obstacle2.y, Map.Obstacle2.w, Map.Obstacle2.h);
										DrawSurface(screen, map[20], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
										DrawSurface(screen, map[20], Map.Obstacle2.x, Map.Obstacle2.y, Map.Obstacle2.w, Map.Obstacle2.h);
										DrawSurface(screen, run[Unicorn.Frame_Run], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);
										DrawSurface(screen, life[Newgame], 0, 0, life[Newgame]->w, life[Newgame]->h);
										Unicorn.Frame_Run++;
										if (Unicorn.Frame_Run == 8)
											Unicorn.Frame_Run = 0;
										ApplySurface(screen, 50, renderer, scrtex);
									}

									else if (event.key.keysym.sym == SDLK_n)
										newgame = 0;

									else if(event.key.keysym.sym == SDLK_d)
									{
										newgame = Max_Life+1;
										quit = 1;
									}
									break;
								case SDL_KEYUP:
									if (Unicorn.jumpindex > -1)
									{
										Unicorn.UnicornRect.y += Unicorn.jump[Unicorn.jumpindex];
										Unicorn.jumpindex--;
									}
									Unicorn.Frame_Run = 0;
									DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);						
									DrawSurface(screen, mapbg[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
									DrawSurface(screen, mapbg[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
									DrawSurface(screen, map[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
									DrawSurface(screen, map[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
									DrawSurface(screen, mapbg[20], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
									DrawSurface(screen, mapbg[20], Map.Obstacle2.x, Map.Obstacle2.y, Map.Obstacle2.w, Map.Obstacle2.h);
									DrawSurface(screen, map[20], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
									DrawSurface(screen, map[20], Map.Obstacle2.x, Map.Obstacle2.y, Map.Obstacle2.w, Map.Obstacle2.h);
									DrawSurface(screen, run[0], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);
									DrawSurface(screen, life[Newgame], 0, 0, life[Newgame]->w, life[Newgame]->h);
									ApplySurface(screen, 0, renderer, scrtex);
									break;
								case SDL_QUIT:
									SDL_FreeSurface(charset);
									SDL_FreeSurface(screen);
									SDL_DestroyTexture(scrtex);
									SDL_DestroyRenderer(renderer);
									SDL_DestroyWindow(window);
									SDL_Quit();
									return 0;
								};
							};
						}
						Newgame = newgame;
					}
				}
				break;
			case SDL_QUIT:
				SDL_FreeSurface(charset);
				SDL_FreeSurface(screen);
				SDL_DestroyTexture(scrtex);
				SDL_DestroyRenderer(renderer);
				SDL_DestroyWindow(window);
	
				SDL_Quit();
				return 0;
			};
		};
	};

	// New mode game -- z_jump and x_dash
	quit = 0;
	while (quit == 0)
	{
		int Newgame = 0;	//cost all lives
		int newgame = 0;	//cost one life
		gap = 370;
		Unicorn.UnicornSpeed = 20;
		
		while (Newgame < Max_Life)
		{
			int level = 1;
			Unicorn.dash = 10;
			Unicorn.jump_time = 0;
			Unicorn.jump_max = 0;
			Unicorn.jumpindex = -1;
			Map.Map1Index = 0;
			Map.Map2Index = 0;

			animation(21, start, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 50, screen, renderer, scrtex);										//load the Cutscenes
			DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);																	//load the first platform
			
			Map.MapRect.x = 0;
			Map.MapRect.y = 300;
			Map.MapRect.w = mapbg[0]->w;
			Map.MapRect.h = mapbg[0]->h;
			Map.MapRect2.x = Map.MapRect.w + 1 +gap;
			Map.MapRect2.y = 350;
			Map.MapRect2.w = mapbg[0]->w;
			Map.MapRect2.h = mapbg[0]->h;
			//for test collision it will not be used
			Map.Obstacle.x = SCREEN_WIDTH + 1;
			Map.Obstacle2.x = SCREEN_WIDTH + 1;

			DrawSurface(screen, mapbg[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);									//load the first/second platform
			DrawSurface(screen, mapbg[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
			DrawSurface(screen, map[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
			DrawSurface(screen, map[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
			
			Unicorn.UnicornRect.x = 0;
			Unicorn.UnicornRect.y = 220;
			Unicorn.UnicornRect.w = run[8]->w;
			Unicorn.UnicornRect.h = run[8]->h;
			DrawSurface(screen, run[8], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);	//load the unicorn drop down .bmp
			ApplySurface(screen, 0, renderer, scrtex);

			//if the unicorn is in the sky, it will drop down to the platform
			Unicorn.Unicorn_Down = true;
			Unicorn.Unicorn_Up = false;
			Unicorn.Collision = false;
			while (Unicorn.Unicorn_Down == true)
			{
				Unicorn.UnicornRect.y++;
				DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);																	//load the first platform
				DrawSurface(screen, mapbg[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);									//load the first platform
				DrawSurface(screen, run[8], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);	//load the unicorn drop down .bmp
				ApplySurface(screen, 10, renderer, scrtex);
				CheckUnicornDownUp(mapbg[0], mapbg[0], window, renderer, Unicorn.UnicornRect, Map.MapRect, Map.MapRect2, &Unicorn.Unicorn_Down, &Unicorn.Unicorn_Up);
			}

			DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);																		//load the bg
			DrawSurface(screen, life[Newgame], 0, 0, life[Newgame]->w, life[Newgame]->h);													//load the life
			DrawSurface(screen, mapbg[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);										//load the first platform
			DrawSurface(screen, map[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);										//load the first platform
			DrawSurface(screen, run[0], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);		//load the unicorn drop down .bmp
			ApplySurface(screen, 0, renderer, scrtex);

			Unicorn.Frame_Run = 0;
			Unicorn.jumpindex = -1;
			const Uint8* state = SDL_GetKeyboardState(NULL);	//used to press two or more keys at the same time
			t0 = SDL_GetTicks();
			srand((int)time(0));								//show map randomly

			while (newgame == Newgame)
			{
				//check collision and drop out of screen
				CheckCollison(mapbg[Map.Map1Index], mapbg[Map.Map2Index], mapbg[20], mapbg[20], window, renderer, Unicorn.UnicornRect, Map.MapRect, Map.MapRect2, Map.Obstacle, Map.Obstacle2, &Unicorn.Collision);
				if (Unicorn.Collision == true)
				{
					newgame++;
					animation(17, explosion, Unicorn.UnicornRect.x + Unicorn.UnicornRect.w / 3, Unicorn.UnicornRect.y - 30, explosion[0]->w, explosion[0]->h, 15, screen, renderer, scrtex);
					break;
				}
				if(Unicorn.UnicornRect.y >= SCREEN_HEIGHT)
				{
					newgame++;
					for (int i = 0; i < 23; i++)
					{
						DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
						DrawSurface(screen, life[Newgame], 0, 0, life[Newgame]->w, life[Newgame]->h);
						DrawSurface(screen, mapbg[Map.Map1Index], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
						DrawSurface(screen, map[Map.Map1Index], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
						DrawSurface(screen, mapbg[Map.Map2Index], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
						DrawSurface(screen, map[Map.Map2Index], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
						DrawSurface(screen, dolphins[i], 0, SCREEN_HEIGHT-dolphins[i]->h+5, dolphins[i]->w, dolphins[i]->h);
						ApplySurface(screen, 20, renderer, scrtex);
					}	
					break;
				}

				//if dash
				if (Unicorn.dash != 10)
				{
					//loop the map
					if (Map.MapRect.x + Map.MapRect.w <= 0)
					{
						Map.MapRect.x = Map.MapRect2.x + Map.MapRect2.w + gap;
						Map.Map1Index = rand() % (Num_Map);
						Map.MapRect.y = 300 - mapbg[Map.Map1Index]->h * 2 / 3;			//the h/3 always on the 300 height
						Map.MapRect.w = mapbg[Map.Map1Index]->w;
						Map.MapRect.h = mapbg[Map.Map1Index]->h;
					}
					if (Map.MapRect2.x + Map.MapRect2.w <= 0)
					{
						Map.MapRect2.x = Map.MapRect.x + Map.MapRect.w + gap;
						Map.Map2Index = rand() % (Num_Map);
						Map.MapRect2.y = 300 - mapbg[Map.Map2Index]->h * 2 / 3;			//the h/3 always on the 300 height
						Map.MapRect2.w = mapbg[Map.Map2Index]->w;
						Map.MapRect2.h = mapbg[Map.Map2Index]->h;
					}

					Map.MapRect.x -= Unicorn.UnicornSpeed * 2;
					Map.MapRect2.x -= Unicorn.UnicornSpeed * 2;

					DrawSurface(screen, mapbg[Map.Map1Index], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
					DrawSurface(screen, mapbg[Map.Map2Index], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
					DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
					//time
					t1 = SDL_GetTicks();
					t = (t1 - t0) * 0.001;
					DrawRectangle(screen, SCREEN_WIDTH - 11 * w_TimeFont - 4, 1, 11 * w_TimeFont + 4, 22, czerwony, czarny);
					sprintf(text, "TIME:%.1lf", t);
					DrawString(screen, SCREEN_WIDTH - 11 * w_TimeFont - 2, 2, w_TimeFont, h_TimeFont, text, charset);
					DrawSurface(screen, map[Map.Map1Index], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
					DrawSurface(screen, map[Map.Map2Index], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
					DrawSurface(screen, dash[Unicorn.dash], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y-(dash[Unicorn.dash]->h-Unicorn.UnicornRect.h)/2, dash[Unicorn.dash]->w, dash[Unicorn.dash]->h);
					DrawSurface(screen, life[Newgame], 0, 0, life[Newgame]->w, life[Newgame]->h);
					Unicorn.Frame_Run = 8;
					Unicorn.dash++;
					ApplySurface(screen, 50, renderer, scrtex);
					continue;
				}

				//if jump
				if (Unicorn.jump_time != 0)
				{
					if (Unicorn.jump_time > 2)
					{
						Unicorn.UnicornRect.y -= 50;
						Unicorn.jump_time--;
					}
					else
					{
						if (Unicorn.jumpindex >= 5)
						{
							Unicorn.UnicornRect.y += Unicorn.jump[Unicorn.jumpindex];
							Unicorn.jumpindex--;
						}
						else if (Unicorn.UnicornRect.x > Map.MapRect.x + 15)
						{
							Unicorn.UnicornRect.y += 15;
						}
						else 
						{
							Unicorn.UnicornRect.y += 2;
						}
						Unicorn.jump_time = 1;
					}
				}

				//check the unicorn whether on the plat form
				CheckUnicornDownUp(mapbg[Map.Map1Index], mapbg[Map.Map2Index], window, renderer, Unicorn.UnicornRect, Map.MapRect, Map.MapRect2, &Unicorn.Unicorn_Down, &Unicorn.Unicorn_Up);
				if (Unicorn.Unicorn_Down == true)
					Unicorn.UnicornRect.y += 2;
				if (Unicorn.Unicorn_Down == false)
				{
					Unicorn.jump_max = 0;
					Unicorn.jump_time = 0;
					Unicorn.jumpindex = -1;
				}
				if (Unicorn.Unicorn_Up == true)
					Unicorn.UnicornRect.y -= 2;
				//if in the gap or under the map
				if ((Map.MapRect.x - Unicorn.UnicornRect.x < gap && Map.MapRect.x - Unicorn.UnicornRect.x>Unicorn.UnicornRect.w) || (Map.MapRect2.x - Unicorn.UnicornRect.x < gap && Map.MapRect2.x - Unicorn.UnicornRect.x>Unicorn.UnicornRect.w)||Unicorn.UnicornRect.y+Unicorn.UnicornRect.h>Map.MapRect.y+Map.MapRect.h|| Unicorn.UnicornRect.y + Unicorn.UnicornRect.h > Map.MapRect2.y + Map.MapRect2.h)
				{
					Unicorn.UnicornRect.y += 5;
					Unicorn.Frame_Run = 8;
				}

				//loop the map
				if (Map.MapRect.x + Map.MapRect.w <= 0)
				{
					Map.MapRect.x = Map.MapRect2.x + Map.MapRect2.w + gap;
					Map.Map1Index = rand() % (Num_Map);
					Map.MapRect.y = 300 - mapbg[Map.Map1Index]->h * 2 / 3;			//the h/3 always on the 300 height
					Map.MapRect.w = mapbg[Map.Map1Index]->w;
					Map.MapRect.h = mapbg[Map.Map1Index]->h;
				}
				if (Map.MapRect2.x + Map.MapRect2.w <= 0)
				{
					Map.MapRect2.x = Map.MapRect.x + Map.MapRect.w + gap;
					Map.Map2Index = rand() % (Num_Map);
					Map.MapRect2.y = 300 - mapbg[Map.Map2Index]->h * 2 / 3;			//the h/3 always on the 300 height
					Map.MapRect2.w = mapbg[Map.Map2Index]->w;
					Map.MapRect2.h = mapbg[Map.Map2Index]->h;
				}

				Map.MapRect.x -= Unicorn.UnicornSpeed;
				Map.MapRect2.x -= Unicorn.UnicornSpeed;

				DrawSurface(screen, mapbg[Map.Map1Index], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
				DrawSurface(screen, mapbg[Map.Map2Index], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
				DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
				//time
				t1 = SDL_GetTicks();
				t = (t1 - t0) * 0.001;
				//level up
				if (t * Unicorn.UnicornSpeed > 600 * level)
				{
					Unicorn.UnicornSpeed += 5;
					level++;
				}

				DrawRectangle(screen, SCREEN_WIDTH - 11 * w_TimeFont - 4, 1, 11 * w_TimeFont + 4, 22, czerwony, czarny);
				sprintf(text, "TIME:%.1lf", t);
				DrawString(screen, SCREEN_WIDTH - 11 * w_TimeFont - 2, 2, w_TimeFont, h_TimeFont, text, charset);
				DrawSurface(screen, map[Map.Map1Index], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
				DrawSurface(screen, map[Map.Map2Index], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
				DrawSurface(screen, run[Unicorn.Frame_Run], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);
				DrawSurface(screen, life[Newgame], 0, 0, life[Newgame]->w, life[Newgame]->h);
				Unicorn.Frame_Run++;
				if (Unicorn.Frame_Run >= 8)
					Unicorn.Frame_Run = 0;
				ApplySurface(screen, 50, renderer, scrtex);

				while (SDL_PollEvent(&event))
				{
					switch (event.type)
					{
					case SDL_KEYDOWN:
						if (event.key.keysym.sym == SDLK_ESCAPE)
						{
							SDL_FreeSurface(charset);
							SDL_FreeSurface(screen);
							SDL_DestroyTexture(scrtex);
							SDL_DestroyRenderer(renderer);
							SDL_DestroyWindow(window);
							SDL_Quit();
							return 0;
						}

						else if (event.key.keysym.sym == SDLK_z)	//press z to jump
						{
							if (Unicorn.jump_max < 2)
							{
								Unicorn.jump_time++;
								Unicorn.jumpindex = 10;
							}
						}

						else if (event.key.keysym.sym == SDLK_x)	//press z to dash
						{
							if (Unicorn.dash == 10)
							{
								Unicorn.dash = 0;
								Unicorn.jumpindex = 10;
							}
						}

						else if (event.key.keysym.sym == SDLK_n)	//press n to start game
							newgame = 0;
						break;

					case SDL_KEYUP:
						if (event.key.keysym.sym == SDLK_z)
						{
							if (Unicorn.jump_time <= 2)
								Unicorn.jump_max++;
						}
						if (Unicorn.jumpindex > -1)
						{
							Unicorn.UnicornRect.y += Unicorn.jump[Unicorn.jumpindex];
							Unicorn.jumpindex--;
						}
						Unicorn.Frame_Run = 0;
						DrawSurface(screen, mapbg[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
						DrawSurface(screen, mapbg[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
						DrawSurface(screen, bg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
						DrawSurface(screen, map[0], Map.MapRect.x, Map.MapRect.y, Map.MapRect.w, Map.MapRect.h);
						DrawSurface(screen, map[0], Map.MapRect2.x, Map.MapRect2.y, Map.MapRect2.w, Map.MapRect2.h);
						DrawSurface(screen, mapbg[9], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
						DrawSurface(screen, mapbg[9], Map.Obstacle2.x, Map.Obstacle2.y, Map.Obstacle2.w, Map.Obstacle2.h);
						DrawSurface(screen, map[9], Map.Obstacle.x, Map.Obstacle.y, Map.Obstacle.w, Map.Obstacle.h);
						DrawSurface(screen, map[9], Map.Obstacle2.x, Map.Obstacle2.y, Map.Obstacle2.w, Map.Obstacle2.h);
						DrawSurface(screen, run[0], Unicorn.UnicornRect.x, Unicorn.UnicornRect.y, Unicorn.UnicornRect.w, Unicorn.UnicornRect.h);
						DrawSurface(screen, life[Newgame], 0, 0, life[Newgame]->w, life[Newgame]->h);
						ApplySurface(screen, 0, renderer, scrtex);
						break;
					case SDL_QUIT:
						SDL_FreeSurface(charset);
						SDL_FreeSurface(screen);
						SDL_DestroyTexture(scrtex);
						SDL_DestroyRenderer(renderer);
						SDL_DestroyWindow(window);
						SDL_Quit();
						return 0;
					};
				};
			}
			Newgame = newgame;
		}
		while (Newgame == Max_Life)
		{
			DrawSurface(screen, result, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
			sprintf(text, "SCORE:");
			DrawString(screen, 20, 20, 20, 20, text, charset);
			ApplySurface(screen, 0, renderer, scrtex);

			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
					else if (event.key.keysym.sym == SDLK_n)
					{
						Newgame = 0;
					}
					break;
				case SDL_QUIT:
					quit = 1;
					break;
				}
			}
		}
	}

	// freeing all surfaces
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
};
