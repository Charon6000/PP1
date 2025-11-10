#include <stdio.h>      // Standard input/output (printf, fprintf)
#include <stdlib.h>     // Standard library (malloc, free, exit)
#include <unistd.h>     // Unix standard (usleep for timing)
#include <ncurses.h>    // Text-based UI library
#include <time.h>

#define START_TIME 60
#define REFRESH_TIME 100
#define PLAYER_SPEED 1
#define MAX_STARS_COUNT 10
#define MAX_HUNTERS_COUNT 3
#define MAX_STARS_SPEED 2
#define MAX_HUNTERS_SPEED 3
#define MAX_HUNTERS_BOUNDS 5

#define ESCAPE      'q'
#define REPEAT      'r'

#define BORDER		1		// Border width (in characters)
#define ROWS		50		// Play window height (rows)
#define COLS		120		// Play window width (columns)
#define OFFY		5		// Y offset from top of screen
#define LIFEWINY    3		// Height of life status window
#define OFFX		5		// X offset from left of screen

#define MAIN_COLOR	            1		// Main window color
#define STAT_COLOR	            2		// Status bar color
#define PLAY_COLOR	            3		// Play area color
#define SWALLOW_COLOR	        4       // Swallow (player) color
#define HUNTER_COLOR	        5       // Hunter Enemy color
#define ALBATROS_TAXI_COLOR     6       // Friendly albatros taxi color
#define AGAIN_COLOR             7       // Color of Play Again Screen
#define END_COLOR               8       // Color of End Screen
#define STAR_COLOR              9       // Stars color
#define STAR2_COLOR             10      // Stars color while shifting color


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
	int animationFrame;		    // Color scheme
	int hp;		                // Health points of swallow
} Swallow;

typedef struct {
	WIN* playWin;               // Play Window where hunter is shown
	int x, y;		            // Position on screen
    int size;                   // Size of Hunter (1-3)
	int color;		            // Color scheme
    int dx;                     // direction to move the swallow
    float a, b;                 // Values of func to move huntery=ax+b
    int speed;                  // Hunters speed
	int animationFrame;		    // Color scheme
	int boundsCounter;		    // Value of possible bounds
} Hunter;

typedef struct {
	WIN* playWin;               // Play Window where star is shown
	int x, y;		            // Position on screen
    int fallingSpeed;           // Stars falling speed
	int color;		            // Color scheme
	int color2;		            // Color scheme while shifting
	int animationFrame;		    // Color scheme
} Star;

void CheckStarsCollision(Swallow* swallow, Star* star)
{
    float dx = star->x-swallow->x;
    float dy = star->y-swallow->y;
    if((dx*dx+dy*dy) <= (swallow->hp * swallow->hp))
    {
        //respawn
        star->y = -10;
        star->x = rand()%(COLS-1) +1;
        swallow->wallet +=1;
    }
}

void CheckHuntersCollision(Swallow* swallow, Hunter* hunter)
{
    float dx = hunter->x-swallow->x;
    float dy = hunter->y-swallow->y;
    float minimum_distance = hunter->size + swallow->hp-2;
    float distance =dx*dx+dy*dy;
    if(distance <= (minimum_distance * minimum_distance))
    {
        //respawn
        hunter->y = 10;
        hunter->x = rand()%(COLS-1) +1;

        swallow->hp -=1;
    }
}

void CheckSwallowsCollision(Swallow* swallow, Star** stars, Hunter** hunters)
{
    for (int i = 0; i < MAX_STARS_COUNT; i++)
    {
        CheckStarsCollision(swallow, stars[i]);
    }
    
    for (int i = 0; i < MAX_HUNTERS_COUNT; i++)
    {
        CheckHuntersCollision(swallow, hunters[i]);
    }
    
}

void PlayerMovement(Swallow* swallow, int input, Star** stars, Hunter** hunters)
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
        
            CheckSwallowsCollision(swallow, stars, hunters);
    }
    

    
}

void DrawSwallow(WIN* playWin, Swallow* swallow)
{
	wattron(playWin->window, COLOR_PAIR(swallow->color));

    switch (swallow->animationFrame % 4)
    {
    case 0:
        for (int i = 0; i < swallow->hp; i++)
        {
            if(swallow->y-i >=0 && swallow->x-(i+1)>=0)
                mvwprintw(playWin->window, swallow->y-i, swallow->x-(i+1), "\\");
            if(swallow->y-i >=0 && swallow->x+i>=0)
                mvwprintw(playWin->window, swallow->y-i, swallow->x+i, "/");
        }
        break;
    case 1:
        for (int i = 0; i < swallow->hp; i++)
        {
            if(swallow->y >=0 && swallow->x-(i+1)-i>=0)
                mvwprintw(playWin->window, swallow->y, swallow->x-(i+1), "-");
            if(swallow->y >=0 && swallow->x+i>=0)
                mvwprintw(playWin->window, swallow->y, swallow->x+i, "-");
        }
        break;
    case 2:
        for (int i = 0; i < swallow->hp; i++)
        {
            if(swallow->y+i >=0 && swallow->x-(i+1)>=0)
                mvwprintw(playWin->window, swallow->y+i, swallow->x-(i+1), "/");
            if(swallow->y+i >=0 && swallow->x+i>=0)
                mvwprintw(playWin->window, swallow->y+i, swallow->x+i, "\\");
        }
        break;

    case 3:
        for (int i = 0; i < swallow->hp; i++)
        {
            if(swallow->y >=0 && swallow->x-(i+1)>=0)
                mvwprintw(playWin->window, swallow->y, swallow->x-(i+1), "-");
            if(swallow->y >=0 && swallow->x+i>=0)
                mvwprintw(playWin->window, swallow->y, swallow->x+i, "-");
        }
        break;
    
    default:
        break;
    }

    swallow->animationFrame +=1;
}

void MoveHunter(Hunter* hunter, Swallow* swallow)
{
    for (int i = 0; i < hunter->speed; i++)
    {
        hunter->x += hunter->dx;
        hunter->y = (hunter->a * hunter->x) + hunter->b;
        if (hunter->y < 0)
        {
            hunter->y = 0;
            hunter->a *= -1;
            hunter->b = hunter->y - (hunter->a * hunter->x);
        }
        else if (hunter->y > ROWS - 2)
        {
            hunter->y = ROWS - 2;
            hunter->a *= -1;
            hunter->b = hunter->y - (hunter->a * hunter->x);
        }


        if (hunter->x < 1)
        {
            hunter->x = 1;
            hunter->a *= -1;
            hunter->b = hunter->y - (hunter->a * hunter->x);
            hunter->dx *= -1;
        }
        else if (hunter->x > COLS - 2)
        {
            hunter->x = COLS - 2;
            hunter->a *= -1;
            hunter->b = hunter->y - (hunter->a * hunter->x);
            hunter->dx *= -1;
        }

        CheckHuntersCollision(swallow, hunter);
    }
    /*int tempdy = hunter->dy;
    int tempdx = hunter->dx;

    CheckHuntersCollision(swallow, hunter);

    //movement
    /*while(tempdy != 0 || tempdx != 0)
    {

        if(tempdy> 0)
        {
            hunter->y += 1;
            tempdy -=1;
        }
        else if(tempdy < 0)
        {
            hunter->y -= 1;
            tempdy +=1;
        }

        if(tempdx> 0)
        {
            hunter->x += 1;
            tempdx -=1;
        }
        else if(tempdx < 0)
        {
            hunter->x -= 1;
            tempdx +=1;
        }

        if(hunter->y <= 0)
        {
            hunter->y = 0;
            hunter->dy *= -1;
            tempdy *= -1;
            hunter->boundsCounter -= 1;
        }
        else if(hunter->y >= ROWS-1)
        {
            hunter->y = ROWS-1;
            hunter->dy *= -1;
            tempdy *= -1;
            hunter->boundsCounter -= 1;
        }

        if(hunter->x <= 0)
        {
            hunter->x = 0;
            hunter->dx *= -1;
            tempdx *= -1;
            hunter->boundsCounter -= 1;
        }
        else if(hunter->x >= COLS-1)
        {
            hunter->x = COLS-1;
            hunter->dx *= -1;
            tempdx *= -1;
            hunter->boundsCounter -= 1;
        }

        CheckHuntersCollision(swallow, hunter);
    }*/
}

void DrawHunter(WIN* playWin, Hunter* hunter, Swallow* swallow)
{
	wattron(playWin->window, COLOR_PAIR(hunter->color));

    char counter[2];
    snprintf(counter, sizeof(counter), "%d", hunter->boundsCounter);
    mvwprintw(playWin->window, hunter->y-1, hunter->x, counter);

    switch (hunter->animationFrame % 4)
    {
    case 0:
        for (int i = 0; i < hunter->size; i++)
        {
            if(hunter->y-i >=0 && hunter->x-(i+1)>=0)
                mvwprintw(playWin->window, hunter->y-i, hunter->x-(i+1), "\\");
            if(hunter->y-i >=0 && hunter->x+i>=0)
                mvwprintw(playWin->window, hunter->y-i, hunter->x+i, "/");
        }
        break;
    case 1:
        for (int i = 0; i < hunter->size; i++)
        {
            if(hunter->y >=0 && hunter->x-(i+1)-i>=0)
                mvwprintw(playWin->window, hunter->y, hunter->x-(i+1), "-");
            if(hunter->y >=0 && hunter->x+i>=0)
                mvwprintw(playWin->window, hunter->y, hunter->x+i, "-");
        }
        break;
    case 2:
        for (int i = 0; i < hunter->size; i++)
        {
            if(hunter->y+i >=0 && hunter->x-(i+1)>=0)
                mvwprintw(playWin->window, hunter->y+i, hunter->x-(i+1), "/");
            if(hunter->y+i >=0 && hunter->x+i>=0)
                mvwprintw(playWin->window, hunter->y+i, hunter->x+i, "\\");
        }
        break;

    case 3:
        for (int i = 0; i < hunter->size; i++)
        {
            if(hunter->y >=0 && hunter->x-(i+1)>=0)
                mvwprintw(playWin->window, hunter->y, hunter->x-(i+1), "-");
            if(hunter->y >=0 && hunter->x+i>=0)
                mvwprintw(playWin->window, hunter->y, hunter->x+i, "-");
        }
        break;
    
    default:
        break;
    }

    hunter->animationFrame +=1;
}

void DrawStars(WIN* playWin, Star* star, Swallow* swallow)
{
    if(star->animationFrame == 1)
	    wattron(playWin->window, COLOR_PAIR(star->color));
    else
        wattron(playWin->window, COLOR_PAIR(star->color2));

    star->animationFrame += 1;
    star->animationFrame %= 3;
    
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

Star** InitStars(WIN* playWin, int color, int color2)
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
        tempStar->color = color2;
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

Hunter** InitHunters(WIN* playWin, int color, Swallow* swallow)
{
    srand(time(NULL));
    Hunter** list = (Hunter**)malloc(MAX_HUNTERS_COUNT * sizeof(Hunter*));
    for (int i = 0; i < MAX_HUNTERS_COUNT; i++)
    {
        Hunter* tempHunter = (Hunter*)malloc(sizeof(Hunter));
        tempHunter->playWin = playWin;
        tempHunter->speed = rand() % MAX_HUNTERS_SPEED+1;

        

        //spawn
        tempHunter->x = rand()%COLS;
        tempHunter->y = rand()%ROWS;

        if (tempHunter->x < swallow->x)
            tempHunter->dx = 1;
        else
            tempHunter->dx = -1;

        if (tempHunter->x - swallow->x != 0)
            tempHunter->a = (tempHunter->y - swallow->y) / (tempHunter->x - swallow->x);
        else
            tempHunter->a = 0;

        tempHunter->b = swallow->y - (tempHunter->a * swallow->x);

        tempHunter->size = rand()%3+1;
        tempHunter->color = color;
        tempHunter->animationFrame = 0;
        tempHunter->boundsCounter = rand()% MAX_HUNTERS_BOUNDS+1;
        list[i] = tempHunter;
    }

    return list;
    
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
    init_pair(END_COLOR, COLOR_RED, COLOR_BLACK);
    init_pair(AGAIN_COLOR, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(STAR_COLOR, COLOR_YELLOW, COLOR_BLACK);
    init_pair(STAR2_COLOR, COLOR_WHITE, COLOR_BLACK);

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
	mvwprintw(lifeinfo->window, 1, COLS/3-OFFX, time);

	// Update display
	wrefresh(lifeinfo->window);
}

void EndScreen(WIN* playWin)
{
    CleanWin(playWin, BORDER);

    // Set text color
	wattron(playWin->window, COLOR_PAIR(END_COLOR));

	// Draw border around text
	box(playWin->window, 0, 0);

    char endText[] = "Bye bye, thanks for playing!";

    //Display end text
	mvwprintw(playWin->window, ROWS/2, (COLS-(sizeof(endText)/sizeof(char)))/2, endText);

	// Update display
	wrefresh(playWin->window);
    sleep(1);
}

void AgainScreen(WIN* playWin, char *resultText)
{
    CleanWin(playWin, BORDER);

    // Set text color
	wattron(playWin->window, COLOR_PAIR(AGAIN_COLOR));

	// Draw border around text
	box(playWin->window, 0, 0);

    char* question = "Click 'r' to play again and 'q' if you don't ;)";

    //Display result text
	mvwprintw(playWin->window, ROWS/2-2, (COLS-(sizeof(resultText)/sizeof(char)))/2.2f, resultText);
	mvwprintw(playWin->window, ROWS/2, (COLS-(sizeof(question)/sizeof(char)))/3.2f, question);

	// Update display
	wrefresh(playWin->window);
}

void PlayAgain(WIN* playWin, Swallow* swallow, bool* isPlaying)
{
    int ch;
    char* resultText;
    if(swallow->hp >0)
        resultText = "You won, congarts!";
    else
        resultText = "You have lost!";
    
    AgainScreen(playWin, resultText);

    while(1)
    {
        ch = wgetch(playWin->window);
        if(ch == ESCAPE)
        {
            *isPlaying = false;
            break;
        }
        else if(ch == REPEAT)
            break;
    }
}

void Update(WIN *playWin, WIN *statusWin,WIN* lifeWin, Swallow* swallow, Star** stars, Hunter** hunters)
{
    int ch;
    float *timer = (float*)malloc(sizeof(float));
    *timer = (float)START_TIME;
    while(1)
    {
        ch = wgetch(playWin->window);

        CleanWin(playWin, BORDER);

        *timer -= 0.1;
        UpdateLifeInfo(lifeWin, swallow, timer);

        if(ch == ESCAPE || *timer <= 0 || swallow->hp<= 0) break;
        else if(*timer <= 0) break;
        else PlayerMovement(swallow, ch, stars, hunters);

        DrawSwallow(playWin, swallow);

        for (int i = 0; i < MAX_STARS_COUNT; i++)
        {
            DrawStars(playWin, stars[i], swallow);
        }

        for (int i = 0; i < MAX_HUNTERS_COUNT; i++)
        {
            DrawHunter(playWin, hunters[i], swallow);
            MoveHunter(hunters[i], swallow);
        }

        

        wrefresh(playWin->window);

        UpdateStatus(statusWin, swallow);

        flushinp();

        usleep(REFRESH_TIME * 1000);
    }
}

int main()
{
    bool* isPlaying = (bool*)malloc(sizeof(bool));
    *isPlaying = true;
    while (*isPlaying)
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
            20,
            20,
            0,
            -1,
            PLAYER_SPEED,
            SWALLOW_COLOR
        );

        Star** stars = InitStars(playWin, STAR_COLOR, STAR2_COLOR);
        
        Hunter** hunters = InitHunters(playWin, HUNTER_COLOR, swallow);
        
        wrefresh(mainWin);

        Update(playWin, statusWin, lifeWin, swallow, stars, hunters);

        PlayAgain(playWin, swallow, isPlaying);

        if(!(*isPlaying))
            EndScreen(playWin);

        endwin();

        // Free allocated memory
        free(swallow);
        free(stars);
        free(hunters);
        free(playWin);
        free(statusWin);
        free(lifeWin);
    }

    return 0;
}