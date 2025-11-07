#include <stdio.h>      // Standard input/output (printf, fprintf)
#include <stdlib.h>     // Standard library (malloc, free, exit)
#include <unistd.h>     // Unix standard (usleep for timing)
#include <ncurses.h>    // Text-based UI library
#include <time.h>       // Time for random seed

#define REFRESH_TIME 100
#define PLAYER_SPEED 1
#define MAX_STARS_COUNT 10
#define MAX_STARS_SPEED 2

#define ESCAPE      'q'

#define BORDER		1		// Border width (in characters)
#define ROWS		30		// Play window height (rows)
#define COLS		100		// Play window width (columns)
#define OFFY		5		// Y offset from top of screen
#define LIFEWINY    3		// Height of life status window
#define OFFX		5		// X offset from left of screen

#define MAIN_COLOR	            1		// Main window color
#define STAT_COLOR	            2		// Status bar color
#define PLAY_COLOR	            3		// Play area color
#define SWALLOW_COLOR	        4       // Swallow (player) color
#define HUNTER_COLOR	        5       // Hunter Enemy color
#define ALBATROS_TAXI_COLOR     6       // Friendly albatros taxi color
#define EAGLE_BOSS_COLOR        7       // Eagle boss color
#define STAR_COLOR              8       // Stars color

typedef struct {
	WINDOW* window; // ncurses window pointer
	int x, y;       // Position on screen
	int rows, cols; // Size of window
	int color;		// Color scheme
} WIN;

typedef struct {
	WIN* playWin;               // Play Window where swallow is shown
	int x, y;		            // Position on screen
    int speed;                  // Swallows flying speed
    int wallet;                 // number of gained stars
	int color;		            // Color scheme
    int dx, dy;                 // directions to move the swallow
	int animationFrame;		// Color scheme
	int hp;		            // Health points of swallow
} Swallow;

typedef struct {
	WIN* playWin;               // Play Window where star is shown
	int x, y;		            // Position on screen
    int fallingSpeed;           // Stars falling speed
	int color;		            // Color scheme
	int animationFrame;		// Color scheme
} Star;

void CheckStarsCollision(Swallow* swallow, Star* star)
{
    if(star->y == swallow->y && (star->x == swallow->x || star->x == swallow->x-1))
    {
        star->y = -10;
        star->x = rand()%(COLS-1) +1;
        swallow->wallet +=1;
    }
}

void CheckSwallowsCollision(Swallow* swallow, Star** stars)
{
    for (int i = 0; i < MAX_STARS_COUNT; i++)
    {
        CheckStarsCollision(swallow, stars[i]);
    }
    
}

void PlayerMovement(Swallow* swallow, int input, Star** stars)
{
    switch (input)
    {
    case 's':
        swallow->dx = 0;
        swallow->dy = 1;
        break;
    
    case 'w':
        swallow->dx = 0;
        swallow->dy = -1;
        break;

    case 'd':
        swallow->dx = 1;
        swallow->dy = 0;
        break;

    case 'a':
        swallow->dx = -1;
        swallow->dy = 0;
        break;

    case 'o':
        if(swallow->speed > 1)
            swallow->speed -= 1;
        break;

    case 'p':
        if(swallow->speed < 5)
            swallow->speed = swallow->speed +1;
        break;
    
    default:
        break;
    }
    for (int i = 0; i < swallow->speed; i++)
    {
        swallow->y += swallow->dy;
        if(swallow->y > 0)
            swallow->y %= ROWS;
        else
            swallow->y = ROWS;

        swallow->x += swallow->dx;
        if(swallow->x > 0)
            swallow->x %= COLS;
        else
            swallow->x = COLS;
        
            CheckSwallowsCollision(swallow, stars);
    }
    

    
}

void DrawSwallow(WIN* playWin, Swallow* swallow)
{
	wattron(playWin->window, COLOR_PAIR(swallow->color));

    switch (swallow->animationFrame % 4)
    {
    case 0:
        if(swallow->y >=0 && swallow->x-1>=0)
        mvwprintw(playWin->window, swallow->y, swallow->x-1, "\\");
        if(swallow->y >=0 && swallow->x>=0)
            mvwprintw(playWin->window, swallow->y, swallow->x, "/");
        break;
    case 1:
        if(swallow->y >=0 && swallow->x-1>=0)
        mvwprintw(playWin->window, swallow->y, swallow->x-1, "-");
        if(swallow->y >=0 && swallow->x>=0)
            mvwprintw(playWin->window, swallow->y, swallow->x, "-");
        break;

    case 2:
        if(swallow->y >=0 && swallow->x-1>=0)
        mvwprintw(playWin->window, swallow->y, swallow->x-1, "/");
        if(swallow->y >=0 && swallow->x>=0)
            mvwprintw(playWin->window, swallow->y, swallow->x, "\\");
        break;

    case 3:
        if(swallow->y >=0 && swallow->x-1>=0)
        mvwprintw(playWin->window, swallow->y, swallow->x-1, "-");
        if(swallow->y >=0 && swallow->x>=0)
            mvwprintw(playWin->window, swallow->y, swallow->x, "-");
        break;
    
    default:
        break;
    }

    swallow->animationFrame +=1;
}

void DrawStars(WIN* playWin, Star* star, Swallow* swallow)
{
	wattron(playWin->window, COLOR_PAIR(star->color));
    mvwprintw(playWin->window, star->y, star->x, "*");
    star->animationFrame+=1;

    for (int i = 0; i < star->fallingSpeed; i++)
    {
        star->y += 1;

        CheckStarsCollision(swallow, star);

        if(star->y >= ROWS-1)
        {
            star->y %= ROWS-1;
            star->x = rand()%(COLS-1) +1;
        }

        
    }

    

}

void CleanWin(WIN* W, int border)
{
	int i, j;

	// Set window color
	wattron(W->window, COLOR_PAIR(W->color));

	// Draw border if requested
	if (border) box(W->window, 0, 0);

	// Fill window with spaces (clearing it)
	for (i = border; i < W->rows - border; i++)
		for (j = border; j < W->cols - border; j++)
			mvwprintw(W->window, i, j, " ");

	// Refresh to show changes
	wrefresh(W->window);
}

WIN* InitWin(WINDOW* parent, int rows, int cols, int y, int x, int color, int border, int delay)
{
	// Allocate memory for WIN structure
	WIN* W = (WIN*)malloc(sizeof(WIN));

	W->x = x;
	W->y = y;
	W->rows = rows;
	W->cols = cols;
	W->color = color;

	W->window = subwin(parent, rows, cols, y, x);

	// Clear the window
	CleanWin(W, border);

	// Set input mode: delay==0 means non-blocking (for real-time games)
	if (delay == 0) nodelay(W->window, TRUE);

	// Display the window
	wrefresh(W->window);

	return W;
}

Star** InitStars(WIN* playWin, int color)
{
    srand(time(NULL));
    Star** list = (Star**)malloc(MAX_STARS_COUNT * sizeof(Star*));
    for (int i = 0; i < MAX_STARS_COUNT; i++)
    {
        Star* tempStar = (Star*)malloc(sizeof(Star));
        tempStar->playWin = playWin;
        tempStar->x = rand()%COLS;
        tempStar->y = -rand()%ROWS;
        tempStar->fallingSpeed = rand()%MAX_STARS_SPEED+1;
        tempStar->color = color;
        tempStar->animationFrame = 0;
        list[i] = tempStar;
    }

    return list;
    
}

Swallow* InitSwallow(WIN* playWin, int x, int y, int dx, int dy, int speed, int color)
{
    //Allocating memory for Swallow
    Swallow* swallow = (Swallow*)malloc(sizeof(Swallow));

    //Seting swallow's properties
    swallow->playWin = playWin;
    swallow->x = x;
    swallow->y = y;
    swallow->dx = dx;
    swallow->dy = dy;
    swallow->wallet = 0;
    swallow->speed = speed;
    swallow->color = color;
    swallow->animationFrame = 0;
    swallow->hp = 3;

    return swallow;
}

WINDOW* Start()
{
    WINDOW* win;
    //checking inicialization of the window (ncurses)
    if(!(win = initscr()))
    {
        //wyświetlenie błędu w przypadku niepowodzenia
        fprintf(stderr, "Error while initializing ncurses!");
        exit(1);
    }

    start_color();

	init_pair(MAIN_COLOR, COLOR_WHITE, COLOR_BLACK);
	init_pair(STAT_COLOR, COLOR_YELLOW, COLOR_BLACK);
	init_pair(PLAY_COLOR, COLOR_CYAN, COLOR_BLACK);
	init_pair(SWALLOW_COLOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(HUNTER_COLOR, COLOR_RED, COLOR_BLACK);
    init_pair(ALBATROS_TAXI_COLOR, COLOR_BLUE, COLOR_BLACK);
    init_pair(EAGLE_BOSS_COLOR, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(STAR_COLOR, COLOR_YELLOW, COLOR_BLACK);

    //makes input invisible
    noecho();

    //makes cursor invisible
    curs_set(0);

    return win;

}

void UpdateStatus(WIN* statusWin, Swallow* swallow)
{
	// Set status bar color
	wattron(statusWin->window, COLOR_PAIR(statusWin->color));

	// Draw border around status bar
	box(statusWin->window, 0, 0);

    char info[50];
    snprintf(info, sizeof(info), "Number of gained stars: %d    Swallows speed: %d", swallow->wallet, swallow->speed);
    char controls[] = "W (up), S (down), D (right), A (left) | O (slow down), P (go faster)";

    // Display status info
	mvwprintw(statusWin->window, 1, (COLS-(sizeof(info)/sizeof(char)))/2, info);

	// Display controls
	mvwprintw(statusWin->window, 3, (COLS-(sizeof(controls)/sizeof(char)))/2, controls);

	// Update display
	wrefresh(statusWin->window);
}

void UpdateLifeInfo(WIN* lifeinfo, Swallow* swallow, float *timer)
{
    // Set status bar color
	wattron(lifeinfo->window, COLOR_PAIR(lifeinfo->color));

	// Draw border around status bar
	box(lifeinfo->window, 0, 0);

    char life[50];
    snprintf(life, sizeof(life), "Health Points: %d", swallow->hp);
    char time[50];
    snprintf(time, sizeof(time), "Time Left: %.1f", *timer);

    // Display status info
	mvwprintw(lifeinfo->window, 1, OFFX, life);

	// Display controls
	mvwprintw(lifeinfo->window, 1, COLS/4, time);

	// Update display
	wrefresh(lifeinfo->window);
}

void EndScreen(WIN* playWin)
{
    CleanWin(playWin, BORDER);

    // Set text color
	wattron(playWin->window, COLOR_RED);

	// Draw border around text
	box(playWin->window, 0, 0);

    char endText[] = "You have lost!!!";

    //Display end text
	mvwprintw(playWin->window, ROWS/2, (COLS-(sizeof(endText)/sizeof(char)))/2, endText);

	// Update display
	wrefresh(playWin->window);
    sleep(2);
}

void Update(WIN *playWin, WIN *statusWin,WIN* lifeWin, Swallow* swallow, Star** stars)
{
    int ch;
    float *timer = (float*)malloc(sizeof(float));
    *timer = 0.0;
    while(1)
    {
        ch = wgetch(playWin->window);

        CleanWin(playWin, BORDER);

        *timer += 0.1;

        if(ch == ESCAPE) break;
        else PlayerMovement(swallow, ch, stars);

        DrawSwallow(playWin, swallow);

        for (int i = 0; i < MAX_STARS_COUNT; i++)
        {
            DrawStars(playWin, stars[i], swallow);
        }
        

        wrefresh(playWin->window);

        UpdateStatus(statusWin, swallow);

        UpdateLifeInfo(lifeWin, swallow, timer);

        flushinp();

        usleep(REFRESH_TIME * 1000);
    }
}

int main()
{
    WINDOW *mainWin = Start();

    WIN *lifeWin = InitWin(
        mainWin, 
        LIFEWINY, 
        COLS/2, 
        LIFEWINY-1, 
        OFFY+COLS/4, 
        STAT_COLOR, 
        BORDER, 
        0);

    WIN *playWin = InitWin(
        mainWin, 
        ROWS, 
        COLS, 
        OFFY, 
        OFFX, 
        PLAY_COLOR, 
        BORDER, 
        0);

    WIN *statusWin = InitWin(
        mainWin, 
        OFFY, 
        COLS, 
        ROWS+OFFY, 
        OFFX, 
        STAT_COLOR, 
        BORDER, 
        0);
    

    Swallow* swallow = InitSwallow(
        playWin,
        (COLS/2),
        (ROWS/2),
        0,
        -1,
        PLAYER_SPEED,
        SWALLOW_COLOR
    );

    Star** stars = InitStars(playWin, STAR_COLOR);
    
    wrefresh(mainWin);

    Update(playWin, statusWin, lifeWin, swallow, stars);

    EndScreen(playWin);
    endwin();

    // Free allocated memory
    free(swallow);
    free(playWin);
    free(statusWin);

    return 0;
}