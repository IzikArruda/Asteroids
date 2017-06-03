/*
 * This file contains all the pertinent constants, variables and functions to run the asteroids game
 */
#include "simple.h"
//
/* These values indicate the maximum amount of said objects that can be active at one moment in the game. Can be replaced with linked list */
#define MAX_PHOTONS	8
#define MAX_ASTEROIDS	16
#define MAX_VERTICES	16
#define MAX_STARS 6
#define MAX_DEBRIS 25
#define MAX_DUST 		100
#define MAX_POINTS 10
#define MAX_ASTEROID_VARIANCE 	3
#define MIN_ASTEROID_VARIANCE   2

#define ASTEROID_LARGE 5
#define ASTEROID_MEDIUM 3
#define ASTEROID_SMALL 2

#define drawCircle() glCallList(circle)
/* --- Type Definitions -------------------------------------------------------- */

/* Coordinates for a position on a 2D plane */
typedef struct Coords {
	double		x, y;
} Coords;

/* All pertinent values used for tracking a ship */
typedef struct Ship{
	int LDmg, RDmg, BDmg, type, shipUpgrade;
	double	x, y, phi, dx, dy, size, LHp, RHp, BHp, shipSpeed, shipControl;
} Ship;

/* Values used with a single photon shot */
typedef struct Photon{
	int	active;
	double	x, y, dx, dy;
} Photon;

/* An asteroid for the game Asteroids. Contains coordinates for each of its vertices and rotational speed */
typedef struct Asteroid{
	int	active, nVertices, size;
	double	x, y, phi, dx, dy, dphi;
	Coords	coords[MAX_VERTICES];
} Asteroid;

/* Static dots in the background of the asteroids game. Uses the occilating global value along with it's flicker to simulate shining */
typedef struct BackgroundStar{
    int active;
    double x, y, flicker, flickerRate;
} BackgroundStar;

/* Dust that comes from an asteroid. Has a lifetime and a velocity */
typedef struct Dust{
	int active, lifetime;
	double x, y, dx, dy;
} Dust;

/* Debris that comes from an asteroid being completly destroyed. Contains 2D position along with Coords for a triangle (the debris) */
typedef struct Debris{
	int active, lifetime, type;
	double x, y, phi, dx, dy, dphi;
	Coords coords[3];
} Debris;

/* Temporary display for how much score the user obtained. Has a lifetime and position on a 2D plane */
typedef struct Points{
    int active, amount, lifetime;
    double x, y;
} Points;

/* Temporary text with a lifetime. Contains a message and a position in a 2D plane */
typedef struct Text{
	int active, lifetime;
	double x, y;
	char* msg;
} Text;

/* --- Local Variables ------------------------------------------------------------------------------ */

static int level, maxLevel, respawn, cooldown, currentCooldown, photonUpgrade;
static int selectedOption;
static double oscillating, photonSize, photonSpeed;

/* Track the amount of times the user gains score from a certain event to display the end-game score
 * recount and track the amount of digits in the combined score for center alligning the displayed score */
static int scoreDigits, scoreAL, scoreAM, scoreAS, debrisM, debrisA, metalCount, alloyCount;

/* Use a ship object to track values specified for the ship in this game of asteroids */
Ship ship;

/* How many ticks are left before the text dissapears. The text displays level stats after a level in asteroids */
static int levelTextLifetime;

/* Photon shots fired from the player's ship in the asteroids game */
static Photon photons[MAX_PHOTONS];

/* Large asteroids that can collide with photon shots. Bigger asteroids turn into smaller ones on hit */
static Asteroid	asteroids[MAX_ASTEROIDS];

/* That background stars that dress-up the asteroids game */
static BackgroundStar backgroundStars[MAX_STARS*MAX_STARS];

/* Large pieces of asteroids that can be collected when an asteroid is fully destroyed */
static Debris debris[MAX_DEBRIS];

/* Single points representing space dust that come from destroying asteroids */
static Dust dust[MAX_DUST];

/* Temporary integer value that appears above an area where the user obtained points/score */
static Points points[MAX_POINTS];

/* Temporary text that appears on the bottom-left of the screen when getting an upgrade */
static Text upgradeText;

/* --- Function prototypes --------------------------------------------------- */

/* Initilization functions that create new in-game objects or activate previously disabled ones */
static void initAsteroids();
static void initShip();
static void initDust(Asteroid *a, Asteroid *b);
static void initBackground();

/* Destroy/deactivate the object given from the parameter by deactivating them in their own way */
static void destroyAsteroid(Asteroid *a, Photon *p);
static void destroyShip();
static void destroyDebris(Debris *d);

/* General functions that are run on specific event triggers */
static void upgradeShip();
static void changeUpgradeText(char* t);
static void changeShip();
static void firePhoton();
static void addScore(int p, double x, double y);
static void nextLevel();
static void initAsteroid(Asteroid *a, int size);

/* Drawing functions which end up adding some kind of visual element to the window */
static void drawPoints(Points *p);
static void drawShip();
static void drawPhoton(Photon *p);
static void drawAsteroid(Asteroid *a);
static void drawBackground(BackgroundStar *s);
static void drawDust(Dust *d);
static void drawDebris(Debris *d);
static void drawString(char* s);
static void drawScore();
static void drawUpgradeText();
static void drawUpgrade();
static void drawTitle();
static void drawHelp();
static void drawShipSelect();
static void drawLevelText();

/* General functions that are run on every tick. Used to increment/decrement values or check conditions */
static void incrementOscillation();
static void advanceDust();
static void advanceDebris();
static void advanceShip();
static void advancePhoton();
static void advanceAsteroid();
static void advancePoints();
static void updateUpgradeText();
static void updateLevelText();
static void updateBackground();
static void updateRespawn();
static void updateDamage();
static void updateHighscore();
static void collisionAsteroidPhoton();
static void collisionAsteroidShip();
static void collisionDebrisShip();
static void lowerCooldown();

/* Helper functions used to provide mathematical equations simplified into a function */
static int lineCollision(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

/* -- Display list for drawing a circle ----------------------------------------------------------------- */

static GLuint	circle;

void
buildCircle(double x, double y, double r)
{
	/*
	 * build a circle at the coordinates (x, y). "r" determines the radius of the circle
	 */
	GLint   i;

    circle = glGenLists(1);
    glNewList(circle, GL_COMPILE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
    	for(i=0; i<40; i++)
    		glVertex2d(x + cos(i*M_PI/20.0)*r, y + sin(i*M_PI/20.0)*r);
    glEnd();
    glEndList();
}


/* --- Initilization/Destroy functions ------------------------------------------------------------------ */

void
initAsteroids(Player *newPlayer)
{
	/*
	 * Initilize variables and structures that are used for the asteroids mini-game
	 */
	//Set the oscilatting value to begin at 0
	oscillating = 0;

	//set a star background by generating random dots within a grid
	initBackground(xMax, yMax);

	/* Set the score digit tracker to 0 and set the default highscore to 10 thousand */
	player.asteroidsHighScore = 100;
	scoreDigits = 0;

	/* initilize the score tracking variables */
	scoreAL = 0;
	scoreAM = 0;
	scoreAS = 0;
	debrisM = 0;
	debrisA = 0;
	metalCount = 0;
	alloyCount = 0;
}

void
initShip()
{
	/*
	 * Initilize/reset the ship's values back to their default
	 */
	ship.LHp = 0;
	ship.RHp = 0;
	ship.BHp = 0;
	ship.LDmg = -1;
	ship.RDmg = -1;
	ship.BDmg = -1;
	ship.shipUpgrade = 0;
	ship.shipSpeed = 1;
	ship.shipControl = 0.05;
}

void
initAsteroid( Asteroid *a, int s)
{
    /*
     *	generate an asteroid at the screen edges. velocity, rotational
     *	velocity, and shape are generated randomly; size serves as a scale
     *	parameter that allows generating asteroids of different sizes; feel
     *	free to adjust the parameters according to your needs
     */
    double	theta, r;
    int		i;

    /* Set the asteroid's position to the screen edges: Either left side or on top*/
    a->size = s;
    if(rand()%2){
        a->x = myRandom(0, xMax);
        a->y = -1*a->size*MAX_ASTEROID_VARIANCE;
    }else{
        a->x = -1*a->size*MAX_ASTEROID_VARIANCE;
        a->y = myRandom(0, yMax);
    }

    /* Prevent the asteroid from starting with a low velocity to allow it to come into view once created */
    a->dx = myRandom(0.2, 0.8);
    a->dy = myRandom(0.2, 0.8);
    if(rand()%2){
    	a->dx *= -1;
    }
    if(rand()%2){
       	a->dy *= -1;
    }

    /* Give the asteroid an amount of vertexes relative to it's size */
    a->dphi = myRandom(-0.2, 0.2);
    a->nVertices = 3+a->size+rand()%(MAX_VERTICES-3-a->size);
    for (i=0; i<a->nVertices; i++){
    	theta = 2.0*M_PI*i/a->nVertices;
    	//The randomness of the vertices' distance from the origin will be contained within two global variance variables
    	r = a->size*myRandom(MIN_ASTEROID_VARIANCE, MAX_ASTEROID_VARIANCE);
    	a->coords[i].x = -r*sin(theta);
    	a->coords[i].y = r*cos(theta);
    }
    a->active = 1;
}

void
initDust(Asteroid *a, Asteroid *b)
{
    /*
     * Create a random amount of dust particles between two asteroids.
     * Will simulate an asteroid exploding if given the same asteroid twice
     */
	int dustCount, i;
    double x, y, dx, dy;

    /* Get the point of origin for the dust particles to be between both asteroids */
    x = (a->x + b->x)/2;
    y = (a->y + b->y)/2;
    dx = (a->dx + b->dx)/2.0;
    dy = (a->dy + b->dy)/2.0;
    /* The dust count is directly proportional to asteroid size and uses values relative to the asteroid's */
    dustCount = a->size + b->size;
    for(i = 0; i < MAX_DUST; i++){
        if(dust[i].active != 1){
            dust[i].x = x;
            dust[i].y = y;
            dust[i].dx = myRandom(-1.3, 1.3)*dx;
            dust[i].dy = myRandom(-1.3, 1.3)*dy;
            dust[i].active = 1;
            dust[i].lifetime = 100;
            dustCount--;
            if(dustCount <= 0){
                i = MAX_DUST;
            }
        }
    }
}

void
initBackground(double x, double y)
{
	/*
	 * Generate a starry background within the frame of x and y
	 */
	int i, j, starCount;
	starCount = 0;

	for(i = 0; i < MAX_STARS; i++){
	     for(j = 0; j < MAX_STARS; j++){
	       backgroundStars[starCount].x = (i+myRandom(-0.2, 1.2))*(x/MAX_STARS);
           backgroundStars[starCount].y = (j+myRandom(-0.2, 1.2))*(y/MAX_STARS);
           backgroundStars[starCount].active = 1;
           backgroundStars[starCount].flicker = myRandom(0.0, 2*M_PI);
           backgroundStars[starCount].flickerRate = myRandom(0.025, 0.125);
           starCount++;
        }
    }
}

void
destroyShip()
{
	/*
	 * Destroy either a piece of the current player's ship if it's type 0, or the entire ship for type 1 and 2.
	 * A Dmg value can't reach a negative number (it stops at 0) unless its specifically set to negative.
	 * Therefore, we know a ship piece has been destroyed/made as debris once we set their Dmg value to negative
	 */
	int i, count;
	Debris *piece;
	piece = NULL;

	/* each ship type gets destroyed in it's own way */
	if(ship.type == 0){
		/* Check if any other ship parts have been removed yet */
		if(ship.LDmg >= 0 && ship.RDmg >= 0 && ship.BDmg >= 0){
			if(ship.LHp <= 0){
				/* Create left-side ship debris by using the inactive debris pieces and values from the ship */
				for(i = 0; i < MAX_DEBRIS; i++){
					if(!(debris[i].active)){
						piece = &debris[i];
						i = MAX_DEBRIS;
					}
				}
				if(piece != NULL){
					piece->x = ship.x;
					piece->y = ship.y;
					piece->dx = myRandom(-0.25, 0.25)*ship.dx + myRandom(-0.1, 0.1);
					piece->dy = myRandom(-0.25, 0.25)*ship.dy + myRandom(-0.1, 0.1);
					piece->phi = ship.phi;
					piece->dphi = myRandom(-0.2, 0.2);
					piece->coords[0].x = 0.0;
					piece->coords[0].y = 0.0;
					piece->coords[1].x = ship.size*sin(ship.phi);
					piece->coords[1].y = ship.size*cos(ship.phi);
					piece->coords[2].x = sqrt(ship.size)*sin(ship.phi + (225*M_PI/180));
					piece->coords[2].y = sqrt(ship.size)*cos(ship.phi + (225*M_PI/180));
					piece->lifetime = 125*myRandom(0.95, 1.05);
					piece->active = 1;
					piece->type = 0;
				}
				ship.LDmg = -1;
			}

			piece = NULL;
			if(ship.RHp <= 0){
				/* Create right-side ship debris by using the inactive debris pieces and values from the ship */
				for(i = 0; i < MAX_DEBRIS; i++){
					if(!(debris[i].active)){
						piece = &debris[i];
						i = MAX_DEBRIS;
					}
				}
				if(piece != NULL){
					piece->x = ship.x;
					piece->y = ship.y;
					piece->dx = myRandom(-0.25, 0.25)*ship.dx + myRandom(-0.1, 0.1);
					piece->dy = myRandom(-0.25, 0.25)*ship.dy + myRandom(-0.1, 0.1);
					piece->phi = ship.phi;
					piece->dphi = myRandom(-0.2, 0.2);
					piece->coords[0].x = 0.0;
					piece->coords[0].y = 0.0;
					piece->coords[1].x = ship.size*sin(ship.phi);
					piece->coords[1].y = ship.size*cos(ship.phi);
					piece->coords[2].x = sqrt(ship.size)*sin(ship.phi + (135*M_PI/180));
					piece->coords[2].y = sqrt(ship.size)*cos(ship.phi + (135*M_PI/180));
					piece->lifetime = 125*myRandom(0.95, 1.05);
					piece->active = 1;
					piece->type = 0;
				}
				ship.RDmg = -1;
			}

			piece = NULL;
			if(ship.BHp <= 0){
				/* Create back-side ship debris by using the inactive debris pieces and values from the ship */
				for(i = 0; i < MAX_DEBRIS; i++){
					if(!(debris[i].active)){
						piece = &debris[i];
						i = MAX_DEBRIS;
					}
				}
				if(piece != NULL){
					piece->x = ship.x;
					piece->y = ship.y;
					piece->dx = myRandom(-0.25, 0.25)*ship.dx + myRandom(-0.1, 0.1);
					piece->dy = myRandom(-0.25, 0.25)*ship.dy + myRandom(-0.1, 0.1);
					piece->phi = ship.phi;
					piece->dphi = myRandom(-0.2, 0.2);
					piece->coords[0].x = 0.0;
					piece->coords[0].y = 0.0;
					piece->coords[1].x = sqrt(ship.size)*sin(ship.phi + (135*M_PI/180));
					piece->coords[1].y = sqrt(ship.size)*cos(ship.phi + (135*M_PI/180));
					piece->coords[2].x = sqrt(ship.size)*sin(ship.phi + (225*M_PI/180));
					piece->coords[2].y = sqrt(ship.size)*cos(ship.phi + (225*M_PI/180));
					piece->lifetime = 125*myRandom(0.95, 1.05);
					piece->active = 1;
					piece->type = 0;
				}
				ship.BDmg = -1;
			}
		}else{
			/* Destroy what's left of the ship pieces that are still attached if there
			 * is already a piece gone to prevent the ship being a single piece */
			piece = NULL;
			if(ship.LDmg >= 0){
				/* Create left-side ship debris by using the inactive debris pieces and values from the ship */
				for(i = 0; i < MAX_DEBRIS; i++){
					if(!(debris[i].active)){
						piece = &debris[i];
						i = MAX_DEBRIS;
					}
				}
				if(piece != NULL){
					piece->x = ship.x;
					piece->y = ship.y;
					piece->dx = myRandom(-0.25, 0.25)*ship.dx + myRandom(-0.1, 0.1);
					piece->dy = myRandom(-0.25, 0.25)*ship.dy + myRandom(-0.1, 0.1);
					piece->phi = ship.phi;
					piece->dphi = myRandom(-0.2, 0.2);
					piece->coords[0].x = 0.0;
					piece->coords[0].y = 0.0;
					piece->coords[1].x = ship.size*sin(ship.phi);
					piece->coords[1].y = ship.size*cos(ship.phi);
					piece->coords[2].x = sqrt(ship.size)*sin(ship.phi + (225*M_PI/180));
					piece->coords[2].y = sqrt(ship.size)*cos(ship.phi + (225*M_PI/180));
					piece->lifetime = 125*myRandom(0.95, 1.05);
					piece->active = 1;
					piece->type = 0;
				}
				ship.LDmg = -1;
			}

			piece = NULL;
			if(ship.RDmg >= 0){
				/* Create right-side ship debris by using the inactive debris pieces and values from the ship */
				for(i = 0; i < MAX_DEBRIS; i++){
					if(!(debris[i].active)){
						piece = &debris[i];
						i = MAX_DEBRIS;
					}
				}
				if(piece != NULL){
					piece->x = ship.x;
					piece->y = ship.y;
					piece->dx = myRandom(-0.25, 0.25)*ship.dx + myRandom(-0.1, 0.1);
					piece->dy = myRandom(-0.25, 0.25)*ship.dy + myRandom(-0.1, 0.1);
					piece->phi = ship.phi;
					piece->dphi = myRandom(-0.2, 0.2);
					piece->coords[0].x = 0.0;
					piece->coords[0].y = 0.0;
					piece->coords[1].x = ship.size*sin(ship.phi);
					piece->coords[1].y = ship.size*cos(ship.phi);
					piece->coords[2].x = sqrt(ship.size)*sin(ship.phi + (135*M_PI/180));
					piece->coords[2].y = sqrt(ship.size)*cos(ship.phi + (135*M_PI/180));
					piece->lifetime = 125*myRandom(0.95, 1.05);
					piece->active = 1;
					piece->type = 0;
				}
				ship.RDmg = -1;
			}

			piece = NULL;
			if(ship.BDmg >= 0){
				/* Create back-side ship debris by using the inactive debris pieces and values from the ship */
				for(i = 0; i < MAX_DEBRIS; i++){
					if(!(debris[i].active)){
						piece = &debris[i];
						i = MAX_DEBRIS;
					}
				}
				if(piece != NULL){
					piece->x = ship.x;
					piece->y = ship.y;
					piece->dx = myRandom(-0.25, 0.25)*ship.dx + myRandom(-0.1, 0.1);
					piece->dy = myRandom(-0.25, 0.25)*ship.dy + myRandom(-0.1, 0.1);
					piece->phi = ship.phi;
					piece->dphi = myRandom(-0.2, 0.2);
					piece->coords[0].x = 0.0;
					piece->coords[0].y = 0.0;
					piece->coords[1].x = sqrt(ship.size)*sin(ship.phi + (135*M_PI/180));
					piece->coords[1].y = sqrt(ship.size)*cos(ship.phi + (135*M_PI/180));
					piece->coords[2].x = sqrt(ship.size)*sin(ship.phi + (225*M_PI/180));
					piece->coords[2].y = sqrt(ship.size)*cos(ship.phi + (225*M_PI/180));
					piece->lifetime = 125*myRandom(0.95, 1.05);
					piece->active = 1;
					piece->type = 0;
				}
			}
			/* set the ship's health values to 0 and set the respawn
			 * time variable to start counting down to the ship respawn */
			ship.LHp = 0;
			ship.RHp = 0;
			ship.BHp = 0;
			respawn = 200;
		}
	}
	else if(ship.type == 1){
		/* destroy the ship by creating 4 debris pieces floating into each direction */
		/* get the next non-active available debris piece and track the amount of pieces created */
		count = 0;
		for(i = 0; i < MAX_DEBRIS; i++){
			if(!(debris[i].active)){
				piece = &debris[i];
				piece->x = ship.x;
				piece->y = ship.y;
				/* Pieces are generated starting from the right and going clockwise */
				piece->dx = (count == 0)*0.3 - (count == 2)*0.3 + 0.5*ship.dx + myRandom(-0.1, 0.1) ;
				piece->dy = (count == 3)*0.3 - (count == 1)*0.3 + 0.5*ship.dy + myRandom(-0.1, 0.1);
				piece->coords[1].x = sqrt(ship.size) - (count == 0 || count == 1)*2*sqrt(ship.size);
				piece->coords[1].y = sqrt(ship.size) - (count == 1 || count == 2)*2*sqrt(ship.size);
				piece->coords[2].x = sqrt(ship.size) - (count == 1 || count == 2)*2*sqrt(ship.size);
				piece->coords[2].y = sqrt(ship.size) - (count == 2 || count == 3)*2*sqrt(ship.size);
				piece->phi = ship.phi;
				piece->dphi = myRandom(-0.2, 0.2);
				piece->coords[0].x = 0.0;
				piece->coords[0].y = 0.0;
				piece->lifetime = 125*myRandom(0.95, 1.05);
				piece->active = 1;
				piece->type = 0;

				count++;
				if(count >= 4){
					i = MAX_DEBRIS;
				}
			}
		}
		/* set the ship's health value to 0 and set the respawn time variable */
		ship.BHp = 0;
		ship.BDmg = -1;
		respawn = 200;
	}else if(ship.type == 2){
		/* destroy the ship by filling the debris array with ship pieces, overriding any active debris pieces */
		for(i = 0; i < MAX_DEBRIS; i++){
			piece = &debris[i];
			piece->dx = myRandom(-0.15, 0.15)*ship.dx + myRandom(-0.3, 0.3);
			piece->dy = myRandom(-0.15, 0.15)*ship.dy + myRandom(-0.3, 0.3);
			piece->x = ship.x + piece->dx*10;
			piece->y = ship.y + piece->dy*10;
			piece->phi = ship.phi;
			piece->dphi = myRandom(-0.3, 0.3);
			piece->coords[0].x = 0 + myRandom(-0.5, 0.5);
			piece->coords[0].y = 2 + myRandom(-0.5, 0.5);
			piece->coords[1].x = sqrt(2) + myRandom(-0.5, 0.5);
			piece->coords[1].y = -sqrt(2) + myRandom(-0.5, 0.5);;
			piece->coords[2].x = -sqrt(2) + myRandom(-0.5, 0.5);;
			piece->coords[2].y = -sqrt(2) + myRandom(-0.5, 0.5);;
			piece->lifetime = 125*myRandom(0.85, 1.15);
			piece->active = 1;
			piece->type = 0;
		}
		/* set the ship's health values to 0 and start the respawn time variable */
		ship.BHp = 0;
		ship.LHp = 0;
		ship.RHp = 0;
		ship.BDmg = -1;
		ship.LDmg = -1;
		respawn =200;
	}
}

void
destroyAsteroid(Asteroid *a, Photon *p)
{
	/*
	 * when an asteroid is shot, reduce it's size by creating a smaller asteroid in it's place.
	 * When a "small" asteroid is shot, it is fully destroyed. Use values from the photon
	 * that destroyed the asteroid to help build the new asteroids.
	 * The original asteroid's velocity has the photon's velocity transfered into it.
	 * The children asteroids will mostly retain a random amount of the
	 * photons velocity along with a fraction of the parent's velocity.
	 */
	int space;
	int children;
	int theta;
	int r;
	int i;
	int j;
	int debrisCount;
	p->active = 0;

	/* give the user points relative to the asteroid's current size and update the appropriate score tracking value */
	addScore(a->size, a->x, a->y);
	if(a->size == ASTEROID_LARGE){
		scoreAL++;
	}else if(a->size == ASTEROID_MEDIUM){
		scoreAM++;
	}else if(a->size == ASTEROID_SMALL){
		scoreAS++;
	}

	if(a->size == ASTEROID_SMALL){
		initDust(a, a);
		a->active = 0;
		children = 0;
		/* create 3 or 2 debris objects to simulate the asteroid being destroyed. each debris piece is made of three vertexes */
		debrisCount = floor(myRandom(2.5, 3.5));
		for(i = 0; i < MAX_DEBRIS; i++){
			if(!(debris[i].active)){
				debris[i].active = 1;
				//The debris type has a 10% chance to be an upgrade
				debris[i].type = 1 + (myRandom(0.0, 1.0) > 0.9);
				debris[i].dx = (myRandom(0.25, 0.55)*a->dx + myRandom(-0.1, 0.1))*sin(debrisCount*2*M_PI/3);
				debris[i].dy = (myRandom(0.25, 0.55)*a->dy + myRandom(-0.1, 0.1))*cos(debrisCount*2*M_PI/3);
				debris[i].x = a->x + debris[i].dx*3;
				debris[i].y = a->y + debris[i].dx*3;
				debris[i].phi = myRandom(0.0, M_PI);
				debris[i].dphi = myRandom(-0.2, 0.2);
				debris[i].coords[0].x = 0.0 + myRandom(-0.5, 0.5);
				debris[i].coords[0].y = 2.0 + myRandom(-0.5, 0.5);
				debris[i].coords[1].x = -sqrt(2.0) + myRandom(-0.5, 0.5);
				debris[i].coords[1].y = -sqrt(2.0) + myRandom(-0.5, 0.5);
				debris[i].coords[2].x = sqrt(2.0) + myRandom(-0.5, 0.5);
				debris[i].coords[2].y = -sqrt(2.0) + myRandom(-0.5, 0.5);
				debris[i].lifetime = 125*myRandom(0.75, 1.25);
				debrisCount--;
			}
			if(debrisCount <= 0){
				i = MAX_DEBRIS;
			}
		}

	/* Set asteroid parameters used with child asteroid creation */
	}else if(a->size == ASTEROID_MEDIUM){
		a->size = ASTEROID_SMALL;
		children = floor(myRandom(1.2, 2.2));
	}else if(a->size == ASTEROID_LARGE){
		a->size = ASTEROID_MEDIUM;
		children = floor(myRandom(2.3, 3.3));
	}

	/*reconstruct the parent asteroid using it's new size */
	a->nVertices = 3+a->size+rand()%(MAX_VERTICES-3-a->size);
	for (i=0; i<a->nVertices; i++){
		theta = 2.0*M_PI*i/a->nVertices;
		r = a->size*myRandom(MIN_ASTEROID_VARIANCE, MAX_ASTEROID_VARIANCE);
		a->coords[i].x = -r*sin(theta);
		a->coords[i].y = r*cos(theta);

	}

	/* depending on the velocity of the photon shot and the current size of the asteroid,
	 * change the asteroid's velocity slightly. This is to simulate an actual "collision" */
	a->dx = (a->dx*a->size + p->dx*photonSize/2) / a->size;
	a->dy = (a->dy*a->size + p->dy*photonSize/2) / a->size;

	/* prevent the asteroid's speed from going above a reasonable amount */
	if(a->dx > 1){
		a->dx = 1;
	}else if(a->dx < -1){
		a->dx = -1;
	}
	if(a->dy > 1){
		a->dy = -1;
	}else if(a->dy < -1){
		a->dy = -1;
	}

	/* Create a child asteroid if there's memory and the original asteroid was large enough (size is not "small") */
	Asteroid *child;

	/* check the asteroids array to see how many children asteroids
	 * will be added compared to how much space is left in the array.
	 * Change the values to find out how many children will be made */
	space = 0;
	for(i = 0; i < MAX_ASTEROIDS; i++){
		child = &asteroids[i];
		if(!(child->active)){
			space++;
		}
	}
	if(children > space){
		children = space;
	}else{
		space = children;
	}

	/* Use inactive asteroids to create children asteroids using their parent's values */
	for(i = 0; i < MAX_ASTEROIDS; i++){
		child = &asteroids[i];
		if(!(child->active)){
			if(space > 0){
				child->size = a->size;
				/* add some distance relative to the photon's direction to
				 * the child's position so all asteroids are not overlapping */
				if(p->dy < 0){
					child->x = a->x - sin(atan(p->dx/p->dy) + M_PI/2 - space*M_PI/(1+children))*(child->size + a->size);
					child->y = a->y - cos(atan(p->dx/p->dy) + M_PI/2 - space*M_PI/(1+children))*(child->size + a->size);
				}else{
					child->x = a->x + sin(atan(p->dx/p->dy) + M_PI/2 - space*M_PI/(1+children))*(child->size + a->size);
					child->y = a->y + cos(atan(p->dx/p->dy) + M_PI/2 - space*M_PI/(1+children))*(child->size + a->size);
				}
				/* inherent a portion of its parents rotational speed and prevent it from spinning too fast */
				child->dphi = a->dphi*myRandom(0.5, 1.4);
				if(child->dphi > 0.2){
				    child->dphi = 0.2;
				}else if(child->dphi < -0.2){
				    child->dphi = -0.2;
				}
				/* Use the photon's speed, the parent's speed and position of origin to determine the child's velocity */
				if(a->y - child->y >= 0){
					child->dx = -sin(atan((a->x - child->x) / (a->y - child->y)))/child->size + a->dx;
					child->dy = -cos(atan((a->x - child->x) / (a->y - child->y)))/child->size + a->dy;
				}else{
					child->dx = sin(atan((a->x - child->x) / (a->y - child->y)))/child->size + a->dx;
					child->dy = cos(atan((a->x - child->x) / (a->y - child->y)))/child->size + a->dy;
				}
				/* Create the child's vertexes */
				child->phi = a->phi;
				child->nVertices = 6+rand()%(MAX_VERTICES-6);
				initDust(child, a);
				child->active = 1;
				for (j=0; j<child->nVertices; j++){
					theta = 2.0*M_PI*j/child->nVertices;
					r = child->size*myRandom(MIN_ASTEROID_VARIANCE, MAX_ASTEROID_VARIANCE);
					child->coords[j].x = -r*sin(theta);
					child->coords[j].y = r*cos(theta);
				}
				space--;
			}
		}
	}
}

void
destroyDebris(Debris *d)
{
	/*
	 * Destroy the debris and spawn a group of space dust in it's place.
	 */
	int i, dustCount;

	dustCount = myRandom(8, 15);
	for(i = 0; i < MAX_DUST; i++){
		if(dust[i].active != 1){
			dustCount--;
			dust[i].active = 1;
			dust[i].dx = myRandom(-1.0, 1.0);
			dust[i].dy = myRandom(-1.0, 1.0);
			dust[i].x = d->x + dust[i].dx*3;
			dust[i].y = d->y + dust[i].dy*3;
			dust[i].lifetime = 45;
			if(dustCount <= 0){
				i = MAX_DUST;
			}
		}
	}
	d->active = 0;
}


/* --- Drawing functions -------------------------------------------------------------------------- */

void
drawPoints(Points *p)
{
    /*
     * Draw a floating integer and a + symbol that represents the amount of points the user gained from an action
     */
	int i, s;

	glColor3f(1, 1, 1);
	glRasterPos2f(p->x, p->y);
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, '+');

	i = 0;
	for(s = p->amount; s > 0; s = s/10){
		i++;
	}
	for(s = p->amount; i > 0; i--){
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (s/((int) pow(10,i-1)))%(10) + '0');
	}
}

void
drawShip()
{
	/*
	 * draw the current player's ship's vertices around it's origin point. The ship will be within
	 * a circle with radius "s->size" around it's origin. Each ship  type has a
	 * different rendering style. When the ship takes damage, (when  a xDmg variable
	 * is not 0), have the piece associated with it flicker. A ship's color will turn
	 * red when the piece is bellow 100Hp. If the Hp is 0, do not draw then piece.
	 */
	int i, k;
	double r;

	if(ship.type == 0){
		/* draw the left triangle piece of the ship */
		if(ship.LHp > 0){
			glColor3f((2.0/ship.LHp + sin(oscillating)*15.0/ship.LHp + ship.LHp/100.0)/(1 + ship.LDmg%2),
					(ship.LHp/100.0)/(1 + ship.LDmg%2),
					(ship.LHp/100.0)/(1 + ship.LDmg%2));
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBegin(GL_TRIANGLE_FAN);
			glVertex2d(ship.x, ship.y);
			glVertex2d(ship.x + ship.size*sin(ship.phi), ship.y + ship.size*cos(ship.phi));
			glVertex2d(ship.x + sqrt(ship.size)*sin(ship.phi + (225*M_PI/180)),
					ship.y + sqrt(ship.size)*cos(ship.phi + (225*M_PI/180)));
			glEnd();
			glFlush();
		}
		/* draw the right triangle piece of the ship */
		if(ship.RHp > 0){
			glColor3f((2.0/ship.RHp + sin(oscillating)*15.0/ship.RHp + ship.RHp/100.0)/(1 + ship.RDmg%2),
					(ship.RHp/100.0)/(1 + ship.RDmg%2),
					(ship.RHp/100.0)/(1 + ship.RDmg%2));
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBegin(GL_TRIANGLE_FAN);
			glVertex2d(ship.x, ship.y);
			glVertex2d(ship.x + ship.size*sin(ship.phi), ship.y + ship.size*cos(ship.phi));
			glVertex2d(ship.x + sqrt(ship.size)*sin(ship.phi + (135*M_PI/180)), ship.y + sqrt(ship.size)*cos(ship.phi + (135*M_PI/180)));
			glEnd();
			glFlush();
		}
		/* draw the back triangle piece of the ship */
		if(ship.BHp > 0){
			glColor3f((2.0/ship.BHp + sin(oscillating)*15.0/ship.BHp + ship.BHp/100.0)/(1 + ship.BDmg%2),
					(ship.BHp/100.0)/(1 + ship.BDmg%2),
					(ship.BHp/100.0)/(1 + ship.BDmg%2));
			glBegin(GL_TRIANGLE_FAN);
			glVertex2d(ship.x, ship.y);
			glVertex2d(ship.x + sqrt(ship.size)*sin(ship.phi + (135*M_PI/180)), ship.y + sqrt(ship.size)*cos(ship.phi + (135*M_PI/180)));
			glVertex2d(ship.x + sqrt(ship.size)*sin(ship.phi + (225*M_PI/180)), ship.y + sqrt(ship.size)*cos(ship.phi + (225*M_PI/180)));
			glEnd();
			glFlush();
		}
	}else if(ship.type == 1){
		/* Draw a cube made of 4 triangles, starting from the top right vertex and going clockwise */
		if(ship.BHp > 0){
			glColor3f((2.0/ship.BHp + sin(oscillating)*15.0/ship.BHp + ship.BHp/100.0)/(1 + ship.BDmg%2),
						(ship.BHp/100.0)/(1 + ship.BDmg%2),
						(ship.BHp/100.0)/(1 + ship.BDmg%2));
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBegin(GL_POLYGON);
			glVertex2d(ship.x + sqrt(ship.size), ship.y + sqrt(ship.size));
			glVertex2d(ship.x - sqrt(ship.size), ship.y + sqrt(ship.size));
			glVertex2d(ship.x - sqrt(ship.size), ship.y - sqrt(ship.size));
			glVertex2d(ship.x + sqrt(ship.size), ship.y - sqrt(ship.size));
			glEnd();
			glFlush();
		}
	}else if(ship.type == 2){
		if(ship.BHp > 0){
			/* draw the ship's shields if they are not recharging */
			if(ship.LDmg > 0){
				/* To indicate how much shields are left, there will be 3 shield layers that change colors and dissapear */
				for(i = 0; i <= 3; i++){
					if(i*ship.RHp/3 < ship.LHp){
						/* draw the i+1th layer. The color indicates how much health is left in the layer */
						glColor3f((1-(ship.LHp - (i*ship.RHp/3))/(ship.RHp/3)), 0.0, (ship.LHp - (i*ship.RHp/3))/(ship.RHp/3));
						circle = glGenLists(1);
						glNewList(circle, GL_COMPILE);
						glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
						glBegin(GL_POLYGON);
						//Have the shield retract back into the ship when it's about to start recharging
						r = (ship.size + (i+1)) - (ship.LDmg < 30)*(1 - (ship.LDmg/30.0))*(ship.size);
						for(k=0; k<40; k++)
							glVertex2d(ship.x + cos(k*M_PI/20.0)*r + (ship.LHp < (i+1)*ship.RHp/3)*(1-(ship.LHp - (i*ship.RHp/3))/(ship.RHp/3))*myRandom(-0.5, 0.5) + myRandom(-0.1, 0.1)
									, ship.y + sin(k*M_PI/20.0)*r + (ship.LHp < (i+1)*ship.RHp/3)*(1-(ship.LHp - (i*ship.RHp/3))/(ship.RHp/3))*myRandom(-0.5, 0.5) + myRandom(-0.1, 0.1));
						glEnd();
						glEndList();
						drawCircle();
					}
				}
			}
			/* Draw the ship itself, a large shape made of 13 outside vertexes. */
			glColor3f((2.0/ship.BHp + sin(oscillating)*15.0/ship.BHp + ship.BHp/100.0)/(1 + ship.BDmg%2),
					(ship.BHp/100.0)/(1 + ship.BDmg%2),
					(ship.BHp/100.0)/(1 + ship.BDmg%2));
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBegin(GL_TRIANGLES);

			/* List of all the vertexes used. 0 is in the lower middle to connect most vertexes.
			 * From 1 and onward, they are listed in clockwise order, starting with the nose of the ship */
			/*
			glVertex2d(ship.x + ship.size*sin(ship.phi), ship.y - ship.size/2*cos(ship.phi)); //0
			glVertex2d(ship.x + ship.size*sin(ship.phi), ship.y + ship.size*cos(ship.phi)); //1
			glVertex2d(ship.x + sqrt(ship.size)*cos(ship.phi), ship.y - sqrt(ship.size)*sin(ship.phi)); //2
			glVertex2d(ship.x + sqrt(ship.size*2)*cos(ship.phi), ship.y - sqrt(ship.size*2)*sin(ship.phi)); //3
			glVertex2d(ship.x + ship.size*cos(ship.phi + (330*M_PI/180)), ship.y - ship.size*sin(ship.phi + (330*M_PI/180))); //4
			glVertex2d(ship.x + ship.size*cos(ship.phi), ship.y - ship.size*sin(ship.phi)); //5
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (135*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (135*M_PI/180))); //6
			glVertex2d(ship.x + ship.size*sin(ship.phi + (150*M_PI/180)), ship.y + ship.size*cos(ship.phi + (150*M_PI/180))); //7
			glVertex2d(ship.x + ship.size*sin(ship.phi + (210*M_PI/180)), ship.y + ship.size*cos(ship.phi + (210*M_PI/180))); //8
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (225*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (225*M_PI/180))); //9
			glVertex2d(ship.x - ship.size*cos(ship.phi), ship.y + ship.size*sin(ship.phi)); //10
			glVertex2d(ship.x - ship.size*cos(ship.phi + (30*M_PI/180)), ship.y + ship.size*sin(ship.phi + (30*M_PI/180))); //11
			glVertex2d(ship.x - sqrt(ship.size*2)*cos(ship.phi), ship.y + sqrt(ship.size*2)*sin(ship.phi)); //12
			glVertex2d(ship.x - sqrt(ship.size)*cos(ship.phi), ship.y + sqrt(ship.size)*sin(ship.phi)); //13
			*/

			/*
			 * Draw out of the ship out of triangles by using many sets of 3 vertexes
			 */
			/* 1-2-13 */
			glVertex2d(ship.x + ship.size*sin(ship.phi), ship.y + ship.size*cos(ship.phi));
			glVertex2d(ship.x + sqrt(ship.size)*cos(ship.phi), ship.y - sqrt(ship.size)*sin(ship.phi));
			glVertex2d(ship.x - sqrt(ship.size)*cos(ship.phi), ship.y + sqrt(ship.size)*sin(ship.phi));
			/* 2-5-6 */
			glVertex2d(ship.x + sqrt(ship.size)*cos(ship.phi), ship.y - sqrt(ship.size)*sin(ship.phi));
			glVertex2d(ship.x + ship.size*cos(ship.phi), ship.y - ship.size*sin(ship.phi));
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (135*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (135*M_PI/180)));
			/* 3-4-5 */
			glVertex2d(ship.x + sqrt(ship.size*2)*cos(ship.phi), ship.y - sqrt(ship.size*2)*sin(ship.phi));
			glVertex2d(ship.x + ship.size*cos(ship.phi + (330*M_PI/180)), ship.y - ship.size*sin(ship.phi + (330*M_PI/180)));
			glVertex2d(ship.x + ship.size*cos(ship.phi), ship.y - ship.size*sin(ship.phi));
			/* 6-7-8 */
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (135*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (135*M_PI/180)));
			glVertex2d(ship.x + ship.size*sin(ship.phi + (150*M_PI/180)), ship.y + ship.size*cos(ship.phi + (150*M_PI/180)));
			glVertex2d(ship.x + ship.size*sin(ship.phi + (210*M_PI/180)), ship.y + ship.size*cos(ship.phi + (210*M_PI/180)));
			/* 6-8-9 */
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (135*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (135*M_PI/180)));
			glVertex2d(ship.x + ship.size*sin(ship.phi + (210*M_PI/180)), ship.y + ship.size*cos(ship.phi + (210*M_PI/180)));
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (225*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (225*M_PI/180)));
			/* 9-10-13 */
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (225*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (225*M_PI/180)));
			glVertex2d(ship.x - ship.size*cos(ship.phi), ship.y + ship.size*sin(ship.phi));
			glVertex2d(ship.x - sqrt(ship.size)*cos(ship.phi), ship.y + sqrt(ship.size)*sin(ship.phi));
			/* 10-11-12 */
			glVertex2d(ship.x - ship.size*cos(ship.phi), ship.y + ship.size*sin(ship.phi));
			glVertex2d(ship.x - ship.size*cos(ship.phi + (30*M_PI/180)), ship.y + ship.size*sin(ship.phi + (30*M_PI/180)));
			glVertex2d(ship.x - sqrt(ship.size*2)*cos(ship.phi), ship.y + sqrt(ship.size*2)*sin(ship.phi));
			/* 6-9-13 */
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (135*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (135*M_PI/180)));
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (225*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (225*M_PI/180)));
			glVertex2d(ship.x - sqrt(ship.size)*cos(ship.phi), ship.y + sqrt(ship.size)*sin(ship.phi));
			/* 2-6-13 */
			glVertex2d(ship.x + sqrt(ship.size)*cos(ship.phi), ship.y - sqrt(ship.size)*sin(ship.phi));
			glVertex2d(ship.x + ship.size*0.80*sin(ship.phi + (135*M_PI/180)), ship.y + ship.size*0.80*cos(ship.phi + (135*M_PI/180)));
			glVertex2d(ship.x - sqrt(ship.size)*cos(ship.phi), ship.y + sqrt(ship.size)*sin(ship.phi));

			glEnd();
			glFlush();
		}
	}
}

void
drawPhoton(Photon *p)
{
	/*
	 * Draw a photon with it's vertexes at a varying distance from the origin
	 */
	int i;

	glColor3f(0.0, 0.0, 1.0);
    circle = glGenLists(1);
    glNewList(circle, GL_COMPILE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_POLYGON);
    for(i=0; i<40; i++)
    	glVertex2d(p->x + cos(i*M_PI/20.0)*photonSize + myRandom(-0.1, 0.1)*photonSize, p->y + sin(i*M_PI/20.0)*photonSize + myRandom(-0.1, 0.1)*photonSize);
    glEnd();
    glEndList();
    drawCircle();
}

void
drawAsteroid(Asteroid *a)
{

	/*
	 * Draw two asteroids, an white outer shell and an inner black polygon, which is 90% the white shell's radius
	 */
	int i;

	/* Draw the white outer ring */
	glColor3f(1, 1, 1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBegin(GL_POLYGON);
	for(i = 0; i < a->nVertices; i++){
		glVertex2d(a->x + sqrt(pow(a->coords[i].x, 2) + pow(a->coords[i].y, 2))*sin((a->phi)+i*(2*M_PI)/a->nVertices)
				, a->y + sqrt(pow(a->coords[i].x, 2) + pow(a->coords[i].y, 2))*cos((a->phi)+i*(2*M_PI)/a->nVertices));
	}
	glEnd();
	glFlush();

	/* Draw the asteroid's inner black polygon */
	glColor3f(0, 0, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBegin(GL_POLYGON);
	for(i = 0; i < a->nVertices; i++){
		glVertex2d(a->x + sqrt(pow(a->coords[i].x*0.95, 2) + pow(a->coords[i].y*0.95, 2))*sin((a->phi)+i*(2*M_PI)/a->nVertices)
				, a->y + sqrt(pow(a->coords[i].x*0.95, 2) + pow(a->coords[i].y*0.95, 2))*cos((a->phi)+i*(2*M_PI)/a->nVertices));
	}
	glEnd();
	glFlush();

}

void
drawBackground(BackgroundStar *s)
{
	/*
	 * Draw a single pixel at the star's location and slowly fade in and out
	 */

    glColor3f(sin(s->flicker), sin(s->flicker), sin(s->flicker));
    glBegin(GL_POINTS);
    glVertex2i(s->x, s->y);
    glEnd();
    glFlush();
}

void
drawDust(Dust *d)
{
	/*
	 * Draw each unit of dust. Once a dust piece's lifetime reaches bellow 60 ticks, start fading
	 */

	glColor3f(d->lifetime/60.0, d->lifetime/60.0, d->lifetime/60.0);
	glBegin(GL_POINTS); // render with points
	glVertex2d(d->x, d->y);
	glEnd();
	glFlush();
}

void
drawDebris(Debris *d)
{
	/*
	 * Draw each debris piece. Debris has 3 vertexes and rotates in only 2D to simulate a 3D rotation
	 * Debris explodes into dust after it's lifetime expires. Each debris object has a type:
	 * 0 = Ship pieces. 1 = Asteroid pieces. 2 = upgrades. Asteroid pieces can be picked up for points
	 * while ship pieces will flash red and cannot be picked up. Upgrades can be picked up for
	 * a large sum of points along with a random upgrade to the user
	 */
	int i;

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBegin(GL_TRIANGLE_FAN);
	if(d->type == 0){
		//make the ship pieces oscillate a red flash like when the ship has low HP
		glColor3f((sin(oscillating) + 0.5), 0.5, 0.5);
	}
	if(d->type == 1){
		//give asteroid pieces a slight random shimmer
		glColor3f(1 - myRandom(0.0, 0.2), 1 - myRandom(0.0, 0.2), 1 - myRandom(0.0, 0.2));
	}
	if(d->type == 2){
		//make the upgrade pieces be gold and randomly shimmer to green
		glColor3f(0.85*myRandom(1.3, 0.7), 0.65*myRandom(1.3, 0.7), 0.15*myRandom(1.3, 0.7));
	}
	/* Draw the debri's vertexes */
	glBegin(GL_POLYGON);
	for(i = 0; i < 3; i++){
		glVertex2d(d->x + sin(d->phi)*d->coords[i].x,
				d->y + cos(d->phi)*d->coords[i].y);
	}
	glEnd();
	glFlush();
}

void
drawScore()
{
	/*
	 * Show the user how much metal and alloys thet have on the top left. This is done by finding the amount of
	 * digits in the score, then cycling through the score amount backwards to print it in the value correct order.
	 *
	 * The amount of metal and alloy a user has is they current amount + the amout they picked up so far in the current session.
	 * If they die, they wont gain the metal/alloy they picked up. They need to escape the asteroid field to keep the metal/alloy.
	 */
	int i, s;

	/* draw the user's total metal */
	glColor3f(1, 1, 1);
	glRasterPos2i(1, yMax - 2);
	drawString("metals:");
	i = 0;
	for(s = metalCount + player.metal; s > 0; s = s/10){
		i++;
	}
	for(s = metalCount + player.metal; i > 0; i--){
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (s/((int) pow(10,i-1)))%(10) + '0');
	}
	if(metalCount + player.metal == 0){
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, '0');
	}

	/* draw the user's total alloy */
	glColor3f(1, 1, 1);
	glRasterPos2i(1, yMax - 5);
	drawString("alloys:");

	i = 0;
	for(s = alloyCount + player.alloy; s > 0; s = s/10){
		i++;
	}
	for(s = alloyCount + player.alloy; i > 0; i--){
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, (s/((int) pow(10,i-1)))%(10) + '0');
	}
	if(alloyCount + player.alloy == 0){
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, '0');
	}
}

void
drawUpgradeText()
{
	/*
	 * Draw the string of characters currently saved in the text object used to indicate the obtained update
	 */

	if (upgradeText.active){
		glColor3f(1, 1, 1);
		glRasterPos2f(2, 2);
		drawString(upgradeText.msg);
	}
}

void
drawUpgrade()
{
	/*
	 * Draw an indicator for the user as to how many upgrades they have for the ship and photons
	 * by using green "0"s. Upgrades will only reach off-screen when the user shrinks the window
	 */
	int i;

	glColor3f(1, 1, 1);
	glRasterPos2f(xMax*0.70, yMax*0.98);
	drawString("Photon");
	glColor3f(0, 1, 0);
	glRasterPos2f(xMax*0.82, yMax*0.98);
	for(i = 0; i < photonUpgrade; i++){
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'O');
	}

	glColor3f(1, 1, 1);
	glRasterPos2f(xMax*0.70, yMax*0.95);
	drawString("  Ship");
	glColor3f(0, 1, 0);
	glRasterPos2f(xMax*0.82, yMax*0.95);
	for(i = 0; i < ship.shipUpgrade; i++){
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'O');
	}
}

void
drawTitle()
{
	/*
	 * Draw the titlescreen, which consists of the name of the game with flickering chracters and
	 * a menu with a couple options to select from. A ship indicates the selected menu option.
	 */
	int currentLetterCount, currentSpaceCount;
	double color, startPosition, textMaxHeight, textMinHeight, textWidth, spaceWidth;

	/* Draw the title of ASTEROIDS. Have it occupy 80% width, 50% height*/
	glLineWidth(2);
	glBegin(GL_LINES);
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	glVertex2f(xMax*0.05, yMax*0.9);
	glVertex2f(xMax*0.95, yMax*0.9);
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	glVertex2f(xMax*0.95, yMax*0.6);
	glVertex2f(xMax*0.05, yMax*0.6);

	/*
	 * The equation used to draw the current character's initial position:
	 * startPosition = 10 + 8*currentLetterCount + 1*currentSpaceCount
	 * where startPosition is the starting spot of the current character
	 * currentLetterCount is the character number in the string "asteroids"
	 * currentSpaceCount is the current space between characters
	 */
	textMaxHeight = 0.85;
	textMinHeight = 0.65;
	textWidth = 0.08;
	spaceWidth = 0.01;

	currentLetterCount = 0;
	currentSpaceCount = 0;
	startPosition = 0.10 + textWidth*currentLetterCount + spaceWidth*currentSpaceCount;
	/* Draw the A */
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	glVertex2f(xMax*startPosition, yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*2/3);
	glVertex2f(xMax*startPosition, yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*2/3);
	glVertex2f(xMax*(startPosition+textWidth/2), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth/2), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*2/3);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*2/3);
	glVertex2f(xMax*startPosition, yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)/2);
	currentLetterCount++;
	currentSpaceCount++;
	startPosition = 0.10 + 0.08*currentLetterCount + 0.01*currentSpaceCount;

	/* Draw the S */
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	currentLetterCount++;
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*startPosition, yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*startPosition, yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMaxHeight);
	currentSpaceCount++;
	startPosition = 0.10 + 0.08*currentLetterCount + 0.01*currentSpaceCount;

	/* Draw the T */
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	currentLetterCount++;
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth/2), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth/2), yMax*textMinHeight);
	currentSpaceCount++;
	startPosition = 0.10 + 0.08*currentLetterCount + 0.01*currentSpaceCount;

	/* Draw the E */
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	currentLetterCount++;
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*startPosition, yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMaxHeight);
	currentSpaceCount++;
	startPosition = 0.10 + 0.08*currentLetterCount + 0.01*currentSpaceCount;

	/* Draw the R */
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	currentLetterCount++;
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*startPosition, yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*(startPosition+textWidth/2), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	currentSpaceCount++;
	startPosition = 0.10 + 0.08*currentLetterCount + 0.01*currentSpaceCount;

	/* Draw the O */
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	currentLetterCount++;
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	currentSpaceCount++;
	startPosition = 0.10 + 0.08*currentLetterCount + 0.01*currentSpaceCount;

	/* Draw the I */
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	currentLetterCount++;
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth/2), yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth/2), yMax*textMinHeight);
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	currentSpaceCount++;
	startPosition = 0.10 + 0.08*currentLetterCount + 0.01*currentSpaceCount;

	/* Draw the D */
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	currentLetterCount++;
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*(textMinHeight + (textMaxHeight-textMinHeight)*3/4));
	glVertex2f(xMax*(startPosition+textWidth), yMax*(textMinHeight + (textMaxHeight-textMinHeight)*3/4));
	glVertex2f(xMax*(startPosition+textWidth), yMax*(textMinHeight + (textMaxHeight-textMinHeight)/4));
	glVertex2f(xMax*(startPosition+textWidth), yMax*(textMinHeight + (textMaxHeight-textMinHeight)/4));
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	currentSpaceCount++;
	startPosition = 0.10 + 0.08*currentLetterCount + 0.01*currentSpaceCount;

	/* Draw the S */
	color = myRandom(0.7, 1.5);
	glColor3f(color, color, color);
	currentLetterCount++;
	glVertex2f(xMax*startPosition, yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*startPosition, yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*startPosition, yMax*textMinHeight + yMax*(textMaxHeight-textMinHeight)*0.5);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*startPosition, yMax*textMaxHeight);
	glVertex2f(xMax*(startPosition+textWidth), yMax*textMaxHeight);
	currentSpaceCount++;
	startPosition = 0.10 + 0.08*currentLetterCount + 0.01*currentSpaceCount;

	glEnd();

	/* Draw the.selectable.menu options */
	glColor3f(1.0, 1.0, 1.0);
	glRasterPos2f(xMax/2, yMax*0.05);
	drawString("Help");
	glRasterPos2f(xMax/2, yMax*0.1);
	drawString("Start game");

	/* Draw a ship on an offset relative to the currently selected option */
	glColor3f(1.0, 1.0, 1.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBegin(GL_TRIANGLE_FAN);

	/* Use the proper syntax to draw a type 0 ship, but fill in the variables with set values dependent of screen size */
	glVertex2d(xMax/2-4, 0.1*yMax - selectedOption*0.05*yMax+0.5);
	glVertex2d(xMax/2-4 + 3*sin(M_PI/2), 0.1*yMax - selectedOption*0.05*yMax+0.5 + 3*cos(M_PI/2));
	glVertex2d(xMax/2-4 + sqrt(3)*sin(M_PI/2 + (225*M_PI/180)),
			0.1*yMax - selectedOption*0.05*yMax+0.5 + sqrt(3)*cos(M_PI/2 + (225*M_PI/180)));

	glVertex2d(xMax/2-4, 0.1*yMax - selectedOption*0.05*yMax+0.5);
	glVertex2d(xMax/2-4 + 3*sin(M_PI/2), 0.1*yMax - selectedOption*0.05*yMax+0.5 + 3*cos(M_PI/2));
	glVertex2d(xMax/2-4 + sqrt(3)*sin(M_PI/2 + (135*M_PI/180)), 0.1*yMax - selectedOption*0.05*yMax+0.5 + sqrt(3)*cos(M_PI/2 + (135*M_PI/180)));

	glVertex2d(xMax/2-4, 0.1*yMax - selectedOption*0.05*yMax+0.5);
	glVertex2d(xMax/2-4 + sqrt(3)*sin(M_PI/2 + (135*M_PI/180)), 0.1*yMax - selectedOption*0.05*yMax+0.5 + sqrt(3)*cos(M_PI/2 + (135*M_PI/180)));
	glVertex2d(xMax/2-4 + sqrt(3)*sin(M_PI/2 + (225*M_PI/180)), 0.1*yMax - selectedOption*0.05*yMax+0.5 + sqrt(3)*cos(M_PI/2 + (225*M_PI/180)));


	glEnd();
	glFlush();
}

void
drawHelp()
{
	/*
	 * Draw the text for the help screen.
	 */

	/* General game info */
	glColor3f(1.0, 1.0, 1.0);
	glRasterPos2i(1, yMax - 2);
	drawString("Welcome to Asteroids! You play as a commander of a ship");
	glRasterPos2i(1, yMax - 5);
	drawString("trying to survive in an active asteroid field. Use ");
	glRasterPos2i(1, yMax - 8);
	drawString("evasive maneuvers to dodge asteroids while collecting ");
	glRasterPos2i(1, yMax - 11);
	drawString("their rare metals extracted using your photon cannon.");
	glRasterPos2i(1, yMax - 14);
	drawString("Try to reach the top of the scoreboard if you dare!");

	/* Ship info */
	glRasterPos2i(11, yMax - 24);
	drawString("This is you! Control your ship using the arrow ");
	glRasterPos2i(11, yMax - 27);
	drawString("keys. You can test the movement on this screen.");

	/* Photon info */
	glRasterPos2i(1, yMax - 39);
	drawString("Once your in an asteroid field, you can fire");
	glRasterPos2i(1, yMax - 42);
	drawString("photon shots using the spacebar. Aim for the");
	glRasterPos2i(1, yMax - 45);
	drawString("asteroids to destroy them using them photons");

	/* Asteroid info */
	glRasterPos2i(17, yMax - 57);
	drawString("This is an asteroid. They come in a variety");
	glRasterPos2i(17, yMax - 60);
	drawString("of sizes. Damage from a photon shot will split");
	glRasterPos2i(17, yMax - 63);
	drawString("it into smaller asteroids. Once small enough,");
	glRasterPos2i(17, yMax - 66);
	drawString("a final photon shot will destroy it completly.");

	/* Debris info */
	glRasterPos2i(1, yMax - 78);
	drawString("When an asteroid is shot and is too small to be split,");
	glRasterPos2i(1, yMax - 81);
	drawString("the metals will be extracted and can be scooped up.");
	glRasterPos2i(10, yMax - 84);
	drawString("Most extracted metals are worth 100 points");
	glRasterPos2i(10, yMax - 87);
	drawString("and have a white color to them. When there");
	glRasterPos2i(10, yMax - 90);
	drawString("is a gold colored metal, collecting it will");
	glRasterPos2i(10, yMax - 93);
	drawString("award the player with 1000 points and a ship");
	glRasterPos2i(10, yMax - 96);
	drawString("upgrade indicated text on the bottom left.");
	/* reset certain options with the debris to prevent it from slowing down and getting destroyed */
	debris[0].lifetime = 2;
	debris[1].lifetime = 2;
	debris[2].lifetime = 2;
	debris[0].dphi = 0.05;
	debris[1].dphi = 0.15;
	debris[2].dphi = 0.08;

}

void
drawShipSelect()
{
	/*
	 * create a menu where you can select your starting ship,''
	 */

	glColor3f(1.0, 1.0, 1.0);
	glRasterPos2i(1, yMax - 2);
	drawString("Select you starting ship by pressing spacebar");
	/* Draw the specific ship and description depending on the selected option.
	  * If there is currently a cooldown active and cooldown is negative,
	  * draw the selected ship entering. If theres a cooldown and no negative
	  * cooldown, draw the ship identified by cooldown leaving */
	glRasterPos2i(1, 3);
	/* Draw the ships leaving */
	if(cooldown != -1){
		if(cooldown == 0){
			drawString("Type 0 is leaving");
		}else if(cooldown == 1){
			drawString("Type 1 is leaving");
		}else if(cooldown == 2){
			drawString("Type 2 is leaving");
		}

	/* Draw the ships entering */
	}else if(currentCooldown > 0){
		if(selectedOption == 0){
			drawString("Type 0 entering");
		}else if(selectedOption == 1){
			drawString("Type 1 entering");
		}else if(selectedOption == 2){
			drawString("Type 2 entering");
		}

	/* Draw the ships idleing */
	}else{
		if(selectedOption == 0){
			drawString("Type 0 selected");
		}else if(selectedOption == 1){
			drawString("Type 1 selected");
		}else if(selectedOption == 2){
			drawString("Type 2 selected");
		}
	}

	/* Once the preiously selected ship is finished leaving,
	  * reset the cooldown and set the previous ship to -1 */
	if(currentCooldown <= 0 && cooldown != -1){
		cooldown = -1;
		currentCooldown = 30;
	}
}

void
drawLevelText()
{
	/*
	 * Draw the text used to inform the player of the level being complete. When the final level
	 * is complete, show the stats or other stuff related to the level
	 */


	printf("%d__%d\n", scoreAL*5 + scoreAM*3 + scoreAS*2 + debrisM*10 + debrisA*100, player.asteroidsHighScore);

	/* Used to display the level the user is on and "area complete" when they finish the final level */
	int i, score, tempScore;
	char startingLevelNumber[30];
	int centerOffsetLevel = 0;

	/* Displays the amount of metal added to the player's inventory */
	char metalEarned[15];
	char metalEarnedValue[15];
	int centerOffsetMetalText = 0;
	int centerOffsetMetalValue = 0;

	/* Display the amount of alloy added to the player's inventory */
	char alloyEarned[15];
	char alloyEarnedValue[15];
	int centerOffsetAlloyText = 0;
	int centerOffsetAlloyValue = 0;

	/* Display the amount of asteroid points added to the player's score */
	char scoreALEarned[15];
	char scoreALEarnedValue[15];
	int centerOffsetALText = 0;
	int centerOffsetALValue = 0;

	char scoreAMEarned[15];
	char scoreAMEarnedValue[15];
	int centerOffsetAMText = 0;
	int centerOffsetAMValue = 0;

	char scoreASEarned[15];
	char scoreASEarnedValue[15];
	int centerOffsetASText = 0;
	int centerOffsetASValue = 0;
	int centerOffsetScoreValue = 0;



	char damageTaken[15], damageTakeValue[15], endLevelText[30];
	int centerOffsetEndLevelText = 0;

	/* convert the level int to a string and append it to the text indicating the next level */
	if(level < maxLevel){
		sprintf(startingLevelNumber, "Starting level %d of %d", level+1, maxLevel);
	}else{
		sprintf(startingLevelNumber, "Area completed!");

		sprintf(metalEarned, "Metal earned");
		sprintf(metalEarnedValue, "%d", metalCount);
		centerOffsetMetalText = strlen(metalEarned);
		centerOffsetMetalValue = strlen(metalEarnedValue);

		sprintf(alloyEarned, "Alloy earned");
		sprintf(alloyEarnedValue, "%d", alloyCount);
		centerOffsetAlloyText = strlen(alloyEarned);
		centerOffsetAlloyValue = strlen(alloyEarnedValue);

		sprintf(damageTaken, "Damage Taken");
		sprintf(damageTakeValue, "temp value");
		sprintf(endLevelText, "Proceed to the next area");

		sprintf(scoreALEarned, "Large");
		sprintf(scoreALEarnedValue, "%d", scoreAL);
		centerOffsetALText = strlen(scoreALEarned);
		centerOffsetALValue = strlen(scoreALEarnedValue);

		sprintf(scoreAMEarned, "Medium");
		sprintf(scoreAMEarnedValue, "%d", scoreAM);
		centerOffsetAMText = strlen(scoreAMEarned);
		centerOffsetAMValue = strlen(scoreAMEarnedValue);

		sprintf(scoreASEarned, "Small");
		sprintf(scoreASEarnedValue, "%d", scoreAS);
		centerOffsetASText = strlen(scoreASEarned);
		centerOffsetASValue = strlen(scoreASEarnedValue);

		centerOffsetEndLevelText = strlen(endLevelText);
	}
	//Count the characters in the string to center it properly. This should be done not every tick
	centerOffsetLevel = strlen(startingLevelNumber);


	glColor3f(1, 1, 1);
	/* Display the level text depending on the level and lifetime left */
	if(levelTextLifetime > 0 && level < maxLevel){
		/* Display only the current level the user is updated to */
		//It's 9/5 since each character is 9 pixels wide on a screen that's 500 pixels big (the screen width w is 500 by default)
		glRasterPos2f((xMax/2)-(9.0/(w/xMax))*centerOffsetLevel/2.0, 3*yMax/4);
		drawString(startingLevelNumber);
	}else if(levelTextLifetime > 0 && level >= maxLevel){
		/* Display a run-down of what the user accomplished during this session of asteroids.
		 * Display text in order of the levelTextLifetime threshold while it counts down */

		/* Show the "area completed" text */
		glRasterPos2f((xMax/2)-(9.0/(w/xMax))*centerOffsetLevel/2.0, 5*yMax/6);
		drawString(startingLevelNumber);

		/* Show the alloy and metal text to indicate the purpose of the metal and alloy count values */
		if(levelTextLifetime < 300 + scoreDigits*20){
			glRasterPos2f((1*xMax/3)-(9.0/(w/xMax))*centerOffsetMetalText/2.0, 4*yMax/6);
			drawString(metalEarned);

			glRasterPos2f((2*xMax/3)-(9.0/(w/xMax))*centerOffsetAlloyText/2.0, 4*yMax/6);
			drawString(alloyEarned);
		}

		/* Show the values of how much metal and alloy the player gained during this session */
		if(levelTextLifetime < 280 + scoreDigits*20){
			glRasterPos2f((1*xMax/3)-(9.0/(w/xMax))*centerOffsetMetalValue/2.0, 4*yMax/6-(15.0/(w/xMax)));
			drawString(metalEarnedValue);

			glRasterPos2f((2*xMax/3)-(9.0/(w/xMax))*centerOffsetAlloyValue/2.0, 4*yMax/6-(15.0/(w/xMax)));
			drawString(alloyEarnedValue);
		}

		/* Show the text to indicate what value coresponds to what asteroid size */
		if(levelTextLifetime < 240 + scoreDigits*20){
			glRasterPos2f((1*xMax/4)-(9.0/(w/xMax))*centerOffsetALText/2.0, 3*yMax/6);
			drawString(scoreASEarned);

			glRasterPos2f((2*xMax/4)-(9.0/(w/xMax))*centerOffsetAMText/2.0, 3*yMax/6);
			drawString(scoreAMEarned);

			glRasterPos2f((3*xMax/4)-(9.0/(w/xMax))*centerOffsetASText/2.0, 3*yMax/6);
			drawString(scoreALEarned);
		}

		/* Show the values of how many points obtained from the given asteroid size */
		if(levelTextLifetime < 220 + scoreDigits*20){
			glRasterPos2f((1*xMax/4)-(9.0/(w/xMax))*centerOffsetALValue/2.0, 3*yMax/6-(15.0/(w/xMax)));
			drawString(scoreASEarnedValue);

			glRasterPos2f((2*xMax/4)-(9.0/(w/xMax))*centerOffsetAMValue/2.0, 3*yMax/6-(15.0/(w/xMax)));
			drawString(scoreAMEarnedValue);

			glRasterPos2f((3*xMax/4)-(9.0/(w/xMax))*centerOffsetASValue/2.0, 3*yMax/6-(15.0/(w/xMax)));
			drawString(scoreALEarnedValue);
		}


		/* Show the text to indicate highscore and the current session's score */
		if(levelTextLifetime < 180 + scoreDigits*20){
			glRasterPos2f((1*xMax/3)-(9.0/(w/xMax))*9.0/2.0, 2*yMax/6);
			drawString("Highscore");

			glRasterPos2f((2*xMax/3)-(9.0/(w/xMax))*5.0/2.0, 2*yMax/6);
			drawString("Score");
		}

		/* Show the value of the highscore and the score values */
		if(levelTextLifetime < 160 + scoreDigits*20){

			/* Loop through the highscore's value to find how many digits are present for the offset */
			tempScore = player.asteroidsHighScore;
			for(centerOffsetScoreValue = 0; tempScore != 0; centerOffsetScoreValue++){
				tempScore /= 10;
			}

			/* Put the value of the highscore into a string to properly use the printString function */
			char highscoreValue[centerOffsetScoreValue];
			sprintf(highscoreValue, "%d", player.asteroidsHighScore);
			glRasterPos2f((1*xMax/3)-(9.0/(w/xMax))*centerOffsetScoreValue/2.0, 2*yMax/6-(15.0/(w/xMax)));
			/* Draw the whole highscore value unchanged */
			drawString(highscoreValue);


			/* Loop through the current session's score to find how many digits are present for the offset */
			score = scoreAL*5 + scoreAM*3 + scoreAS*2 + debrisM*10 + debrisA*100;
			tempScore = score;
			for(centerOffsetScoreValue = 0; tempScore != 0; centerOffsetScoreValue++){
				tempScore /= 10;
			}
			/* Put the value of the highscore into a string to properly use the printString function */
			glRasterPos2f((2*xMax/3)-(9.0/(w/xMax))*centerOffsetScoreValue/2.0, 2*yMax/6-(15.0/(w/xMax)));
			char number[1];
			for(i = 0; i < centerOffsetScoreValue; i++){
				if(levelTextLifetime < 120 + scoreDigits*20 - (centerOffsetScoreValue - i)*20){
					sprintf(number, "%d", (int) (score/pow(10, (centerOffsetScoreValue - 1 - i)))%10);
					drawString(number);
				}else{
					sprintf(number, "%d", (int) myRandom(0, 9));
					drawString(number);
				}
			}

			/* Once the score is fully displayed, check to see if it beats out the highscore */
			if(levelTextLifetime < 120 + scoreDigits*20 - (centerOffsetScoreValue + 2)*20){
				updateHighscore();
			}
		}

		if(levelTextLifetime <= 1){
			glRasterPos2f((xMax/2)-(9.0/(w/xMax))*centerOffsetEndLevelText/2.0, 1*yMax/6);
			drawString(endLevelText);
		}
	}
}

void
drawString(char* s)
{
	/*
	 * Draw the given string using GLUT's character printer
	 */
	int i;

	for(i = 0; s[i] != '\0'; i++){
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, s[i]);
	}
}


/* -- Timer Update Functions ----------------------------------------------------------------------- */

void
updateBackground()
{
	/*
	 * Change the star's brightness using it's flickerRate
	 */
	int i;

	for(i = 0; i < pow(MAX_STARS, 2); i++){
		if(backgroundStars[i].active){
			backgroundStars[i].flicker += backgroundStars[i].flickerRate;
			if(backgroundStars[i].flicker > M_PI){
				backgroundStars[i].flicker -= M_PI;
			}
		}
	}
}

void
updateRespawn()
{
	/*
	 * Decrement the respawn variable until it reaches 0, and then send the game state back to the title
	 */

	if(respawn > 0){
	    respawn--;
	}else if(respawn == 0){
		changeState(STATE_TITLE);
	}
}

void
updateHighscore()
{
	/*
	 * Update the highscore value if the user breaks the record
	 */

	if(scoreAL*5 + scoreAM*3 + scoreAS*2 + debrisM*10 + debrisA*100 > player.asteroidsHighScore){
		player.asteroidsHighScore = scoreAL*5 + scoreAM*3 + scoreAS*2 + debrisM*10 + debrisA*100;
	}
}

void
updateUpgradeText()
{
	/*
	 * Decrement the upgradeText's lifetime until it reaches 0, then deactivate the object
	 */

	upgradeText.lifetime--;
	if (upgradeText.lifetime <= 0){
		upgradeText.active = 0;
	}
}

void
updateLevelText()
{
	/*
	 * Update the level text by decrementing a timer used to display certain texts
	 */

	if(levelTextLifetime >= 0){
		//Let levelTextLifetime reach -1 to indicate it's not counting down anymore
		levelTextLifetime--;
		//Prevent the game from proceeding to the next level when on the last level
		if(level >= maxLevel && levelTextLifetime == 0){
			levelTextLifetime = 1;
		}
	}
	if(levelTextLifetime == 0){
		nextLevel();
	}
}

void
updateDamage()
{
	/*
	 * Decrement the ship's Dmg variables, which are used to prevent
	 * multiple direct line collisions in the same few frames
	 */

	if(ship.LDmg > 0 && ship.LHp > 0){
		ship.LDmg--;
	}
	if(ship.RDmg > 0 && ship.RHp > 0){
		ship.RDmg--;
	}
	if(ship.BDmg > 0 && ship.BHp > 0){
		ship.BDmg--;
	}
	/* if the ship is of type 2, increment the shield back to it's max if it has not recently been damaged or destroyed
	 * The shield will begin to recharge faster if it is not already destroyed thanks to the first statement in this function */
	if(ship.type == 2){
		if(ship.LDmg > 0){
			ship.LDmg--;
		}
		if(ship.LDmg == 0){
			ship.LHp += ship.RHp/200;
			if(ship.LHp > ship.RHp){
				ship.LHp = ship.RHp;
			}
		}
	}
}

void
incrementOscillation()
{
	/*
	 * Increment the oscillating value. This value is used for visual effects,
	 * so this keeps the game animated even when paused. It will only oscillate
	 * between 0 and  Pi, so it will only reach the positives of a sinusoid
	 */

	oscillating += M_PI/30;
	if(oscillating > M_PI){
		oscillating -= M_PI;
	}
}

void
advanceDust()
{
	/*
	 * Advance the dust particles and reduce their lifetime. Once it reaches 0, deactivate the dust
	 */
	int i;

	for(i = 0; i < MAX_DUST; i++){
		if(dust[i].active){
			dust[i].x += dust[i].dx;
	    	dust[i].y += dust[i].dy;
	        //decelerate the particles. Not realistic, but looks good
	        dust[i].dx *= 0.99;
	        dust[i].dy *= 0.99;

	        //reduce the lifetime and deactivate the dust if needed
	        dust[i].lifetime--;
	         if(dust[i].lifetime <= 0){
	        	 dust[i].active = 0;
	     	}
		}
	}
}

void
advanceDebris()
{
    /*
     * Advance, rotate and lower the lifetime of debris in the field. Slow the debris'
     * rotation and velocity overtime to allow the user to collect them easier despite
     * not being physically accurate. Once a debris' lifetime reaches 0, destroy it
     */
	int i;

    for(i = 0; i <MAX_DEBRIS; i++){
    	if(debris[i].active){
    		debris[i].lifetime--;
    		debris[i].x += debris[i].dx;
    		debris[i].y += debris[i].dy;
    		debris[i].phi += debris[i].dphi;
    		debris[i].dx *= 0.99;
    		debris[i].dy *= 0.99;
    		debris[i].dphi *= 0.99;
    		/* prevent the debri's phi from overflowing */
    		if(debris[i].phi > 2*M_PI){
    			debris[i].phi -= 2*M_PI;
    		}else if(debris[i].phi < 0){
    			debris[i].phi += 2*M_PI;
    		}
    		if(debris[i].lifetime <= 0){
    			destroyDebris(&debris[i]);
    		}
    	}
    }
}

void
advanceShip()
{
	/*
	 * Depending on the ship type, it will use different movements. Each ship uses all 4
	 * Arrow keys to allow the user to control the ship's movement. Making a ship go forward
	 * will slowly change the ship's velocity to the ship's direction, making the ship
	 * slide across the view as in floating in space and no other forces are being applied.
	 */

	if(ship.type == 0){
		/* turn the ship only if either left or right is pressed (left XOR right). Prevent phi from overflowing.
		 * If the ship is missing it's left/right piece, it cannot turn left/right. If the user has both pieces
		 * and holds down both keys, the ship will not turn. If the user holds down both keys and is missing a
		 * piece, the ship will spin as if the thruster on the missing piece is not working */
		if(!((left && (ship.LHp > 0)) && (right && (ship.RHp > 0))) &&  ((left && (ship.LHp > 0)) || (right && (ship.RHp > 0)))){
			ship.phi += (((ship.RHp > 0 && right)*2*M_PI)/30) + (-1*((ship.LHp > 0 && left)*2*M_PI)/30);
			if(ship.phi > 2*M_PI){
				ship.phi -= 2*M_PI;
			}else if(ship.phi < 0){
				ship.phi += 2*M_PI;
			}
		}

		/* advance the ship only if either up or down is pressed. 5% (default value of "shipControl")
		 * of the ship's current speed vector  is lost, then 5% of the ship's "thrust vector"
		 * (where the ship's thrusters are pushing the ship)is added the ship current speed vector.
		 * This will prevent the ship from exceeding a dx or dy of 0.05*up*sin(ship.phi + (90*M_PI/180)).
		 * Holding both directions would implement an "air brake" where the ship loses 5% of it's current
		 * speed every tick, eventually setting it to a complete standstill. If the ship is missing it's
		 * back piece, it cannot apply forward or backwards thrust or use the "air brake" */
		if((up || down) && ship.BHp > 0){
			ship.dy = (1-ship.shipControl)*ship.dy + ship.shipControl*up*sin(ship.phi + (90*M_PI/180)) - ship.shipControl*down*sin(ship.phi + (90*M_PI/180));
			ship.dx = (1-ship.shipControl)*ship.dx + ship.shipControl*up*cos(ship.phi - (90*M_PI/180)) - ship.shipControl*down*cos(ship.phi - (90*M_PI/180));
		}
	}else if(ship.type == 1){
		/* This ship does not use the arrow keys to turn, but can move in two dimensions. Implementation is
		 * simpler than a type 0, because speed is only limited in dx and dy instead of the full vector. This
		 * will allow the ship to cover more distance by maxing/minimizing both dx and dy.  */
		if((left || right) && (!(left && right))){
			ship.dx += 0.1*right - 0.1*left;
		}
		if((up || down) && (!(up && down))){
			ship.dy += 0.1*up - 0.1*down;
		}

		/* prevent the ship's displacement from going above it's max (1) */
		if(ship.dx > 1){
			ship.dx = 1;
		}else if(ship.dx < -1){
			ship.dx = -1;
		}
		if(ship.dy > 1){
			ship.dy = 1;
		}else if(ship.dy < -1){
			ship.dy = -1;
		}
	}else if(ship.type == 2){
		/* similar movement to a type 0, but doesn't have a space brake,
		 * is slower when moving and turning, and has deceleration */
		ship.dy = (1-(ship.shipControl/5))*ship.dy + (ship.shipControl/5)*up*sin(ship.phi + (90*M_PI/180)) - (ship.shipControl/5)*down*sin(ship.phi + (90*M_PI/180));
		ship.dx = (1-(ship.shipControl/5))*ship.dx + (ship.shipControl/5)*up*cos(ship.phi - (90*M_PI/180)) - (ship.shipControl/5)*down*cos(ship.phi - (90*M_PI/180));

		if(!(left && right) &&  (left || right )){
			ship.phi += ((right*(1+ship.shipControl*10)*2*M_PI)/90) + (-1*(left*(1+ship.shipControl*10)*2*M_PI)/90);
			if(ship.phi > 2*M_PI){
				ship.phi -= 2*M_PI;
			}else if(ship.phi < 0){
				ship.phi += 2*M_PI;
			}
		}
	}

	/* The following lines are universal for each ship type */
	/* displace the ship's origin depending in it's speed (dx and dy, which will not go above 1).
	 * Increasing shipSpeed will increase the ship's acceleration along with it's max displacement distance */
	ship.x += ship.shipSpeed*ship.dx;
	ship.y += ship.shipSpeed*ship.dy;

	/* if the ship passes the screen boundaries + the ship length
	 * (ship's variable "size"), wrap it around to the other side */
	if(ship.x < -ship.size){
		ship.x = xMax+ship.size;
	}else if(ship.x > xMax+ship.size){
		ship.x = -ship.size;
	}
	if(ship.y < -ship.size){
		ship.y = yMax+ship.size;
	}else if(ship.y > yMax+ship.size){
		ship.y = -ship.size;
	}
}

void
advancePhoton()
{
    /*
     * advance photon laser shots, eliminating those that have gone past the window boundaries
     * */
	int i;

	for(i = 0; i < MAX_PHOTONS; i++){
		if(photons[i].active){
			photons[i].x += photons[i].dx;
			photons[i].y += photons[i].dy;
			if(photons[i].x < -photonSize || photons[i].x > xMax+photonSize || photons[i].y < -photonSize || photons[i].y > yMax+photonSize){
				photons[i].active = 0;
			}
		}
	}
}

void
advanceAsteroid(){
    /*
     * advance and rotate the asteroids (and prevent overflow). Once
     * the entire list is full of inactive asteroids, increase the
     * level variable and spawn more asteroids relative to the current level
     */
	int i, empty;
	empty = 1;
	for(i = 0; i < MAX_ASTEROIDS; i++){
		if(asteroids[i].active){
			empty = 0;
			asteroids[i].x += asteroids[i].dx;
			asteroids[i].y += asteroids[i].dy;
			asteroids[i].phi += asteroids[i].dphi;
			if(asteroids[i].phi > 2*M_PI){
				asteroids[i].phi -= 2*M_PI;
			}else if(asteroids[i].phi < 0){
				asteroids[i].phi += 2*M_PI;
			}
		}
	}

	/* If all the asteroids have become inactive, set the timer to advance to the next level */
	if(empty && levelTextLifetime == -1){
		levelTextLifetime = 120;
		if(level == maxLevel){
			/* Extend the level complete timer if it was the final level */
			levelTextLifetime = 360 + scoreDigits*20;
		}
	}

	/* roll the asteroids to the other side of the screen if they pass the edge of the screen */
	for(i = 0; i < MAX_ASTEROIDS; i++){
		if(asteroids[i].active){
			//Too far left, move to the right side
			if(asteroids[i].x < -1*MAX_ASTEROID_VARIANCE*asteroids[i].size){
				asteroids[i].x = xMax + MAX_ASTEROID_VARIANCE*asteroids[i].size;
			}
			//Too far right, move to the left side
			else if(asteroids[i].x > xMax + MAX_ASTEROID_VARIANCE*asteroids[i].size){
				asteroids[i].x = -1*MAX_ASTEROID_VARIANCE*asteroids[i].size;
			}
			//Too far down, move to the top side
			if(asteroids[i].y < -1*MAX_ASTEROID_VARIANCE*asteroids[i].size){
				asteroids[i].y = yMax + MAX_ASTEROID_VARIANCE*asteroids[i].size;
			}
			//Too far up, move to the bottom side
			else if(asteroids[i].y > xMax + MAX_ASTEROID_VARIANCE*asteroids[i].size){
				asteroids[i].y = -1*MAX_ASTEROID_VARIANCE*asteroids[i].size;
			}
		}
	}
}

void
advancePoints(){
    /*
     * Move the points' position upwards. Decrement it's lifetime and set it to inactive once it's 0
     */
	int i;
    for(i = 0; i < MAX_POINTS; i++){
        if(points[i].active){
            points[i].y += 0.1;
        }
        points[i].lifetime--;
        if(points[i].lifetime <= 0){
            points[i].active = 0;
        }
    }
}

void
collisionAsteroidPhoton(){
	/*
	 * Detect collisions with active asteroids and active photons by
	 * using circle-cirlce and circle-polygon collision detection
	 */
	int i, j, k;
	double x0, x1, x2, y0, y1, y2, lambda, dist;
	for(i = 0; i < MAX_ASTEROIDS; i++){
		for(j = 0; j < MAX_PHOTONS; j++){
			if(photons[j].active && asteroids[i].active){
				/* check whether the photons are close to an asteroid by comparing their radius.
				* If a photon is within the asteroids minimum circle, which has a radius of
				* MIN_ASTEROID_VARIANCE*size, then it is impossible for the photon's origin
				* to NOT be in contact with the asteroid. */
				if(pow(asteroids[i].x - photons[j].x,2) + pow(asteroids[i].y - photons[j].y,2) <=
					pow(asteroids[i].size*MAX_ASTEROID_VARIANCE + photonSize,2)){
					/* photon is touching the asteroids minimum circle, impossible to miss */
					if(pow(asteroids[i].x - photons[j].x,2) + pow(asteroids[i].y - photons[j].y,2) <=
							pow(asteroids[i].size*MIN_ASTEROID_VARIANCE + photonSize,2)){
						//Photon destroys asteroid with a circle-circle check
					    destroyAsteroid(&asteroids[i], &photons[j]);
					    i = MAX_ASTEROIDS;
					    j = MAX_PHOTONS;
					/* photon is within/touching the maximum circle, but not the minimum circle */
					}else{
					    /*Check whether a collision has happened using the astroid's vertices and
					     *the photon's radius. Start by checking the first and last vertex' line*/
					    x1 = asteroids[i].x + sqrt(pow(asteroids[i].coords[asteroids[i].nVertices-1].x, 2) + pow(asteroids[i].coords[asteroids[i].nVertices-1].y, 2))*sin((asteroids[i].phi)+(asteroids[i].nVertices-1)*(2*M_PI)/asteroids[i].nVertices);
					    y1 = asteroids[i].y + sqrt(pow(asteroids[i].coords[asteroids[i].nVertices-1].x, 2) + pow(asteroids[i].coords[asteroids[i].nVertices-1].y, 2))*cos((asteroids[i].phi)+(asteroids[i].nVertices-1)*(2*M_PI)/asteroids[i].nVertices);
				    	for(k = 0; k < asteroids[i].nVertices; k++){
						    x0 = photons[j].x;
					    	y0 = photons[j].y;
					    	/* set the vertexes to the next line of the asteroid */
					    	x2 = x1;
						    y2 = y1;
						    x1 = asteroids[i].x + sqrt(pow(asteroids[i].coords[k].x, 2) + pow(asteroids[i].coords[k].y, 2))*sin((asteroids[i].phi)+k*(2*M_PI)/asteroids[i].nVertices);
					    	y1 = asteroids[i].y + sqrt(pow(asteroids[i].coords[k].x, 2) + pow(asteroids[i].coords[k].y, 2))*cos((asteroids[i].phi)+k*(2*M_PI)/asteroids[i].nVertices);

						    lambda = ((x0 - x1)*(x2 - x1) + (y0 -y1)*(y2 - y1)) / ( pow((x2 - x1),2) + pow((y2 - y1),2) );
						    dist = pow((x1 - x0 + lambda*(x2 - x1)),2) + pow((y1 - y0 + lambda*(y2 - y1)),2);
						    /* This if statement only checks if the photon touches a vertexof the asteroid.
						     * A photon can pass the vertex and get inside the asteroid if they are both moving
						     * at each other fast enough for the distance they cover to be over twice the length
						     * of the photon's radius in one frame. Having  a photon pass through an asteroid
						     * becomes more difficult the closer it is heading towards the center due To the
						     * minimum asteroid variance check. But, the more the photon is pointed towards the
						     * edge, the easier the photon can pass through the asteroid. */
					    	if((lambda >= 0 && lambda <=1 && dist <= pow(photonSize,2))){
							    //Photon destroys asteroid with a polygon-circle check
							    destroyAsteroid(&asteroids[i], &photons[j]);
						    	k = asteroids[i].nVertices;
					    		j = MAX_PHOTONS;
						    }
				    	}
					}
				}
			}
		}
	}
}

void
collisionAsteroidShip()
{
	/*
	 * Depending on the ship type, check for collisions with the ship and the active asteroids.
	 * Once a ship gets a "direct collision" (when a line-line collision is met for a type 0 and 1,
	 * while a simple circle-circle collision is met with a type 2), it cannot get hit with another
	 * "direct collision" until it's Dmg variable is back to 0. It will still take "tick damage"
	 * (Hp-- for simply being too close to the asteroid) independent of the Dmg variables though
	 */
	int i, k, count;
	double x1, x2, y1, y2, angle;

	for(i = 0; i < MAX_ASTEROIDS; i++){
		/* Track the values of the asteroid's final vertex to be used with the asteroids
		 * first vertex as the first line when checking for polygon-polygon collisions */
	    x1 = asteroids[i].x + sqrt(pow(asteroids[i].coords[asteroids[i].nVertices-1].x, 2) + pow(asteroids[i].coords[asteroids[i].nVertices-1].y, 2))*sin((asteroids[i].phi)+(asteroids[i].nVertices-1)*(2*M_PI)/asteroids[i].nVertices);
	    y1 = asteroids[i].y + sqrt(pow(asteroids[i].coords[asteroids[i].nVertices-1].x, 2) + pow(asteroids[i].coords[asteroids[i].nVertices-1].y, 2))*cos((asteroids[i].phi)+(asteroids[i].nVertices-1)*(2*M_PI)/asteroids[i].nVertices);

	    if(ship.type == 0){
			/*check whether the asteroid is active and if the ship is within it's max range*/
			if(asteroids[i].active && pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2) <=
					pow(asteroids[i].size*MAX_ASTEROID_VARIANCE + ship.size,2)){

				/* Check each asteroid's vertex to see whether any of the ship's vertexes are touching the asteroid */
				for(k = 0; k < asteroids[i].nVertices; k++){
					x2 = x1;
					y2 = y1;
					x1 = asteroids[i].x + sqrt(pow(asteroids[i].coords[k].x, 2) + pow(asteroids[i].coords[k].y, 2))*sin((asteroids[i].phi)+k*(2*M_PI)/asteroids[i].nVertices);
					y1 = asteroids[i].y + sqrt(pow(asteroids[i].coords[k].x, 2) + pow(asteroids[i].coords[k].y, 2))*cos((asteroids[i].phi)+k*(2*M_PI)/asteroids[i].nVertices);

					//check for collision with the ship's right side
					if(lineCollision(
							//ship's back right dot
							ship.x + sqrt(ship.size)*sin(ship.phi + (135*M_PI/180)), ship.y + sqrt(ship.size)*cos(ship.phi + (135*M_PI/180)),
							//ship's nose dot
							ship.x + ship.size*sin(ship.phi), ship.y + ship.size*cos(ship.phi),
							//asteroid's vertex dots
							x1, y1, x2, y2)){
						if(ship.RDmg == 0 && ship.RHp > 0){
							ship.RHp -= 20;
							ship.RDmg = 50;
							if(ship.RHp <= 0){
								destroyShip(ship);
							}
						}
					}

					//check for collision with the ship's left side
					if(lineCollision(
							//ship's back left dot
							ship.x + sqrt(ship.size)*sin(ship.phi + (225*M_PI/180)), ship.y + sqrt(ship.size)*cos(ship.phi + (225*M_PI/180)),
							//ship's nose dot
							ship.x + ship.size*sin(ship.phi), ship.y + ship.size*cos(ship.phi),
							//asteroid's vertex dots
							x1, y1, x2, y2)){
						if(ship.LDmg == 0 && ship.LHp > 0){
							ship.LHp -= 20;
							ship.LDmg = 50;
							if(ship.LHp <= 0){
								destroyShip(ship);
							}
						}
					}

					//check for collision with the ship's back side
					if(lineCollision(
							//ship's back left dot
							ship.x + sqrt(ship.size)*sin(ship.phi + (225*M_PI/180)), ship.y + sqrt(ship.size)*cos(ship.phi + (225*M_PI/180)),
							//ship's back right dot
							ship.x + sqrt(ship.size)*sin(ship.phi + (135*M_PI/180)), ship.y + sqrt(ship.size)*cos(ship.phi + (135*M_PI/180)),

							//asteroid's vertex dots
							x1, y1, x2, y2)){
						if(ship.BDmg == 0 && ship.BHp > 0){
							ship.BHp -= 20;
							ship.BDmg = 50;
							if(ship.BHp <= 0){
								destroyShip(ship);
							}
						}
					}
				}
			}
			/* Check whether the ship is too much inside the asteroid (the ship's origin is touching the asteroid's inner circle)
			 * Decrement the ship's health when inside said radius and trigger a damage event on each piece of the ship.
			 * Also, give the ship an increase in speed away from the asteroid's origin */
			if(asteroids[i].active && ((pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2)) <=
				pow(asteroids[i].size*MIN_ASTEROID_VARIANCE,2))){
				/* Increase the ship's velocity away from the asteroid to push it out */
				angle = atan((asteroids[i].y - ship.y) / (asteroids[i].x - ship.x));
				if(asteroids[i].x - ship.x < 0){
					ship.dx += 0.1*cos(angle);
					ship.dy += 0.1*sin(angle);
				}else{
					ship.dx -= 0.1*cos(angle);
					ship.dy -= 0.1*sin(angle);
				}

				/* Damage the ship parts on every tick of being inside the inner
				 * circle and do a "direct collision" if it's Dmg values allow one */
				if(ship.RHp > 0){
					ship.RHp--;
					if(ship.RDmg == 0){
						ship.RHp -= 20;
						ship.RDmg = 75;
					}
					if(ship.RHp <= 0){
						destroyShip(ship);
					}
				}
				if(ship.LHp > 0){
					ship.LHp--;
					if(ship.LDmg == 0){
						ship.LHp -= 20;
						ship.LDmg = 75;
					}
					if(ship.LHp <= 0){
						destroyShip(ship);
					}
				}
				if(ship.BHp > 0){
					ship.BHp--;
					if(ship.BDmg == 0){
						ship.BHp -= 20;
						ship.BDmg = 75;
					}
					if(ship.BHp <= 0){
						destroyShip(ship);
					}
				}
			}
	    }else if(ship.type == 1){
			/*check whether the asteroid is active and if the ship is within it's max range */
			if(asteroids[i].active && pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2) <=
					pow(asteroids[i].size*MAX_ASTEROID_VARIANCE + ship.size,2)){

				/* Check each asteroid's vertex to see whether the ship's vertexes are touching any.
				  * This ship has only one piece, so only one collision needs to be found to take damage */
				for(k = 0; k < asteroids[i].nVertices; k++){
					x2 = x1;
					y2 = y1;
					x1 = asteroids[i].x + sqrt(pow(asteroids[i].coords[k].x, 2) + pow(asteroids[i].coords[k].y, 2))*sin((asteroids[i].phi)+k*(2*M_PI)/asteroids[i].nVertices);
					y1 = asteroids[i].y + sqrt(pow(asteroids[i].coords[k].x, 2) + pow(asteroids[i].coords[k].y, 2))*cos((asteroids[i].phi)+k*(2*M_PI)/asteroids[i].nVertices);

					/* Only run collisions checks if the ship has more than 0 HP */
					if(ship.BHp > 0){
						/* Only check for direct vertex collisions if the ship is able to take damage (BDmg <= 0) */
						if(ship.BDmg == 0){
							/* check for collision with the ship's right side */
							if(lineCollision(
									//ship's top-right dot
									ship.x + sqrt(ship.size), ship.y + sqrt(ship.size),
									//ship's bottom-right dot
									ship.x + sqrt (ship.size), ship.y - sqrt (ship.size),
									//asteroid's vertex dots
									x1, y1, x2, y2)){
								ship.BHp -= 20;
								ship.BDmg = 30;
							}else
							/* check for collisions with the ship's bottom side */
							if(lineCollision(
									//ship's bottom-right dot
									ship.x + sqrt(ship.size), ship.y - sqrt(ship.size),
									//ship's bottom-left dot
									ship.x - sqrt (ship.size), ship.y - sqrt (ship.size),
									//asteroid's vertex dots
									x1, y1, x2, y2)){
								ship.BHp -= 20;
								ship.BDmg = 30;
							}else
							/* check for collisions with the ship's left side */
							if(lineCollision(
									//ship's bottom-left dot
									ship.x - sqrt(ship.size), ship.y - sqrt(ship.size),
									//ship's top-left dot
									ship.x - sqrt (ship.size), ship.y + sqrt (ship.size),
									//asteroid's vertex dots
									x1, y1, x2, y2)){
								ship.BHp -= 20;
								ship.BDmg = 30;
							}else
							/* check for collisions with the ship's top side */
							if(lineCollision(
									//ship's top-left dot
									ship.x + sqrt(ship.size), ship.y - sqrt(ship.size),
									//ship's top-right dot
									ship.x + sqrt (ship.size), ship.y + sqrt (ship.size),
									//asteroid's vertex dots
									x1, y1, x2, y2)){
								ship.BHp -= 20;
								ship.BDmg = 30;
							}
						}
						/* Destroy the ship if it's out of Hp */
						if(ship.BHp <= 0){
							destroyShip(ship);
						}
					}
				}
			}
			/* check if the ship is too far within an asteroid's inner circle and is not already destroyed*/
			if(asteroids[i].active && ship.BDmg != -1 && pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2) <=
					pow(asteroids[i].size*MIN_ASTEROID_VARIANCE,2)){
				//Take twice the amount of damage on every tick like a type 0 would normally take
				ship.BHp -= 2;
				/* Increase the ship's velocity away from the asteroid to push it out.
				 * Similar to a type 0, but is thrown away at a faster rate */
				angle = atan((asteroids[i].y - ship.y) / (asteroids[i].x - ship.x));
				if(asteroids[i].x - ship.x < 0){
					ship.dx += 0.5*cos(angle);
					ship.dy += 0.5*sin(angle);
				}else{
					ship.dx -= 0.5*cos(angle);
					ship.dy -= 0.5*sin(angle);
				}
				/* Destroy the ship if it's out of Hp */
				if(ship.BHp <= 0){
					destroyShip(ship);
				}
			}
	    }else if(ship.type == 2){
	    	/* This ship has two healthbars : a shield (being the LHp) and it's normal health (being the BHp).
	    	 * Damage will be done to LHp before going to BHp, but LHp takes damage easier than BHp by having
	    	 * different collision checks and able to take damage from multiple asteroids at once. Whenever
	    	 * LHp takes damage,  it will reset it's LDmg variable to 50. Once LDmg is at 0, LHp will increment
	    	 * back up to it's  max health, which is saved in the variable RHp. The ship is also only knocked
	    	 * away from the asteroid once it's shields are gone, but knocks asteroids away with it's shields */
			if(ship.BDmg != -1 && asteroids[i].active && pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2) <=
					pow(asteroids[i].size*MAX_ASTEROID_VARIANCE + ship.size,2)){
				/* If the shield health (LHp value) is not 0, damage is directed to the shield */
				if(ship.LHp > 0){
					/* The shield (LHp) takes damage in many different ways, all of which involve two circle collision checks */
					//simply being close enough to the asteroid will damage shields (both ship's and asteroid's max radius is touching)
					ship.LHp--;
					ship.LDmg = 100;
					/* if the ship is touching the asteroid's inner radius, take shield damage */
					if((pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2)) <=
						pow(asteroids[i].size*MIN_ASTEROID_VARIANCE + ship.size,2)){
						ship.LHp -= 3;

						/* if the ship's origin is within the asteroid's inner radius */
						if((pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2)) <=
							pow(asteroids[i].size*MIN_ASTEROID_VARIANCE,2)){
							ship.LHp -= asteroids[i].size;
						}

						/* if the ship is touching the asteroid's center */
						if((pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2)) <=
							pow(ship.size,2)){
							ship.LHp -= asteroids[i].size;
							/* Touching the asteroid's center will push it away from the ship.
							 * This may allow an asteroid to gain a large amount of speed */
							angle = atan((asteroids[i].y - ship.y) / (asteroids[i].x - ship.x));
								if(asteroids[i].x - ship.x < 0){
									asteroids[i].dx -= 0.5*cos(angle);
									asteroids[i].dy -= 0.5*sin(angle);
								}else if(asteroids[i].x - ship.x > 0){
									asteroids[i].dx += 0.5*cos(angle);
									asteroids[i].dy += 0.5*sin(angle);
								}
						}

						/*if the ship's origin is within 1 unit of the asteroid's origin */
						if((pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2)) <=
							pow(1,2)){
							ship.LHp -= asteroids[i].size*2;
						}
					}
					/* prevent the shield health from going bellow 0 and "destroy" the shield */
					if(ship.LHp <= 0){
						ship.LHp = 0;
						/* create 32 dust particles to indicate the shield is "destroyed" */
						count = 0;
						for(k = 0; k < MAX_DUST; k++){
							if(dust[k].active != 1){
								count++;
								dust[k].active = 1;
								dust[k].x = ship.x + ship.size*sin(count*M_PI/16) + myRandom(-0.2, 0.2);
								dust[k].y = ship.y + ship.size*cos(count*M_PI/16) + myRandom(-0.2, 0.2);
								dust[k].dx = ship.dx + 0.2*sin(count*M_PI/16) + myRandom(-0.2, 0.2);
								dust[k].dy = ship.dy + 0.2*cos(count*M_PI/16) + myRandom(-0.2, 0.2);
								dust[k].lifetime = 45;
								if(count >= 32){
									k = MAX_DUST;
								}
							}
						}
					}
				}else{
					/* The ship's health (BHp value) will take damage just like other types - Once a collision happens, the LDmg will increase
					 * to allow the ship to escape the asteroid before taking more damage, but it will still take incrementing damage if too close */
					/*check whether the ship is touching the asteroid's inner radius */
					if((pow(asteroids[i].x - ship.x,2) + pow(asteroids[i].y - ship.y,2)) <=
							pow(asteroids[i].size*MIN_ASTEROID_VARIANCE + ship.size,2)){
						/* Take collision damage if possible. This damage will reset the shield's recharge */
						if(ship.BDmg == 0){
							ship.BHp -= 20;
							ship.BDmg = 40;
							ship.LDmg = 100;
						}
						/* knock both asteroid and ship away from eachother relative to asteroid size */
						angle = atan((asteroids[i].y - ship.y) / (asteroids[i].x - ship.x));
						if(asteroids[i].x - ship.x < 0){
							asteroids[i].dx -= 0.2*cos(angle)/asteroids[i].size;
							asteroids[i].dy -= 0.2*sin(angle)/asteroids[i].size;
							ship.dx += 0.02*cos(angle)*asteroids[i].size;
							ship.dy += 0.02*sin(angle)*asteroids[i].size;
						}else if(asteroids[i].x - ship.x > 0){
							asteroids[i].dx += 0.2*cos(angle)/asteroids[i].size;
							asteroids[i].dy += 0.2*sin(angle)/asteroids[i].size;
							ship.dx -= 0.02*cos(angle)*asteroids[i].size;
							ship.dy -= 0.02*sin(angle)*asteroids[i].size;
						}
					}
					/* Destroy the ship if it's out of BHp */
					if(ship.BHp <= 0){
						ship.BHp = 0;
						destroyShip(ship);
					}
				}
			}
	    }
	}
}

void
collisionDebrisShip()
{
	/*
	 * Check whether the ship collides into a piece of debris (ship radius touches debris origin)
	 * to give them points and remove the debris to simulate the ship picking up the debris.
	 * The ship must not be respawning, the debris must be active and not be type 0
	 */
	int i;

	if(respawn == -1){
		for(i = 0; i < MAX_DEBRIS; i++){
			/* check whether the origin of the debris touches the ship's size radius */
			if(debris[i].active && debris[i].type != 0 && pow(debris[i].x - ship.x,2) + pow(debris[i].y - ship.y,2) <= pow(ship.size,2)){
				debris[i].active = 0;
				debris[i].lifetime = 0;
				if(debris[i].type == 2){
					/* Upon picking up a shining debris piece, increase the score by 100 and give the user some alloys */
					upgradeShip(ship);
					addScore(100, debris[i].x, debris[i].y);
					debrisA++;
					alloyCount += floor(myRandom(1, 4.2));
				}else{
					/* Upon picking up a common debris piece, increase the score by 10 and give the user some metals */
				    addScore(10 , debris[i].x, debris[i].y);
				    debrisM++;
				    metalCount += (myRandom(1.8, 9.1));
				}
			}
		}
	}
}

void
lowerCooldown()
{
	/*
	 * Lower the currentCooldown variable. Prevent it from underflowing
	 */

	currentCooldown--;
	if(currentCooldown < 0){
		currentCooldown = 0;
	}
}


/* -- Event Trigger Functions ------------------------------------------------------------------------- */

void
setShipHelpScreen()
{
	/*
	 * Set the ship's values for when the help screen is selected
	 */
	ship.type = 0;
	ship.x = xMax*0.06;
	ship.y = yMax*0.74;
	ship.phi = 0;
	ship.dx = 0;
	ship.dy = 0;
	ship.BHp = 100;
	ship.LHp = 100;
	ship.RHp = 100;
	ship.size = 4;
	ship.LDmg = 0;
	ship.RDmg = 0;
	cooldown = 0;
	ship.shipSpeed = 1;
	ship.shipControl = 0.05;
	respawn = -1;
	ship.shipUpgrade = 0;
	photonUpgrade = 0;
	photonSize = 1;
	photonSpeed = 2;
	currentCooldown = 0;
}

void
upgradeShip()
{
	/*
	 * Give the ship a random upgrade from the set of upgrades available.
	 * Do not let certain upgrades occur more than 10 times.
	 */
	double rand;
	rand = myRandom (0.0, 1.0);

	/* If the generated upgrade cannot be added due to being upgraded 10
	 * times already, regenerate a new upgrade until a usuable one is found */
	while((photonUpgrade >= 10 && rand > 0.1 && rand <= 0.4) || (ship.shipUpgrade >= 10 && rand > 0.4 && rand <= 0.7)){
		rand = myRandom (0.0, 1.0);
	}
	/* Add 10 metals and 1 alloy. 10% chance */
	if(rand <= 0.1){
		metalCount += 10;
		alloyCount += 1;
		changeUpgradeText("metal/alloy deposit");
	}
	/* Increase the photon size and speed by 10% up to 10 times and lower the cooldown by 10%. 30% chance */
	else if(rand <= 0.4){
		photonSpeed *= 1.1;
		photonSize *= 1.1;
		cooldown *= 0.9;
		changeUpgradeText("Photon upgrade");
		photonUpgrade++;
	}
	/* Increase ship control and speed by 10% up to 10 times.  30% chance */
	else if(rand <= 0.7){
		ship.shipSpeed *= 1.1;
		ship.shipControl *= 1.1;
		changeUpgradeText("Ship upgrade");
		if (ship.shipControl > 1){
			ship.shipControl = 1;
		}
		ship.shipUpgrade++;
	}
	/* Increases ship health by 25 per piece along with 200 time units
	 * of invincibility against vertex collisions. If the ship is of type 2,
	 * set LDmg to 0 to begin shield regeneration. 30% chance */
	else if(rand <= 1.0){
		ship.LHp += 25;
		ship.RHp+= 25;
		ship.BHp += 25;
		ship.LDmg = 200;
		ship.RDmg = 200;
		ship.BDmg = 200;
		if(ship.type == 2){
			ship.LDmg = 0;
		}
		changeUpgradeText("Health boost");
	}
}

void
changeUpgradeText(char* t)
{
	/*
	 * change the text currently being displayed and restart it's lifetime
	 */

	upgradeText.msg = t;
	upgradeText.active = 1;
	upgradeText.lifetime = 250;
}

void
firePhoton()
{

	/*
	 * Fire a photon in different manners depending on the given ship type. The ship must not be respawning,
	 * the currentcooldown variable must be 0 and the game expects there to be inactive photons in the array
	 */
	int i, fired;

	if(ship.type == 0){
		/* Fire a photon in the direction that the ship is facing */
		for(i = 0; i < MAX_PHOTONS; i++){
			if(!(photons[i].active)){
				photons[i].active = 1;
				photons[i].x = ship.x + photonSize*sin(ship.phi);
				photons[i].y = ship.y + photonSize*cos(ship.phi);
				photons[i].dx = photonSpeed*sin(ship.phi);
				photons[i].dy = photonSpeed*cos(ship.phi);
				i = MAX_PHOTONS;
			}
		}
	}else if(ship.type == 1){
		/* Fire a photon in all 4 directions. Each shot fired increases cooldown */
		if(currentCooldown <= 0){
			fired = 0;
			for(i = 0; i < MAX_PHOTONS; i++){
				if(!(photons[i].active)){
					currentCooldown += cooldown;
					photons[i].active = 1;
					photons[i].x = ship.x + photonSize*sin(fired*M_PI/2);
					photons[i].y = ship.y + photonSize*cos(fired*M_PI/2);
					photons[i].dx = photonSpeed*sin(fired*M_PI/2);
					photons[i].dy = photonSpeed*cos(fired*M_PI/2);
					fired++;
					if(fired >= 4){
						i = MAX_PHOTONS;
					}
				}
			}
		}
	}else if(ship.type == 2){
		/* fire 3 photon shots, two of which are offset to either sides of the ship. Each shot increases the cooldown */
		if(currentCooldown <= 0){
			fired = 0;
			for(i = 0; i < MAX_PHOTONS; i++){
				if(!(photons[i].active)){
					currentCooldown += cooldown;
					photons[i].active = 1;
					photons[i].x = ship.x + (fired == 0)*ship.size*sin(ship.phi) + (fired == 1)*ship.size*cos(ship.phi + (330*M_PI/180)) - (fired == 2)*ship.size*cos(ship.phi + (30*M_PI/180)) + photonSize*sin(ship.phi);
					photons[i].y = ship.y + (fired == 0)*ship.size*cos(ship.phi) - (fired == 1)*ship.size*sin(ship.phi + (330*M_PI/180)) + (fired == 2)*ship.size*sin(ship.phi + (30*M_PI/180)) + photonSize*cos(ship.phi);

					photons[i].dx = photonSpeed*sin(ship.phi);
					photons[i].dy = photonSpeed*cos(ship.phi);
					fired++;
					if(fired >= 3){
						i = MAX_PHOTONS;
					}
				}
			}
		}
	}
}

void
changeShip(int t)
{
	/*
	 * Change the ship type to the given value and reset the game to default values
	 */

	ship.type = t;
	//Remove all asteroids, dust and debris and set the level to 0 (advanceAsteroids will spawn one for level 1)
	clear();
	/* reset universal values before unique ship type values */
	ship.x = xMax/2;
	ship.y = yMax/2;
	ship.phi = 0;
	ship.dx = 0;
	ship.dy = 0;
	ship.BHp = 100;
	ship.LHp = 100;
	ship.RHp = 100;
	ship.BDmg = 0;
	ship.LDmg = -1;
	ship.RDmg = -1;
	ship.shipSpeed = 1;
	ship.shipControl = 0.05;
	respawn = -1;
	ship.shipUpgrade = 0;
	photonUpgrade = 0;
	photonSize = 1;
	photonSpeed = 2;
	currentCooldown = 0;
	/* Reset the score values */
	scoreAL = 0;
	scoreAM = 0;
	scoreAS = 0;
	debrisM = 0;
	debrisA = 0;
	metalCount = 0;
	alloyCount = 0;
	/* Set default values dependent on ship type */
	if(ship.type == 0){
		ship.size = 4;
		ship.LDmg = 0;
		ship.RDmg = 0;
		cooldown = 0;
	}else if(ship.type == 1){
		ship.size = 2;
		cooldown = 10;
	}else if(ship.type == 2){
		ship.size = 8;
		ship.LHp = 300;
		ship.RHp = 300;
		ship.LDmg = 0;
		cooldown = 10;

	}
}

void
addScore(int p, double x, double y){
    /*
     * create a displayable object to indicate the amount of points the user got from doing an action and
     * save the amount of digits that encompass the combined score to center-allign it when drawing
     */
	int i, score;

	score = scoreAL*5 + scoreAM*3 + scoreAS*2 + debrisM*10 + debrisA*100;
	/* Tally the score and save the amount of digits of the score */
	for(i = 0; score != 0; i++){
		score /= 10;
	}
	printf("score digits: %d\n", i);

	/* Create the points object to display the amount of points the user got */
    for(i = 0; i < MAX_POINTS; i++){
        if(!(points[i].active)){
            points[i].active = 1;
            points[i].amount = p;
            points[i].x = x;
            points[i].y = y;
            points[i].lifetime = 30;
            i = MAX_POINTS;
        }
    }
}

void
nextLevel()
{

	/*
	 * Advance to the next level.
	 */
	int i;

	if(level < maxLevel){
		/* Respawn the asteroids for the next level to begin */
		level++;
		for(i = 0; i < level; i++){
			initAsteroid(&asteroids[i], ASTEROID_LARGE);
		}
	}
}


/* -- helper function ------------------------------------------------------- */

int lineCollision(double Ax, double Ay, double Bx, double By, double Cx, double Cy, double Dx, double Dy)
{
	/*
	 * return 1 if there is a collision between the line A(x1, y1) B(x2,y2) and C(x3,y3) D(x4,y4)
	 */
	int collision = 0;
	int oriA;
	int oriB;
	int oriC;
	int oriD;
	/* get the orientations of the 4 possible 3 line combinations. 0 = colinear. 1 = clockwise. 2 = counterclockwise */
	//orientation of A, B and C
	oriD = (By - Ay) * (Cx - Bx) - (Bx - Ax) * (Cy - By);
	if(oriD > 0){
		oriD = 1;
	}else if(oriD < 0){
		oriD = 2;
	}

	//orientation of A,B and D
	oriC = (By - Ay) * (Dx - Bx) - (Bx - Ax) * (Dy - By);
	if(oriC > 0){
		oriC = 1;
	}else if(oriC < 0){
		oriC = 2;
	}

	//orientation of C, D and A
	oriB = (Dy - Cy) * (Ax - Dx) - (Dx - Cx) * (Ay - Dy);
	if(oriB > 0){
		oriB = 1;
	}else if(oriB < 0){
		oriB = 2;
	}

	//orientation of C, D and B
	oriA = (Dy - Cy) * (Bx - Dx) - (Dx - Cx) * (By - Dy);
	if(oriA > 0){
		oriA = 1;
	}else if(oriA < 0){
		oriA = 2;
	}

	/* There is a collision if the orientation between a line and either dots of the other line are different */
	if(oriD != oriC && oriB != oriA){
		collision = 1;
	}

	/*
	 * Check whether a point is positioned on a line while its colinear with said line
	 */
	/*Check if C is on the line AB and if they are colinear */
	if(oriD == 0 && ((Cx <= Ax || Cx <= Bx) && (Cx >= Ax || Cx >= Bx) &&
	((Cy <= Ay || Cy <= By) && (Cy >= Ay || Cy >= By)))){
		collision = 1;
	}

	/* Check if D is on the line AB and if they are colinear */
	if(oriC == 0 && ((Dx <= Ax || Dx <= Bx) && (Dx >= Ax || Dx >= Bx) &&
	((Dy <= Ay || Dy <= By) && (Dy >= Ay || Dy >= By)))){
		collision = 1;
	}

	/* Check if A is on the line CD and if they are colinear */
	if(oriB == 0 && ((Ax <= Cx || Ax <= Dx) && (Ax >= Cx || Ax >= Dx) &&
	((Ay <= Cy || Ay <= Dy) && (Ay >= Cy || Ay >= Dy)))){
		collision = 1;
	}

	/* Check if B is on the line CD and if they are colinear */
	if(oriA == 0 && ((Bx <= Cx || Bx <= Dx) && (Bx >= Cx || Bx >= Dx) &&
	((By <= Cy || By <= Dy) && (By >= Cy || By >= Dy)))){
		collision = 1;
	}

	return collision;
}

