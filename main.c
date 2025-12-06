#include <stdio.h>                      // Standard input/output (printf, fprintf)
#include <stdlib.h>                     // Standard library (malloc, free, exit)
#include <unistd.h>                     // Unix standard (usleep for timing)
#include <ncurses.h>                    // Text-based UI library
#include <time.h>                       // Time needed to random the seed
#include <dirent.h>                       
#include <string.h>

#include <math.h>                       // Helps with the mathematic problems that couldn't be solved without it

#define REFRESH_TIME 100                // Frequency of refreshing the screen
#define START_PLAYER_SPEED 1            // Speed that player have on the start of a game         
#define MAX_LEVELS_COUNT 5              // Speed of Boss at maximum

#define ESCAPE      'q'                 // Button to quit the game
#define REPEAT      'r'                 // Button to play again
#define SAFE_ZONE   ' '                 // Button to call the safe zone

#define BORDER		1		            // Border width (in characters)
#define OFFY		5		            // Y offset from top of screen
#define LIFEWINY    3		            // Height of life status window
#define OFFX		5		            // X offset from left of screen
#define RANKING_COLS 20

#define MAIN_COLOR	            1		// Main window color
#define STAT_COLOR	            2		// Status bar color
#define PLAY_COLOR	            3		// Play area color
#define SWALLOW_COLOR	        4       // Swallow (player) color
#define HUNTER_COLOR	        5       // Hunter Enemy color
#define ALBATROS_TAXI_COLOR     6       // Friendly albatros taxi color
#define AGAIN_COLOR             7       // Color of Play Again Screen
#define END_COLOR               8       // Color of End Screen
#define STAR_COLOR              9       // First stars color while shifting color
#define STAR2_COLOR             10      // Second stars color while shifting color
#define SAFE_ZONE_COLOR         11      // Safe zone color
#define TAXI_COLOR              12      // Color of a friendly albatros taxi
#define BOSS_COLOR              13      // Color of a Boss

typedef struct {

    float start_time;           // Time to survive in the game
    int seed;                   // Number of seed to randomize the game
    int rows;                   
    int cols;                   
    int max_stars_count;        // Maximum amound of stars showed at the same time
    int max_stars_speed;        // Maximum speed that star can have
    int stars_scoring_weight;        // Maximum speed that star can have
    int max_hunters_size;       // Maximum size of hunter
    int max_hunters_count;      // Maximum amound of hunters in game
    int max_hunters_speed;      // Maximum speed that hunter can have
    int max_hunters_bounds;     // Maximum amound of bounces that hunter can make
    int max_swallow_health;
    int hunter_attack_after_time;
    int albatros_taxi_speed;
    int max_boss_speed;
    int boss_enter_part;
    int boss_damage;

} CONFIG_FILE;

typedef struct {
	WINDOW* window;             // ncurses window pointer
	int x, y;                   // Position on screen
	int rows, cols;             // Size of window
	int color;		            // Color scheme
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
    int dx;                     // direction to move the hunter
    float a, b;                 // Values of func to move huntery=ax+b
    int speed;                  // Hunters speed
	int animationFrame;		    // Animation frame of hunter
    int boundsCounter;		    // Value of possible bounds
    short int onTheScreen;		// Says if the hunter already jumped on the screen (0-1)
    short int huntersStage;
    float hunterWaitTime;
} Hunter;

typedef struct {
	WIN* playWin;               // Play Window where star is shown
	int x, y;		            // Position on screen
    int fallingSpeed;           // Stars falling speed
	int color;		            // Color scheme
	int color2;		            // Color scheme while shifting
    int animationFrame;		    // Color scheme
    int stars_scoring_weight;	
} Star;

typedef struct {
    WIN* playWin;               // Play Window where SafeZone is shown
    int x, y;		            // Position on screen
    int range;		            // Range of the safe zone circle
    bool active;		        
    int color;                  // Color of the safe zone
} SafeZone;

typedef struct {
    WIN* playWin;               // Play Window where hunter is shown
    float x, y;		            // Position on screen
    float dx, dy;		        // Vectors of moving taxi
    int speed;		            // Speed of the albatros
    int color;		            // Color scheme
    int stage;                  // Stage of rescuing the swallow
} TAXI;

typedef struct {
    WIN* playWin;               // Play Window where hunter is shown
    int x, y;		            // Position on screen
    int color;		            // Color scheme
    int dx;                     // direction to move the Boss
    float a, b;                 // Values of func to move Boss y=ax+b
    int speed;                  // Bosses speed
    float enterTime;		    // Time when boss shoud enter the game
    int size;		            // Actual size of boss
    bool onTheScreen;		    // Says if the Boss already jumped on the screen True/False
    int bossDamage;             // Damage that boss gives the swallow
} Boss;

typedef struct{
    int index;
    char nick[180];
    float points;
}Ranking;

void UpdateBoss(Boss* boss, Swallow* swallow, CONFIG_FILE* config)
{
    // Resizing the boss and randomizing his speed to match the level
    boss->size = config->max_swallow_health - swallow->hp + 1;
    boss->speed = rand() % (config->max_boss_speed-1) + 2;

    // Calculating swallows predicted future velocity
    float swallows_velocity_x = swallow->dx * swallow->speed;
    float swallows_velocity_y = swallow->dy * swallow->speed;

    // Calculating vector from boss to swallow
    float boss_to_swallow_x = swallow->x - boss->x;
    float boss_to_swallow_y = swallow->y - boss->y;

    // Calculating predicted future swallows path with linear function
    // We want to find time after which, swallow and boss will be at the same point. 
    // This leads us to formula: | swallow position - boss_position + swallow_vector * time | = bosses_vector * time
    float swallow_predicted_a = swallows_velocity_x * swallows_velocity_x + swallows_velocity_y * swallows_velocity_y - boss->speed * boss->speed;
    float swallow_predicted_b = 2 * (boss_to_swallow_x * swallows_velocity_x + boss_to_swallow_y * swallows_velocity_y);
    float swallow_predicted_c = boss_to_swallow_x * boss_to_swallow_x + boss_to_swallow_y * boss_to_swallow_y;

    // We are using it to find the time of both boss and swallow predicted travel to meet eachother
    float time;
    float delta = swallow_predicted_b * swallow_predicted_b - 4 * swallow_predicted_a * swallow_predicted_c;
    float delta_res = sqrt(delta);

    // We need the time that is positiv
    if (swallow_predicted_a != 0 && delta >= 0)
    {
        float t1 = (-swallow_predicted_b + delta_res) / (2 * swallow_predicted_a);
        float t2 = (-swallow_predicted_b - delta_res) / (2 * swallow_predicted_a);
        if (t1 > 0)
            time = t1;
        else if (t2 > 0)
            time = t2;
        else
            time = swallow->speed;
    }
    else
        time = swallow->speed;

    // Calculating the meeting point, having the necessary time of the travel
    float meeting_x = swallow->x + swallows_velocity_x * time;
    float meeting_y = swallow->y + swallows_velocity_y * time;

    // We need to know if we are moving boss left or right
    if (boss->x < meeting_x)
        boss->dx = 1;
    else
        boss->dx = -1;

    // Calculating a and b of the linear function being the future path of boss if possible
    if (boss->x != meeting_x)
        boss->a = (boss->y - meeting_y) / (boss->x - meeting_x);
    else
    {
        // To unaible "teleportation" we need to change the position of the Boss
        boss->x = config->cols * (rand() % 2);
        boss->y = rand() % config->rows;
        UpdateBoss(boss, swallow, config);
    }

    boss->b = meeting_y - boss->a * meeting_x;
}

void DrawBoss(Boss* boss)
{
    wattron(boss->playWin->window, COLOR_PAIR(boss->color));
    for (int x = boss->x - 2 * boss->size; x <= boss->x + 2 * boss->size; x++)
    {
        for (int y = boss->y - boss->size; y <= boss->y + boss->size; y++)
        {
            float dx = (x - boss->x);
            float dy = (y - boss->y);
            if (0.25 * dx * dx + dy * dy <= 4 * boss->size * boss->size)
            {
                mvwprintw(boss->playWin->window, y, x, " ");
            }
        }
    }
}

void SpawnBoss(Boss* boss, CONFIG_FILE* config, Swallow* swallow)
{
    boss->playWin = swallow->playWin;
    boss->x = config->cols * (rand() % 2);
    boss->y = rand() % config->rows;
    boss->color = BOSS_COLOR;
    UpdateBoss(boss, swallow, config);
    boss->enterTime = (config->start_time / config->boss_enter_part);
    boss->onTheScreen = false;
    boss->bossDamage = config->boss_damage;
}

void CheckBossCollision(Swallow* swallow, Boss* boss, CONFIG_FILE* config, SafeZone* safeZone, float* timer)
{
    float dx = boss->x - swallow->x;
    float dy = boss->y - swallow->y;
    float minimum_distance = boss->size + swallow->hp;
    float distance = dx * dx + dy * dy;
    

    // for spawn zone
    if (safeZone->active)
    {
        dx = boss->x - config->cols / 2;
        dy = boss->y - config->rows / 2;
        distance = dx * dx + dy * dy;
        if (distance <= 4 * safeZone->range * safeZone->range)
            SpawnBoss(boss, config, swallow);
    }else if (distance <= (minimum_distance * minimum_distance))
    {
        swallow->hp -= boss->bossDamage;
        SpawnBoss(boss, config, swallow);
    }

}

void MoveBoss(Boss* boss, Swallow* swallow, CONFIG_FILE* config, SafeZone* safeZone, float* timer)
{
    if (*timer > boss->enterTime)
        return;
    DrawBoss(boss);

    for (int i = 0; i < boss->speed; i++)
    {
        boss->x += boss->dx;
        boss->y = (boss->a * boss->x) + boss->b;

        if (!boss->onTheScreen)
        {
            if (boss->x <= config->cols - 2 && boss->x >= 1 && boss->y <= config->rows - 2 && boss->y >= 0)
                boss->onTheScreen = true;
            else
                break;
        }


        if (boss->y < 0)
        {
            boss->y = 0;
            boss->a *= -1;
            boss->b = boss->y - (boss->a * boss->x);
            UpdateBoss(boss, swallow, config);
        }
        else if (boss->y > config->rows - 2)
        {
            boss->y = config->rows - 2;
            boss->a *= -1;
            boss->b = boss->y - (boss->a * boss->x);
            UpdateBoss(boss, swallow, config);
        }


        if (boss->x < 1)
        {
            boss->x = 1;
            boss->a *= -1;
            boss->b = boss->y - (boss->a * boss->x);
            boss->dx *= -1;
            UpdateBoss(boss, swallow, config);
        }
        else if (boss->x > config->cols - 2)
        {
            boss->x = config->cols - 2;
            boss->a *= -1;
            boss->b = boss->y - (boss->a * boss->x);
            boss->dx *= -1;
            UpdateBoss(boss, swallow, config);
        }

        if (!safeZone->active)
            CheckBossCollision(swallow, boss, config, safeZone, timer);
    }
}

void SpawnHunter(Hunter* tempHunter, Swallow* swallow, CONFIG_FILE* config, float* timer)
{
    tempHunter->speed = rand() % config->max_hunters_speed + 1;

    tempHunter->onTheScreen = false;
    tempHunter->x = config->cols * (rand() % 2 );
    tempHunter->y = rand() % config->rows;
    tempHunter->huntersStage = 0;
    tempHunter->size = rand() % config->max_hunters_size + 1;
    tempHunter->hunterWaitTime = 0;

    if (tempHunter->x < swallow->x)
        tempHunter->dx = 1;
    else
        tempHunter->dx = -1;

    if (tempHunter->x - swallow->x != 0)
        tempHunter->a = (float)(tempHunter->y - swallow->y) / (float)(tempHunter->x - swallow->x);
    else
        tempHunter->a = 0;

    tempHunter->b = swallow->y - (tempHunter->a * swallow->x);

    tempHunter->animationFrame = 0;
    if (*timer == 0)
        tempHunter->boundsCounter = 0;
    else
        tempHunter->boundsCounter = (int)(config->max_hunters_bounds *  (config->start_time - *timer) / config->start_time + 1);
}

CONFIG_FILE* getConfigInfo(char* adress)
{
    FILE* ofile;
    CONFIG_FILE* cfile = (CONFIG_FILE*)malloc(sizeof(CONFIG_FILE));
    char output[100];

    ofile = fopen(adress, "r");
    if (!ofile)
    {
        printf("The .conf file can't be found. Please add configuration file to play a game.");
        return cfile;
    }

    fscanf(
        ofile, 
        "start time = %f\n"
        "seed = %i\n"
        "window rows = %d\n"
        "window cols = %d\n"
        "max stars count = %d\n"
        "max stars speed = %d\n"
        "stars scoring weight = %d\n"
        "max hunters size = %d\n"
        "max hunters count = %d\n"
        "max hunters speed = %d\n"
        "max hunters bounds = %d\n"
        "max swallow health = %d\n"
        "hunter attack after time = %d\n"
        "albatros taxi speed = %d\n"
        "max boss speed = %d\n"
        "boss enter part = %d\n"
        "boss damage = %d",
        &cfile->start_time, 
        &cfile->seed,
        &cfile->rows,
        &cfile->cols,
        &cfile->max_stars_count, 
        &cfile->max_stars_speed,
        &cfile->stars_scoring_weight,
        &cfile->max_hunters_size,
        &cfile->max_hunters_count,
        &cfile->max_hunters_speed, 
        &cfile->max_hunters_bounds, 
        &cfile->max_swallow_health,
        &cfile->hunter_attack_after_time,
        &cfile->albatros_taxi_speed,
        &cfile->max_boss_speed,
        &cfile->boss_enter_part,                    // The part of the game in which Boss enters the game (2 -> 1/2, 4 -> 1/4 etz)
        &cfile->boss_damage);
    fclose(ofile);

    return cfile;

}

void CheckStarsCollision(Swallow* swallow, Star* star, CONFIG_FILE* config)
{
    float dx = star->x-swallow->x;
    float dy = star->y-swallow->y;
    if((dx*dx+dy*dy) <= (swallow->hp * swallow->hp))
    {
        //respawn
        star->y = -10;
        star->x = rand()%(config->cols-1) +1;
        swallow->wallet +=star->stars_scoring_weight;
    }
}

void CheckHuntersCollision(Swallow* swallow, Hunter* hunter, CONFIG_FILE* config, SafeZone* safeZone, float* timer)
{
    float dx = hunter->x-swallow->x;
    float dy = hunter->y-swallow->y;
    float minimum_distance = hunter->size + swallow->hp - 2;
    float second_minimum_distance = hunter->size + config->max_swallow_health;
    float distance =dx*dx+dy*dy;

    // for spawn zone
    if(safeZone->active)
    {
        dx = hunter->x - config->cols / 2;
        dy = hunter->y - config->rows / 2;
        distance = dx * dx + dy * dy;
        if (distance <= 4 * safeZone->range * safeZone->range)
            SpawnHunter(hunter, swallow, config, timer);
    }
    else if(distance <= (minimum_distance * minimum_distance))
    {
        SpawnHunter(hunter, swallow, config, timer);

        swallow->hp -= 1;
    }
    else if (distance <= 4 * (second_minimum_distance * second_minimum_distance) && hunter->huntersStage == 0)
    {
        hunter->huntersStage = 1;
        hunter->hunterWaitTime = *timer;
    }
}

void CheckSwallowsCollision(Swallow* swallow, Star** stars, Hunter** hunters, CONFIG_FILE* config, SafeZone* safeZone, float* timer)
{
    if (safeZone->active)
        return;

    for (int i = 0; i < config->max_stars_count; i++)
    {
        CheckStarsCollision(swallow, stars[i], config);
    }
    
    for (int i = 0; i < config->max_hunters_count; i++)
    {
        if (*timer  >= i * config->start_time / config->max_hunters_count)
            continue;
        CheckHuntersCollision(swallow, hunters[i], config, safeZone, timer);
    }
    
}

void SetSafeZone(SafeZone* zone, Swallow* swallow, bool active, CONFIG_FILE* config)
{
    zone->playWin = swallow->playWin;
    zone->range = swallow->hp;
    zone->x = config->cols / 2;
    zone->y = config->rows / 2;
    zone->active = active;
    zone->color = SAFE_ZONE_COLOR;
}

void SetTaxi(TAXI* taxi, Swallow* swallow, CONFIG_FILE* config)
{
    taxi->playWin = swallow->playWin;
    taxi->x = config->cols/2;
    taxi->y = 1;
    taxi->speed = config->albatros_taxi_speed;
    taxi->dx = 0;
    taxi->dy = 0;

    taxi->color = ALBATROS_TAXI_COLOR;
    taxi->stage = 4;
}

void PlayerMovement(Swallow* swallow, int input, Star** stars, Hunter** hunters, CONFIG_FILE* config, float* timer, SafeZone* safeZone, TAXI* taxi)
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

    case ' ':
        if (taxi->stage >= 3)
        {
            SetTaxi(taxi, swallow, config);
            taxi->stage = 0;
        }
        break;
    
    default:
        break;
    }
    for (int i = 0; i < swallow->speed; i++)
    {
        swallow->y += swallow->dy;
        if(swallow->y > 0)
            swallow->y %= config->rows;
        else
            swallow->y = config->rows;

        swallow->x += swallow->dx;
        if(swallow->x > 0)
            swallow->x %= config->cols;
        else
            swallow->x = config->cols;
        
        CheckSwallowsCollision(swallow, stars, hunters, config, safeZone, timer);
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

void MoveHunter(Hunter* hunter, Swallow* swallow, CONFIG_FILE* config,SafeZone* safeZone, float* timer)
{
    if (hunter->boundsCounter <= 0)
        SpawnHunter(hunter, swallow, config, timer);

    if (hunter->huntersStage != 1)
    {
        for (int i = 0; i < hunter->speed; i++)
        {
            hunter->x += hunter->dx;
            hunter->y = (hunter->a * hunter->x) + hunter->b;

            if (hunter->onTheScreen == 0)
            {
                if (hunter->x <= config->cols - 2 && hunter->x >= 1 && hunter->y <= config->rows - 2 && hunter->y >= 0)
                    hunter->onTheScreen = 1;
                else
                    break;
            }


            if (hunter->y < 0)
            {
                hunter->y = 0;
                hunter->a *= -1;
                hunter->b = hunter->y - (hunter->a * hunter->x);
                hunter->boundsCounter -= 1;
                hunter->huntersStage = 0;
            }
            else if (hunter->y > config->rows - 2)
            {
                hunter->y = config->rows - 2;
                hunter->a *= -1;
                hunter->b = hunter->y - (hunter->a * hunter->x);
                hunter->boundsCounter -= 1;
                hunter->huntersStage = 0;
            }


            if (hunter->x < 1)
            {
                hunter->x = 1;
                hunter->a *= -1;
                hunter->b = hunter->y - (hunter->a * hunter->x);
                hunter->dx *= -1;
                hunter->boundsCounter -= 1;
                hunter->huntersStage = 0;
            }
            else if (hunter->x > config->cols - 2)
            {
                hunter->x = config->cols - 2;
                hunter->a *= -1;
                hunter->b = hunter->y - (hunter->a * hunter->x);
                hunter->dx *= -1;
                hunter->boundsCounter -= 1;
                hunter->huntersStage = 0;
            }

            if (!safeZone->active)
                CheckHuntersCollision(swallow, hunter, config, safeZone, timer);
        }
    }
    else
    {
        if (hunter->hunterWaitTime - *timer < config->hunter_attack_after_time)
        {
            hunter->speed = 0;
            return;
        }

        if (hunter->x < swallow->x)
            hunter->dx = 1;
        else
            hunter->dx = -1;

        if (hunter->x - swallow->x != 0)
            hunter->a = (float)(hunter->y - swallow->y) / (float)(hunter->x - swallow->x);
        else
            hunter->a = 0;

        hunter->b = swallow->y - (hunter->a * swallow->x);
        hunter->huntersStage = 2;
        hunter->speed = 1;
    }
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

void DrawStars(WIN* playWin, Star* star, Swallow* swallow, CONFIG_FILE* config)
{
    if(star->animationFrame % 3)
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

        CheckStarsCollision(swallow, star, config);

        if(star->y >= config->rows-1)
        {
            star->y %= config->rows-1;
            star->x = rand()%(config->cols-1) +1;
        }
    }
}

void DrawSafeZone(SafeZone* safeZone, Swallow* swallow, CONFIG_FILE* config)
{
    wattron(safeZone->playWin->window, COLOR_PAIR(safeZone->color));
    for (int x = config->cols / 2 - 2 * safeZone->range; x <= config->cols / 2 + 2 * safeZone->range; x++)
    {
        for (int y = config->rows / 2 - safeZone->range; y <= config->rows / 2 + safeZone->range; y++)
        {
            float dx = (x - safeZone->x);
            float dy = (y - safeZone->y);

            if (0.25 * dx * dx + dy * dy <= 4 * safeZone->range * safeZone->range)
            {
                dx = x - swallow->x;
                dy = y - swallow->y;

                float distance = dx * dx + dy * dy;
                if (distance > swallow->hp * swallow->hp * 2)
                {
                    mvwprintw(safeZone->playWin->window, y, x, " ");
                }
            }
        }
    }
}

void DrawTaxi(TAXI* taxi)
{
    wattron(taxi->playWin->window, COLOR_PAIR(taxi->color));

    mvwprintw(taxi->playWin->window, (int)taxi->y, (int)taxi->x, " ");
    mvwprintw(taxi->playWin->window, (int)taxi->y, (int)taxi->x + 1, " ");
    mvwprintw(taxi->playWin->window, (int)taxi->y, (int)taxi->x - 1, " ");
    mvwprintw(taxi->playWin->window, (int)taxi->y, (int)taxi->x + 2, " ");
    mvwprintw(taxi->playWin->window, (int)taxi->y, (int)taxi->x - 2, " ");

    if (taxi->dx > 0)
    {
        mvwprintw(taxi->playWin->window, (int)taxi->y - 1, (int)taxi->x, ">");
        mvwprintw(taxi->playWin->window, (int)taxi->y, (int)taxi->x + 3, " ");
    }
    else
    {
        mvwprintw(taxi->playWin->window, (int)taxi->y - 1, (int)taxi->x, "<");
        mvwprintw(taxi->playWin->window, (int)taxi->y, (int)taxi->x - 3, " ");

    }

    wattron(taxi->playWin->window, COLOR_PAIR(MAIN_COLOR));
    mvwprintw(taxi->playWin->window, (int)taxi->y+1, (int)taxi->x + 2, "O");
    mvwprintw(taxi->playWin->window, (int)taxi->y+1, (int)taxi->x - 2, "O");


}

void MoveTaxi(TAXI* taxi, Swallow* swallow, SafeZone* safeZone, CONFIG_FILE* config)
{
    if (taxi->stage > 2)
    {
        safeZone->active = false;
        return;
    }

    float x, y;
    if (taxi->stage == 0)
    {
        x = swallow->x;
        y = swallow->y;
    }
    else if(taxi->stage == 1)
    {
        SetSafeZone(safeZone, swallow, true, config);
        x = config->cols/2;
        y = config->rows/2;
    }
    else
    {
        x = config->cols / 2;
        y = -1;
    }

    taxi->dx = x - taxi->x;
    taxi->dy = y - taxi->y;

    float distance = sqrt(taxi->dx * taxi->dx + taxi->dy * taxi->dy);

    if (distance <= swallow->hp)
        taxi->stage += 1;

    if (distance != 0) {
        taxi->dx /= distance;
        taxi->dy /= distance;
    }

    taxi->x += taxi->dx * taxi->speed;
    taxi->y += taxi->dy * taxi->speed;

    DrawTaxi(taxi);
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

Star** InitStars(WIN* playWin, int color, int color2, CONFIG_FILE* config)
{
    Star** list = (Star**)malloc(config->max_stars_count * sizeof(Star*));
    for (int i = 0; i < config->max_stars_count; i++)
    {
        Star* tempStar = (Star*)malloc(sizeof(Star));
        tempStar->playWin = playWin;
        tempStar->x = rand()%config->cols;
        tempStar->y = -rand()%config->rows;
        tempStar->fallingSpeed = rand()%config->max_stars_speed+1;
        tempStar->color = color;
        tempStar->color2 = color2;
        tempStar->animationFrame = 0;
        tempStar->stars_scoring_weight = config->stars_scoring_weight;
        list[i] = tempStar;
    }

    return list;
    
}

Swallow* InitSwallow(WIN* playWin, int x, int y, int dx, int dy, int speed, int color, CONFIG_FILE* config)
{
    Swallow* swallow = (Swallow*)malloc(sizeof(Swallow)); //Allocating memory for Swallow

    //Seting swallow's properties
    swallow->playWin = playWin;
    swallow->x = x;
    swallow->y = y;
    swallow->dx = dx;
    swallow->dy = dy;
    swallow->wallet = 0;
    swallow->speed = START_PLAYER_SPEED;
    swallow->color = color;
    swallow->animationFrame = 0;
    swallow->hp = config->max_swallow_health;

    return swallow;
}

Hunter** InitHunters(WIN* playWin, int color, Swallow* swallow, CONFIG_FILE* config)
{
    Hunter** list = (Hunter**)malloc(config->max_hunters_count * sizeof(Hunter*));
    float* timer;
    *timer = 0;
    for (int i = 0; i < config->max_hunters_count; i++)
    {
        Hunter* tempHunter = (Hunter*)malloc(sizeof(Hunter));
        tempHunter->playWin = playWin;
        SpawnHunter(tempHunter, swallow, config, timer);

        tempHunter->color = color;
        list[i] = tempHunter;
    }

    return list;
    
}

Ranking** GetScores()
{
    char line[180];
    FILE* f = fopen("./ranking", "r+");

    Ranking** rankingList = (Ranking**)malloc(sizeof(Ranking*) * 100);

    int i = 0;
    while (fgets(line, sizeof(line), f))
    {
        Ranking* playerRanking = (Ranking*)malloc(sizeof(Ranking));
        sscanf(line, "%d %s %f", &playerRanking->index, playerRanking->nick, &playerRanking->points);
        rankingList[i++] = playerRanking;
    }
    fclose(f);

    rankingList[i] = NULL;

    return rankingList;
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
    init_pair(ALBATROS_TAXI_COLOR, COLOR_WHITE, COLOR_YELLOW);
    init_pair(END_COLOR, COLOR_RED, COLOR_BLACK);
    init_pair(AGAIN_COLOR, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(STAR_COLOR, COLOR_YELLOW, COLOR_BLACK);
    init_pair(STAR2_COLOR, COLOR_WHITE, COLOR_BLACK);
    init_pair(SAFE_ZONE_COLOR, COLOR_CYAN, COLOR_CYAN);
    init_pair(BOSS_COLOR, COLOR_MAGENTA, COLOR_MAGENTA);

    //makes input invisible
    noecho();

    //makes cursor invisible
    curs_set(0);

    return win;

}

void RankingStatus(WIN* rankingWin,CONFIG_FILE* config, char* level)
{
    // Set status bar color
    wattron(rankingWin->window, COLOR_PAIR(rankingWin->color));

    // Draw border around status bar
    box(rankingWin->window, 0, 0);

    char levelInfo[50], rankingInfo[50];
    snprintf(levelInfo, sizeof(levelInfo), "Level: %s", level);

    // Display status info
    mvwprintw(rankingWin->window, 1, 2, levelInfo);

    Ranking** ranking = GetScores();
    for (int i = 0; ranking[i] != NULL; i++)
    {
        snprintf(rankingInfo, sizeof(rankingInfo), "%d %s %f", ranking[i]->index, ranking[i]->nick, ranking[i]->points);
    }

    // Update display
    wrefresh(rankingWin->window);
}

void UpdateStatus(WIN* statusWin, Swallow* swallow, CONFIG_FILE* config)
{
	// Set status bar color
	wattron(statusWin->window, COLOR_PAIR(statusWin->color));

	// Draw border around status bar
	box(statusWin->window, 0, 0);

    char info[50];
    snprintf(info, sizeof(info), "Number of gained stars: %d    Swallows speed: %d", swallow->wallet, swallow->speed);
    char controls[] = "W (up), S (down), D (right), A (left) | O (slow down), P (go faster), SPACE (safe zone)";

    // Display status info
	mvwprintw(statusWin->window, 1, (config->cols-(sizeof(info)/sizeof(char)))/2, info);

	// Display controls
	mvwprintw(statusWin->window, 3, (config->cols-(sizeof(controls)/sizeof(char)))/2, controls);

	// Update display
	wrefresh(statusWin->window);
}

void UpdateLifeInfo(WIN* lifeinfo, Swallow* swallow, float *timer, CONFIG_FILE* config)
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
	mvwprintw(lifeinfo->window, 1, config->cols/3-OFFX, time);

	// Update display
	wrefresh(lifeinfo->window);
}

void EndScreen(WIN* playWin, CONFIG_FILE* config)
{
    CleanWin(playWin, BORDER);

    // Set text color
	wattron(playWin->window, COLOR_PAIR(END_COLOR));

	// Draw border around text
	box(playWin->window, 0, 0);

    char endText[] = "Bye bye, thanks for playing!";

    //Display end text
	mvwprintw(playWin->window, config->rows/2, (config->cols-(sizeof(endText)/sizeof(char)))/2, endText);

	// Update display
	wrefresh(playWin->window);
    sleep(1);
}

void AgainScreen(WIN* playWin, char *resultText, CONFIG_FILE* config)
{
    CleanWin(playWin, BORDER);

    // Set text color
	wattron(playWin->window, COLOR_PAIR(AGAIN_COLOR));

	// Draw border around text
	box(playWin->window, 0, 0);

    char* question = "Click 'r' to play again and 'q' if you don't ;)";

    //Display result text
	mvwprintw(playWin->window, config->rows/2-2, (config->cols-(sizeof(resultText)/sizeof(char)))/2.2f, resultText);
	mvwprintw(playWin->window, config->rows/2, (config->cols-(sizeof(question)/sizeof(char)))/3.2f, question);

	// Update display
	wrefresh(playWin->window);
}

void SortRankingList(Ranking** rankinglist)
{
    for (int i = 0; rankinglist[i] != NULL; i++)
    {
        for (int j = 0; rankinglist[j+1] != NULL; j++)
        {
            if (rankinglist[j]->points < rankinglist[j + 1]->points)
            {
                Ranking* tempRanking = rankinglist[j];

                rankinglist[j] = rankinglist[j + 1];
                rankinglist[j]->index = j + 1;

                rankinglist[j + 1] = tempRanking;
                rankinglist[j+1]->index= j+2;
            }
        }
    }
}

void SaveRanking(Ranking** ranking)
{
    FILE* f = fopen("./ranking", "r+");
    for (int i = 0; ranking[i] != NULL; i++)
    {
        fprintf(f, "%d %s %f\n", ranking[i]->index, ranking[i]->nick, ranking[i]->points);
    }
    fclose(f);
}

void AddScore(float points, char playerName[])
{
    Ranking** rankingList = GetScores();
    bool found = false;
    int i = 0;
    while (rankingList[i] != NULL)
    {
        if (strcmp(rankingList[i]->nick, playerName) == 0)
        {
            found = true;
            rankingList[i]->points += points;
            break;
        }
        i++;
    }

    // Add new Player
    if (!found)
    {
        rankingList[i] = (Ranking*)malloc(sizeof(Ranking));
        rankingList[i]->index = i + 1;
        strcpy(rankingList[i]->nick, playerName);
        rankingList[i]->points = points;
        rankingList[i + 1] = NULL;
    }
    SortRankingList(rankingList);

    SaveRanking(rankingList);
}

void PlayAgain(WIN* playWin, Swallow* swallow, bool* isPlaying, CONFIG_FILE* config, char* playerName)
{
    int ch;
    char* resultText;
    if(swallow->hp >0)
        resultText = "You won, congarts!";
    else
        resultText = "You have lost!";
    
    AgainScreen(playWin, resultText, config);

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

bool CheckConfigName(char** namesList, char fileName[100])
{
    for (int i = 0; i < MAX_LEVELS_COUNT && namesList[i]; i++)
        if (strcmp(namesList[i], fileName)==0)
            return true;

    return false;
}

void AskPlayer(char* playerName, char configAdress[100], char* level)
{
    DIR* dir;
    struct dirent* plik;
    char fileName[100];

    char** namesList = (char**)malloc(sizeof(char*) * MAX_LEVELS_COUNT);
    for (int i = 0; i < MAX_LEVELS_COUNT; i++)
        namesList[i] = NULL;

    printf("Podaj nazwę gracza: \n");
    scanf("%s", playerName);

    if (dir = opendir("./levels/"))
    {
        printf("\nWybierz poziom trudności:\n");

        int i = 0;
        while ((plik = readdir(dir)) != NULL && i < MAX_LEVELS_COUNT)
        {
            if (strcmp(plik->d_name, ".") == 0 || strcmp(plik->d_name, "..") == 0)
                continue;

            printf("%s\n", plik->d_name);

            namesList[i] = malloc(strlen(plik->d_name) + 1);
            strcpy(namesList[i], plik->d_name);

            i++;
        }

        closedir(dir);
        scanf("%s", fileName);
        strcat(configAdress, "./levels/");
        strcat(configAdress, fileName);
    }
    else
        strcpy(configAdress, ".conf");

    if (CheckConfigName(namesList, fileName))
    {
        strcpy(level, fileName);
    }
    else
    {
        strcpy(configAdress, ".conf");
        strcpy(level, "default");
    }


    free(namesList);
}

void Update(WIN *playWin, WIN *statusWin,WIN* lifeWin,WIN* rankingWin,CONFIG_FILE* config, Swallow* swallow, Star** stars, Hunter** hunters, SafeZone* safeZone, TAXI* taxi, Boss* boss, char* level)
{
    int ch;
    float *timer = (float*)malloc(sizeof(float));
    *timer = config->start_time;

    Ranking** rankingList = GetScores();
    while(1)
    {
        ch = wgetch(playWin->window);

        CleanWin(playWin, BORDER);

        *timer -= 0.1;
        UpdateLifeInfo(lifeWin, swallow, timer, config);
        if (ch == ESCAPE || *timer <= 0 || swallow->hp <= 0) break;
        else if (*timer <= 0) break;
        else PlayerMovement(swallow, ch, stars, hunters, config, timer, safeZone, taxi);

        for (int i = 0; i < config->max_stars_count; i++)
        {
            DrawStars(playWin, stars[i], swallow, config);
        }

        MoveBoss(boss, swallow, config, safeZone, timer);

        if (taxi->stage >= 0 && taxi->stage <= 3)
        {
            if (taxi->stage == 1)
            {
                swallow->x = taxi->x;
                swallow->y = taxi->y;
            }
            else
                DrawSwallow(playWin, swallow);

            MoveTaxi(taxi, swallow, safeZone, config);

            if(safeZone->active)
                DrawSafeZone(safeZone, swallow, config);
        }
        else
            DrawSwallow(playWin, swallow);


        for (int i = 0; i < config->max_hunters_count; i++)
        {
            if (*timer >= i * config->start_time / config->max_hunters_count)
                continue;
            MoveHunter(hunters[i], swallow, config, safeZone, timer);
            DrawHunter(playWin, hunters[i], swallow);
        }
        
        UpdateStatus(statusWin, swallow, config);
        RankingStatus(rankingWin, config, level);

        wrefresh(playWin->window);
        flushinp();
        usleep(REFRESH_TIME * 1000);
    }
}

int main()
{
    char playerName[100], configAdress[100], level[50];
    AskPlayer(playerName, configAdress, level);
    CONFIG_FILE* config = getConfigInfo(configAdress);
    srand(config->seed);
    bool* isPlaying = (bool*)malloc(sizeof(bool));
    *isPlaying = true;
    while (*isPlaying)
    {
        WINDOW *mainWin = Start();

        WIN *rankingWin = InitWin(
            mainWin,
            config->rows,
            20,
            OFFY,
            OFFX + config->cols,
            PLAY_COLOR,
            BORDER,
            0);


        WIN *lifeWin = InitWin(
            mainWin, 
            LIFEWINY, 
            config->cols/2, 
            LIFEWINY-1, 
            OFFY+config->cols/4, 
            STAT_COLOR, 
            BORDER, 
            0);

        WIN *playWin = InitWin(
            mainWin, 
            config->rows, 
            config->cols, 
            OFFY, 
            OFFX, 
            PLAY_COLOR, 
            BORDER, 
            0);

        WIN *statusWin = InitWin(
            mainWin, 
            OFFY, 
            config->cols, 
            config->rows+OFFY, 
            OFFX, 
            STAT_COLOR, 
            BORDER, 
            0);
        

        Swallow* swallow = InitSwallow(
            playWin,
            config->cols/2,
            config->rows/2,
            0,
            -1,
            START_PLAYER_SPEED,
            SWALLOW_COLOR,
            config
        );

        Boss* boss = (Boss*)malloc(sizeof(Boss));
        SpawnBoss(boss, config, swallow);

        UpdateBoss(boss, swallow, config);

        Star** stars = InitStars(playWin, STAR_COLOR, STAR2_COLOR, config);
        
        Hunter** hunters = InitHunters(playWin, HUNTER_COLOR, swallow, config);

        SafeZone* safeZone = (SafeZone*)malloc(sizeof(SafeZone));
        TAXI* taxi = (TAXI*)malloc(sizeof(TAXI));

        SetSafeZone(safeZone, swallow, false, config);
        SetTaxi(taxi, swallow, config);
        
        wrefresh(mainWin);

        Update(playWin, statusWin, lifeWin, rankingWin, config, swallow, stars, hunters, safeZone, taxi, boss, level);

        PlayAgain(playWin, swallow, isPlaying, config, playerName);

        if(!(*isPlaying))
            EndScreen(playWin, config);

        endwin();

        // Free allocated memory
        free(swallow);
        for (int i = 0; i < config->max_stars_count; i++)
            free(stars[i]);
        for (int i = 0; i < config->max_hunters_count; i++)
            free(hunters[i]);

        free(stars);
        free(hunters);
        free(playWin);
        free(statusWin);
        free(safeZone);
        free(taxi);
    }

    return 0;
}