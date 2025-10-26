#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#define MAP_WIDTH   8
#define MAP_HEIGHT  8
#define TILE_SIZE   64

int map_data[MAP_WIDTH * MAP_HEIGHT];

//  DO NOT CHANGE IF YOU KNOW WHAT YOU'RE DOING:

//    - CORE RAYCASTING MATH (VERTICAL/HORIZONTAL CHECKS)
//    - MAP SIZE DEFINES (UNLESS YOU KNOW HOW TO FIX COLLISIONS)
//    - PLAYER STRUCT VARIABLE NAMES (USED THROUGHOUT CODE)
//
//  SAFE TO CHANGE:

//    - COLORS, TILE SIZE, OR PLAYER SPEED
//    - MAP FILE CONTENT (res/map.txt) CHANGE WHATEVER MAP YOU WANT
//    - WINDOW SIZE. THIS IS PARTIAL, BECAREFUL WITH TWEAKING THIS.


//--------------------------- MAP ---------------------------//
void map_draw()
{
    int x, y, tile_x, tile_y;

    for (y = 0; y < MAP_HEIGHT; y++) {
        for (x = 0; x < MAP_WIDTH; x++) {

            if (map_data[y * MAP_WIDTH + x] == 1)
                glColor3f(1, 1, 1);
            else
                glColor3f(0, 0, 0);

            tile_x = x * TILE_SIZE;
            tile_y = y * TILE_SIZE;

            glBegin(GL_QUADS);
            glVertex2i(tile_x + 1, tile_y + 1);
            glVertex2i(tile_x + 1, tile_y + TILE_SIZE - 1);
            glVertex2i(tile_x + TILE_SIZE - 1, tile_y + TILE_SIZE - 1);
            glVertex2i(tile_x + TILE_SIZE - 1, tile_y + 1);
            glEnd();
        }
    }
}

void map_load(const char* path)
{
    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Failed to open map file: %s\n", path);				// DEBUG ERROR IF YOUR TXT FILE WASN'T FOUND
        exit(1);
    }

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            int val;
            fscanf(file, "%d", &val);
            map_data[y * MAP_WIDTH + x] = val;
        }
    }

    fclose(file);
}

//--------------------------- MATH UTILS ---------------------------//
float deg_to_rad(int angle) { return angle * M_PI / 180.0; }

int fix_angle(int angle)
{
    if (angle > 359) angle -= 360;
    if (angle < 0)   angle += 360;
    return angle;
}

//--------------------------- PLAYER ---------------------------//
typedef struct {
    float posX, posY;   // POSITION
    float posA;         // ANGLE
    float posdX, posdY; // DIRECTION VECTORS
} Player;

Player p;

void player_draw()
{
    glColor3f(1, 1, 0);
    glPointSize(8);
    glLineWidth(4);

    glBegin(GL_POINTS);
    glVertex2i(p.posX, p.posY);
    glEnd();

    glBegin(GL_LINES);
    glVertex2i(p.posX, p.posY);
    glVertex2i(p.posX + p.posdX * 20, p.posY + p.posdY * 20);
    glEnd();
}

//--------------------------- INPUT ---------------------------//
void input_handle_key(unsigned char key, int x, int y)
{
    if (key == 'a') {
        p.posA += 5;
        p.posA = fix_angle(p.posA);
        p.posdX = cos(deg_to_rad(p.posA));
        p.posdY = -sin(deg_to_rad(p.posA));
    }

    if (key == 'd') {
        p.posA -= 5;
        p.posA = fix_angle(p.posA);
        p.posdX = cos(deg_to_rad(p.posA));
        p.posdY = -sin(deg_to_rad(p.posA));
    }

    if (key == 'w') {
        p.posX += p.posdX * 5;
        p.posY += p.posdY * 5;
    }

    if (key == 's') {
        p.posX -= p.posdX * 5;
        p.posY -= p.posdY * 5;
    }

    glutPostRedisplay();
}

//--------------------------- RAYCASTER ---------------------------//
void raycaster_draw()
{
    //---- SKY ----//
    glColor3f(0, 0, 0); glBegin(GL_QUADS); glVertex2i(526, 0); glVertex2i(1006, 0); 
    
    glVertex2i(1006, 160); glVertex2i(526, 160); glEnd();

    //---- FLOOR ----//
    glColor3f(0, 0, 0); glBegin(GL_QUADS); glVertex2i(526,160); glVertex2i(1006,160);
    
    glVertex2i(1006,320); glVertex2i(526,320); glEnd();

    int ray, mapX, mapY, mapPos, depthOfField, hitSide;
    
    float rayAngle, tanVal;
    float rayX, rayY, offsetX, offsetY;
    
    float vertX, vertY, vertDist;
    float horzX, horzY, horzDist;

    rayAngle = fix_angle(p.posA + 30); // START RAYCASTING 30° TO THE RIGHT OF PLAYER ANGLE

    for (ray = 0; ray < 60; ray++) {
    
        //---------------- VERTICAL ----------------//
        depthOfField = 0;
        vertDist = 100000;			  // MAX DISTANCE, BE CAREFUL CHANGING THIS.
        hitSide = 0;
        
        tanVal = tan(deg_to_rad(rayAngle));

		//------- CHECKS IF FACING RIGHT
        if (cos(deg_to_rad(rayAngle)) > 0.001) {rayX = (((int)p.posX >> 6) << 6) + 64;
        		rayY = (p.posX - rayX) * tanVal + p.posY; offsetX = 64; offsetY = -offsetX * tanVal;}
        		
        //------- CHECKS IF FACING LEFT
        else if (cos(deg_to_rad(rayAngle)) < -0.001) {rayX = (((int)p.posX >> 6) << 6) - 0.0001; 
        		rayY = (p.posX - rayX) * tanVal + p.posY; offsetX = -64; offsetY = -offsetX * tanVal;
        }

        // RAY FACING STRAIGHT UP OR DOWN
        else {
            rayX = p.posX; rayY = p.posY; depthOfField = 8;
        }

		// VERTICAL LOOP
        while (depthOfField < 8) {mapX = (int)(rayX) >> 6; mapY = (int)(rayY) >> 6; mapPos = mapY * MAP_WIDTH + mapX;

		// WALL HIT DETECTED
		// THIS CHECKS IF THE CURRENT TILE THE RAY IS IN CONTAINS A WALL (VALUE == 1)
		// IF TRUE, THE RAY HAS COLLIDED WITH A WALL BLOCK ON THE MAP
		// WE THEN STOP THE RAYCAST (depthOfField = 8) AND CALCULATE THE DISTANCE
		// DISTANCE IS COMPUTED USING A FORM OF DOT PRODUCT TO AVOID FISHEYE DISTORTION
		// vertDist / horzDist STORE HOW FAR THE WALL IS FROM THE PLAYER FOR THAT RAY
        if 	(mapPos > 0 && mapPos < MAP_WIDTH * MAP_HEIGHT && map_data[mapPos] == 1) {depthOfField = 8; 
           		vertDist = cos(deg_to_rad(rayAngle)) * (rayX - p.posX) - sin(deg_to_rad(rayAngle)) * (rayY - p.posY);
                
            } else {
                rayX += offsetX; rayY += offsetY; depthOfField++;
            }
        }

        vertX = rayX; vertY = rayY; // STOREs VERTICAL HIT POSITION

        //---------------- HORIZONTAL ----------------//
        depthOfField = 0;
        horzDist = 100000;
        tanVal = 1.0 / tanVal;

        if (sin(deg_to_rad(rayAngle)) > 0.001) {rayY = (((int)p.posY >> 6) << 6) - 0.0001; 
        	rayX = (p.posY - rayY) * tanVal + p.posX; offsetY = -64; offsetX = -offsetY * tanVal;
        }
        
        else if (sin(deg_to_rad(rayAngle)) < -0.001) {rayY = (((int)p.posY >> 6) << 6) + 64; rayX = (p.posY - rayY) * tanVal + p.posX;
            offsetY = 64; offsetX = -offsetY * tanVal;
        }
        
        else 
        {
            rayX = p.posX;rayY = p.posY;
            depthOfField = 8;
        }

        while (depthOfField < 8) {
            mapX = (int)(rayX) >> 6;
            mapY = (int)(rayY) >> 6;
            mapPos = mapY * MAP_WIDTH + mapX;

            if (mapPos > 0 && mapPos < MAP_WIDTH * MAP_HEIGHT && map_data[mapPos] == 1) {
                depthOfField = 8;
                horzDist = cos(deg_to_rad(rayAngle)) * (rayX - p.posX) - sin(deg_to_rad(rayAngle)) * (rayY - p.posY);
                
            } else {
                rayX += offsetX; rayY += offsetY;
                depthOfField++;
            }
        }

        glColor3f(0.0, 1.0, 1.0); // BASE COLOR
        if (vertDist < horzDist) {
            rayX = vertX;
            rayY = vertY;
            horzDist = vertDist;
            glColor3f(0.0, 0.7, 0.7); // SHADE COLOR
        }

        //--- DRAW RAYS ---//
        glLineWidth(2);								   // SETS THE THICKNESS OF RAYCAST
        glBegin(GL_LINES);							   // STARTS DRAWING
        glVertex2i(p.posX, p.posY);
        glVertex2i(rayX, rayY);
        glEnd();

        //-- FISH EYE EFFECT
        //-- TO DO: FIX IT
        int correctedAngle = fix_angle(p.posA - rayAngle);
        horzDist = horzDist * cos(deg_to_rad(correctedAngle));

        //-- CALCULATE WALL HEIGHT AND VERTICAL OFFSET
        int wallHeight = (TILE_SIZE * 320) / horzDist; // PROJECT WALL HEIGHT BASED ON DISTANCE (CLOSER = TALLER)
        if (wallHeight > 320) wallHeight = 320;		   // LIMIT MAX HEIGHT TO SCREEN SIZE
        int wallOffset = 160 - (wallHeight >> 1);	   // CENTER WALL VERTICALLY ON SCREEN

        // Draw 3D wall slice
        glLineWidth(8);								   // SET WALL COLUMN THICKNESS
        glBegin(GL_LINES);							   
        glVertex2i(ray * 8 + 530, wallOffset);		   // WALL TOP POSITION
        glVertex2i(ray * 8 + 530, wallOffset + wallHeight);
        glEnd();

        rayAngle = fix_angle(rayAngle - 1);
    }
}

//--------------------------- GAME ---------------------------//
void game_init()
{
    glClearColor(0.3, 0.3, 0.3, 0);				       // SET BACKGROUND COLOR (GRAY)
    gluOrtho2D(0, 1024, 510, 0);					   // SCREEN RESOLUTION AND COORDINATE SYSTEM
    map_load("res/map.txt");

    p.posX = 150;
    p.posY = 400;
    p.posA = 90;
    p.posdX = cos(deg_to_rad(p.posA));
    p.posdY = -sin(deg_to_rad(p.posA));
}

void game_render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    map_draw();
    player_draw();
    raycaster_draw();
    glutSwapBuffers();
}

//--------------------------- MAIN ---------------------------//
int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1024, 510);				       // SCREEN RESOLUTION (1024 x 510)
    glutCreateWindow("Wolfstein Raycaster");

    game_init();
    glutDisplayFunc(game_render);
    glutKeyboardFunc(input_handle_key);
    glutMainLoop();

    return 0;
}
