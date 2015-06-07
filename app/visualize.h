#ifndef _visualize_h_
#define _visualize_h_

#include <GL/gl.h>
#include <GL/freeglut.h>

#include "CsiTSurf.h"

class Visualize
{
public:
	CsiTSurf *d_surf; // Surface to be displayed
	float d_angle;    // Angle of vision
	double d_xcenter;  // center vector x coordinate
	double d_ycenter;  // center vector y coordinate
	double d_zcenter;  // center vector z coordinate
  int d_width;      // Window width  
	int d_height;     // Window Height
	std::string d_title; // Window Title
	int d_id;
	GLfloat d_x;
	GLfloat d_y;


	Visualize(CsiTSurf *tsurf);


	void initialize();
	void display();

	static void InitVisu(int argc, char *argv[]);
};

#endif
