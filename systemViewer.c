/*
 * This file contains all pertinent variables, constants and functions used with the system viewer
 * that moves the player between systems and planets
 */
#include "simple.h"


/* --- Type Definitions -------------------------------------------------------- */

/* The variables used to describe a planet/star
 * radius = radius of the sphere
 * orbitLong = the furthest a sphere can be in its orbit
 * orbitShort = the closest a sphere can be in its orbit
 * tiltAngleX = angle of the orbit rotated on the X axis
 * tiltAngleY = angle of the orbit rotated on the y axis
 */
typedef struct SystemStar{
	char name[NAME_LENGTH];
	double radius, orbitRadius, axialTilt, orbitTilt, orbitOffset, dayOffset, daySpeed, yearOffset, yearSpeed;
} SystemStar;

/* A basic struct which gives the information of a section of the surface on a planet. There are many of these on each planet */
typedef struct Surface{
	double satellite;
	double probe;
	double energy;
	double mineral;
} Surface;

/* Holds a complete definition of a planet, such as type and surface */
typedef struct SystemPlanet{
	char name[NAME_LENGTH];
	int surfaceRows, surfaceColumns;
	double radius, orbitRadius, axialTilt, orbitTilt, orbitOffset, dayOffset, daySpeed, yearOffset, yearSpeed;
	/* type determines the kind of planet. Different planet types so far:
	 * ringed 			= [0.00, 0.20]
	 * Normal/default 	= {0.20, 1.00]
	 */
	double type;
	Surface*** surface;
} SystemPlanet;

/* A unique type to hold more system information than the regular System struct. Used only by currentSystem */
typedef struct CurrentSystemType{
	char name[NAME_LENGTH];
	double x, y, z;
	int planetCount;
	SystemStar *star;
	SystemPlanet **planet;
} CurrentSystemType;

/* Hold basic information of a system */
typedef struct System{
	char name[NAME_LENGTH];
	double x, y, z;
	int planetCount;
} System;

/*
 * The camera's focus values determines what kind of view the user will see.
 * Certain things can only been seen in certain views, such as orbit lines or nearby systems.
 * (requires updating)
 * 0 = focus on the currentSystem with selectedEntity choosing what object to focus on (entire system, planet, etc).
 * 		Orbit lines are shown as transparent white lines. Selecting a planet makes it's orbit more clear.
 * 1 = focus on the currentSystem as a whole and show nearby systems with paths, names and distance.
 * 		selectedSystem being 0 means show all systems. Any other value indicates only drawing the selected system.
 * 2 = derived from 1, have the same camera angle of focus = 1 but show different text on the HUD. Obtained from
 * 		selecting an object in focus = 1 view. This view cannot change selectedSystem, but can change the camera angle.
 */
typedef struct Camera{
	int focus;
	double x, y, z, distance, xAngle, yAngle, focusLength;
	int camR, camC;
} Camera;

/* This may want to be formed with the coords, but that will require a lot of changes with asteroids.
 * This is just the coordinates of a point in 3D space */
typedef struct Point{
	double x, y, z;
} Point;


/* --- Local Variables ------------------------------------------------------------------------------ */

/* A pointer to the current system that the player resides in. Changes to this value will affect the values in the systemArray */
static CurrentSystemType *currentSystem;

/* The main systemArray which will hold pointers to all possible systems. Many variables point to systems here using indexes */
static System **systemArray;

/* The main view of the player when using the system view */
static Camera *camera;

/* A 2D array for each system holding their position in the 3D sky relative to the currentSystem. Changes on each system jump.
 * First values is the system's index in the systemArray and the second ranges between 0 and 2 to indicate it's x, y and z position */
static Point **BGStars;

/* The distance bewteen the current system and a system in the systemArray (equal sized arrays). Set when changing systems/jump distance */
static double* systemDistanceArray;

/* The index of the current system in the systemArray. Set when changing systems */
static int currentSystemIndex;

/* How many systems are 1 jumpDistance away from the current system. Set when calcualting the background stars */
static int nearbySystems;

/* A temporary value used to indicate which option is selected. Does not save between menus, so it is constantly overwritten between HUD changes */
static int selectedHUD;
/* The highest option that the selectedHUD values can be */
static int selectedHUDMax;

/* The max amount of default options for a planet when selected in the currentSystem view (selectedHUDMax) */
static int maxDefaultPlanetOptions;

/* The current position in it's path that the launched ship is in. Ranges from 0 to 1, decrements if above 0 */
static double launchedShipPath;

/* Preserve selected options in menus using these values. Initialized on creation and resets on system jump */
//Selected astronomical object in the system (star, planet). 0 is system, 1 is star, >1 is all planets. the values - 2 gives the first planet in the array
static int selectedAstronomicalObject;
//Selected nearby system that is within jumping range
static int selectedSystem;
/* What is displayed in front of the camera, usually in form of text forming the HUD.
 * 0 = nearby systems/intersystem objects
 * 1 = available options for the selectedAstronomicalObject
 * 2 = the selected planet's stats
 * 3 = scanning the selected planet
 * 4 = no text (flying to a ring/new planet)*/
static int displayedHUD;

/* The limit as to how well the user's satellite can detect energy and mineral sources on surfaces */
static double mineralLimit;
static double energyLimit;

/* The state of the communication window, with any value above 0 being "visibile" and prevent inputs */
static double windowState;

/* -- Color variables (3D) ------------------------------------------------------------------ */

/* HUD's text */
#define COLOR_HUD 0
GLfloat material_a_HUD[] = {1.0, 1.0, 1.0, 1.0};
GLfloat material_d_HUD[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_sp_HUD[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_HUD[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_HUD[] = {0.0};

/* Star in the current system */
#define COLOR_STAR 1
GLfloat material_a_Star[] = {1.0, 0.5, 0.0, 1.0};
GLfloat material_d_Star[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_sp_Star[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_Star[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_Star[] = {0.0};

/* Planet in the current system */
#define COLOR_PLANET 2
GLfloat material_a_planet[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_d_planet[] = {0.0, 0.0, 1.0, 1.0};
GLfloat material_sp_planet[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_planet[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_planet[] = {0.0};

/* Far away systems */
#define COLOR_BG 3
GLfloat material_a_BG[] = {1.0, 1.0, 1.0, 1.0};
GLfloat material_d_BG[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_sp_BG[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_BG[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_BG[] = {0.0};

/* Orbit lines */
#define COLOR_ORBIT 4
GLfloat material_a_orbit[] = {0.2, 0.2, 0.2, 0.2};
GLfloat material_d_orbit[] = {0.0, 0.0, 0.0, 0.2};
GLfloat material_sp_orbit[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_e_orbit[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_orbit[] = {0.0};

/* Planet rings */
#define COLOR_RING 5
GLfloat material_a_ring[] = {1.0, 1.0, 1.0, 0.5};
GLfloat material_d_ring[] = {1.0, 1.0, 1.0, 0.5};
GLfloat material_sp_ring[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_ring[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_ring[] = {0.0};

/* Scan grid */
#define COLOR_SCAN_GRID 6
GLfloat material_a_scan_grid[] = {1.0, 0.0, 0.0, 1.0};
GLfloat material_d_scan_grid[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_sp_scan_grid[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_scan_grid[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_scan_grid[] = {0.0};

/* selected scan grid */
#define COLOR_SCAN_GRID_SELECTED 7
GLfloat material_a_scan_grid_selected[] = {0.0, 1.0, 0.0, 0.5};
GLfloat material_d_scan_grid_selected[] = {0.0, 0.0, 0.0, 0.5};
GLfloat material_sp_scan_grid_selected[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_scan_grid_selected[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_scan_grid_selected[] = {0.0};

/* Satellites (satellites, probes) */
#define COLOR_SATELLITE 8
GLfloat material_a_satellite[] = {0.0, 1.0, 1.0, 1.0};
GLfloat material_d_satellite[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_sp_satellite[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_satellite[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_satellite[] = {0.0};

/* Energy icon (lightning bolt) */
#define COLOR_ENERGY 9
GLfloat material_a_energy[] = {1.0, 1.0, 0.0, 1.0};
GLfloat material_d_energy[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_sp_energy[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_energy[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_energy[] = {0.0};

/* Mineral icon (diamond) */
#define COLOR_MINERAL 10
GLfloat material_a_mineral[] = {0.0, 1.0, 1.0, 1.0};
GLfloat material_d_mineral[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_sp_mineral[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_mineral[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_mineral[] = {0.0};

/* Solid black color */
#define COLOR_BLACK 11
GLfloat material_a_black[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_d_black[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_sp_black[] = {0.0, 0.0, 0.0, 1.0};
GLfloat material_e_black[] = {0.0, 0.0, 0.0, 0.0};
GLfloat material_sh_black[] = {0.0};


/* --- Function prototypes --------------------------------------------------- */

/* Initilization functions that create new in-game objects */
static void initSystemViewer();

/* General functions that are run on specific event triggers */
static void randomSystem();
static void calculateBackgroundStars();
static void systemJump();
static void systemSelectMenu();
static void gridIncrement(int incrementor, double* value, double radius, double multiplier);
static void systemLaunchSatellite();
static void setColor(int colorID);
static void setColorValue(double r, double g, double b);

/* Drawing functions which end up adding some kind of visual element to the window */
static void drawSystem();
static void drawSystemStar();
static void drawSystemPlanet();
static void drawSystemBackground();
static void drawCameraHUD();
static void drawCamera();
static void drawOrbitLines();
static void drawRings();
static void drawShipPath();
static void draw3DShip();
static void draw3DSatellite();
static void drawSatellites();
static void drawSurfaceContents(int i, int r, int c, double leftX, double leftY, double leftZ, double middleX, double middleY, double middleZ, double rightX, double rightY, double rightZ);
static void drawPlanetStation();
static void drawWindow();
static void drawWindowPlayerIcon(double L, double R, double T, double B);
static void drawWindowStationIcon(double L, double R, double T, double B);

/* General functions that are run on every tick. Used to increment/decrement values or check conditions */
static void advanceCamera();
static void advanceSystem();
static void updateShipPath();
static void updateSatellitePath(int increment);

/* Functions that operate with outside files such as the savefile */
static void loadSystem(int newSystemIndex);
static void saveSystem();


/* --- Initilization functions ------------------------------------------------------------------ */

void
initSystemViewer()
{
	int i;
	/*
	 * Initilize all the variables and structures needed to run the 3D system viewer
	 */
	camera = malloc(sizeof(Camera));
	camera->focus = 0;
	camera->xAngle = 0;
	camera->yAngle = M_PI/3;
	camera->focusLength = 1.0;

	/* Initialize the currentSystem variable */
//	currentSystem = malloc(sizeof(CurrentSystemType*));
//	currentSystem->star = malloc(sizeof(SystemStar*));
//	currentSystem->planet = malloc(sizeof(SystemPlanet**));
	systemArray = malloc(sizeof(System*)*SYSTEM_COUNT);
	for(i = 0; i < SYSTEM_COUNT; i++){
		systemArray[i] = malloc(sizeof(System));
	}

	/* Initialize a set amount of systems to create and the array of nearby systems*/
	/* Initialize values that determine the state of the system viewer such as HUD. Selected planet is set at system load */
	selectedSystem = 0;
	selectedAstronomicalObject = 0;
	displayedHUD = 0;
	selectedHUD = 0;
	selectedHUDMax = 0;
	systemDistanceArray = malloc(sizeof(double)*SYSTEM_COUNT);

	/* Set the initial value for when the ship i launched in 3D space */
	launchedShipPath = 0;

	/* Set the amount of options that can be selected when selecting a planet. Start with only 2 selectable option: stats and scan */
	maxDefaultPlanetOptions = 2;

	/* Initialize the array to hold all background stars */
	BGStars = malloc(sizeof(Point*)*SYSTEM_COUNT);
	for(i = 0; i < SYSTEM_COUNT; i++){
		BGStars[i] = malloc(sizeof(Point));
	}

	/* Set the window state to start at 0 */
	windowState = 0;
}


/* --- Drawing functions -------------------------------------------------------------------------- */

void
drawSystem()
{
	/*
	 * Draw the planets and stars that make up the current selected system using the
	 * current camera's focus values as a reference as to what to draw
	 */

	/* Depending on the state of the window, either display the HUD text or the "vidwindow" */
	if(windowState <= 0){
		/* Draw text to be relative to the camera to simulate a HUD */
		setColor(COLOR_HUD);
		drawCameraHUD();
	}else{
		/* Draw a communication window when selecting a station */
		setColor(COLOR_SCAN_GRID_SELECTED);
		drawWindow();
	}

	/* Retain the matrix position to be able to apply drawing functions using a matrix relative to the camera's position */
	glPushMatrix();

	/* Set the camera's focus point */
	drawCamera();

	/* Draw the system's stars */
	setColor(COLOR_STAR);
	drawSystemStar();

	/* Draw the system's planets */
	setColor(COLOR_PLANET);
	drawSystemPlanet();

	/* Draw the planet's stations for colonized planets */
	setColor(COLOR_HUD);
	drawPlanetStation();

	/* Draw dots far from the origin to simulate far away stars. To keep consistency, they must
	 * all have the same distance between them and the center, forming a circle "skybox".
	 * Take into account the camera posittion when observing stars. Stars within a certain
	 * distance will have their names displayed. Ignore the current system in the systemArray. */
	setColor(COLOR_BG);
	drawSystemBackground();

	/* Draw the system's planets orbit lines */
	setColor(COLOR_ORBIT);
	drawOrbitLines();

	/* Draw transparent rings on certain planets */
	setColor(COLOR_RING);
	drawRings();

	glPopMatrix();
	/* Draw a white wall in front of the camera to simulate a fade effect when entering a ring */
	if(launchedShipPath != 0){
		/* Start fading at 0.3 and reach max at 0.2 */
		GLfloat material_a_fade[] = {1.0, 1.0, 1.0, ((1-launchedShipPath) - 0.7)/0.1};
		GLfloat material_d_fade[] = {0.0, 0.0, 0.0, ((1-launchedShipPath) - 0.7)/0.1};
		GLfloat material_sp_fade[] = {0.0, 0.0, 0.0, 0.0};
		GLfloat material_e_fade[] = {0.0, 0.0, 0.0, 0.0};
		GLfloat material_sh_fade[] = {0.0};
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_fade);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_fade);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_fade);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_fade);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_fade);
		glBegin(GL_QUADS);
		glVertex3d(-w/1000.0, h/1000.0, -1);
		glVertex3d(-w/1000.0, -h/1000.0, -1);
		glVertex3d(w/1000.0, -h/1000.0, -1);
		glVertex3d(w/1000.0, h/1000.0, -1);
		glEnd();
		/* change state at 0.15 */
		if(launchedShipPath <= 0.15){
			changeState(STATE_TITLE);
		    glMatrixMode(GL_PROJECTION);
		    glLoadIdentity();
		    glOrtho(0.0, xMax, 0.0, yMax, -1.0, 1.0);
		    glMatrixMode(GL_MODELVIEW);
		    displayedHUD = 1;
		}
	}
}

void
drawCameraHUD()
{
	/*
	 * Draw text close to the camera that displays the system's astronomical objects to simmulate a HUD
	 * relative to the camera. Further information can be accessed from a system when the user selects an
	 * object, changing the value of displayedHUD. This can entail object stats or planet type specifics
	 */
	int close;
	int i;
	int position;
	char output[NAME_LENGTH];

	/* Draw text 1 unit infront of the camera (-z values). Anything closer wont render due to Znear.
	 * Screen extremities are [-0.5, 0.5], with x being left and right, y being up and down. */
	/* Draw the name of the system on the top left corner along with it's solar entities */
	glColor3f(1.0, 1.0, 1.0);
	position = 1;
	//An arbitrary equation for y keeps the words together on resolution changes. This line is at the top-left
	glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
	/* Depending on the value of displayedHUD and camera->focus, have different information drawn on the top left of the screen*/
	if(displayedHUD == 0){
		close = 0;
		/* The default display view which shows current system objects and nearby systems. Allows changing of
		 * the selectedAstronomicalObject and selectedSystem, making this the default dynamic option */
		if(camera->focus == 1){
			/* Display the names of nearby outside systems as a HUD*/
		    for(i = 0; i < SYSTEM_COUNT; i++){
		    	/* Ignore the current system and systems too far away */
		    	if(currentSystemIndex != i && systemDistanceArray[i] < player.jumpDistance){
					/* Display the name of every system that reaches this point */
					close++;
					glRasterPos3f((-xMax/2)/107.25, 0.4475 - (close-1)*0.0225 + close*((h-500)/60000), -1);
					if(selectedSystem == close){
						drawString("> ");
					}
					drawString(systemArray[i]->name);
		    	}
		    }
		}else if(camera->focus == 0){
			/* Display currentSystem's stars and planets names*/
			if(selectedAstronomicalObject == 0){
				drawString("> ");
			}
			drawString(currentSystem->name);
			position++;
			/* Display the star */
			glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
			if(selectedAstronomicalObject == 1){
				drawString("> ");
			}
			drawString(currentSystem->star->name);
			position++;

			/* Display the planets */
			for(i = 0; i < currentSystem->planetCount; i++){
				glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
				if(selectedAstronomicalObject == i + 2){
					drawString("> ");
				}
				drawString(currentSystem->planet[i]->name);
				position++;
			}
		}
	}else if(displayedHUD == 1){
		/* First level deep into the HUD, there are four selectable options with planets:
		 * 0 - stats = change the menu text to a bunch of stats of the planet
		 * 1 - scan = scan the planet for any information
		 * 2 - ring = If the planet has a ring, give the user the option to enter
		 * 2 - station = Colonized planets have stations which the player can visit as a store */
		if(camera->focus == 0){
			if(selectedAstronomicalObject == 0){
				/* Selected the entire system */
			}else if(selectedAstronomicalObject <= 1){
				/* Selected a system star */
			}else if(selectedAstronomicalObject <= 1 + currentSystem->planetCount){
				/* selected a system planet */
				/* Check if the first option, stats, is selected */
				glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
				position++;
				if(selectedHUD % selectedHUDMax == 0){
					drawString("> ");
				}
				drawString("Stats");

				/* Check if the second option, scan, is selected */
				glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
				position++;
				if(selectedHUD % selectedHUDMax == 1){
					drawString("> ");
				}
				drawString("Scan");

				/* Check if the third option is selected */
				if(currentSystem->planet[selectedAstronomicalObject - 2]->type <= PLANET_TYPE_COLONIZED_LIMIT){
					/* Check if the third option, station, is selected for a colonized planet */
					glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
					position++;
					if(selectedHUD % selectedHUDMax == 2){
						drawString("> ");
					}
					drawString("Station");
				}else if(currentSystem->planet[selectedAstronomicalObject - 2]->type <= PLANET_TYPE_RING_LIMIT){
					/* Check if the third option, ring, is selected for a ringed planet */
					glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
					position++;
					if(selectedHUD % selectedHUDMax == 2){
						drawString("> ");
					}
					drawString("Ring");
				}
			}
		}
	}else if(displayedHUD == 2){
		/* Display the current planet's stats on the top left */
		/* Display the planet's name */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		sprintf(output, "%s", currentSystem->planet[selectedAstronomicalObject - 2]->name);
		drawString(output);

		/* Display the planet's type */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		if(currentSystem->planet[selectedAstronomicalObject - 2]->type <= 0.2){
			drawString("Ringed Planet");
		}else if(currentSystem->planet[selectedAstronomicalObject - 2]->type > 0.2){
			drawString("Planet");
		}

		/* Display the planet's radius */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		sprintf(output, "Planet radius: %f", currentSystem->planet[selectedAstronomicalObject - 2]->radius);
		drawString(output);

		/* Display the planet's radius */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		sprintf(output, "Orbit radius: %f", currentSystem->planet[selectedAstronomicalObject - 2]->orbitRadius);
		drawString(output);

		/* Display the planet's axial tilt in degrees */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		sprintf(output, "Axial tilt: %f", currentSystem->planet[selectedAstronomicalObject - 2]->axialTilt);
		drawString(output);

		/* Display the planet's orbit tilt in degrees */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		sprintf(output, "Orbit tilt: %f", currentSystem->planet[selectedAstronomicalObject - 2]->orbitTilt);
		drawString(output);

		/* Display the planet's day length */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		sprintf(output, "Day length: %f", 1/currentSystem->planet[selectedAstronomicalObject - 2]->daySpeed);
		drawString(output);

		/* Display the planet's year length */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		sprintf(output, "Year length: %f", 1/currentSystem->planet[selectedAstronomicalObject - 2]->yearSpeed);
		drawString(output);
	}else if(displayedHUD == 3){
		/* Show surface scan information for the user */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		if(selectedHUD % selectedHUDMax == 0){
			drawString("> ");
		}
		drawString("Scan the planet's surface");
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		if(selectedHUD % selectedHUDMax == 1){
			drawString("> ");
		}
		drawString("Probe the planet's surface");
	}else if(displayedHUD == 4){
		/* Tell the user they are entering the planet's rings */
		glRasterPos3f((-xMax/2)/107.25, 0.4475 - (position-1)*0.0225 + position*((h-500)/60000), -1);
		position++;
		sprintf(output, "Entering %s's ring...", currentSystem->planet[selectedAstronomicalObject - 2]->name);
		drawString(output);
	}
}

void
drawCamera()
{
	/*
	 * Draw the camera's position and lookAt target. The target and position is
	 * modified by the camera's focus value
	 */

	if((camera->focus == 0 && selectedAstronomicalObject == 0) || camera->focus == 1){
		/* Focus on the entire system */
		//An odd error comes up when gluLookAt(0, ...) is done. Prevent this from happening by limiting the angles
		gluLookAt(((currentSystem->planet[currentSystem->planetCount-1]->orbitRadius + currentSystem->star->radius)*1.2)*cos(camera->xAngle),
				((currentSystem->planet[currentSystem->planetCount-1]->orbitRadius + currentSystem->star->radius)*1.2)*cos(camera->yAngle),
				((currentSystem->planet[currentSystem->planetCount-1]->orbitRadius + currentSystem->star->radius)*1.2)*sin(camera->xAngle),
				0, 0, 0, 0.0, 1.0, 0.0);
		glPushMatrix();
		glTranslated(currentSystem->planet[currentSystem->planetCount-1]->orbitRadius*1.2*cos(camera->xAngle),
				currentSystem->planet[currentSystem->planetCount-1]->orbitRadius*cos(camera->yAngle),
				currentSystem->planet[currentSystem->planetCount-1]->orbitRadius*1.2*sin(camera->xAngle));
		//glRotatef(360*camera->yAngle/(2*M_PI), 0, 0, 1);
		//glutWireSphere(2.0, 5, 5);
		glPopMatrix();
	}else if(selectedAstronomicalObject <= 1){
		/* Focus on the system's selected star */
		gluLookAt((currentSystem->star->radius*7.5)*cos(camera->xAngle)*sin(camera->yAngle),
				(currentSystem->star->radius*7.5)*cos(camera->yAngle),
				(currentSystem->star->radius*7.5)*sin(camera->xAngle)*sin(camera->yAngle),
				0, 0, 0, 0.0, 1.0, 0.0);
		/* Reverse the render process of the star to focus on to return the matrix to the origin (inside the main star))*/
		glRotated(90, 1, 0, 0);
		glRotated(360*-currentSystem->star->dayOffset, 0.0, 0.0, 1.0);
		glRotated(-currentSystem->star->axialTilt, 0.0, 1.0, 0.0);
		glRotated(-(90.0 + currentSystem->star->orbitTilt), 1.0, 0.0, 0.0);
		glTranslated(-cos(2*M_PI*currentSystem->star->yearOffset)*currentSystem->star->orbitRadius, 0,
				-sin(2*M_PI*currentSystem->star->yearOffset)*currentSystem->star->orbitRadius);
		glRotated(-currentSystem->star->orbitTilt, 1.0, 0.0, 0.0);
		glRotated(-currentSystem->star->orbitOffset, 0.0, 1.0, 0.0);
	}else if(selectedAstronomicalObject <= currentSystem->planetCount + 1){
		/* Focus on the system's selected planet */
		gluLookAt(((2 + currentSystem->planet[selectedAstronomicalObject - 2]->radius/4.0)*7.5)*cos(camera->xAngle)*sin(camera->yAngle),
				((2 + currentSystem->planet[selectedAstronomicalObject - 2]->radius/4.0)*7.5)*cos(camera->yAngle),
				((2 + currentSystem->planet[selectedAstronomicalObject - 2]->radius/4.0)*7.5)*sin(camera->xAngle)*sin(camera->yAngle),
				0, 0, 0, 0.0, 1.0, 0.0);
		/* Reverse the render process of the planet to focus on to return the matrix to the origin (inside the main star)*/
		glRotated(90, 1, 0, 0);
		glRotated(360*-currentSystem->planet[selectedAstronomicalObject-2]->dayOffset, 0.0, 0.0, 1.0);
		glRotated(-currentSystem->planet[selectedAstronomicalObject-2]->axialTilt, 0.0, 1.0, 0.0);
		glRotated(-(90.0 + currentSystem->planet[selectedAstronomicalObject-2]->orbitTilt), 1.0, 0.0, 0.0);
		glTranslated(-cos(2*M_PI*currentSystem->planet[selectedAstronomicalObject-2]->yearOffset)*currentSystem->planet[selectedAstronomicalObject-2]->orbitRadius, 0,
				-sin(2*M_PI*currentSystem->planet[selectedAstronomicalObject-2]->yearOffset)*currentSystem->planet[selectedAstronomicalObject-2]->orbitRadius);
		glRotated(-currentSystem->planet[selectedAstronomicalObject-2]->orbitTilt, 1.0, 0.0, 0.0);
		glRotated(-currentSystem->planet[selectedAstronomicalObject-2]->orbitOffset, 0.0, 1.0, 0.0);
	}
}

void
drawSystemStar()
{
	/*
	 * Draw the current system's star
	 */

	glColor3f(1.0, 0.5, 0.2);
	glPushMatrix();
	glRotated(currentSystem->star->orbitTilt, 1.0, 0.0, 0.0);
	//Initial rotation of the orbit line
	glTranslated(currentSystem->star->orbitRadius, 0, 0);
	glRotated(90.0 + currentSystem->star->orbitTilt, 1.0, 0.0, 0.0);
	glRotated(360*currentSystem->star->dayOffset, 0.0, 0.0, 1.0);
	glRotated(currentSystem->star->axialTilt, 0.0, 1.0, 0.0);
	glutSolidSphere(currentSystem->star->radius,30,20);

	/* Position the primary light source inside the star*/
	GLfloat position[] = { 0, 0, 0, 1.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glPopMatrix();
}

void
drawSystemPlanet()
{
	/*
	 * Draw the current system's planets
	 */
	int i;
	int r, c;
	camera->camR = -1;
	camera->camC = -1;
	int odd = 0;
	double ii, iii;
	double nextii;
	double largest = 0;

	for(i = 0; i < currentSystem->planetCount; i++){
		/* Position and draw the planet */
		glColor3f(0.5, 1.0, 1.0);
		glPushMatrix();
		glRotated(currentSystem->planet[i]->orbitOffset, 0.0, 1.0, 0.0);
		glRotated(currentSystem->planet[i]->orbitTilt, 1.0, 0.0, 0.0);
		glTranslated(cos(2*M_PI*currentSystem->planet[i]->yearOffset)*currentSystem->planet[i]->orbitRadius, 0,
				sin(2*M_PI*currentSystem->planet[i]->yearOffset)*currentSystem->planet[i]->orbitRadius);
		glRotated(90.0 + currentSystem->planet[i]->orbitTilt, 1.0, 0.0, 0.0);
		glRotated(currentSystem->planet[i]->axialTilt, 0.0, 1.0, 0.0);
		glRotated(360*currentSystem->planet[i]->dayOffset, 0.0, 0.0, 1.0);
		setColor(COLOR_PLANET);
		glutSolidSphere(currentSystem->planet[i]->radius,30,20);

		/* Draw the satellites currently in flight for the planet that is currently selected */
		if(selectedAstronomicalObject == i + 2){
			glPushMatrix();
			glRotated(90, 1, 0, 0);
			setColor(COLOR_SATELLITE);
			drawSatellites(i);
			glPopMatrix();
			setColor(COLOR_PLANET);
		}

		/* Draw a ship flying to the planet's rings if the ring option was selected for this planet and this planet is currently focused on */
		if(launchedShipPath != 0 && selectedAstronomicalObject == i + 2){
			glPushMatrix();
			glRotated(180*camera->xAngle/M_PI, 0.0, 0.0, 1.0);
			drawShipPath();
			glPopMatrix();
		}

		/* Draw the scanning grid around the planet if the user selected the scanning option for this planet. Initialize a set
		 * of variables used to draw the grid and to specify which section the camera is focusing on */
		if(displayedHUD == 3 && selectedAstronomicalObject == i + 2){
			double Ax, Ay, Az, Bx, By, Bz, Cx, Cy, Cz;
			double ABx, ABy, ABz, ACx, ACy, ACz;
			double Nx, Ny, Nz, d;
			double camX, camY, camZ, value;
			glRotated(90, 1, 0, 0);
			setColor(COLOR_SCAN_GRID);

			/* To find whether the camera's focus point is within a section, use the three points of the section's triangle to form
			 * a plane. If the camera's focus point is above the plane (positive when put into the plane equation) then it's within the section.
			 * Any value behind the plane (negative) would be on the outside of the sector due to the spherical nature of the planet */
			/* Find the value obtained when placing the camera's point onto the planet's scanning grid */
			camX = currentSystem->planet[i]->radius*1.2*cos(camera->xAngle)*sin(camera->yAngle);
			camY = -currentSystem->planet[i]->radius*1.2*cos(camera->yAngle);
			camZ = -currentSystem->planet[i]->radius*1.2*sin(camera->xAngle)*sin(camera->yAngle);

			/* Draw a grid comprised of triangles around the planet that ends before the top and bottom of the planet */
			odd = 0;
			r = 0;
			/* The ii value determines the rows from the top to bottom of the planet, where 0.0 is at
			 * the bottom and 0.5 is at the top. values above 0.5 simply continue past the top and around the sphere. */
			for(ii = 0.15; ceil(ii*100000)/100000 < 0.35; gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1)){
				r++;
				c = 0;
				/* The iii value determines the width of the columns around the sphere. The larger the jump between values, the wider the space */
				for(iii = 0; ceil(iii*100000)/100000 < 1; gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1)){
					/* Calcualte the points used to for a triangle section of the grid. The grid is formed of two triangle types:
					 * One points up and the other points down. There are equal amounts of both and always begin with the right-side up */
					c++;
					if(odd){
						/* If it's an odd row (1st from the bottom, 3rd from the bottom, etc) add an offset to the triangles */
						gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5);
					}
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.025000);
					Ax = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					Ay = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Az = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.025000);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -0.025000);

					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1 -0.025000);
					Bx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					By = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Bz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -1 +0.025000);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -0.025000);

					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1 -0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5);
					Cx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					Cy = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Cz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -1 +0.025000);
					if(odd){
						/* Revert the offset if there was one applied */
						gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5);
					}

					/* Use the set points to draw the section */
					/* Make the grid color relative to the status of it's satellites */
					if(currentSystem->planet[i]->surface[r-1][c-1]->satellite >= 1 || currentSystem->planet[i]->surface[r-1][c-1]->probe >= 1){
						if(currentSystem->planet[i]->surface[r-1][c-1]->satellite >= 1){
							if(currentSystem->planet[i]->surface[r-1][c-1]->probe >= 1){
								/* satellite and probe are present, making the color green */
								setColorValue(0, 1, 0);
							}else if(currentSystem->planet[i]->surface[r-1][c-1]->energy > energyLimit && currentSystem->planet[i]->surface[r-1][c-1]->mineral > mineralLimit){
								/* Satellite is present without a probe and cannot detect energy/mineral, making the color green */
								setColorValue(0, 1, 0);
							}else{
								/* Satellite is present without a probe, and there is a source of energy/mineral detected, making it yellow */
								setColorValue(1, 1, 0);
							}
						}else if(currentSystem->planet[i]->surface[r-1][c-1]->probe >= 1){
							/* No satellite but probe is present, making it blue */
							setColorValue(0, 0, 1);
						}
					}else{
						/* Set the color to red if there are no satellites in orbit */
						setColorValue(1, 0, 0);
					}


					glBegin(GL_LINES);
					glVertex3f(Ax, Ay, Az);
					glVertex3f(Bx, By, Bz);
					glVertex3f(Bx, By, Bz);
					glVertex3f(Cx, Cy, Cz);
					glVertex3f(Cx, Cy, Cz);
					glVertex3f(Ax, Ay, Az);
					glEnd();

					/* Check if this is the row that the camera is focusing on to find the section the user is selecting */
					nextii = ii;
					gridIncrement(0, &nextii, currentSystem->planet[i]->radius, 1);
					//(ii + 0.2/ceil((currentSystem->planet[i]->radius + 5)/3))*2*M_PI
					if(currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI) >=
							-currentSystem->planet[i]->radius*1.2*cos(camera->yAngle) &&
							currentSystem->planet[i]->radius*1.2*cos(nextii*2*M_PI) <
							-currentSystem->planet[i]->radius*1.2*cos(camera->yAngle)){
						/* Use the cross product to find two vectors used to form the plane of the section */
						ABx = Bx - Ax;
						ABy = By - Ay;
						ABz = Bz - Az;
						ACx = Cx - Ax;
						ACy = Cy - Ay;
						ACz = Cz - Az;

						/* Find the normal vector of the plane */
						Nx = (ABy * ACz) - (ABz * ACy);
						Ny = (ABz * ACx) - (ABx * ACz);
						Nz = (ABx * ACy) - (ABy * ACx);

						/* Plane equation: Nx*x + Ny*y + Nz*z + d = 0. Plug in any point (in our case, [Ax, Ay, Az]) to find the value of d */
						d = -(Nx*Ax + Ny*Ay + Nz*Az);

						/* Find the value obtained when placing the camera's point onto the plane */
						camX = currentSystem->planet[i]->radius*1.2*cos(camera->xAngle)*sin(camera->yAngle);
						camY = -currentSystem->planet[i]->radius*1.2*cos(camera->yAngle);
						camZ = -currentSystem->planet[i]->radius*1.2*sin(camera->xAngle)*sin(camera->yAngle);
						value = Nx*camX + Ny*camY + Nz*camZ + d;

						/* Save the largest value and track what row and column it was in */
						if(largest < value){
							largest = value;
							camera->camC = c;
							camera->camR = r;
						}
					}

					/* Draw the surface's contents if there is a probe or satellite in orbit */
					if(currentSystem->planet[i]->surface[r-1][c-1]->satellite >= 1 || currentSystem->planet[i]->surface[r-1][c-1]->probe >= 1){
						drawSurfaceContents(i, r-1, c-1, Ax, Ay, Az, Cx, Cy, Cz, Bx, By, Bz);
					}

					/*
					 * Now initialize and draw the upside-down triangle that points down. It is the same general process as before
					 */
					c++;
					if(odd){
						/* If it's an odd row (1st from the bottom, 3rd from the bottom, etc) add an offset to the triangles */
						gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5);
					}

					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1.5 -0.025000);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1 -0.025000);
					Ax = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					Ay = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Az = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -1 +0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -1.5 +0.025000);

					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1 -0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5 +0.025000);
					Bx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					By = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Bz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5 -0.025000);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -1 +0.025000);

					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1);
					Cx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					Cy = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Cz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -1);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -0.025000);

					if(odd){
						/* Revert the offset if there was one applied */
						gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5);
					}

					/* Use the set points to draw the section */
					/* Make the grid color relative to the status of it's satellites */
					if(currentSystem->planet[i]->surface[r-1][c-1]->satellite >= 1 || currentSystem->planet[i]->surface[r-1][c-1]->probe >= 1){
						if(currentSystem->planet[i]->surface[r-1][c-1]->satellite >= 1){
							if(currentSystem->planet[i]->surface[r-1][c-1]->probe >= 1){
								/* satellite and probe are present, making the color green */
								setColorValue(0, 1, 0);
							}else if(currentSystem->planet[i]->surface[r-1][c-1]->energy > energyLimit && currentSystem->planet[i]->surface[r-1][c-1]->mineral > mineralLimit){
								/* Satellite is present without a probe and cannot detect energy/mineral, making the color green */
								setColorValue(0, 1, 0);
							}else{
								/* Satellite is present without a probe, and there is a source of energy/mineral detected, making it yellow */
								setColorValue(1, 1, 0);
							}
						}else if(currentSystem->planet[i]->surface[r-1][c-1]->probe >= 1){
							/* No satellite but probe is present, making it blue */
							setColorValue(0, 0, 1);
						}
					}else{
						/* Set the color to red if there are no satellites in orbit */
						setColorValue(1, 0, 0);
					}

					glBegin(GL_LINES);
					glVertex3f(Ax, Ay, Az);
					glVertex3f(Bx, By, Bz);
					glVertex3f(Bx, By, Bz);
					glVertex3f(Cx, Cy, Cz);
					glVertex3f(Cx, Cy, Cz);
					glVertex3f(Ax, Ay, Az);
					glEnd();

					/* Check if this is the row that the camera is focusing on to find the section the user is selecting */
					nextii = ii;
					gridIncrement(0, &nextii, currentSystem->planet[i]->radius, 1);
					if(currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI) >=
							-currentSystem->planet[i]->radius*1.2*cos(camera->yAngle) &&
							currentSystem->planet[i]->radius*1.2*cos(nextii*2*M_PI) <
							-currentSystem->planet[i]->radius*1.2*cos(camera->yAngle)){
						/* Use the cross product to find two vectors used to form the plane of the section */
						ABx = Bx - Ax;
						ABy = By - Ay;
						ABz = Bz - Az;
						ACx = Cx - Ax;
						ACy = Cy - Ay;
						ACz = Cz - Az;

						/* Find the normal vector of the plane */
						Nx = (ABy * ACz) - (ABz * ACy);
						Ny = (ABz * ACx) - (ABx * ACz);
						Nz = (ABx * ACy) - (ABy * ACx);

						/* Plane equation: Nx*x + Ny*y + Nz*z + d = 0. Plug in any point (in our case, [Ax, Ay, Az]) to find the value of d */
						d = -(Nx*Ax + Ny*Ay + Nz*Az);

						/* Find the value obtained when placing the camera's point onto the plane */
						camX = currentSystem->planet[i]->radius*1.2*cos(camera->xAngle)*sin(camera->yAngle);
						camY = -currentSystem->planet[i]->radius*1.2*cos(camera->yAngle);
						camZ = -currentSystem->planet[i]->radius*1.2*sin(camera->xAngle)*sin(camera->yAngle);
						value = Nx*camX + Ny*camY + Nz*camZ + d;

						/* Save the largest value and track what row and column it was in */
						if(largest < value){
							largest = value;
							camera->camC = c;
							camera->camR = r;
						}
					}

					/* Draw the surface's contents if there is a probe or satellite in orbit */
					if(currentSystem->planet[i]->surface[r-1][c-1]->satellite >= 1 || currentSystem->planet[i]->surface[r-1][c-1]->probe >= 1){
						drawSurfaceContents(i, r-1, c-1, Bx, By, Bz, Cx, Cy, Cz, Ax, Ay, Az);
					}
				}

				/* switch between odd and even after each row change */
				if(odd){
					odd = 0;
				}else{
					odd = 1;
				}
			}
			glEnd();

			printf("____LARGEST VALUE___%d_%d\n", camera->camR, camera->camC);

			/* Using what we found to be the currently selected row and column, use camJ and camK as indicators for the selected section
			 * and get the appropriate values for ii and iii to specify the bouderies of the selected section*/
			/* Ensure that there is a section that is selected - that the camera's position is neither above or bellow the selectable region */
			if(camera->camC != -1 && camera->camR != -1){
				r = 1;
				for(ii = 0.15; r < camera->camR; gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1)){
					r++;
				}
				c = 2;
				/* Change the starting value depending on whether an upside down triangle is selected */
				if(camera->camC % 2){
					c--;
				}
				for(iii = 0; c < camera->camC; gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1)){
					c++;
					c++;
				}

				/* If the row is odd, insert an offset */
				if(!(camera->camR % 2)){
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5);
				}

				/* Fill in the point values using the values of ii and iii as the selected sector values */
				if(c % 2){
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.025000);
					Ax = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					Ay = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Az = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.025000);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -0.025000);

					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1 -0.025000);
					Bx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					By = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Bz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -1 +0.025000);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -0.025000);

					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1 -0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5);
					Cx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					Cy = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Cz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -1 +0.025000);
				}else{
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1.5 -0.025000);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1 -0.025000);
					Ax = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					Ay = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Az = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -1 +0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -1.5 +0.025000);

					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1 -0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5 +0.025000);
					Bx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					By = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Bz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5 -0.025000);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -1 +0.025000);

					gridIncrement(0, &ii, currentSystem->planet[i]->radius, 0.025000);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1);
					Cx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
					Cy = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
					Cz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -1);
					gridIncrement(0, &ii, currentSystem->planet[i]->radius, -0.025000);
				}

//				/* Draw spheres on each point used to find the sections */
//				glPushMatrix();
//				glTranslatef(Ax, Ay, Az);
//				glutWireSphere(0.25, 10, 10);
//				glPopMatrix();
//
//				glPushMatrix();
//				glTranslatef(Bx, By, Bz);
//				glutWireSphere(0.25, 10, 10);
//				glPopMatrix();
//
//				glPushMatrix();
//				glTranslatef(Cx, Cy, Cz);
//				glutWireSphere(0.25, 10, 10);
//				glPopMatrix();

				/* Draw the plane of the selected triangle and change the color and opacity of it */
				setColor(COLOR_SCAN_GRID_SELECTED);
				glBegin(GL_TRIANGLES);
				/* put the vertexes slightly closer to the planet so it does not cover up any other vertexes when selected */
				glVertex3f(0.99*Ax, 0.99*Ay, 0.99*Az);
				glVertex3f(0.99*Bx, 0.99*By, 0.99*Bz);
				glVertex3f(0.99*Cx, 0.99*Cy, 0.99*Cz);
				glEnd();

			}
//			/* Draw a sphere at where the camera is pointing */
//			glPushMatrix();
//			glTranslatef(camX, camY, camZ);
//			glutWireSphere(0.25, 10, 10);
//			glPopMatrix();
		}

		glColor3f(0.5, 0.0, 0.0);
		//glutWireSphere(currentSystem->planet[i]->radius*1.25,currentSystem->planet[i]->radius*3,currentSystem->planet[i]->radius*3);
		glPopMatrix();
	}
}

void
drawSystemBackground()
{
	/*
	 * Draw the background of
	 */
	char output[NAME_LENGTH];
	int close;
	int i;
	close = 0;

	for(i = 0; i < SYSTEM_COUNT; i++){
		/* Don't draw the current System as a point */
		if(currentSystemIndex != i){
			glColor3f(1, 1, 1);
			glBegin(GL_POINTS);
			glVertex3d(BGStars[i]->x - currentSystem->x, BGStars[i]->y - currentSystem->y, BGStars[i]->z - currentSystem->z);
			glEnd();
			if(systemDistanceArray[i] < player.jumpDistance){
				close++;
				if(selectedSystem > 0){
					if(selectedSystem == close){
						/* Draw the name of the system and distance*/
						glColor4f(1, 1, 1, 1.0);
						glRasterPos3f(BGStars[i]->x - currentSystem->x, BGStars[i]->y - currentSystem->y, BGStars[i]->z - currentSystem->z);
						sprintf(output, "%s (%f)",systemArray[i]->name ,systemDistanceArray[i]);
						drawString(output);

						/* Draw a line from the origin to the system far away */
						glColor4f(1, 1, 1, 0.5);
						glBegin(GL_LINE_STRIP);
						glVertex3d(0,0,0);
						glVertex3d(BGStars[i]->x - currentSystem->x, BGStars[i]->y - currentSystem->y, BGStars[i]->z - currentSystem->z);
						glEnd();
					}
				}else if(camera->focus == 1){
					/* Draw the name of the system and distance*/
					glColor4f(1, 1, 1, 1.0);
					glRasterPos3f(BGStars[i]->x - currentSystem->x, BGStars[i]->y - currentSystem->y, BGStars[i]->z - currentSystem->z);
					sprintf(output, "%s (%f)",systemArray[i]->name ,systemDistanceArray[i]);
					drawString(output);

					/* Draw a line from the origin to the system far away */
					glColor4f(1, 1, 1, 0.1);
					glBegin(GL_LINE_STRIP);
					glVertex3d(0,0,0);
					glVertex3d(BGStars[i]->x - currentSystem->x, BGStars[i]->y - currentSystem->y, BGStars[i]->z - currentSystem->z);
					glEnd();
				}
			}
		}
	}
}

void
drawOrbitLines()
{
	/*
	 * Draw the orbit lines of the current system's planets
	 */
	int i, ii;

	for(i = 0; i < currentSystem->planetCount; i++){
		/* Rotate the matrix to form the proper shape of the planet's orbit */
		glPushMatrix();
		glRotated(currentSystem->planet[i]->orbitOffset, 0.0, 1.0, 0.0);
		glRotated(currentSystem->planet[i]->orbitTilt, 1.0, 0.0, 0.0);
		if(camera->focus == 0){
			glBegin(GL_LINE_STRIP);
			if(selectedAstronomicalObject == i + 2){
				glColor4f(1, 1, 1, 0.75);
			}else if(selectedAstronomicalObject == 0){
				glColor4f(1, 1, 1, 0.15);
			}else {
				glColor4f(1, 1, 1, 0.05);
			}
			for(ii = 0; ii < currentSystem->planet[i]->orbitRadius; ii++){
				glVertex3d(currentSystem->planet[i]->orbitRadius*cos(2*M_PI*ii/currentSystem->planet[i]->orbitRadius), 0, currentSystem->planet[i]->orbitRadius*sin(2*M_PI*ii/currentSystem->planet[i]->orbitRadius));
			}
			glVertex3d(currentSystem->planet[i]->orbitRadius*cos(2*M_PI*0/currentSystem->planet[i]->orbitRadius), 0, currentSystem->planet[i]->orbitRadius*sin(2*M_PI*0/currentSystem->planet[i]->orbitRadius));
			glEnd();
		}else if(camera->focus == 1){
			if(selectedAstronomicalObject == i + 2){
				glColor4f(1, 1, 1, 0.75);
				glBegin(GL_LINE_STRIP);
				for(ii = 0; ii < currentSystem->planet[i]->orbitRadius; ii++){
					glVertex3d(currentSystem->planet[i]->orbitRadius*cos(2*M_PI*ii/currentSystem->planet[i]->orbitRadius), 0, currentSystem->planet[i]->orbitRadius*sin(2*M_PI*ii/currentSystem->planet[i]->orbitRadius));
				}
				glVertex3d(currentSystem->planet[i]->orbitRadius*cos(2*M_PI*0/currentSystem->planet[i]->orbitRadius), 0, currentSystem->planet[i]->orbitRadius*sin(2*M_PI*0/currentSystem->planet[i]->orbitRadius));
				glEnd();
			}
		}
		glPopMatrix();
	}
}

void
drawRings()
{
	/*
	 * Draw the asteroid belts that form the ring of certain planets
	 */
	int i;
	double sections;

	for(i = 0; i < currentSystem->planetCount; i++){
		if(currentSystem->planet[i]->type <= PLANET_TYPE_RING_LIMIT && currentSystem->planet[i]->type > PLANET_TYPE_COLONIZED_LIMIT){
			/* Position the matrix to correctly place the rings */
			glPushMatrix();
			glRotated(currentSystem->planet[i]->orbitOffset, 0.0, 1.0, 0.0);
			glRotated(currentSystem->planet[i]->orbitTilt, 1.0, 0.0, 0.0);
			glTranslated(cos(2*M_PI*currentSystem->planet[i]->yearOffset)*currentSystem->planet[i]->orbitRadius, 0,
					sin(2*M_PI*currentSystem->planet[i]->yearOffset)*currentSystem->planet[i]->orbitRadius);
			glRotated(90.0 + currentSystem->planet[i]->orbitTilt, 1.0, 0.0, 0.0);
			glRotated(currentSystem->planet[i]->axialTilt, 0.0, 1.0, 0.0);
			glRotated(360*currentSystem->planet[i]->dayOffset, 0.0, 0.0, 1.0);
			/* Draw some rings around the planet by drawing flat polygons from the outer to inner edges. These polygons are drawn
			 * twice since backside culling is enabled, so we need two rings to be overalapped on t op of eachother to be seen */
			setColor(COLOR_HUD);

			/* Set the amount of sections to form the rings and draw them. */
			sections = 100;
			drawTexturedRings(currentSystem->planet[i]->radius, sections, currentSystem->planet[i]->type);
			glPopMatrix();
		}
	}
}

void
draw3DShip()
{
	/*
	 * Draw the current ship in 3D space facing in the positive X direction
	 */

	glBegin(GL_LINES);
	glVertex3d(0, 0, 0);
	glVertex3d(0, launchedShipPath/1, 0);

	glVertex3d(0, launchedShipPath/1, 0);
	glVertex3d(0, launchedShipPath/2.0, launchedShipPath/2.0);

	glVertex3d(0, launchedShipPath/1, 0);
	glVertex3d(0, launchedShipPath/2.0, launchedShipPath/-2.0);
	glEnd();
}

void
drawShipPath()
{
	/*
	 * Draw a ship following a path made of 4 points forming a bezier curve.
	 */
	double qx, qy, qz;
	double b1, b2, b3, b4;
	double p1x, p1y, p1z, p2x, p2y, p2z, p3x, p3y, p3z, p4x, p4y, p4z;
	double dqx, dqy, dqz, db1, db2, db3, db4, n;
	int i = selectedAstronomicalObject - 2;
	double ii = 1 - launchedShipPath;

	/* Set the bezier curve point's values */
	//b1 is near the camera
	p1x = ((2 + currentSystem->planet[i]->radius/4)*7.5)*cos(camera->yAngle - M_PI/2)-1;
	p1y = 7;
	p1z = ((2 + currentSystem->planet[i]->radius/4)*7.5)*sin(camera->yAngle - M_PI/2)-1;
	//b2 is one step from the camera
	p2x = ((2 + currentSystem->planet[i]->radius/4)*7.5)*cos(camera->yAngle - M_PI/2)*0.90;
	p2y = -10;
	p2z = ((2 + currentSystem->planet[i]->radius/4)*7.5)*sin(camera->yAngle - M_PI/2)*0.90;
	//b3 is one step from the ring
	p3x = ((2 + currentSystem->planet[i]->radius/4)*7.5)*cos(camera->yAngle - M_PI/2)*0.65;
	p3y = -7;
	p3z = ((2 + currentSystem->planet[i]->radius/4)*7.5)*sin(camera->yAngle - M_PI/2)*0.65;
	//b4 is on the rings
	p4x = currentSystem->planet[i]->radius*1.8;
	p4y = 0;
	p4z = 0;

	/* Find the ship's position on the curve */
	glPushMatrix();

	b1 = pow((1-ii), 3);
	b2 = 3*ii*pow((1-ii), 2);
	b3 = 3*ii*ii*(1-ii);
	b4 = pow(ii, 3);
	qx = b1*p1x + b2*p2x + b3*p3x + b4*p4x;
	qy = b1*p1y + b2*p2y + b3*p3y + b4*p4y;
	qz = b1*p1z + b2*p2z + b3*p3z + b4*p4z;
	glTranslated(qx, qy, qz);

	//rotate so the ship faces the right direction using the curves velocity
	db1 = -3*pow((ii - 1), 2);
	db2 = 3*(ii-1)*(3*ii-1);
	db3 = 6*ii - 9*pow(ii, 2);
	db4 = 3*pow(ii, 2);
	dqx = db1*p1x + db2*p2x + db3*p3x + db4*p4x;
	dqy = db1*p1y + db2*p2y + db3*p3y + db4*p4y;
	dqz = db1*p1z + db2*p2z + db3*p3z + db4*p4z;

	//normalize the vectors
	n = sqrt(pow(dqx, 2) + pow(dqy, 2) + pow(dqz, 2));

	glRotated(180*acos(dqy/n)/M_PI, 0, 0, 1);
	glRotated(180*asin(dqx/n)/M_PI, 0, 1, 0);
	glRotated(180*asin(dqz/n)/M_PI, 1, 0, 0);

	/* Draw the ship in the appropriate position along the path */
	draw3DShip();

	glPopMatrix();
}

void
draw3DSatellite()
{
	/*
	 * Draw a satellite in 3D space facing in the positive X direction
	 */

	glBegin(GL_LINES);
	glVertex3d(0, -1, 0);
	glVertex3d(0, 1, 0);

	glVertex3d(0, 1, 0);
	glVertex3d(0, 1/2.0, 1/2.0);

	glVertex3d(0, 1, 0);
	glVertex3d(0, 1/2.0, 1/-2.0);
	glEnd();
}

void
drawSatellites(int i)
{
	/*
	 * Draw satellites that are entering the selected planet's orbit (planet given by i). Sattelites include sattelites and probes currently within
	 * the value of {0, 1}. The path these satellites follow use a bezier curve from the camera to the section of the selection grid they represent.
	 *
	 * It searches the entire planet's surface value for a satellite in motion (value between 0 and 1). The satellites path is found using a bezier
	 * curve where it's path is completly relative to it's selection grid. The end point is the average of the three points comprising the plane
	 */
	double ii, iii, iiii;
	double Ax ,Ay, Az, Bx, By, Bz, Cx, Cy, Cz;
	int r, c;
	double qx, qy, qz;
	double b1, b2, b3, b4;
	double p1x, p1y, p1z, p2x, p2y, p2z, p3x, p3y, p3z, p4x, p4y, p4z;
	double dqx, dqy, dqz, db1, db2, db3, db4, n, ny, nz;
	int odd = 0;
	r = 0;

	for(ii = 0.15; ceil(ii*100000)/100000 < 0.35; gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1)){
		c = 0;
		for(iii = 0; ceil(iii*100000)/100000 < 1; gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1)){
			/* Find all the surface values which currently have a satellite within the value {0, 1} */
			/* Check this column and row's rightside up triangle for a satellite and a probe */
			if(currentSystem->planet[i]->surface[r][c]->satellite > 0 || currentSystem->planet[i]->surface[r][c]->probe > 0){
				/* Found a satellite in this selected grid and set the points that form it's plane */
				if(odd){
					/* If it's an odd row (1st from the bottom, 3rd from the bottom, etc) add an offset to the triangles */
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5);
				}
				gridIncrement(0, &ii, currentSystem->planet[i]->radius, 0.025000);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.025000);
				Ax = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
				Ay = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
				Az = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.025000);
				gridIncrement(0, &ii, currentSystem->planet[i]->radius, -0.025000);

				gridIncrement(0, &ii, currentSystem->planet[i]->radius, 0.025000);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1 -0.025000);
				Bx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
				By = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
				Bz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, -1 +0.025000);
				gridIncrement(0, &ii, currentSystem->planet[i]->radius, -0.025000);

				gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1 -0.025000);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5);
				Cx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
				Cy = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
				Cz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5);
				gridIncrement(0, &ii, currentSystem->planet[i]->radius, -1 +0.025000);
				if(odd){
					/* Revert the offset if there was one applied */
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5);
				}

				/* normalize the 3 points forming the grid's plane */
				n = sqrt(pow((Ax + Bx + Cx), 2) + pow((Ay + By + Cy), 2) + pow((Az + Bz + Cz), 2));

				/* Draw the satellite on it's way to the planet's orbit if it's on it's way */
				if(currentSystem->planet[i]->surface[r][c]->satellite > 0 && currentSystem->planet[i]->surface[r][c]->satellite < 1){
					/* Set the bezier curve point's values */
					//b1 is near the camera
					p1x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n);
					p1y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n);
					p1z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n);
					//b2 is one step from the camera. Use an arbitrary equation to add curves to the curve
					p2x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((-Az + -Bz + -Cz)/n)*0.2 + ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.6;
					p2y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n)*0.8;
					p2z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.2 + ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.6;
					//b2 is one step from the grid's plane. Use an arbitrary equation to add curves to the curve
					p3x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.1 + ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.4;
					p3y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n)*0.5;
					p3z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((-Ax + -Bx + -Cx)/n)*0.1 + ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.4;
					//b4 is at the center of the grid's plane.
					p4x = (Ax + Bx + Cx)/3.0;
					p4y = (Ay + By + Cy)/3.0;
					p4z = (Az + Bz + Cz)/3.0;

					/* Find the satellite's position on the curve */
					iiii = currentSystem->planet[i]->surface[r][c]->satellite;

					b1 = pow((1-iiii), 3);
					b2 = 3*iiii*pow((1-iiii), 2);
					b3 = 3*iiii*iiii*(1-iiii);
					b4 = pow(iiii, 3);
					qx = b1*p1x + b2*p2x + b3*p3x + b4*p4x;
					qy = b1*p1y + b2*p2y + b3*p3y + b4*p4y;
					qz = b1*p1z + b2*p2z + b3*p3z + b4*p4z;

					/* Find the velocity of a point on the curve to rotate the satellite to face it's path */
					db1 = -3*pow((iiii - 1), 2);
					db2 = 3*(iiii-1)*(3*iiii-1);
					db3 = 6*iiii - 9*pow(iiii, 2);
					db4 = 3*pow(iiii, 2);
					dqx = db1*p1x + db2*p2x + db3*p3x + db4*p4x;
					dqy = db1*p1y + db2*p2y + db3*p3y + db4*p4y;
					dqz = db1*p1z + db2*p2z + db3*p3z + db4*p4z;

					/* normalize the vectors and commit the rotations */
					ny = sqrt(pow(dqx, 2) + pow(dqy, 2) + pow(dqz, 2));
					nz = sqrt(pow(dqx, 2) + pow(dqz, 2));
					glPushMatrix();
					glTranslated(qx, qy, qz);
					glRotated(90, 0, 0, 1);
					if((dqx/nz) < 0){
						glRotated(90*asin(dqz/nz)/(M_PI/2), 1, 0, 0);
					}else{
						glRotated(180 + 90*asin(-dqz/nz)/(M_PI/2), 1, 0, 0);
					}
					glRotated(-90*asin(dqy/ny)/(M_PI/2), 0, 0, 1);

					/* Draw the satellite in the appropriate position along the path */
					draw3DSatellite();
					glPopMatrix();
				}

				/* Draw the probe on it's way to the planet's orbit if it's on it's way */
				if(currentSystem->planet[i]->surface[r][c]->probe > 0 && currentSystem->planet[i]->surface[r][c]->probe < 1){
					/* Set the bezier curve point's values */
					//b1 is near the camera
					p1x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n);
					p1y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n);
					p1z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n);
					//b2 is one step from the camera. Use an arbitrary equation to add curves to the curve
					p2x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.8;
					p2y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n)*0.8;
					p2z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.8;
					//b2 is one step from the grid's plane. Use an arbitrary equation to add curves to the curve
					p3x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.5;
					p3y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((2*Ay + 2*By + 2*Cy)/n)*0.5;
					p3z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.5;
					//b4 is at the center of the grid's plane.
					p4x = currentSystem->planet[i]->radius*(Ax + Bx + Cx)/n;
					p4y = currentSystem->planet[i]->radius*(Ay + By + Cy)/n;
					p4z = currentSystem->planet[i]->radius*(Az + Bz + Cz)/n;

					/* Find the satellite's position on the curve */
					iiii = currentSystem->planet[i]->surface[r][c]->probe;

					b1 = pow((1-iiii), 3);
					b2 = 3*iiii*pow((1-iiii), 2);
					b3 = 3*iiii*iiii*(1-iiii);
					b4 = pow(iiii, 3);
					qx = b1*p1x + b2*p2x + b3*p3x + b4*p4x;
					qy = b1*p1y + b2*p2y + b3*p3y + b4*p4y;
					qz = b1*p1z + b2*p2z + b3*p3z + b4*p4z;

					/* Find the velocity of a point on the curve to rotate the satellite to face it's path */
					db1 = -3*pow((iiii - 1), 2);
					db2 = 3*(iiii-1)*(3*iiii-1);
					db3 = 6*iiii - 9*pow(iiii, 2);
					db4 = 3*pow(iiii, 2);
					dqx = db1*p1x + db2*p2x + db3*p3x + db4*p4x;
					dqy = db1*p1y + db2*p2y + db3*p3y + db4*p4y;
					dqz = db1*p1z + db2*p2z + db3*p3z + db4*p4z;

					/* normalize the vectors and commit the rotations */
					ny = sqrt(pow(dqx, 2) + pow(dqy, 2) + pow(dqz, 2));
					nz = sqrt(pow(dqx, 2) + pow(dqz, 2));
					glPushMatrix();
					glTranslated(qx, qy, qz);
					glRotated(90, 0, 0, 1);
					if((dqx/nz) < 0){
						glRotated(90*asin(dqz/nz)/(M_PI/2), 1, 0, 0);
					}else{
						glRotated(180 + 90*asin(-dqz/nz)/(M_PI/2), 1, 0, 0);
					}
					glRotated(-90*asin(dqy/ny)/(M_PI/2), 0, 0, 1);

					/* Draw the satellite in the appropriate position along the path */
					draw3DSatellite();
					glPopMatrix();
				}


			}
			c++;

			/* Check this column and row's upside down triangle for satellites */
			if(currentSystem->planet[i]->surface[r][c]->satellite > 0 || currentSystem->planet[i]->surface[r][c]->probe > 0){
				/* Found a satellite in this selected grid and set the points that form it's plane */
				if(odd){
					/* If it's an odd row (1st from the bottom, 3rd from the bottom, etc) add an offset to the triangles */
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5);
				}
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1.5 -0.025000);
				gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1 -0.025000);
				Ax = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
				Ay = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
				Az = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
				gridIncrement(0, &ii, currentSystem->planet[i]->radius, -1 +0.025000);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, -1.5 +0.025000);

				gridIncrement(0, &ii, currentSystem->planet[i]->radius, 1 -0.025000);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, 0.5 +0.025000);
				Bx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
				By = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
				Bz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5 -0.025000);
				gridIncrement(0, &ii, currentSystem->planet[i]->radius, -1 +0.025000);

				gridIncrement(0, &ii, currentSystem->planet[i]->radius, 0.025000);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, 1);
				Cx = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*cos(iii*2*M_PI);
				Cy = currentSystem->planet[i]->radius*1.2*cos(ii*2*M_PI);
				Cz = currentSystem->planet[i]->radius*1.2*sin(ii*2*M_PI)*sin(iii*2*M_PI);
				gridIncrement(1, &iii, currentSystem->planet[i]->radius, -1);
				gridIncrement(0, &ii, currentSystem->planet[i]->radius, -0.025000);
				if(odd){
					/* Revert the offset if there was one applied */
					gridIncrement(1, &iii, currentSystem->planet[i]->radius, -0.5);
				}

				/* normalize the 3 points forming the grid's plane */
				n = sqrt(pow((Ax + Bx + Cx), 2) + pow((Ay + By + Cy), 2) + pow((Az + Bz + Cz), 2));

				/* Draw the satellite on it's way to the planet's orbit if it's on it's way */
				if(currentSystem->planet[i]->surface[r][c]->satellite > 0 && currentSystem->planet[i]->surface[r][c]->satellite < 1){
					/* Set the bezier curve point's values */
					//b1 is near the camera
					p1x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n);
					p1y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n);
					p1z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n);
					//b2 is one step from the camera. Use an arbitrary equation to add curves to the curve
					p2x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((-Az + -Bz + -Cz)/n)*0.2 + ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.6;
					p2y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n)*0.8;
					p2z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.2 + ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.6;
					//b2 is one step from the grid's plane. Use an arbitrary equation to add curves to the curve
					p3x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.1 + ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.4;
					p3y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n)*0.5;
					p3z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((-Ax + -Bx + -Cx)/n)*0.1 + ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.4;
					//b4 is at the center of the grid's plane.
					p4x = (Ax + Bx + Cx)/3.0;
					p4y = (Ay + By + Cy)/3.0;
					p4z = (Az + Bz + Cz)/3.0;

					/* Find the satellite's position on the curve */
					iiii = currentSystem->planet[i]->surface[r][c]->satellite;

					b1 = pow((1-iiii), 3);
					b2 = 3*iiii*pow((1-iiii), 2);
					b3 = 3*iiii*iiii*(1-iiii);
					b4 = pow(iiii, 3);
					qx = b1*p1x + b2*p2x + b3*p3x + b4*p4x;
					qy = b1*p1y + b2*p2y + b3*p3y + b4*p4y;
					qz = b1*p1z + b2*p2z + b3*p3z + b4*p4z;

					/* Find the velocity of a point on the curve to rotate the satellite to face it's path */
					db1 = -3*pow((iiii - 1), 2);
					db2 = 3*(iiii-1)*(3*iiii-1);
					db3 = 6*iiii - 9*pow(iiii, 2);
					db4 = 3*pow(iiii, 2);
					dqx = db1*p1x + db2*p2x + db3*p3x + db4*p4x;
					dqy = db1*p1y + db2*p2y + db3*p3y + db4*p4y;
					dqz = db1*p1z + db2*p2z + db3*p3z + db4*p4z;

					/* normalize the vectors and commit the rotations */
					ny = sqrt(pow(dqx, 2) + pow(dqy, 2) + pow(dqz, 2));
					nz = sqrt(pow(dqx, 2) + pow(dqz, 2));
					glPushMatrix();
					glTranslated(qx, qy, qz);
					glRotated(90, 0, 0, 1);
					if((dqx/nz) < 0){
						glRotated(90*asin(dqz/nz)/(M_PI/2), 1, 0, 0);
					}else{
						glRotated(180 + 90*asin(-dqz/nz)/(M_PI/2), 1, 0, 0);
					}
					glRotated(-90*asin(dqy/ny)/(M_PI/2), 0, 0, 1);

					/* Draw the satellite in the appropriate position along the path */
					draw3DSatellite();
					glPopMatrix();
				}

				/* Draw the probe on it's way to the planet's orbit if it's on it's way */
				if(currentSystem->planet[i]->surface[r][c]->probe > 0 && currentSystem->planet[i]->surface[r][c]->probe < 1){
					/* Set the bezier curve point's values */
					//b1 is near the camera
					p1x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n);
					p1y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n);
					p1z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n);
					//b2 is one step from the camera. Use an arbitrary equation to add curves to the curve
					p2x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.8;
					p2y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ay + By + Cy)/n)*0.8;
					p2z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.8;
					//b2 is one step from the grid's plane. Use an arbitrary equation to add curves to the curve
					p3x = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Ax + Bx + Cx)/n)*0.5;
					p3y = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((2*Ay + 2*By + 2*Cy)/n)*0.5;
					p3z = ((2 + currentSystem->planet[i]->radius/4.0)*7.5)*((Az + Bz + Cz)/n)*0.5;
					//b4 is at the center of the grid's plane.
					p4x = (Ax + Bx + Cx)/3.0;
					p4y = (Ay + By + Cy)/3.0;
					p4z = (Az + Bz + Cz)/3.0;

					/* Find the satellite's position on the curve */
					iiii = currentSystem->planet[i]->surface[r][c]->probe;

					b1 = pow((1-iiii), 3);
					b2 = 3*iiii*pow((1-iiii), 2);
					b3 = 3*iiii*iiii*(1-iiii);
					b4 = pow(iiii, 3);
					qx = b1*p1x + b2*p2x + b3*p3x + b4*p4x;
					qy = b1*p1y + b2*p2y + b3*p3y + b4*p4y;
					qz = b1*p1z + b2*p2z + b3*p3z + b4*p4z;

					/* Find the velocity of a point on the curve to rotate the probe to face it's path */
					db1 = -3*pow((iiii - 1), 2);
					db2 = 3*(iiii-1)*(3*iiii-1);
					db3 = 6*iiii - 9*pow(iiii, 2);
					db4 = 3*pow(iiii, 2);
					dqx = db1*p1x + db2*p2x + db3*p3x + db4*p4x;
					dqy = db1*p1y + db2*p2y + db3*p3y + db4*p4y;
					dqz = db1*p1z + db2*p2z + db3*p3z + db4*p4z;

					/* normalize the vectors and commit the rotations */
					ny = sqrt(pow(dqx, 2) + pow(dqy, 2) + pow(dqz, 2));
					nz = sqrt(pow(dqx, 2) + pow(dqz, 2));
					glPushMatrix();
					glTranslated(qx, qy, qz);
					glRotated(90, 0, 0, 1);
					if((dqx/nz) < 0){
						glRotated(90*asin(dqz/nz)/(M_PI/2), 1, 0, 0);
					}else{
						glRotated(180 + 90*asin(-dqz/nz)/(M_PI/2), 1, 0, 0);
					}
					glRotated(-90*asin(dqy/ny)/(M_PI/2), 0, 0, 1);

					/* Draw the probe in the appropriate position along the path */
					draw3DSatellite();
					glPopMatrix();
				}
			}
			c++;
		}
		r++;
		/* switch between odd and even after each row change */
		if(odd){
			odd = 0;
		}else{
			odd = 1;
		}
	}

//	rows = 1;
//	for(iii = 0.15; iii < 0.35; gridIncrement(0, &iii, newPlanetArray[ii]->radius, 1)){
//		rows++;
//	}
//	/* Find out how many columns can fit on this planet's surface. Remember that each column has two section triangles */
//	columns = 2;
//	for(iiii = 0; iiii < 1; gridIncrement(1, &iiii, newPlanetArray[ii]->radius, 1)){
//		columns++;
//		columns++;
//	}
//
//	/* Initialize the first dimension of the surface array */
//	newPlanetArray[ii]->surface = malloc(sizeof(Surface**)*rows);
//
//	for(iii = 0; iii < rows; iii++){
//		/* Initialize the second dimension of the surface array */
//		newPlanetArray[ii]->surface[(int) iii] = malloc(sizeof(Surface*)*columns);
//		for(iiii = 0; iiii < columns; iiii++){
//			/* Assign values to the planet's surfaces */
//			newPlanetArray[ii]->surface[(int)iii][(int)iiii] = malloc(sizeof(Surface));
//			newPlanetArray[ii]->surface[(int)iii][(int)iiii]->satellite = 0;
//			newPlanetArray[ii]->surface[(int)iii][(int)iiii]->scanRate = 0;
//		}
//	}
}

void
drawSurfaceContents(int i, int r, int c, double leftX, double leftY, double leftZ, double middleX, double middleY, double middleZ, double rightX, double rightY, double rightZ)
{
	/*
	 * The given grid has a satellite or a probe currently in it's proper position. When a satellite is present without
	 * a probe, show the contents of that surface section as red icons. When a probe is present without a satellite, show
	 * a blue question mark (Maybe have the probe slowly scan the planet to prevent the need of a satellite?). If both
	 * are present, show the repsective icons with color (energy yellow, minerals blue), as of now.
	 *
	 * The given values are the planet's position in the current system's planet array (i) and the row and column of the
	 * surface in the ith planet's surface array. The 9 doubles represent the 3 points forming the grid`s plane (left, middle and right)
	 */
	double TL[3], TR[3], BL[3], BR[3];

	if(currentSystem->planet[i]->surface[r][c]->satellite >= 1){
		/* Draw an energy icon if the surface's energy is less than or equal to the user's energy limit */
		if(currentSystem->planet[i]->surface[r][c]->energy <= energyLimit){
			/* Set the color for the energy icon */
			if(currentSystem->planet[i]->surface[r][c]->probe >= 1){
				/* Draw the energy icon with its correct color */
				setColor(COLOR_ENERGY);
			}else{
				/* Draw the energy icon as red */
				setColor(COLOR_SCAN_GRID);
			}

			/* Set the point values for the edges of the icon's area */
			if(c % 2){
				BR[0] = (leftX*1.1 + middleX*0.9)/2.0;
				BR[1] = (leftY*1.1 + middleY*0.9)/2.0;
				BR[2] = (leftZ*1.1 + middleZ*0.9)/2.0;

				BL[0] = (((rightX*1.1 + middleX*0.9)/2.0) + BR[0])/2.0;
				BL[1] = (((rightY*1.1 + middleY*0.9)/2.0) + BR[1])/2.0;
				BL[2] = (((rightZ*1.1 + middleZ*0.9)/2.0) + BR[2])/2.0;

				TR[0] = (rightX*0.4 + leftX*1.6)/2.0;
				TR[1] = (rightY*0.4 + leftY*1.6)/2.0;
				TR[2] = (rightZ*0.4 + leftZ*1.6)/2.0;

				TL[0] = (rightX + leftX)/2.0;
				TL[1] = (rightY + leftY)/2.0;
				TL[2] = (rightZ + leftZ)/2.0;
			}else{
				BL[0] = (leftX*1.1 + middleX*0.9)/2.0;
				BL[1] = (leftY*1.1 + middleY*0.9)/2.0;
				BL[2] = (leftZ*1.1 + middleZ*0.9)/2.0;

				BR[0] = (((rightX*1.1 + middleX*0.9)/2.0) + BL[0])/2.0;
				BR[1] = (((rightY*1.1 + middleY*0.9)/2.0) + BL[1])/2.0;
				BR[2] = (((rightZ*1.1 + middleZ*0.9)/2.0) + BL[2])/2.0;

				TL[0] = (rightX*0.4 + leftX*1.6)/2.0;
				TL[1] = (rightY*0.4 + leftY*1.6)/2.0;
				TL[2] = (rightZ*0.4 + leftZ*1.6)/2.0;

				TR[0] = (rightX + leftX)/2.0;
				TR[1] = (rightY + leftY)/2.0;
				TR[2] = (rightZ + leftZ)/2.0;
			}

			/* Draw the two triangles that form the energy sign */
			glBegin(GL_TRIANGLES);
			/* Top triangle */
			//Top point
			glVertex3d(
					(1.5*TR[0] + 0.5*TL[0])/2.0,
					(1.5*TR[1] + 0.5*TL[1])/2.0,
					(1.5*TR[2] + 0.5*TL[2])/2.0
					);
			//Left point
			glVertex3d(
					(1.5*((1.5*BL[0] + 0.5*TL[0])/2.0) + 0.5*((1.5*TR[0] + 0.5*TL[0])/2.0))/2.0,
					(1.5*((1.5*BL[1] + 0.5*TL[1])/2.0) + 0.5*((1.5*TR[1] + 0.5*TL[1])/2.0))/2.0,
					(1.5*((1.5*BL[2] + 0.5*TL[2])/2.0) + 0.5*((1.5*TR[2] + 0.5*TL[2])/2.0))/2.0
					);
			//Center point
			glVertex3d(
					(1.25*(BL[0] + BR[0]) + 0.75*(TL[0] + TR[0]))/4.0,
					(1.25*(BL[1] + BR[1]) + 0.75*(TL[1] + TR[1]))/4.0,
					(1.25*(BL[2] + BR[2]) + 0.75*(TL[2] + TR[2]))/4.0
					);
			//Left point
			glVertex3d(
					(1.5*((1.5*BL[0] + 0.5*TL[0])/2.0) + 0.5*((1.5*TR[0] + 0.5*TL[0])/2.0))/2.0,
					(1.5*((1.5*BL[1] + 0.5*TL[1])/2.0) + 0.5*((1.5*TR[1] + 0.5*TL[1])/2.0))/2.0,
					(1.5*((1.5*BL[2] + 0.5*TL[2])/2.0) + 0.5*((1.5*TR[2] + 0.5*TL[2])/2.0))/2.0
					);
			//Top point
			glVertex3d(
					(1.5*TR[0] + 0.5*TL[0])/2.0,
					(1.5*TR[1] + 0.5*TL[1])/2.0,
					(1.5*TR[2] + 0.5*TL[2])/2.0
					);
			//Center point
			glVertex3d(
					(1.25*(BL[0] + BR[0]) + 0.75*(TL[0] + TR[0]))/4.0,
					(1.25*(BL[1] + BR[1]) + 0.75*(TL[1] + TR[1]))/4.0,
					(1.25*(BL[2] + BR[2]) + 0.75*(TL[2] + TR[2]))/4.0
					);

			/* Bottom Triangle */
			//Bottom point
			glVertex3d(
					(0.5*BR[0] + 1.5*BL[0])/2.0,
					(0.5*BR[1] + 1.5*BL[1])/2.0,
					(0.5*BR[2] + 1.5*BL[2])/2.0
					);
			//Right point
			glVertex3d(
					(1.5*((1.5*TR[0] + 0.5*BR[0])/2.0) + 0.5*((0.5*BR[0] + 1.5*BL[0])/2.0))/2.0,
					(1.5*((1.5*TR[1] + 0.5*BR[1])/2.0) + 0.5*((0.5*BR[1] + 1.5*BL[1])/2.0))/2.0,
					(1.5*((1.5*TR[2] + 0.5*BR[2])/2.0) + 0.5*((0.5*BR[2] + 1.5*BL[2])/2.0))/2.0
					);
			//Center point
			glVertex3d(
					(0.75*(BL[0] + BR[0]) + 1.25*(TL[0] + TR[0]))/4.0,
					(0.75*(BL[1] + BR[1]) + 1.25*(TL[1] + TR[1]))/4.0,
					(0.75*(BL[2] + BR[2]) + 1.25*(TL[2] + TR[2]))/4.0
					);
			//Right point
			glVertex3d(
					(1.5*((1.5*TR[0] + 0.5*BR[0])/2.0) + 0.5*((0.5*BR[0] + 1.5*BL[0])/2.0))/2.0,
					(1.5*((1.5*TR[1] + 0.5*BR[1])/2.0) + 0.5*((0.5*BR[1] + 1.5*BL[1])/2.0))/2.0,
					(1.5*((1.5*TR[2] + 0.5*BR[2])/2.0) + 0.5*((0.5*BR[2] + 1.5*BL[2])/2.0))/2.0
					);
			//Bottom point
			glVertex3d(
					(0.5*BR[0] + 1.5*BL[0])/2.0,
					(0.5*BR[1] + 1.5*BL[1])/2.0,
					(0.5*BR[2] + 1.5*BL[2])/2.0
					);
			//Center point
			glVertex3d(
					(0.75*(BL[0] + BR[0]) + 1.25*(TL[0] + TR[0]))/4.0,
					(0.75*(BL[1] + BR[1]) + 1.25*(TL[1] + TR[1]))/4.0,
					(0.75*(BL[2] + BR[2]) + 1.25*(TL[2] + TR[2]))/4.0
					);
			glEnd();
		}

		/* Draw a mineral icon if the surface's mineral output is less than or equal to the user's mineral limit */
		if(currentSystem->planet[i]->surface[r][c]->mineral <= mineralLimit){
			/* Set the color for the mineral icon */
			if(currentSystem->planet[i]->surface[r][c]->probe >= 1){
				/* Draw the energy icon with its correct color */
				setColor(COLOR_MINERAL);
			}else{
				/* Draw the energy icon as red */
				setColor(COLOR_SCAN_GRID);
			}

			/* Set the point values for the edges of the icon's area */
			if(c % 2){
				BL[0] = (rightX*1.1 + middleX*0.9)/2.0;
				BL[1] = (rightY*1.1 + middleY*0.9)/2.0;
				BL[2] = (rightZ*1.1 + middleZ*0.9)/2.0;

				BR[0] = (((leftX*1.1 + middleX*0.9)/2.0) + BL[0])/2.0;
				BR[1] = (((leftY*1.1 + middleY*0.9)/2.0) + BL[1])/2.0;
				BR[2] = (((leftZ*1.1 + middleZ*0.9)/2.0) + BL[2])/2.0;

				TL[0] = (leftX*0.4 + rightX*1.6)/2.0;
				TL[1] = (leftY*0.4 + rightY*1.6)/2.0;
				TL[2] = (leftZ*0.4 + rightZ*1.6)/2.0;

				TR[0] = (leftX + rightX)/2.0;
				TR[1] = (leftY + rightY)/2.0;
				TR[2] = (leftZ + rightZ)/2.0;
			}else{
				TL[0] = (rightX*1.1 + middleX*0.9)/2.0;
				TL[1] = (rightY*1.1 + middleY*0.9)/2.0;
				TL[2] = (rightZ*1.1 + middleZ*0.9)/2.0;

				TR[0] = (((leftX*1.1 + middleX*0.9)/2.0) + TL[0])/2.0;
				TR[1] = (((leftY*1.1 + middleY*0.9)/2.0) + TL[1])/2.0;
				TR[2] = (((leftZ*1.1 + middleZ*0.9)/2.0) + TL[2])/2.0;

				BL[0] = (leftX*0.4 + rightX*1.6)/2.0;
				BL[1] = (leftY*0.4 + rightY*1.6)/2.0;
				BL[2] = (leftZ*0.4 + rightZ*1.6)/2.0;

				BR[0] = (leftX + rightX)/2.0;
				BR[1] = (leftY + rightY)/2.0;
				BR[2] = (leftZ + rightZ)/2.0;
			}

			/* Draw the triangles that form the diamond */
			glBegin(GL_TRIANGLES);
			/* Left-side triangle */
			//Far left point
			glVertex3d((1.5*TL[0] + 0.5*BL[0])/2.0,
					(1.5*TL[1] + 0.5*BL[1])/2.0,
					(1.5*TL[2] + 0.5*BL[2])/2.0
					);
			//bottom point
			glVertex3d((BL[0] + BR[0])/2.0,
					(BL[1] + BR[1])/2.0,
					(BL[2] + BR[2])/2.0
					);
			//Top left point
			glVertex3d((1.5*TL[0] + 0.5*TR[0])/2.0,
					(1.5*TL[1] + 0.5*TR[1])/2.0,
					(1.5*TL[2] + 0.5*TR[2])/2.0
					);
			//bottom point
			glVertex3d((BL[0] + BR[0])/2.0,
					(BL[1] + BR[1])/2.0,
					(BL[2] + BR[2])/2.0
					);
			//Far left point
			glVertex3d((1.5*TL[0] + 0.5*BL[0])/2.0,
					(1.5*TL[1] + 0.5*BL[1])/2.0,
					(1.5*TL[2] + 0.5*BL[2])/2.0
					);
			//Top left point
			glVertex3d((1.5*TL[0] + 0.5*TR[0])/2.0,
					(1.5*TL[1] + 0.5*TR[1])/2.0,
					(1.5*TL[2] + 0.5*TR[2])/2.0
					);

			/* Right-side triangle */
			//bottom point
			glVertex3d((BL[0] + BR[0])/2.0,
					(BL[1] + BR[1])/2.0,
					(BL[2] + BR[2])/2.0
					);
			//Far right point
			glVertex3d((1.5*TR[0] + 0.5*BR[0])/2.0,
					(1.5*TR[1] + 0.5*BR[1])/2.0,
					(1.5*TR[2] + 0.5*BR[2])/2.0
					);
			//top right point
			glVertex3d((1.5*TR[0] + 0.5*TL[0])/2.0,
					(1.5*TR[1] + 0.5*TL[1])/2.0,
					(1.5*TR[2] + 0.5*TL[2])/2.0
					);
			//Far right point
			glVertex3d((1.5*TR[0] + 0.5*BR[0])/2.0,
					(1.5*TR[1] + 0.5*BR[1])/2.0,
					(1.5*TR[2] + 0.5*BR[2])/2.0
					);
			//bottom point
			glVertex3d((BL[0] + BR[0])/2.0,
					(BL[1] + BR[1])/2.0,
					(BL[2] + BR[2])/2.0
					);
			//top right point
			glVertex3d((1.5*TR[0] + 0.5*TL[0])/2.0,
					(1.5*TR[1] + 0.5*TL[1])/2.0,
					(1.5*TR[2] + 0.5*TL[2])/2.0
					);

			/* Top middle triangle */
			//top right point
			glVertex3d((1.5*TR[0] + 0.5*TL[0])/2.0,
					(1.5*TR[1] + 0.5*TL[1])/2.0,
					(1.5*TR[2] + 0.5*TL[2])/2.0
					);
			//top left point
			glVertex3d((1.5*TL[0] + 0.5*TR[0])/2.0,
					(1.5*TL[1] + 0.5*TR[1])/2.0,
					(1.5*TL[2] + 0.5*TR[2])/2.0
					);
			//bottom point
			glVertex3d((BL[0] + BR[0])/2.0,
					(BL[1] + BR[1])/2.0,
					(BL[2] + BR[2])/2.0
					);
			//top left point
			glVertex3d((1.5*TL[0] + 0.5*TR[0])/2.0,
					(1.5*TL[1] + 0.5*TR[1])/2.0,
					(1.5*TL[2] + 0.5*TR[2])/2.0
					);
			//top right point
			glVertex3d((1.5*TR[0] + 0.5*TL[0])/2.0,
					(1.5*TR[1] + 0.5*TL[1])/2.0,
					(1.5*TR[2] + 0.5*TL[2])/2.0
					);
			//bottom point
			glVertex3d((BL[0] + BR[0])/2.0,
					(BL[1] + BR[1])/2.0,
					(BL[2] + BR[2])/2.0
					);
			glEnd();
		}

	}else{
		if(currentSystem->planet[i]->surface[r][c]->probe >= 1){
			/*
			 * Draw a question mark
			 */
		}else{
			printf("!!!~TRYING TO DRAW SURFACE CONTENTS WHEN THERE ARE NO SATELLITES IN ORBIT~!!!\n");
		}
	}
}

void
drawPlanetStation()
{
	/*
	 * Draw a station that orbits around the colonized planet's
	 */
	int i;

	for(i = 0; i < currentSystem->planetCount; i++){
		if(currentSystem->planet[i]->type >= PLANET_TYPE_COLONIZED_LIMIT){
			//Draw a station
		}
	}
}

void
drawWindow()
{
	/*
	 * Draw a window that indicates the connection status to a station, which is the hidden way to switch the state from
	 * the 3D system view into the 2D communications window view. The window's state depends on the windowState values:
	 *
	 * [0] Draw nothing since the player did not select the "station" option.
	 * {0, 1] The animation of the window opening. Some window objects may also be drawn at this point, like the icons.
	 * {1, 3.5] Populate the window of most objects like the title. Slowly propogate the top trig function to connect both icons.
	 * {3.5, 4] Don't add anything new. Just keep updating the function, which will be "lagging" for the rest of this range.
	 * {4, 4.8] Change the descriptive flavor text to tell the user they have connected to the station.
	 * {4.8, 5] Add the static background at some point during this range, which runs at a very inconsistent speed.
	 * {5, 5.5] Let the window run normally and consistently before closing.
	 * {5.5, 6.5] Start closing the window and once we pass this range, send a request to change the state.
	 *
	 * Whenever the window is not 0, prevent any inputs from affecting the "background" of the window, which is the system view.
	 */

	double i, x, y, L, R, B, T, multi, function1, function2, function3, functionLength, titleHeight, waveLimit, waveSize;
	int j;
	char titleText[NAME_LENGTH], infoText[NAME_LENGTH];
	double textHeight, textWidth;

//+X is right
//+Y is up
//+Z is closer to camera

	/* Set a height and width value for areas that require text. These values will change depending on the screens resolution.
	 * This is the optimal height for a single line/width for a single character. It can be doubled to fit two lines/characters, etc. */
	textHeight = 0.002 + 0.025*((5*yMax)/h);
	textWidth = 0.017*(5*xMax)/w;

	/* Keep the X value at 0.01 while windowState is at [0, 0.5}. Have X expand from 0 to 1 when windowState is [0.5, 1}.
	 * When windowState nears it's end, close the window by reversing the function used when opening the window. */
	if(windowState < 0.5){
		x = 0.01;
	}else if(windowState < 1){
		x = (windowState - 0.5) * 2.0;
	}else if(windowState < 5.5){
		x = 1;
	}else if(windowState < 6){
		x = 1 - (windowState - 5.49) * 2.0;
	}else if(windowState < 6.5){
		x = 0.01;
	}else{
		x = 0;
	}

	/* Have the y value increase at a sinusoidal rate, changing from the first quarter ramp up (<0.5) to the hill of the second half (>=0.5).
	 * When windowState nears it's end, close the window by reversing the function used when opening the window. */
	if(windowState < 0.5){
		y = sin(windowState*M_PI);
	}else if(windowState < 1){
		y = 1 - (-(sin((windowState)*2*M_PI)));
	}else if(windowState < 5.5){
		y = 1;
	}else if(windowState < 6){

		y = 1 - ((sin((windowState - 5.5)*2*M_PI)));
	}else if(windowState < 6.5){
		y = 1 - sin((windowState - 6)*M_PI);
	}else{
		y = 0;
	}


	/* Draw a background to cover the screen and give it a static texture to block out the player's view to make it easier to change states.
	 * Whenever it needs to be drawn, do it before everything else so that it is placed as a background behind the window */
	if(windowState > 4.9){
		setColor(COLOR_HUD);
		drawTexturedStaticBackground((-xMax/2)/107.25, 0.5);
	}

	/* Draw a white box to be the outline of the communication window */
	glBegin(GL_QUADS);
	setColor(COLOR_HUD);
	//Top-left
	glVertex3f(-0.25*x, 0.10*y, -1);
	//Bottom-left
	glVertex3f(-0.25*x, -0.10*y - textHeight, -1);
	//Bottom-right
	glVertex3f(0.25*x, -0.10*y - textHeight, -1);
	//Top-right
	glVertex3f(0.25*x, 0.10*y, -1);
	glEnd();

	/* Draw a black box which will hold all the information for the communication window.
	 * This box will have slightly smaller portions than the white box and is drawn over it. */
	glBegin(GL_QUADS);
	setColor(COLOR_BLACK);
	//Top-left
	glVertex3f(-0.25*x + 0.002, 0.10*y - 0.002, -1);
	//Bottom-left
	glVertex3f(-0.25*x + 0.002, -0.10*y + 0.002 - textHeight, -1);
	//Bottom-right
	glVertex3f(0.25*x - 0.002, -0.10*y + 0.002 - textHeight, -1);
	//Top-right
	glVertex3f(0.25*x - 0.002, 0.10*y - 0.002, -1);
	glEnd();

	/* Draw the icons that represent the player and the station once the window is large enough */
	if(windowState > 0.875 && windowState < 5.625){
		/* Set the corner extremities for the left-side icon (player) and draw it */
		L = -0.25*x;
		T = 0.10*y;
		R = -0.05*x;
		B = -0.10*y;
		drawWindowPlayerIcon(L, R, B, T);

		/* Set the corner extremities for the right-side icon (station) and draw it */
		L = 0.05*x;
		T = 0.10*y;
		R = 0.25*x;
		B = -0.10*y;
		drawWindowStationIcon(L, R, B, T);
	}

	/* Draw a header on the window to tell the player what the window title is */
	if(windowState >= 1 && windowState < 5.5){
		/* Draw an extra box above the window to hold the windows header title */
		if(windowState < 1.1){
			titleHeight = 0;
		}else if(windowState < 1.4){
			/* Have the window's header expand out to it's limit */
			titleHeight = 0.002 + 0.025*((5*yMax)/h)*sin((windowState - 1.1)*3.0*M_PI/2);
		}else{
			titleHeight = textHeight;
		}

		/* Draw a white box to outline the header with white just like the window */
		glBegin(GL_QUADS);
		setColor(COLOR_HUD);
		//Top-left
		glVertex3f(-0.25*x, 0.10*y + 0.002 + titleHeight, -1);
		//Bottom-left
		glVertex3f(-0.25*x, 0.10*y, -1);
		//Bottom-right
		glVertex3f(0.25*x, 0.10*y, -1);
		//Top-right
		glVertex3f(0.25*x, 0.10*y + 0.002 + titleHeight, -1);
		glEnd();

		/* Draw a black box to represent the title's bounderies */
		glBegin(GL_QUADS);
		setColor(COLOR_BLACK);
		//Top-left
		glVertex3f(-0.25*x + 0.002, 0.10*y  + titleHeight, -1);
		//Bottom-left
		glVertex3f(-0.25*x + 0.002, 0.10*y, -1);
		//Bottom-right
		glVertex3f(0.25*x - 0.002, 0.10*y, -1);
		//Top-right
		glVertex3f(0.25*x - 0.002, 0.10*y + titleHeight, -1);
		glEnd();

		/* Draw the text that will go into the header above the window */
		sprintf(titleText, "Communication Window");
		setColor(COLOR_HUD);
		glRasterPos3f(-0.25*x + 0.004, 0.10*y + 0.004, -1);
		if(windowState >= 1.5){
			for(j = 0; j >= 0; j++){
				/* Loop through the title text and depending on the current value of windowState, display only a portion of the title */
				if(titleText[j] == '\0' || windowState - 0.01*j < 1.5){
					/* Place a end-string character mid-string to make the title appear one character at a time */
					strncpy(titleText, titleText, j);
					titleText[j] = '\0';
					j = -2;
				}
			}
			drawString(titleText);
		}
	}

	/* The second step of the window drawing process is to show the player that they are emitting radio waves by having some arbitrary
	 * sinusoidal function protrude from the player icon into the station icon. This wave first connects itself to the icons, which
	 * is all done within the windowState being {2, 3].  */
	if(windowState >= 1 && windowState < 5.5){
		/* Set the distance covered by the function. It should, at most, expand from the right icon to the left icon */
		if(windowState < 1.5){
			functionLength = -0.1*x + 0.2*(windowState - 1)*2;
		}else{
			functionLength = -0.1*x + 0.2;
		}

		/* Draw an arbitrary trig function using random trig functions and the functionLength */
		setColor(COLOR_HUD);
		glBegin(GL_LINE_STRIP);
		multi = 20.0;
		for(i = -0.1*x; i < functionLength; i += 0.0001){
			function1 = sin((((i-(-0.1*x))/0.2) - (windowState))*2*M_PI*multi*1);
			function2 = cos((((i-(-0.1*x))/0.2) - (windowState))*2*M_PI*multi*1.1);
			function3 = sin((((i-(-0.1*x))/0.2) - (windowState))*2*M_PI*multi*0.8);
			glVertex3f(i, 0.05 + 0.0125*function1 + 0.0125*function2 +0.0125*function3, -1);
		}
		glEnd();
	}

	/* Draw another arbitrary trig function from the right icon to the left, but have it end prematurly using waveSize to
	 * shrink the function's y height and waveLimit to have the function stop once it reaches a certain x value */
	if(windowState >= 1 && windowState < 5.5){
		/* Set the distance covered by the icon. It should, at most, expand from the right icon to the left icon.
		 * Once the player "connects" to the station, slowly increase this value so the bottom signal touches both icons */
		waveLimit = 0.025;
		if(windowState > 4){
			waveLimit -= (windowState - 4)*0.5;
		}
		if(windowState < 2){
			functionLength = 0.1*x - 0.2*(windowState - 1.6)*2.5;
			/* Prevent the wave from passing it's limit. If it does, the wave will only be a flat line */
			if(functionLength < waveLimit ){
				functionLength = waveLimit;
			}
		}else{
			functionLength = waveLimit;
			/* Prevent the wave from going past the desired limit and onto the left-side icon */
			if(functionLength < -0.1){
				functionLength = -0.1;
			}
		}

		/* Draw an arbitrary trig function using random trig functions and the functionLength */
		setColor(COLOR_HUD);
		glBegin(GL_LINE_STRIP);
		multi = 20.0;
		for(i = 0.1*x; i > functionLength; i -= 0.0001){
			function1 = sin((((i-(-0.1*x))/0.2) - (-windowState))*2*M_PI*multi*1);
			function2 = cos((((i-(-0.1*x))/0.2) - (-windowState))*2*M_PI*multi*1.3);
			function3 = sin((((i-(-0.1*x))/0.2) - (-windowState))*2*M_PI*multi*1.1);
			/* Depending on how far into windowState we are, change the size of the wave */
			waveSize = 0;
			if(i >= waveLimit){
				waveSize = 1;
				if(i <= waveLimit + 0.05){
					waveSize *= (i - waveLimit)*20;
				}
			}
			glVertex3f(i, -0.05 - waveSize*(0.0125*function1 - 0.0125*function2 - 0.0125*function3), -1);
		}
		glEnd();
	}

	/* Draw text that tells the user the current status of the window. It is either "awaiting repsonse" or "signal recieved"/"connection established" */
	if(windowState >= 1.8 && windowState < 5.5){
		if(windowState < 4){
			sprintf(infoText, "AWAITING RESPONSE");
		}else if(windowState < 4.5){
			sprintf(infoText, "SIGNAL RECIEVED");
		}else if(windowState < 5.1){
			sprintf(infoText, "INITIALIZING DIRECT COMMS");
		}else if(windowState < 5.5){
			sprintf(infoText, "OPENING LIVE COMMS");
		}

		/* Get the length of the text that will be printed to be able to center allign it */
		for(j = 0; infoText[j] != '\0'; j++){}
		glRasterPos3f(0 - j*textWidth/2, -0.10*y + 0.004 - textHeight, -1);
		drawString(infoText);
	}

	/* Draw elipses in-front of the bottom signal function to show that the signal has not yet been recieved. Have the amount of periods oscillate */
	if(windowState >= 2 && windowState < 4){
		setColor(COLOR_HUD);
		glRasterPos3f(waveLimit - 4*textWidth, -0.05, -1);
		/* Draw a different amount of periods depending on the current windowState */
		double windowStateDecimals = (windowState*100  -  floor(windowState)*100);
		if(windowStateDecimals < 20){
			drawString("    ");
		}else if(windowStateDecimals < 40){
			drawString("   .");
		}else if(windowStateDecimals < 60){
			drawString("  ..");
		}else if(windowStateDecimals < 80){
			drawString(" ...");
		}else{
			drawString("....");
		}
	}


}

void
drawWindowStationIcon(double L, double R, double B, double T)
{
	/*
	 * Draw the icon that represents the station in the comms window. The icon resembles a large satellite dish with rotating supports for the feed.
	 */
	double i, x, y, supportCount;
	int j, segments;
	/* Set the amount of segments that will be used when drawing the dish. Larger values mean more rounded circles, but more computation time */
	segments = 40;

	/* Extend the corner extremities for the icon's limited space. Due to the dish being centered at [0, 0], move the extremities up a
	 * bit by increasing the ceiling(T) and reducing the floor(B). The icon will not pass the extremities given as this function's parameters. */
	T *= 1.25;
	B *= 0.75;

	/* Set the width(x) and height(y) of the icon's extremities into easy to access variables to allow easier drawing.
	 * Because of the drawing process, we need to shrink the width and height so they can fit into the given extremities. */
	x = 0.50*(R-L)/2.0;
	y = 0.50*(T-B)/2.0;

	/* Set the color, translate and rotate the icon to be in the correct placement inside the given extremities to allow easy drawing */
	setColor(COLOR_HUD);
	glPushMatrix();
	glTranslatef((L+R)/2.0, (T+B)/2.0, -1);
	glRotated(55, 0, 0, 1);

	/* Draw the large, half-circle exterior shell of the dish */
	glBegin(GL_LINE_STRIP);
	for(i = segments/2; i < segments; i++){
		glVertex2f(cos((i/segments)*2*M_PI)*x, sin((i/segments)*2*M_PI)*y);
	}
	glVertex2f(cos((i/segments)*2*M_PI)*x, sin((i/segments)*2*M_PI)*y);
	glEnd();

	/* 	Draw a thin oval to represent the edge of the dish, showing the interior */
	glBegin(GL_LINE_STRIP);
	for(i = 0; i < segments; i++){
		glVertex2f(cos((i/segments)*2*M_PI)*x, sin((i/segments)*2*M_PI)*y/4);
	}
	glVertex2f(cos((i/segments)*2*M_PI)*x, sin((i/segments)*2*M_PI)*y/4);
	glEnd();

	/* Draw support lines for the "feed" that is placed in-front of the dish. The support lines are placed on the oval and go to the
	 * front of the dish, where a triangle "feed" is placed. This feed does not move, but the supports' position on the oval will rotate
	 * around the oval depending on the value of the windowState */
	glBegin(GL_LINES);
	supportCount = 3.0;
	for(j = 0; j < supportCount; j++){
		//The support beams reach the initial state of the previous beam after a full increment of windowState
		i = (windowState)*segments/supportCount + j*segments/supportCount;
		glVertex2f(cos((i/segments)*2*M_PI)*x, sin((i/segments)*2*M_PI)*y/4);
		glVertex2f(cos(0.25*2*M_PI)*x, sin(0.25*2*M_PI)*y);
	}
	glEnd();

	/* Draw a triangle at the end of the dish's "feed" to represent the dish's actual part of the dish that reads the signals */
	glBegin(GL_TRIANGLES);
	glVertex2f(cos(0.25*2*M_PI)*x, sin(0.25*2*M_PI)*y);
	glVertex2f(cos(1.10*0.25*2*M_PI)*x/1.5, sin(1.10*0.25*2*M_PI)*y/1.25);
	glVertex2f(cos(0.90*0.25*2*M_PI)*x/1.5, sin(0.90*0.25*2*M_PI)*y/1.25);
	glEnd();

	/* Straighten the matrix to easily place the supports on flat ground. This rotation should be the same as the first one done to this current matrix */
	glRotated(-55, 0, 0, 1);

	/* Draw the supports that are bellow the dish */
	glBegin(GL_LINES);

	/* Draw the right half of the support segment */
	i = 1.03*0.75;
	glVertex2f(cos((i)*2*M_PI)*x, sin((i)*2*M_PI)*y);
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.5);
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.5);
	i = 1.1*0.75;
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.25);
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.25);
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.1);

	/* Draw the left half of the support segment */
	i = 0.97*0.75;
	glVertex2f(cos((i)*2*M_PI)*x, sin((i)*2*M_PI)*y);
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.5);
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.5);
	i = 0.9*0.75;
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.25);
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.25);
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.1);

	/* Connect both support segments at the base */
	i = 1.1*0.75;
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.1);
	i = 0.9*0.75;
	glVertex2f(cos((i)*2*M_PI)*x, -1*(y*2)/1.1);
	glEnd();

	glPopMatrix();
}

void
drawWindowPlayerIcon(double L, double R, double B, double T)
{
	/*
	 * Draw the icon that represents the player in the comms window. The icon looks like a satellite, with it's nose being a filled in triangle
	 */
	double x, y;

	/* Find the distance from the center of the icon to the left (x) and top (y) edges to make the drawing process easier */
	x = (R-L)/2.0;
	y = (T-B)/2.0;

	/* Set the color, translate and rotate the icon to be in the correct placement inside the given extremities to allow easy drawing */
	setColor(COLOR_HUD);
	glPushMatrix();
	glTranslatef((L+R)/2.0, (T+B)/2.0, -1);
	glRotated(20, 0, 0, 1);

	/* Draw the body of the satellite in the center. Start with the top line and proceed in a clock-wise pattern */
	glBegin(GL_LINE_STRIP);
	glVertex2f(x*-10/50.0, y*5/50.0);
	glVertex2f(x*10/50.0, y*5/50.0);
	glVertex2f(x*10/50.0, y*-5/50.0);
	glVertex2f(x*-10/50.0, y*-5/50.0);
	glVertex2f(x*-10/50.0, y*5/50.0);
	glEnd();

	/* Draw a small rectangle on the back end of the satellite. Start with the top line, and proceed in a counterclock-wise motion */
	glBegin(GL_LINE_STRIP);
	glVertex2f(x*-10/50.0, y*-2.5/50.0);
	glVertex2f(x*-12.5/50.0, y*-2.5/50.0);
	glVertex2f(x*-12.5/50.0, y*2.5/50.0);
	glVertex2f(x*-10/50.0, y*2.5/50.0);
	glEnd();

	/* Draw a filled trapeze at the front end of the satellite. Start with the top edge and work in a clock-wise motion */
	glBegin(GL_QUADS);
	glVertex2f(x*15/50.0, y*5/50.0);
	glVertex2f(x*10/50.0, y*2.5/50.0);
	glVertex2f(x*10/50.0, y*-2.5/50.0);
	glVertex2f(x*15/50.0, y*-5/50.0);
	glEnd();

	/* Draw supports on both sides of the satellite to hold the pannels */
	glBegin(GL_LINES);
	/* Draw the top supports, starting with the left support line followed by the right */
	glVertex2f(x*2.5/50.0, y*10/50.0);
	glVertex2f(x*2.5/50.0, y*5/50.0);
	glVertex2f(x*-2.5/50.0, y*5/50.0);
	glVertex2f(x*-2.5/50.0, y*10/50.0);

	/* Draw the bottom supports, starting with the left support followed by the right */
	glVertex2f(x*2.5/50.0, y*-5/50.0);
	glVertex2f(x*2.5/50.0, y*-10/50.0);
	glVertex2f(x*-2.5/50.0, y*-5/50.0);
	glVertex2f(x*-2.5/50.0, y*-10/50.0);
	glEnd();


	/* Draw the pannels above and bellow the satellite. Again, start with the top line and work in a clock-wise motion */
	/* Draw the top pannel */
	glBegin(GL_LINE_STRIP);
	glVertex2f(x*-7.5/50.0, y*40/50.0);
	glVertex2f(x*7.5/50.0, y*40/50.0);
	glVertex2f(x*7.5/50.0, y*10/50.0);
	glVertex2f(x*-7.5/50.0, y*10/50.0);
	glVertex2f(x*-7.5/50.0, y*40/50.0);
	glEnd();
	/* Draw the bottom pannel */
	glBegin(GL_LINE_STRIP);
	glVertex2f(x*-7.5/50.0, y*-10/50.0);
	glVertex2f(x*7.5/50.0, y*-10/50.0);
	glVertex2f(x*7.5/50.0, y*-40/50.0);
	glVertex2f(x*-7.5/50.0, y*-40/50.0);
	glVertex2f(x*-7.5/50.0, y*-10/50.0);
	glEnd();
	glPopMatrix();
}


/* -- Timer Update Functions ----------------------------------------------------------------------- */

void
advanceCamera()
{
	/*
	 * Change the cam position values depending on what arrows are pressed.
	 * Certain values will prevent the camera from moving, such as if the ship is on a path to a planet's rings
	 */

	if(launchedShipPath == 0){
		if(left){
			camera->xAngle += 0.025;
		}
		if(right){
			camera->xAngle -= 0.025;
		}
		if(up){
			camera->yAngle -= 0.0125;
		}
		if(down){
			camera->yAngle += 0.0125;
		}
	}

	//Prevent the camera's angles from overflowing
	if(camera->xAngle >= 2*M_PI){
		camera->xAngle -= 2*M_PI;
	}else if(camera->xAngle < 0){
		camera->xAngle += 2*M_PI;
	}
	if(camera->yAngle >= M_PI*11/12){
		camera->yAngle = M_PI*11/12;
	}else if(camera->yAngle < M_PI/12){
		camera->yAngle = M_PI/12;
	}
	camera->focusLength += 0.01;
	if(camera->focusLength > 1){
		camera->focusLength = 1;
	}
}

void
advanceSystem()
{
	/*
	 * Move planets and stars around their system each tick
	 */
	int i;

	currentSystem->star->dayOffset += currentSystem->star->daySpeed;
	currentSystem->star->yearOffset += currentSystem->star->yearSpeed;
	if(currentSystem->star->dayOffset >= 1){
		currentSystem->star->dayOffset -= 1;
	}
	if(currentSystem->star->yearOffset >= 1){
		currentSystem->star->yearOffset -= 1;
	}
	for(i = 0; i < currentSystem->planetCount; i++){
		currentSystem->planet[i]->dayOffset += currentSystem->planet[i]->daySpeed;
		currentSystem->planet[i]->yearOffset += currentSystem->planet[i]->yearSpeed;
		if(currentSystem->planet[i]->dayOffset >= 1){
			currentSystem->planet[i]->dayOffset -= 1;
		}
		if(currentSystem->planet[i]->yearOffset >= 1){
			currentSystem->planet[i]->yearOffset -= 1;
		}
	}
}

void
updateShipPath()
{
	/*
	 * Lower the value of launchedShipPath if it's above 0. The larger the value, the faster it decrements
	 */

	if(launchedShipPath > 0){
		launchedShipPath -= 0.001;
		if(launchedShipPath > 0.25){
			launchedShipPath -= (launchedShipPath - 0.25)/100.0;
		}
	}else if(launchedShipPath < 0){
		launchedShipPath = 0;
	}
}

void
updateSatellitePath(int increment)
{
/*
 * Increment the value of all of the current planet's satellites. This is run on each timer update and when the system changes.
 *
 * On each timer update (increment = 0), it increments the satellites values by a small amount. When leave the scanning menu option
 * (increment = 1), all satellites' values are set to 1 to instantly enter orbit so we don't have to track and increment the
 * values of satellites outside the system. Sattelites included are satellites and probes.
 */
	int r, c;
	double i, ii;

	/* Ensure that the camera has a planet currently selected */
	if(selectedAstronomicalObject - 2 >= 0){
		/* Sort through the selected planet's rows and columns to visit every surface section. This might need to be changed to allow tracking
		 * which specific sections has a satallite on its way to orbit using a linked list to not require visiting all 2D array entries */
		r = 0;
		for(i = 0.15; ceil(i*100000)/100000 < 0.35; gridIncrement(0, &i, currentSystem->planet[selectedAstronomicalObject - 2]->radius, 1)){
			c = 0;
			for(ii = 0; ceil(ii*100000)/100000 < 1; gridIncrement(1, &ii, currentSystem->planet[selectedAstronomicalObject - 2]->radius, 1)){
				/* Check to see if there is currently a satellite on a path to this surface section (right side up triangle) */
				if(currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite > 0 &&
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite < 1){
					if(increment == 0){
						/* Slowly increment the value of the current system's satellites. This should be changed to track what satellites */
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite += 0.01;
						if(!(currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite < 1)){
							currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite = 1;
						}
					}else if(increment == 1){
						/* Instantly place the planet's satellites into orbit */
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite = 1;
					}
				}

				/* Check to see if there is currently a probe on a path to this surface section (right side up triangle) */
				if(currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe > 0 &&
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe < 1){
					if(increment == 0){
						/* Slowly increment the value of the current system's satellites. This should be changed to track what satellites */
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe += 0.01;
						if(!(currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe < 1)){
							currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe = 1;
						}
					}else if(increment == 1){
						/* Instantly place the planet's probes into orbit */
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe = 1;
					}
				}
				c++;

				/* Check to see if there is currently a satellite on a path to this surface section (upside down triangle) */
				if(currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite > 0 &&
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite < 1){
					if(increment == 0){
						/* Slowly increment the value of the current system's satellites. This should be changed to track what satellites */
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite += 0.01;
						if(!(currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite < 1)){
							currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite = 1;
						}
					}else if(increment == 1){
						/* Instantly place the planet's satellites into orbit */
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->satellite = 1;
					}
				}

				/* Check to see if there is currently a probe on a path to this surface section (upside down triangle) */
				if(currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe > 0 &&
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe < 1){
					if(increment == 0){
						/* Slowly increment the value of the current system's satellites. This should be changed to track what satellites */
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe += 0.01;
						if(!(currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe < 1)){
							currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe = 1;
						}
					}else if(increment == 1){
						/* Instantly place the planet's probe into orbit */
						currentSystem->planet[selectedAstronomicalObject - 2]->surface[r][c]->probe = 1;
					}
				}
				c++;
			}
		r++;
		}
	}
}

void
updateWindow()
{
	/*
	 * Advance the window state depending on its current values. drawWindow's function comment gives the different states and what they show.
	 * This function will increment the value at different rates depending on the current windowState value too :
	 *
	 * [0] dont increment the value. This is to cover the windowState = 0 condition to be met early and prevent unnecesairy checks
	 * {0, 1] increment by a consistent amount. This is the animation for opening the window.
	 * {1, 3.5] increment slowly and consistently. This is the population and adjustment of the window's assets
	 * {3.5, 4] increment inconsistently. This is to simulate "lag" when connecting to the station.
	 * {4, 4.8] increment consistently until nearing the end.
	 * {4.8, 5] increment at a very laggy pace. Nearing the end, change the background to static.
	 * {5, 6.5] increment back at a consistent rate for the window to close and change state at the end.
	 */
	double randomIncrement;

	if(windowState == 0){
		/* Don't increment the windowState value if it's at 0 */
	}else if(windowState > 0 && windowState < 1){
		/* Increment the value by a consistent amount (the same amount that is added to it when selecting the "station" option)*/
		if(windowState < 0.5){
			windowState += 0.25;
		}else{
			windowState += 0.125;
		}
		/* Prevent windowState from incrementing too far ahead */
		if(windowState > 1){
			windowState = 1;
		}
	}else if(windowState < 3.5){
		/* Increment slowly and consistently */
		windowState += 0.01;
	}else if(windowState < 4){
		/* Start incrementing at wierd intervals to simulate lag */
		randomIncrement = myRandom(0, 1);
		if(randomIncrement > 0.55){
			windowState += randomIncrement*0.025;
		}
		if(windowState > 4){
			windowState = 4;
		}
	}else if(windowState < 4.75){
		/* Once the player "connects" to the station, start incrementing at a regular pace */
		windowState += 0.01;
	}else if(windowState < 5){
		/* Increment at a very laggy pace. During this time, change the background to static before reaching past a windowState of 5 */
		randomIncrement = myRandom(0, 1);
		if(randomIncrement > 0.95){
			windowState += randomIncrement*0.025;
		}
		if(windowState > 5){
			windowState = 5;
		}
	}else if(windowState <= 5.5){
		/* Increment the value slowly to give the user time to read the flavor text before closing the window */
		windowState += 0.01;
	}else if(windowState < 6.5){
		/* Increment the value quickly as the animation for closing the window plays out */
		if(windowState < 6){
			windowState += 0.125;
		}else{
			windowState += 0.25;
		}
	}else if(windowState >= 6.5){
		/* Increment the value a bit so the game lingers on the static background before changing state */
		windowState += 0.01;
	}else if(windowState >= 6.75){
		/* Change the state to the station menus */
		//changeState();
	}
	printf("CURRENT STATE: %f\n", windowState);
}


/* -- Event Trigger Functions ------------------------------------------------------------------------- */

void
randomSystem()
{
	/*
	 * Populate the savefile with new systems and initialize values for the systemArray. System 0 will always
	 * be the default system for the player, which will have certain conditions always be met:
	 * There is a randomly selected planet in the system which will be a ringed planet
	 */
	int i, ii, r, c, planetCount, rows, columns;
	double iii, iiii;
	char systemName[NAME_LENGTH];
	SystemStar newStar;
	FILE* savefile = fopen("savefile","w+");

	for(i = 0; i < SYSTEM_COUNT; i++){
		// Names for astranomical objects can have all letters, numbers, white spaces and dashes (NO COMAS!)
		strcpy(systemName, "SystemName");
		planetCount = myRandom(1, 10);
		//newPlanetArray = malloc(sizeof(SystemPlanet*)*planetCount);
		strcpy(systemArray[i]->name, systemName);
		systemArray[i]->planetCount = planetCount;
		systemArray[i]->x = myRandom(-100, 100);
		systemArray[i]->y = myRandom(-100, 100);
		systemArray[i]->z = myRandom(-100, 100);
		/* Save the system's stats */
		fprintf(savefile, "[%d] %s, %f, %f, %f, %d\n", i, systemArray[i]->name, systemArray[i]->x, systemArray[i]->y, systemArray[i]->z, systemArray[i]->planetCount);
		/* Generate the systems star */
		strcpy(newStar.name, "new Star");
		newStar.radius = myRandom(60, 100);
		newStar.orbitRadius = 0;
		newStar.axialTilt = 0;
		newStar.orbitTilt = 0;
		newStar.orbitOffset = 0;
		newStar.dayOffset = 0;
		newStar.yearOffset = 0;
		newStar.daySpeed = myRandom(0.001, 0.0001);
		newStar.yearSpeed = 0;
		/* save the star's stats */
		fprintf(savefile, "%s, %f, %f, %f, %f, %f, %f, %f, %f, %f\n", newStar.name, newStar.radius, newStar.orbitRadius, newStar.axialTilt,
				newStar.orbitTilt, newStar.orbitOffset, newStar.dayOffset, newStar.yearOffset, newStar.daySpeed, newStar.yearSpeed);

		/* Genetrate the systems planets */
		SystemPlanet newPlanetArray[planetCount];
		for(ii = 0; ii < planetCount; ii++){
			strcpy(newPlanetArray[ii].name, "new Planet");
			newPlanetArray[ii].radius = myRandom(3, 20);
			/* Keep the radius from having more than 3 decimal point values */
			newPlanetArray[ii].radius = ceil(newPlanetArray[ii].radius*1000)/1000.0;
			//Prevent the planets from clipping into eachother
			if(ii > 0){
				newPlanetArray[ii].orbitRadius = newPlanetArray[ii-1].orbitRadius + newPlanetArray[ii-1].radius/2.0 + newPlanetArray[ii].radius/2.0 + myRandom(30, 100);
			}else{
				newPlanetArray[ii].orbitRadius = newStar.radius + myRandom(30, 100);
			}
			newPlanetArray[ii].type = myRandom(0, 1);
			newPlanetArray[ii].axialTilt = myRandom(0, 180);
			newPlanetArray[ii].orbitTilt = myRandom(-30, 30);
			newPlanetArray[ii].orbitOffset = myRandom(-30, 30);
			newPlanetArray[ii].dayOffset = myRandom(0.0, 1.0);
			newPlanetArray[ii].yearOffset = myRandom(0.0, 1.0);
			newPlanetArray[ii].daySpeed = myRandom(0.001, 0.0001);
			newPlanetArray[ii].yearSpeed = ceil((myRandom(0.05, 0.01)/newPlanetArray[ii].orbitRadius)*100000)/100000;


			/* Initialize the planet's surface stats and set default values */
			/* Find how many rows can fit on this planet's surface */
			rows = 0;
			for(iii = 0.15; ceil(iii*100000)/100000 < 0.35; gridIncrement(0, &iii, newPlanetArray[ii].radius, 1)){
				rows++;
			}
			/* Find out how many columns can fit on this planet's surface. Remember that each column has two section triangles */
			columns = 0;
			for(iiii = 0.0; ceil(iiii*100000)/100000 < 1.0; gridIncrement(1, &iiii, newPlanetArray[ii].radius, 1)){
				columns++;
				columns++;
			}
			newPlanetArray[ii].surfaceRows = rows;
			newPlanetArray[ii].surfaceColumns = columns;

			/* Save the planet's values */
			fprintf(savefile, "{%d, %d}, %s, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n", rows, columns, newPlanetArray[ii].name, newPlanetArray[ii].radius,
					newPlanetArray[ii].type, newPlanetArray[ii].orbitRadius, newPlanetArray[ii].axialTilt, newPlanetArray[ii].orbitTilt, newPlanetArray[ii].orbitOffset,
					newPlanetArray[ii].dayOffset, newPlanetArray[ii].yearOffset, newPlanetArray[ii].daySpeed, newPlanetArray[ii].yearSpeed);

			/* Initilize the surfaceArray */
			Surface planetSurface[rows][columns];

			for(r = 0; r < rows; r++){
				for(c = 0; c < columns; c++){
					/* Assign values to the planet's surfaces */
					planetSurface[r][c].satellite = 0;
					planetSurface[r][c].probe = 0;
					planetSurface[r][c].energy = myRandom(0, 1);
					planetSurface[r][c].mineral = myRandom(0, 1);
					/* Save the surface's values */
					fprintf(savefile, "(%d, %d), %f, %f, %f, %f ", r, c, planetSurface[r][c].satellite,
							planetSurface[r][c].probe, planetSurface[r][c].energy,
							planetSurface[r][c].mineral);
				}
				fprintf(savefile, "\n");
			}
		}
	}

	/* Set a currentSystemIndex for the user to start in and add it to the save file */
	currentSystemIndex = 0;
	fprintf(savefile, "[%d]\n", currentSystemIndex);

	/* Close the file to save it's values. It needs to be opened again to re-access it's values */
	fclose(savefile);

	/* Load the currentSystemIndex into the currentSystem */
	printf("start loading\n");
	loadSystem(currentSystemIndex);
	printf("done loading\n");

	/* Set the currently selected planet to be a ringed planet in the current system and save the changed value */
	int startingPlanet;
	startingPlanet = ceil(myRandom(0, currentSystem->planetCount-1));
	selectedAstronomicalObject = (int) (startingPlanet) + 2;
//	currentSystem->planet[selectedAstronomicalObject - 2]->type = myRandom(PLANET_TYPE_COLONIZED_LIMIT, PLANET_TYPE_RING_LIMIT);
	currentSystem->planet[selectedAstronomicalObject - 2]->type = PLANET_TYPE_COLONIZED_LIMIT;
	printf("start saving\n");
	saveSystem();
	printf("done saving\n");
}

void
calculateBackgroundStars()
{
	/*
	 * Populate the BGStars array and find the distance of each system
	 */
	int i;
	double largest, x ,y, z, distance;

	nearbySystems = 0;
	for(i = 0; i < SYSTEM_COUNT; i++){
		/* Find the distance between the current system and all the others */
		x = systemArray[i]->x - currentSystem->x;
		y = systemArray[i]->y - currentSystem->y;
		z = systemArray[i]->z - currentSystem->z;
		BGStars[i]->x = x;
		BGStars[i]->y = y;
		BGStars[i]->z = z;
		/* Make sure each star is far enough from the origin */
		/* Find the largest value of the three */
		largest = abs(BGStars[i]->x);
		if(largest < abs(BGStars[i]->y)){
			largest = abs(BGStars[i]->y);
		}
		if(largest < abs(BGStars[i]->z)){
			largest = abs(BGStars[i]->z);
		}
		/* multiply the three values to have atleast one of the three pass or reach the "skybox" */
		largest = 5000/largest;
		BGStars[i]->x *= largest;
		BGStars[i]->y *= largest;
		BGStars[i]->z *= largest;

		/* Find the distance between the current system and all the others
		 * and put it in the nearbySystems array used to find nearby systems.
		 * If the distance is jumpable, increment the nearbySystems values */
		distance = sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
		if(distance < player.jumpDistance){
			nearbySystems++;
		}
		systemDistanceArray[i] = distance;
	}
}

void
systemJump()
{
	/*
	 * Jump to the system that is currently selected using the selectedSystem values
	 * along with the jumpDistance values to find the systems currently selected by the user
	 */
	int selectedSystemIndex;
	int i;
	int close = 0;

	if(selectedSystem > 0){
	    for(i = 0; i < SYSTEM_COUNT; i++){
	    	if(currentSystemIndex != i && systemDistanceArray[i] < player.jumpDistance){
				close++;
				if(selectedSystem == close){
					selectedSystemIndex = i;
				}
	    	}
	    }

	    /* First save the currentSystem into the savefile then Load the selected system into the currentSystem variable */
	    saveSystem();
		loadSystem(selectedSystemIndex);

		calculateBackgroundStars();
		selectedSystem = 0;
		selectedAstronomicalObject = 0;
		displayedHUD = 0;
		camera->focus = 0;
		camera->xAngle = 0.0;
		camera->yAngle = M_PI/3;
		camera->focusLength = 1.0;
	}
}

void
systemSelectMenu()
{
	/*
	 * The user selected an option in the HUD's menu. Use the current values of
	 * selectedAstronomicalObject, displayedHUD and selectedHUD to determine what option was selected.
	 * This is the order of checks to narrow down what the user could have selected :
	 *
	 * selectedAstronomicalObject is used to specify what kind of object the user selected. It's also the camera's focus target.
	 * displayedHUD is used to find what stage the HUD is displaying, such as stats/scan/ring for planets or scan/probe for scan
	 * selectedHUD is used in conjunction with displayedHUD to form a tree of options: selectedHUD determines what number option is selected
	 *
	 */

	if(camera->focus == 0){
		/* Focusing on an object in the current system */
		if(selectedAstronomicalObject == 0){
			/* Focusing on the currentSystem as a whole */
			if(displayedHUD == 0){
				/* Selected the currentSystem as a whole */
				printf("selected the system as a whole\n");
			}
		}else if(selectedAstronomicalObject <= 1){
			/* Focusing on the system's star */
			if(displayedHUD == 0){
				/* Selected the currentSystem's star */
				printf("selected the system's star\n");
			}
		}else if(selectedAstronomicalObject <= 1 + currentSystem->planetCount){
			/* Focusing on a planet */
			if(displayedHUD == 0){
				/* Selected one of the system's planets. A planet has a set of default options along with special options available only from it's type:
				 * stats: shows planet stats. Default option
				 * scan: shows what is found on the surface of the planet. Default option
				 * ring: lets the user enter the ring to begin asteroids. Must be a ringed planet
				 * station: visit the station that only orbits colonized planets */
				displayedHUD = 1;
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
			}else if(displayedHUD == 1){
				/* Selected one of the options in the planet's first level menu (stats/scan/ring) */
				if(selectedHUD == 0){
					/* Selected the Stats option for the planet. Change the displayHUD to the stats value */
					displayedHUD = 2;
				}else if(selectedHUD == 1){
					/* Selected the scan option for the planet */
					displayedHUD = 3;
					selectedHUD = 0;
					selectedHUDMax = 2;
				}else if(selectedHUD == 2){
					/* Selected the 3rd option */
					if(currentSystem->planet[selectedAstronomicalObject - 2]->type <= PLANET_TYPE_COLONIZED_LIMIT){
						/* Colonized planets' 3rd option is to visit the orbitting station */
						/*
						 * When selectiing a station, have a "vid window" appear and show the user that they are trying to communicate to
						 * the station. If it succeeds, change the state to the "communication state". The transition would involve the vidwindow
						 * showing a "communication succesfull", then having the background turn to static. The vid window will then close
						 * to show only static, which would be an easy transition point to run the stateChange.
						 *
						 * The vidwindow can show two points and two lines between them. The top line would have a wave of sorts, representing
						 * a signal to the second point. The bottom line would be maybe incrementing points, someting that changes at a consistent rate.
						 * Simulate a connection by having the consistent incrementation stutter, than the bottom line become a signal. Close
						 * the vidwindow to static, and change state.
						 */
						printf("SELECTED THE STATION OPTION\n");
						windowState = 0.125;
					}else if(currentSystem->planet[selectedAstronomicalObject - 2]->type <= PLANET_TYPE_RING_LIMIT){
						/* Ringed planets' 3rd option is to enter the planet's ring */
						launchedShipPath = 1;
						displayedHUD = 4;
					}
				}
			}else if(displayedHUD == 3){
				/* Selected an option in the scan planet menu */
				if(selectedHUD == 0){
					/* Selected the scan planet surface section */
					systemLaunchSatellite(0);
				}else if(selectedHUD == 1){
					/* Selected the probe planet surface section */
					systemLaunchSatellite(1);
				}
			}
		}
	}
}

void
gridIncrement(int incrementor, double* value, double radius, double multiplier)
{
	/*
	 * Increment the value (row or column with 0 and 1 respectivly) for going through a planet's scan grid. This is put
	 * into a function since it uses an arbitrary function to increment the iterator and is useed in multiple scenarios.
	 * The multiplier varaible can be used to add a multiplier to the increment, making it possible to do half/negative increments.
	 *
	 * Whenever this function is used at the final row/columnm, it can end up returning 0.3.4999... or 0.99..., which can cause
	 * extra rows or columns to be created. Whenever checking if the i value that get incremented has reached it max (0.35 or 1.0),
	 * round up the value past a certain decimal point: ceil(i*10000)/10000 assumes that 0.9999 has no other rows/columns past 0.9999,
	 * which is only true IF incrementing is more than 0.0001 at a time, AKA less than 10000 rows/columns.
	 */

	if(incrementor == 0){
		/* Increment the value for ii/rows. the 0.2 is used since 0.35 - 0.15 */
		*value += (0.2/ceil((radius + 5)/3.0))*multiplier;
	}else if(incrementor == 1){
		/* Increment the value for iii/columns */
		*value += (1.0/ceil((radius + 5)))*multiplier;
	}
}

void
systemLaunchSatellite(int i)
{
	/*
	 * Launch a satellite into orbit of the currently selected planet, into the currently selected section of the surface.
	 * Possible satellites that can orbit a planet are satellites (scan the surface) and probes (take items from the surface).
	 *
	 * Use the values of camR and camC to find the currently selected section of the planet to scan. Sattelites will be referenced to through
	 * the planet's surface's sattelire value.  Once a sattelite is launched, it will need to travel to reach the orbit. The value between 0 and 1
	 * determines at what point the satellite is in it's tradjectory, just like the ship path.
	 */

	if(camera->camR == -1 || camera->camC == -1){
		printf("no section selected for satallite launch\n");
	}else{
		if(i == 0){
			/* Launch satellite option selected */
			if(currentSystem->planet[selectedAstronomicalObject - 2]->surface[camera->camR - 1][camera->camC - 1]->satellite > 0){
				printf("There is already a satellite in orbit here\n");
			}else{
				printf("Sent a satellite into orbit\n");
				currentSystem->planet[selectedAstronomicalObject - 2]->surface[camera->camR - 1][camera->camC - 1]->satellite = 0.01;
			}
		}else if(i == 1){
			/* Launch probe option selected */
			if(currentSystem->planet[selectedAstronomicalObject - 2]->surface[camera->camR - 1][camera->camC - 1]->probe > 0){
				printf("There is already a probe in orbit here\n");
			}else{
				printf("Sent a probe into orbit\n");
				currentSystem->planet[selectedAstronomicalObject - 2]->surface[camera->camR - 1][camera->camC - 1]->probe = 0.01;
			}
		}
		printf("_%d_%d_",camera->camR, camera->camC);
	}
}

void
setColor(int colorID)
{
	/*
	 * Change the current color/material to the selected ID using the global color variables as a reference
	 */

	if(colorID == COLOR_HUD){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_HUD);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_HUD);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_HUD);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_HUD);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_HUD);
	}else if(colorID == COLOR_STAR){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_Star);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_Star);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_Star);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_Star);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_Star);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_Star);
	}else if(colorID == COLOR_PLANET){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_planet);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_planet);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_planet);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_planet);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_planet);
	}else if(colorID == COLOR_BG){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_BG);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_BG);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_BG);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_BG);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_BG);
	}else if(colorID == COLOR_ORBIT){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_orbit);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_orbit);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_orbit);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_orbit);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_orbit);
	}else if(colorID == COLOR_RING){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_ring);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_ring);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_ring);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_ring);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_ring);
	}else if(colorID == COLOR_SCAN_GRID){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_scan_grid);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_scan_grid);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_scan_grid);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_scan_grid);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_scan_grid);
	}else if(colorID == COLOR_SCAN_GRID_SELECTED){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_scan_grid_selected);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_scan_grid_selected);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_scan_grid_selected);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_scan_grid_selected);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_scan_grid_selected);
	}else if(colorID == COLOR_SATELLITE){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_satellite);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_satellite);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_satellite);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_satellite);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_satellite);
	}else if(colorID == COLOR_ENERGY){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_energy);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_energy);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_energy);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_energy);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_energy);
	}else if(colorID == COLOR_MINERAL){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_mineral);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_mineral);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_mineral);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_mineral);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_mineral);
	}else if(colorID == COLOR_BLACK){
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_black);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_black);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_black);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_black);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_black);
	}else{
		printf("!!! COLOR ID DOES NOT HAVE A MATERIAL ASSIGNED !!!\n");
	}
}

void
setColorValue(double r, double g, double b)
{
	/*
	 * Set the current color to the given parameters by using a dynamic material. Set only the ambient color.
	 */

	GLfloat material_a_dynamic[] = {r, g, b, 1.0};
	GLfloat material_d_dynamic[] = {0.0, 0.0, 0.0, 1.0};
	GLfloat material_sp_dynamic[] = {0.0, 0.0, 0.0, 0.0};
	GLfloat material_e_dynamic[] = {0.0, 0.0, 0.0, 0.0};
	GLfloat material_sh_dynamic[] = {0.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,  material_a_dynamic);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  material_d_dynamic);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  material_sp_dynamic);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  material_e_dynamic);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, material_sh_dynamic);
}


/* -- File reading/writing functions -------------------------------------------------- */

void
saveSystem()
{
	/*
	 * Save the current system's values into the save file. This should be run on program close and system change.
	 * The process of this is to read in the original savefile into a buffer. It is sent to a new temp file unless it
	 * is the current system, which is where it's new values will be added instead of the saved values. The rest of the file
	 * is sent into the buffer and the temp file is now the new savefile, removing the previous one.
	 */
	int i, ii, iii, pos;
	FILE* saveStream = fopen("savefile", "r+");

	/* Cycle through the savefile looking for the starting position for the given system index */
	fseek(saveStream, 0, SEEK_SET);
	pos = -1;

	while(pos != currentSystemIndex){
		/* Scan the current line assuming it is a system line and fetch the index */
		fscanf(saveStream, "[%d]", &pos);
		if(pos != currentSystemIndex){
			/* Push the stream pointer to the next line */
			fscanf(saveStream, "%*[^\n]%*c");
		}
	}

	/* The stream's pointer position is currently on the systems value line. Skip past the rest of it and the star to reach the planet's surfaces */
	fscanf(saveStream, "%*[^\n]%*c");
	fscanf(saveStream, "%*[^\n]%*c");

	for(i = 0; i < currentSystem->planetCount; i++){
		/* Update the planet's stats to follow along with the surface values */
		fseek(saveStream, 0, SEEK_CUR);
		if(fprintf(saveStream, "{%d, %d}, %s, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf\n", currentSystem->planet[i]->surfaceRows, currentSystem->planet[i]->surfaceColumns,
				currentSystem->planet[i]->name, currentSystem->planet[i]->radius, currentSystem->planet[i]->type, currentSystem->planet[i]->orbitRadius,
				currentSystem->planet[i]->axialTilt, currentSystem->planet[i]->orbitTilt, currentSystem->planet[i]->orbitOffset, currentSystem->planet[i]->dayOffset,
				currentSystem->planet[i]->yearOffset, currentSystem->planet[i]->daySpeed, currentSystem->planet[i]->yearSpeed) < 0){
			printf("ERROR SAVING PLANET VALUES\n");
		}

		for(ii = 0; ii < currentSystem->planet[i]->surfaceRows; ii++){
			for(iii = 0; iii < currentSystem->planet[i]->surfaceColumns; iii++){
				/* Update the surface values */
				fseek(saveStream, 0, SEEK_CUR);
				if(fprintf(saveStream, "(%d, %d), %lf, %lf, %lf, %lf ",ii, iii, currentSystem->planet[i]->surface[ii][iii]->satellite,
						currentSystem->planet[i]->surface[ii][iii]->probe, currentSystem->planet[i]->surface[ii][iii]->energy,
						currentSystem->planet[i]->surface[ii][iii]-> mineral) < 0){
					printf("ERROR SAVING SURFACE VALUES at system %d, planet %d, [%d, %d]\n", currentSystemIndex, i, ii, iii);
				}
			}
			/* Add a line-break after every row change unless it's the final planet */
			fseek(saveStream, 0, SEEK_CUR);
			fprintf(saveStream, "\n");
		}
	}

	/* Close the stream after saving the system's values */
	fclose(saveStream);


	/* Re-open the stream to change the currentSystemIndex */
	saveStream = fopen("savefile", "r+");
	char firstChar = ' ';

	/* Go past the final system line */
	while(pos < SYSTEM_COUNT - 1){
		/* Scan the current line assuming it is a system line and fetch the index */
		fscanf(saveStream, "[%d]", &pos);
		if(pos < SYSTEM_COUNT - 1){
			/* Push the stream pointer to the next line */
			fscanf(saveStream, "%*[^\n]%*c");
		}
	}

	/* Go to the final system line holding the currentSystemIndex value, identified by the "[" */
	while(firstChar != '['){
		/* Scan the current line assuming it is a system line and fetch the index */
		fscanf(saveStream, "%c", &firstChar);

		if(firstChar != '['){
			/* Push the stream pointer to the next line */
			fscanf(saveStream, "%*[^\n]%*c");
		}
	}
	fseek(saveStream, -1, SEEK_CUR);
	if(fprintf(saveStream, "[%d]\n", currentSystemIndex) < 0){
		printf("ERROR SAVING CURRENT INDEX\n");
	}

	/* Close the stream after saving the currentSystemIndex */
	fclose(saveStream);
}

void
loadSystem(int newSystemIndex)
{
	/*
	 * Use the given index to find the given system in the save file and load it into the currentsystem value.
	 * Save the current system before loading a new one. This will be run when changing systems or loading a savefile.
	 */
	int i, ii, iii, pos;
	double saveOffset;
	double lastLineLength;
	FILE* loadStream = fopen("savefile", "r");

	/* Cycle through the savefile looking for the starting position for the given system index */
	fseek(loadStream, 0, SEEK_SET);
	pos = -1;
	saveOffset = 0;

	while(pos != newSystemIndex){
		/* Scan the current line assuming it is a system line and fetch the index */
		lastLineLength = ftell(loadStream);
		fscanf(loadStream, "[%d]", &pos);
		saveOffset =  ftell(loadStream) - lastLineLength;
		if(pos != newSystemIndex){
			/* Push the stream pointer to the next line */
			fscanf(loadStream, "%*[^\n]%*c");
		}
	}

	/* Using the offset value and the length of the previous line, redirect the stream to it's proper position at the start of the system line */
	fseek(loadStream, -saveOffset, SEEK_CUR);

	/* Start by reallocating the memory used by the currentSystem */
	if(currentSystem != NULL){
		/* Free the memory used for the currentSystem if it has been initilized */
		if(currentSystem->star != NULL){
			free(currentSystem->star);
		}

		if(currentSystem->planet != NULL){
			for(i = 0; i < currentSystem->planetCount; i++){
				if(currentSystem->planet[i] != NULL){
					if(currentSystem->planet[i]->surface != NULL){
						for(ii = 0; ii < currentSystem->planet[i]->surfaceRows; ii++){
							for(iii = 0; iii < currentSystem->planet[i]->surfaceColumns; iii++){
								if(currentSystem->planet[i]->surface[ii][iii] != NULL){
									free(currentSystem->planet[i]->surface[ii][iii]);
								}
							}
						}
						free(currentSystem->planet[i]->surface);
					}
					free(currentSystem->planet[i]);
				}
			}
		}
		free(currentSystem);
	}

	/* Get the first line, which gives the system information and star/planet count. Start allocating memory for the currentSystem */
	currentSystem = malloc(sizeof(CurrentSystemType));
	fscanf(loadStream, "[%d] %[a-zA-Z -], %lf, %lf, %lf, %d\n", &pos, currentSystem->name, &currentSystem->x,
			&currentSystem->y, &currentSystem->z, &currentSystem->planetCount);

	/* 	Use the following line to initialize the currentSystem's star */
	currentSystem->star = malloc(sizeof(SystemStar));
	fscanf(loadStream, "%[a-zA-Z -], %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf\n", currentSystem->star->name, &currentSystem->star->radius,
			&currentSystem->star->orbitRadius, &currentSystem->star->axialTilt, &currentSystem->star->orbitTilt, &currentSystem->star->orbitOffset,
			&currentSystem->star->dayOffset, &currentSystem->star->yearOffset, &currentSystem->star->daySpeed, &currentSystem->star->yearSpeed);

	/* Use the following lines and the systems planetCount to populate the planets and their surfaces */
	currentSystem->planet = malloc(sizeof(SystemPlanet*)*currentSystem->planetCount);
	for(i = 0; i < currentSystem->planetCount; i++){
		/* Initialize and populate the planet's values */
		currentSystem->planet[i] = malloc(sizeof(SystemPlanet));
		fscanf(loadStream, "{%d, %d}, %[a-zA-Z -], %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf, %lf\n",&currentSystem->planet[i]->surfaceRows, &currentSystem->planet[i]->surfaceColumns,
				currentSystem->planet[i]->name, &currentSystem->planet[i]->radius, &currentSystem->planet[i]->type, &currentSystem->planet[i]->orbitRadius,
				&currentSystem->planet[i]->axialTilt, &currentSystem->planet[i]->orbitTilt, &currentSystem->planet[i]->orbitOffset, &currentSystem->planet[i]->dayOffset,
				&currentSystem->planet[i]->yearOffset, &currentSystem->planet[i]->daySpeed, &currentSystem->planet[i]->yearSpeed);

		/* Initilize the planet's surface array */
		currentSystem->planet[i]->surface = malloc(sizeof(Surface**)*currentSystem->planet[i]->surfaceRows);
		for(ii = 0; ii < currentSystem->planet[i]->surfaceRows; ii++){
			currentSystem->planet[i]->surface[ii] = malloc(sizeof(Surface*)*currentSystem->planet[i]->surfaceColumns);
			for(iii = 0; iii < currentSystem->planet[i]->surfaceColumns; iii++){
				/* Initialize and populate the give surface object */
				currentSystem->planet[i]->surface[ii][iii] = malloc(sizeof(Surface));
				fscanf(loadStream, "(%*d, %*d), %lf, %lf, %lf, %lf ", &currentSystem->planet[i]->surface[ii][iii]->satellite,
						&currentSystem->planet[i]->surface[ii][iii]->probe, &currentSystem->planet[i]->surface[ii][iii]->energy,
						&currentSystem->planet[i]->surface[ii][iii]->mineral);
			}
			/* Add a line-break for every new row */
			fscanf(loadStream, "\n");
		}
	}

	/* Set the currentSystemIndex to the system that is currently being loaded */
	currentSystemIndex = newSystemIndex;

	/* Now currentSystem has loaded the proper system information from the savefile and we can close the reader */
	fclose(loadStream);
}

