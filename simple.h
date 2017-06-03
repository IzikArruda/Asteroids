/*
 * This header defines all constants, variables and functions that will be used in the main game file
 * and accross the multiple sub-files, such as the myRandom, to add consistency between files.
 */

/* --- Global Constants ---------------------------------------------------------------------------------- */

#define M_PI 3.14159265358979323846
#define NAME_LENGTH 50
#define SYSTEM_COUNT 100

/*
 * Set the limits to represent the percentage of a planet being a certain type.
 * colonized: 5%
 * ring: 20%
 * default: 80%
 */
#define PLANET_TYPE_COLONIZED_LIMIT 0.05
#define PLANET_TYPE_RING_LIMIT 0.25

/* These values indicate the game's state to allow easier redability in later code */
//The first screen to display: shows the title and gives the user options
#define STATE_TITLE 0
//The actual asteroids game where the user flys around shooting asteroids to increment the level
#define STATE_ASTEROIDS 1
//A help screen which shows text on how to play and lets the user fly the basic ship around
#define STATE_HELP 2
//A screen which cycles through the ships selectable. User presses spacebar to continue
#define STATE_SHIPSELECT 3
//3D View of the system with all it's planets
#define STATE_SYSTEM 4


/* -- Global Type Definitions ------------------------------------------------------------------------------------ */

#ifndef GLOBAL_HEADER_PLAYER
#define GLOBAL_HEADER_PLAYER
/* Hold all information that is used with player stats/objects such as mineral scan limit and their ship.
 * Make this a global struct since every file will be using the player for some aspect or another */
typedef struct Player{
	int asteroidsHighScore, cooldown, currentCooldown, photonUpgrade;
	double photonSize, photonSpeed;
	double jumpDistance;
	double mineralLimit;
	double energyLimit;

	/* Common material used with most of all crafting. Constant drop from asteroids */
	double metal;

	/* Rare material used semi-often with crafting. Can be found sometimes from asteroids. */
	double alloy;

	/* A sum of the player's actions or deeds */
	double notoriety;

	/* Un-used fuel counter. Use fuel to jump between systems */
	double fuel;

} Player;
#endif


/* --- Global Variables ------------------------------------------------------------------------------------- */

/* Window size */
extern double xMax, yMax, h, w;

/* Main state of the game. This is the first value checked when determining what the game state is */
extern int state;

/* State of cursor keys */
extern int	up, down, left, right;

/* The main player struct along with their stats */
static Player player;


/* --- Global Functions ----------------------------------------------------------------------------------------- */

/* Change the state of the game using a constant as a perameter. Must be able to be called from anywhere in the code */
static void changeState(int s);

/* Return a random value between min and max. Has a limit to how many digits can preceed the decimal point */
static double myRandom(double min, double max);

/* This should be removed once the clear function has been isolated into the state change function */
static void clear();
