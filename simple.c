/*
 *  Izik Arruda
 *	asteroids.c
 */

/*
 * Comments in this format of empty comment spaces above and bellow describe most if not all of the function
 */

/* Comments that start and end with a slash-star describe an indescripted amount of code, usually an entire loop or statement. */

//These single line comments describe only the following line

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <GL/glut.h>
#include "simple.h"
#include "textures.c"
#include "asteroids.c"
#include "systemViewer.c"
#include "menus.c"


/* -- type definitions ------------------------------------------------------ */


/* -- function prototypes --------------------------------------------------- */

static void endProgram();
static void	myDisplay(void);
static void	myTimer(int value);
static void	myKey(unsigned char key, int x, int y);
static void	keyPress(int key, int x, int y);
static void	keyRelease(int key, int x, int y);
static void	myReshape(int w, int h);

/* Create/activate the given objects (or in init()'s case, set the game's default values) */
static void init(void);

/* General functions that are run on specific event triggers */
static void clear();
static void optionSelect();


/* Functions that operate with outside files such as the savefile */
static void loadSavefile();

/* These functions are used on every timer tick. They are
 * used to update, advance and collision check all the different objects in the game */


/* Useful functions that return values pertaining their mathematical problem */
static double myRandom(double min, double max);


/* -- global variables ------------------------------------------------------ */

/* The state of the arrow keys */
int up=0, down=0, left=0, right=0;

/* The current state of the game using defined constants */
int state;

/* The highest value that selected option can reach. SelectedOption is used in certain menus to determine what option is selected */
int maxOption;

/* The size of the game's window */
double xMax, yMax, h, w;


/* -- main ------------------------------------------------------------------ */

int
main(int argc, char *argv[])
{
	atexit(endProgram);
    srand((unsigned int) time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(500, 500);
    glutCreateWindow("Asteroids");
    glutDisplayFunc(myDisplay);
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(myKey);
    glutSpecialFunc(keyPress);
    glutSpecialUpFunc(keyRelease);
    glutReshapeFunc(myReshape);
    glutTimerFunc(33, myTimer, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_DEPTH_TEST);

    init();

    glutMainLoop();

    return 0;
}


/* -- callback/key functions ---------------------------------------------------- */

void
endProgram()
{
	/*
	 * Free whatever memory has been allocated and save the currentSystem
	 */
	int i, ii, iii;

	printf("exiting...\n");
	saveSystem();
	printf("saved current system values\n");

	printf("start freeing memory\n");
	free(camera);
	free(systemDistanceArray);
	free(BGStars);

	//Free the systemArray
	if(systemArray != NULL){
		for(i = 0; i < SYSTEM_COUNT; i++){
			if(systemArray[i] != NULL){
				free(systemArray[i]);
			}
		}
		free(systemArray);
	}

	//Free the BGStars array
	if(BGStars != NULL){
		for(i = 0; i < SYSTEM_COUNT; i++){
			if(BGStars[i] != NULL){
				free(BGStars[i]);
			}
		}
		free(BGStars);
	}

	printf("started freeing currentSystem\n");

	//Free the current systems planet array along with it's surface arrays
	for(i = 0; i < currentSystem->planetCount; i++){
		for(ii = 0; ii < currentSystem->planet[i]->surfaceRows; ii++){
			for(iii = 0; iii < currentSystem->planet[i]->surfaceColumns; iii++){
				free(currentSystem->planet[i]->surface[ii][iii]);
			}
		}
		free(currentSystem->planet[i]);
	}
	free(currentSystem->planet);
	free(currentSystem->star);
	printf("done freeing memory");
}

void
myDisplay()
{
    /*
     *	display callback function that runs a set of draw functions depending on the current game state
     */
	int	i;

    /* The game can be displayed in 3D or 2D. Certain commands need to be run for 2D and 3D to properly render,
     * so certain states can only work in their own dimension with their proper render options */
	if(state == STATE_TITLE || state == STATE_ASTEROIDS || state == STATE_HELP || state == STATE_SHIPSELECT){
		/* 2D rendering */
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glOrtho(0.0, xMax, 0.0, yMax, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}else if(state == STATE_SYSTEM){
		/* 3D rendering */
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gluPerspective(50.0, xMax/yMax, 1, 10000.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}else{
		/* State has not been given a proper dimension rendering */
		printf("!!!State has not been given a proper dimension rendering!!!\n");
	}

    /* Draw each set of objects is a specific order to create different layers of object sets.
     * 2D order affects which objects will be "above" eachother upon colison. 3D order
     * seems to affect the visual difference with opacity between objects */
    if(state == STATE_TITLE){
		for (i=0; i<pow(MAX_STARS, 2); i++)
			if (backgroundStars[i].active)
				drawBackground(&backgroundStars[i]);

		for (i=0; i<MAX_ASTEROIDS; i++)
			if (asteroids[i].active)
				drawAsteroid(&asteroids[i]);

		drawTitle();

    }else if(state == STATE_ASTEROIDS){
		for (i=0; i<pow(MAX_STARS, 2); i++)
			if (backgroundStars[i].active)
				drawBackground(&backgroundStars[i]);

		for (i=0; i<MAX_PHOTONS; i++)
			if (photons[i].active)
				drawPhoton(&photons[i]);

		for (i=0; i<MAX_ASTEROIDS; i++)
			if (asteroids[i].active)
				drawAsteroid(&asteroids[i]);

		for (i=0; i<MAX_DEBRIS; i++)
			if (debris[i].active)
				drawDebris(&debris[i]);

		for (i=0; i<MAX_DUST; i++)
			if (dust[i].active)
				drawDust(&dust[i]);

		for (i=0; i<MAX_POINTS; i++)
			if(points[i].active)
				drawPoints(&points[i]);

		drawUpgrade();
		drawScore();
		drawUpgradeText();
		drawLevelText();
		drawShip();

    }else if(state == STATE_HELP){
		for (i=0; i<pow(MAX_STARS, 2); i++)
			if (backgroundStars[i].active)
				drawBackground(&backgroundStars[i]);

		for (i=0; i<MAX_PHOTONS; i++)
			if (photons[i].active)
				drawPhoton(&photons[i]);

		for (i=0; i<MAX_ASTEROIDS; i++)
			if (asteroids[i].active)
				drawAsteroid(&asteroids[i]);

		for (i=0; i<MAX_DEBRIS; i++)
			if (debris[i].active)
				drawDebris(&debris[i]);

		for (i=0; i<MAX_DUST; i++)
			if (dust[i].active)
				drawDust(&dust[i]);

		for (i=0; i<MAX_POINTS; i++)
			if(points[i].active)
				drawPoints(&points[i]);

		drawUpgradeText();
		drawShip();
		drawHelp();
    }else if(state == STATE_SHIPSELECT){
        for (i=0; i<pow(MAX_STARS, 2); i++)
			if (backgroundStars[i].active)
				drawBackground(&backgroundStars[i]);

        drawShipSelect();
    }else if(state == STATE_SYSTEM){
    	drawSystem();
    }

    glFlush();
    glutSwapBuffers();
}

void
myTimer(int value)
{
	/*
     * timer callback function runs a list of commands at every in-game tick. In asteroids, the background stars and the ship's red tinge
     * will oscillate periodically, the score and the display will update, certain values will be incremented/decremented,
     * move all objects using their speed and rotation, check for collisions, the timer function will set itself again, etc.
     * This is the main function and for every game state, different functions are run on every tick.
	 */

	/* run different sets of instructions depending on the game state */
	if(state == STATE_TITLE){
		/*Increment the oscillation variable and the background */
		updateBackground();
		incrementOscillation();

		advanceAsteroid();
	}else if(state == STATE_ASTEROIDS){
		/*Increment the oscillation variable and the background */
		updateBackground();
		incrementOscillation();

		/* use a set of functions to advance the objects' positions */
		advanceDust();
		advanceDebris();
		advanceShip();
		advancePhoton();
		advanceAsteroid();
		advancePoints();

		/* test for and handle collisions */
		collisionAsteroidPhoton();
		collisionAsteroidShip();
		collisionDebrisShip();

		/* Lower the cooldown variable */
		lowerCooldown();

		/* Lower the "xDmg" ship variables that give the ship invincibility once hit */
		updateDamage();

		/* Update the text to decrement it's lifetime or to deactivate */
		updateUpgradeText();

		/* Update the next level text to display the current level/end level sequence */
		updateLevelText();

		/* Update the respawn timer used when the ship gets destroyed */
		updateRespawn();

	}else if(state == STATE_HELP){
		/*Increment the oscillation variable and the background */
		updateBackground();
		incrementOscillation();

		/* Enable certain updates to be done to allow the help screen to be helpful */
		advanceAsteroid();
		advanceDebris();
		advanceDust();
		advanceShip();
		advancePoints();
		updateUpgradeText();
		collisionDebrisShip();
	}else if(state  == STATE_SHIPSELECT){
		/*Increment the oscillation variable and the background */
		updateBackground();
		incrementOscillation();

		lowerCooldown();
	}else if(state == STATE_SYSTEM){
		advanceSystem();
		if(windowState == 0){
			/* Don't update the camera's position if there is a communications window up */
			advanceCamera();
		}
		updateWindow();
		updateShipPath();
		updateSatellitePath(0);
	}

	/* Update the display frame and restart a timer for the timer function */
	glutPostRedisplay();
    glutTimerFunc(33, myTimer, value);		/* 30 frames per second */
}

void
myKey(unsigned char key, int x, int y)
{
    /*
     *	keyboard callback function; add code here for firing the laser,
     *	starting and/or pausing the game, etc.
     */


	/* do specific action depending on game state */
	if(state == STATE_TITLE){
		/* Send a request to select the currently selected option */
		if(key == 32){
			optionSelect();
		}
	}else if(state == STATE_ASTEROIDS){
		if(key == 32 && respawn == -1){
			/* send a request to fire a photon */
			firePhoton();
		}

		/* change the ship type to the given number */
		if(key == '1'){
			changeShip(0);
		}
		if(key == '2'){
			changeShip(1);
		}
		if(key == '3'){
			changeShip(2);
		}

		/* These are commands used to allow ease when bug testing.
		 * certain unintended effects can happen that would not arise
		 * In normal play, such as having destroyShip be called for
		 * Non-type 0 ships when their BHp value does not reach zero
		 * or giving the ship HP again when respawning wont stop
		 * the ship from respawning on it's own (but changing ships will). */
		/* Destroy the ship's left/right/back piece using the key b/n/m */
		if(key == 'b'){
			//player.ship.LHp = 0;
			destroyShip();
		}
		if(key == 'n'){
			//player.ship.RHp = 0;
			destroyShip();
		}
		if(key == 'm'){
			//player.ship.BHp = 0;
			destroyShip();
		}

		/* simulate an upgrade */
		if(key == 'u'){
			upgradeShip();
		}

		/* End the asteroids game if the final level is done */
		if(key == 'j'){
//			if(level >= maxLevel && levelTextLifetime > -1){
				//Ideally we would want to run a spesific command in the asteroids to make the ship jump out of the asteroid
				//field and run certain lines that would be expected to be run when they finish the asteroid game properly,
				//such as save the metal/alloy values and set the highscore.

				//Right now just save the metal and alloy values and go to the system view
				player.metal += metalCount;
				player.alloy += alloyCount;
				changeState(STATE_SYSTEM);
//			}
		}

	}else if(state == STATE_HELP){
		if(key == 32){
			changeState(STATE_TITLE);
		}

	}else if(state == STATE_SHIPSELECT){
		if(key == 32 && cooldown == -1 && currentCooldown <= 0){
			changeState(STATE_ASTEROIDS);
		}
	}else if(state == STATE_SYSTEM){
		if(displayedHUD == 0){
			/* The camera's HUD is showing the current system's objects and nearby systems */
			if(key == 32){
				if(camera->focus == 0){
					/* The camera is focusing on the current system which the user has selected */
					systemSelectMenu();
				}else if(camera->focus == 1){
					/* The camera is focusing on the nearby systems relative to the current system */
					if(selectedSystem == 0){
						/* Selecting the current system */
					}else if(selectedSystem < nearbySystems){
						/* Selecting a nearby system to jump to */
						systemJump();
					}
				}
			}
			if(key == 'w'){
				/* Go up (substraction) the list of selectable options for hopefully all lists */
				/* Looking at the current system's astronomical objects */
				if(camera->focus == 0){
					selectedAstronomicalObject--;
					if(selectedAstronomicalObject < 0){
						selectedAstronomicalObject = 1 + currentSystem->planetCount;
					}
				}
				/* Looking at the list of nearby systems within jump distance */
				else if(camera->focus == 1){
					selectedSystem--;
					if(selectedSystem < 0){
						selectedSystem = nearbySystems;
					}
				}
			}
			if(key == 's'){
				/* Go down (addition) the list of selectable options for hopefully all lists */
				/* Looking at the current system's astronomical objects */
				if(camera->focus == 0){
					selectedAstronomicalObject++;
					if(selectedAstronomicalObject > 1 + currentSystem->planetCount){
						selectedAstronomicalObject = 0;
					}
				}
				/* Looking at the list of nearby systems within jump distance */
				else if(camera->focus == 1){
					selectedSystem++;
					if(selectedSystem >= nearbySystems){
						selectedSystem = 0;
					}
				}
			}

			if(key == 'j'){
				/* Pressing 'j' switches between cycling the current system's objects and the jumpable systems */
				/* Looking at the current system's astronomical objects */
				if(camera->focus == 0){
					camera->focus = 1;
				}
				/* Looking at the list of nearby systems within jump distance */
				else if(camera->focus == 1){
					camera->focus = 0;
				}
			}
		}else if(displayedHUD == 1 && windowState == 0){
			/* The camera's HUD is showing options that can be done with the selected astronomical object. Only
			 * allow inputs to be read if the communication window is not currently showing */
			if(key == 8){
				/* Pressing the backspace key will bring the user back one "page" in the HUD. In this case, back to the current system viewer */
				displayedHUD = 0;
			}
			/* Prevent the selectedHUD value from overflowing/underflowing */
			if(key == 'w'){
				selectedHUD--;
				if(selectedHUD < 0){
					selectedHUD = 0;
				}
			}
			if(key == 's'){
				selectedHUD++;
				if(selectedHUD >= selectedHUDMax){
					selectedHUD = selectedHUDMax - 1;
				}
			}
			if(key == 32){
				/* User selected an option with the currently selected object using the spacebar */
				systemSelectMenu();
			}
		}else if(displayedHUD == 2){
			/* The camera's HUD is showing the current selected planet's stats */
			if(key == 32 || key == 8){
				/* Return to the planet's first menu level of stats/scan/ring */
				displayedHUD = 1;
				selectedHUD = 0;
			}
		}else if(displayedHUD == 3){
			/* The camera's HUD is showing information relevent to scanning a planet */
			if(key == 8){
				/* Set all the currently inflight satellites to their end path. this assumes this is the only path our of the scan menu */
				updateSatellitePath(1);
				/* Return to the selected planet's options menu if backspace is pressed */
				displayedHUD = 1;
				selectedHUD = 1;
				/* Reset the selected planet's selectedHUDMax value since it's been overwritten entering this menu */
				selectedHUDMax = maxDefaultPlanetOptions;
				if(currentSystem->planet[selectedAstronomicalObject - 2]->type <= PLANET_TYPE_COLONIZED_LIMIT){
					/* Colonized planets have 1 extra option */
					selectedHUDMax++;
				}else if(currentSystem->planet[selectedAstronomicalObject - 2]->type <= PLANET_TYPE_RING_LIMIT){
					/* Ringed planets have 1 extra option */
					selectedHUDMax++;
				}else{
					/* default planets have no extra options */
				}
			}
			/* Prevent the selectedHUD value from overflowing/underflowing */
			if(key == 'w'){
				selectedHUD--;
				if(selectedHUD < 0){
					selectedHUD = 0;
				}
			}
			if(key == 's'){
				selectedHUD++;
				if(selectedHUD >= selectedHUDMax){
					selectedHUD = selectedHUDMax - 1;
				}
			}
			if(key == 32){
				/* User selected an option of scanning or probing the planet */
				systemSelectMenu();
			}
		}else if(displayedHUD == 4){
			/* The camera's HUD has minimal information and no input when entering a planet's ring */
		}
	}
}

void
keyPress(int key, int x, int y)
{
    /*
     *	this function is called when a special key is pressed; we are
     *	interested in the cursor keys only. Change the selected option
     *	if the game state is currently in a menu and prevent it's overflow
     */

    switch (key)
    {
        case 100:
            left = 1;
            break;
        case 101:
            up = 1;
            if(state == STATE_TITLE){
            	selectedOption++;
            }else if(state == STATE_SHIPSELECT){
                if(currentCooldown == 0){
                    cooldown = selectedOption;
                    selectedOption++;
                    currentCooldown = 30;
                }
            }
            break;
        case 102:
            right = 1;
            break;
        case 103:
            down = 1;
            if(state == STATE_TITLE){
            	selectedOption--;
            }else if (state == STATE_SHIPSELECT){
                if(currentCooldown == 0){
                    cooldown = selectedOption;
                    selectedOption--;
                    currentCooldown = 30;
                }
            }
            break;
    }

    /* prevent the selected option from overflowing and staying within it's bounds */
	if(selectedOption < 0){
		selectedOption += maxOption+1;
	}else if(selectedOption > maxOption){
		selectedOption -= maxOption+1;
	}
}

void
keyRelease(int key, int x, int y)
{
    /*
     *	this function is called when a special key is released; we are
     *	interested in the cursor keys only
     */

    switch (key)
    {
        case 100:
            left = 0; break;
        case 101:
            up = 0; break;
	case 102:
            right = 0; break;
        case 103:
            down = 0; break;
    }
}

void
myReshape(int newW, int newH)
{
    /*
     *	reshape callback function; the upper and lower boundaries of the
     *	window are at 100.0 and 0.0, respectively; the aspect ratio is
     *  determined by the aspect ratio of the viewport
     */

	/* prevent the window from getting too small or too large by stretching or cutting off the game */
	w = newW;
	h = newH;
	if(w < 500){
		w = 500;
	}else if(w > 4000){
		w = 4000;
	}
	if(h < 500){
		h = 500;
	}else if(h > 4000){
		h = 4000;
	}
    xMax = 100.0*w/h;
    yMax = 100.0;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, xMax, 0.0, yMax, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    /* Regenerate the starry background when the window is resized */
    initBackground(xMax, yMax);
}


/* -- Primary init function ------------------------------------------------------- */

void
init()
{
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glDepthFunc(GL_LEQUAL);

	/*
	 * Set parameters relevent to overwall program running such as lighting, window size, etc
	 */
    /* set the screens default size/maximum */
    xMax = 500;
    yMax = 500;

	/* Set values pertinent to lighting */
	GLfloat ambient[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat position[] = { 0, 0, 0, 1.0 };
	GLfloat shininess[] = {50.0};
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glLightfv(GL_LIGHT0, GL_SHININESS, shininess);
	glEnable(GL_LIGHT0);

	/* Initilize a new user's stats for the Player  */
	player.cooldown = 0;
	player.currentCooldown = 0;
	player.photonUpgrade = 0;
	player.photonSize = 0;
	player.photonSpeed = 0;
	player.jumpDistance = 50;
	player.mineralLimit = 0.1;
	player.energyLimit = 0.1;
	player.metal = 0;
	player.alloy = 0;
	player.notoriety = 0;
	player.fuel = 0;

	/*
     * Set parameters including the numbers of asteroids and photons present, the maximum velocity of the ship,
     * the velocity of the laser shots, the ship's coordinates and velocity, etc. Everything used with the asteroids game
     */
	initAsteroids(&player);


	/*
	 * Set parameters and initilize structs relevent to the 3D system explorer such as cameras and systems
	 */
	//Initialize the camera's values
	initSystemViewer();



	/* Load a previously created save file */
	/* If loading a previous savefile, start loading populating systemArray */
//	loadSavefile();

	/* Generate new random system data */
	randomSystem();

	/* Populate the backgroud stars array after the systemArray has been created */
	calculateBackgroundStars();

	/*
	 * start the game on the titlescreen of asteroids
	 */
	state = -1;
	changeState(STATE_TITLE);
//	changeState(STATE_SYSTEM);
}

/* -- Event Trigger Functions ------------------------------------------------- */

void
changeState(int s)
{
	/*
	 * If the new state is different, change values to fit the new state. Each state stays in either 2D or 3D
	 * and certain commands need to be run to ensure proper rendering. These states dont change between 2D or 3D.
	 * States that are in 2D:
	 * STATE_TITLE
	 * STATE_ASTEROIDS
	 * STATE_HELP
	 * STATE_SHIPSELECT
	 *
	 * States that are in 3D:
	 * STATE_SYSTEM
	 */
	//Disable backface culling whenever 3D rendering is done
	//glDisable(GL_CULL_FACE);

	if(state != s){
		clear();
		if(s == STATE_TITLE || s == STATE_ASTEROIDS || s == STATE_HELP || s == STATE_SHIPSELECT){
			/* 2D rendering */
			glDisable(GL_CULL_FACE);
			glDisable(GL_LIGHTING);
			myReshape(w, h);
		}else if(s == STATE_SYSTEM){
			/* 3D rendering */
			glEnable(GL_LIGHTING);
			glEnable(GL_CULL_FACE);
		}else{
			/* State has not been given a proper dimension rendering */
			printf("!!!State has not been given a proper dimension rendering!!!\n");
		}
		/* Set values dependent on the new state */
		if(s == STATE_TITLE){
			/* Spawn a few asteroids to float around the title screen */
			initAsteroid(&asteroids[0], ASTEROID_LARGE);
			asteroids[0].x = myRandom(0.0, xMax);
			asteroids[0].y = myRandom(0.0, yMax);
			initAsteroid(&asteroids[1], ASTEROID_MEDIUM);
			asteroids[1].x = myRandom(0.0, xMax);
			asteroids[1].y = myRandom(0.0, yMax);
			initAsteroid(&asteroids[2], ASTEROID_SMALL);
			asteroids[2].x = myRandom(0.0, xMax);
			asteroids[2].y = myRandom(0.0, yMax);

			/* set the option values to the titlescreen */
			selectedOption = 0;
			maxOption = 1;

		}else if(s == STATE_ASTEROIDS){
			/* change the ship to the default type and start the game */
			changeShip(0);
		}else if(s == STATE_HELP){
			/* set the objects on the screen to show a help guide */
			/* set up a stationary, rotating asteroid */
			initAsteroid(&asteroids[0], ASTEROID_SMALL);
			asteroids[0].x = xMax/12;
			asteroids[0].y = yMax*0.39;
			asteroids[0].phi = 0;
			asteroids[0].dx = 0;
			asteroids[0].dy = 0;
			asteroids[0].dphi = 0.1;
			asteroids[0].size = ASTEROID_SMALL;
			asteroids[0].active = 1;

			/* place a ship on the screen which the user can move aroud */
			setShipHelpScreen();

			upgradeText.lifetime = 0;

			/* place an idle photon shot */
			photons[0].active = 1;
			photonSize = 3;
			photons[0].x = xMax*0.90;
			photons[0].y = yMax*0.59;

			/* place a few pieces of debris */
			debris[0].active = 1;
			debris[0].x = xMax*0.03;
			debris[0].y = yMax*0.07;
			debris[0].dx = 0;
			debris[0].dy = 0;
			debris[0].phi = 0;
			debris[0].dphi = 0.05;
			debris[0].coords[0].x = 1.4;
			debris[0].coords[0].y = 0.9;
			debris[0].coords[1].x =	-1.5;
			debris[0].coords[1].y = -1.0;
			debris[0].coords[2].x = -1.0;
			debris[0].coords[2].y = 0.9;
			debris[0].lifetime = 2;
			debris[0].active = 1;
			debris[0].type = 1;

			debris[1].active = 1;
			debris[1].x = xMax*0.03;
			debris[1].y = yMax*0.15;
			debris[1].dx = 0;
			debris[1].dy = 0;
			debris[1].phi = 0;
			debris[1].dphi = 0.15;
			debris[1].coords[0].x = 1.0;
			debris[1].coords[0].y = 0.8;
			debris[1].coords[1].x =	-0.5;
			debris[1].coords[1].y = -1.4;
			debris[1].coords[2].x = -1.0;
			debris[1].coords[2].y = 0.9;
			debris[1].lifetime = 2;
			debris[1].active = 1;
			debris[1].type = 1;

			debris[2].active = 1;
			debris[2].x = xMax*0.92;
			debris[2].y = yMax*0.14;
			debris[2].dx = 0;
			debris[2].dy = 0;
			debris[2].phi = 0;
			debris[2].dphi = 0.08;
			debris[2].coords[0].x = 1.5;
			debris[2].coords[0].y = 1.8;
			debris[2].coords[1].x =	-1.2;
			debris[2].coords[1].y = -1.8;
			debris[2].coords[2].x = -1.6;
			debris[2].coords[2].y = 1.3;
			debris[2].lifetime = 2;
			debris[2].active = 1;
			debris[2].type = 2;
		}else if(s == STATE_SHIPSELECT){
			/* Show of the available ships the user can use */
			selectedOption = 0;
			maxOption = 2;
			currentCooldown = 30;
			cooldown = -1;
		}else if(s == STATE_SYSTEM){
			launchedShipPath = 0;
		}
	}
	state = s;
}

void
clear()
{
	int i;
	/*
	 * Wipe the screen of objects. Used to reset/change a state
	 */
	/* Remove all asteroids, dust, etc and set the level to 0 (advanceAsteroids will spawn one for level 1 if in the asteroids state) */
	level = 0;
	levelTextLifetime = -1;
	/* Set the max level the asteroids can reach */
	maxLevel = 3;
	for(i = 0; i < MAX_ASTEROIDS; i++){
		asteroids[i].active = 0;
	}
	for(i = 0; i < MAX_DEBRIS; i++){
		debris[i].active = 0;
	}
	for(i = 0; i < MAX_DUST; i++){
		dust[i].active = 0;
	}
	for(i = 0; i < MAX_PHOTONS; i++){
		photons[i].active = 0;
	}
	for(i = 0; i < MAX_POINTS; i++){
		points[i].active = 0;
	}

	/* Set ship default values */
	initShip();

	/* Set photon default values */
	photonSize = 1;
	photonSpeed = 2;
	photonUpgrade = 0;


	/* Set other variables to their default values */
	respawn = -1;
	player.cooldown = 0;
	player.currentCooldown = 0;
}

void
optionSelect(){
	/*
	 * Depending the currently selected option along with game-state, do certain actions
	 */
	if(selectedOption == 0){
		/* "play" is selected, which will put the game into the ship select state */
		changeState(STATE_SHIPSELECT);
	}else if(selectedOption == 1){
		/* "help" is selected, which will show the user a helpful screen on how to play */
		changeState(STATE_HELP);
	}
}

/* -- Timer Update Functions ------------------------------------------------- */


/* -- File reading/writing functions -------------------------------------------------- */

void
loadSavefile(){
	/*
	 * Load the game starting from an empty state. Start by loading systemArray and then use the integer on the
	 * final line to load the selectedSystem, which will be done with loadSystem().
	 */
	int i, ii, iii;
	FILE* loadStream = fopen("savefile", "r");
	/* Create temp variables to hold system values */
	int surfaceRows;
	i = -1;

	/* allocate enough memory to populate the systemArray */
	systemArray = malloc(sizeof(System*)*SYSTEM_COUNT);

	/* Loop the lines that populate the systemArray until we reach the final system line */
	while(i < SYSTEM_COUNT - 1){
		/* Scan the current system line and fetch the index */
		printf("index read: %d\n", fscanf(loadStream, "[%d]", &i));
		/* allocate the memory to create the system */
		systemArray[i] = malloc(sizeof(System));
		/* Scan the rest of the system line and opulate the allocated memory */
		printf("system read: %d\n", fscanf(loadStream, " %[a-zA-Z -], %lf, %lf, %lf, %d\n", systemArray[i]->name, &systemArray[i]->x,
				&systemArray[i]->y, &systemArray[i]->z, &systemArray[i]->planetCount));

		/* Push the stream pointer past the star line */
		fscanf(loadStream, "%*[^\n]%*c");

		/* Push the stream pointer past the planets and their surface values */
		for(ii = 0; ii < systemArray[i]->planetCount; ii++){
			/* Read in the planet's row count and skip past the rest of the line */
			printf("rows read: %d\n", fscanf(loadStream, "{%d, %*[^\n]%*c", &surfaceRows));
			for(iii = 0; iii < surfaceRows; iii++){
				/* Skip past the current surface line */
				fscanf(loadStream, "%*[^\n]%*c");
			}
		}
	}

	/* Passing the final surface along with its other values means the next line will be the final line denoting the last value of currentSystemIndex */
	printf("index read: %d\n", fscanf(loadStream, "[%d]", &i));
	loadSystem(i);

	/* Now that we have populated the systemArray and the currentSystem, close the stream */
	printf("start loading\n");
	fclose(loadStream);
	printf("done loading\n");
}

/* -- helper functions ------------------------------------------------------- */

double myRandom(double min, double max)
{
	/*
	 * return a random number uniformly draw from [min,max]
	 */
	double	d;
	d = min+(max-min)*(rand()%0x7fff)/32767.0;

	/* Return the value with up to 6 values past the decimal */
	return ceil(d*1000000)/1000000;
}
