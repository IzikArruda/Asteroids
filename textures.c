/* 
 * This file contains all the saved values for specific textured objects, such as planet rings and static screens. These objects
 * require coordinates for they're object before they are drawn, so they have their own rendering process.
 */
#include "simple.h"

/* These values are used to identify what object to load */
#define TEXTURED_STATIC_BACKGROUND 1


/* --- Function prototypes --------------------------------------------------- */

static void drawTexturedStaticBackground(double x, double y);
static void drawTexturedRings(double radius, double sections, double type);


/* --- Drawing functions -------------------------------------------------------------------------- */

void
drawTexturedStaticBackground(double x, double y){
	/*
	 * Draw the static background with a random texture of static. The given parameters are the object's extremities.
	 */
	int i, vertexCount, width, height;
	GLuint textureID;

	/* Set useful values before starting the rendering, such as how many vertices and pixels will be renderedto allow a decent looking static effect */
	vertexCount = 4;
	width = (int) w/5;
	height = (int) h/5;

	/* Set the vertices for the background plane and the indices to link the vertices to the DrawElements drawing process */
	double vertices[vertexCount*3];
	GLuint indice[vertexCount];
	for(i = 0; i < vertexCount; i++){
		indice[i] = i;
	}
	vertices[0] = x;
	vertices[1] = y;
	vertices[2] = -1;
	vertices[3] = x;
	vertices[4] = -y;
	vertices[5] = -1;
	vertices[6] = -x;
	vertices[7] = -y;
	vertices[8] = -1;
	vertices[9] = -x;
	vertices[10] = y;
	vertices[11] = -1;

	/* Set the texture coordinates that links the texture to the object. Values range between [0, 1]. Values higher require GL_CLAMP, BL_REPEAT, etc. */
	double textureCoords[vertexCount*2];
	//Top-left
	textureCoords[0] = 0;
	textureCoords[1] = 5;
	//Bottom-left
	textureCoords[2] = 0;
	textureCoords[3] = 0;
	//Bottom-right
	textureCoords[4] = 5;
	textureCoords[5] = 0;
	//Top-right
	textureCoords[6] = 5;
	textureCoords[7] = 5;

	/* Set the texture value for the background's random static. The size of the texture increases with the size of the window */
	float rand;
	float textureStatic[width*height*3];
	for(i = 0; i < width*height; i++){
		rand = (float) myRandom(0.33, 0.66);
		textureStatic[i*3 + 0] = rand;
		textureStatic[i*3 + 1] = rand;
		textureStatic[i*3 + 2] = rand;
	}

	/* Bind the texture and set it's parameters */
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	/* Link the texture, coords and the vertices together */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, textureStatic);
	glTexCoordPointer(2, GL_DOUBLE, 0, textureCoords);
	glVertexPointer(3, GL_DOUBLE, 0, vertices);
	/* Draw the final linked object */
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indice);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void
drawTexturedRings(double radius, double sections, double type){
	/*
	 * Draw textured rings of a planet by using the given parameters to determine the size/radius of the ring. Use the type value in the looks
	 */
	double vertices[4*3];
	double textureCoords[4*2];
	double minR, maxR;
	GLuint indice[4];
	GLuint textureID;
	int i, ringSize;
	int sepperatorDigit, sizeDigit, Func1ModDigit, Func2ModDigit;

	/* Recover the last 4 decimal values of the planet type to use to determine the ring's textures */
	type = 10000*((type*100) - (int) (type*100));
	//Get the first (left-most) digit and use it as a section sepperator that stretches the rings.
	sepperatorDigit = (int)(type/1000)%10;
	//Get the second digit and use it to determine the overall length of the rings. Prevent it from being 0.
	sizeDigit = 1 + (int)(type/100)%10;
	//Get the third and fourth digits, using them as modifiers for the functions used to determine the ring's alpha channel. Prevent these from being 0
	Func1ModDigit = 1 + (int)(type/10)%10;
	Func2ModDigit = 1 + (int)(type)%10;

	/* Save the minimum and maximum distance that the ring's disk */
	minR = 1.5;
	maxR = minR + 0.1*sizeDigit;

	/* Calculate the width of the rings for the texture count */
	ringSize = maxR*100;

	/* Set the indices used to link the drawing process to the vertices */
	indice[0] = 0;
	indice[1] = 1;
	indice[2] = 2;
	indice[3] = 3;

	/* Set the texture coordinates for the rings. These are the same for both sides */
	//Top-left
	textureCoords[0] = 0;
	textureCoords[1] = 1;
	//Bottom-left
	textureCoords[2] = 0;
	textureCoords[3] = 0;
	//Bottom-right
	textureCoords[4] = 1;
	textureCoords[5] = 0;
	//Top-right
	textureCoords[6] = 1;
	textureCoords[7] = 1;

	/* Set the texture for the rings */
	float textureRings[ringSize*4];
	for(i = 0; i < ringSize; i++){
		textureRings[(i*4) + 0] = 1;
		textureRings[(i*4) + 1] = 1;
		textureRings[(i*4) + 2] = 1;
	}

	/* Set the alpha channel for the rings. This uses the type's previously saved digits and a set of trig functions */

	double mult = 0.5 + sepperatorDigit/10.0;
	double function1, function2, function3;
	for(i = 0; i < ringSize; i++){
		function1 = (1 + sin((i/100.0f)*2*M_PI*mult*1))/2.0;
		function2 = (1 + cos((i/100.0f)*2*M_PI*mult*(0.7 + Func1ModDigit/5.0)))/2.0;
		function3 = (1 + sin((i/100.0f)*2*M_PI*mult*(0.7 + Func2ModDigit/5.0)))/2.0;
		textureRings[(i*4) + 3] = 0.5*(function1 + function2 + function3)/3.0;

		/* Have the interior edges fade into full opacity if they are near the center */
		if(i <= 50){
			if(textureRings[(i*4) + 3] > i/50.0){
				textureRings[(i*4) + 3] = i/50.0;
			}
		}

		/* Have the outside edges fade into full opacity if they are too high */
		if(i >= ringSize - 20){
			if(textureRings[(i*4) + 3] > (ringSize - i)/20.0){
				textureRings[(i*4) + 3] = (ringSize - i)/20.0;
			}
		}
	}

	/* Bind the texture and set it's parameters */
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	/* Link the texture, coords and the vertices together */
	glTexCoordPointer(2, GL_DOUBLE, 0, textureCoords);
	glVertexPointer(3, GL_DOUBLE, 0, vertices);

	/* To draw the rings, set the vertices to the current face to draw, then run the glDrawelements command using
	 * all the previously set values for the textures and texCoords. Do this for every face on the given side of the ring */
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	/* Set the vertices and draw the bottom-side of the rings */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ringSize, 1, 0, GL_RGBA, GL_FLOAT, textureRings);
	for(i = 1; i <= sections; i++){
		vertices[0] = radius*minR*sin(((i-1)/sections)*2*M_PI);
		vertices[1] = radius*minR*cos(((i-1)/sections)*2*M_PI);
		vertices[2] = 0;

		vertices[3] = radius*minR*sin((i/sections)*2*M_PI);
		vertices[4] = radius*minR*cos((i/sections)*2*M_PI);
		vertices[5] = 0;

		vertices[6] = radius*maxR*sin((i/sections)*2*M_PI);
		vertices[7] = radius*maxR*cos((i/sections)*2*M_PI);
		vertices[8] = 0;

		vertices[9] = radius*maxR*sin(((i-1)/sections)*2*M_PI);
		vertices[10] = radius*maxR*cos(((i-1)/sections)*2*M_PI);
		vertices[11] = 0;

		/* Draw the bottom face of the rings */
		glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indice);
	}

	/* Set the vertices and draw the top-side of the rings */
	for(i = 1; i <= sections; i++){
		vertices[0] = radius*maxR*sin((i/sections)*2*M_PI);
		vertices[1] = radius*maxR*cos((i/sections)*2*M_PI);
		vertices[2] = 0;

		vertices[3] = radius*minR*sin((i/sections)*2*M_PI);
		vertices[4] = radius*minR*cos((i/sections)*2*M_PI);
		vertices[5] = 0;

		vertices[6] = radius*minR*sin(((i-1)/sections)*2*M_PI);
		vertices[7] = radius*minR*cos(((i-1)/sections)*2*M_PI);
		vertices[8] = 0;

		vertices[9] = radius*maxR*sin(((i-1)/sections)*2*M_PI);
		vertices[10] = radius*maxR*cos(((i-1)/sections)*2*M_PI);
		vertices[11] = 0;

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, ringSize, 0, GL_RGBA, GL_FLOAT, textureRings);
		glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, indice);
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_2D);
}
