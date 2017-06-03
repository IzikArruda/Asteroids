/*
 * This file and state is used as a menu system. The player can use keys or the mouse to interract with the windows that appear.
 * The first window that will appear depends on the request sent to this file. That first window will create many other sub windows,
 * which can be closed or can open other windows. The player will be given many options that will change their player values.
 */
#include "simple.h"

/* These values are used to determine what menu type will be run when the player visits this state */
#define MENU_STATION 1

/* --- Type Definitions -------------------------------------------------------- */
typedef struct Window {
	double x, y;
} Window;


/* --- Function prototypes --------------------------------------------------- */

static void initMenus(int type);


/* --- Initilization functions -------------------------------------------------------------------------- */

void
initMenus(int type)
{
	/*
	 * Initialize the proper menu screen for when the player will enter this state
	 */
	printf("testingsdf\n\n\n");
}
