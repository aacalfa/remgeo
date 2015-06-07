#ifdef _WIN32
#include <windows.h>
#endif
#include <functional>
#include <float.h>
#include "CsiPSet.h"
#include "visualize.h"
#include "manipulator.h"
#include "csidraw.h"
#include "distcalc.h"
#include "distio.h"
#include "utl/UtlString.h"

#define F_SIZE 200
extern CsiTSurf *tsurf;
extern CsiTSurf *newsurf;
extern DistCalc *distObj;

std::vector<Visualize*>_windows;

VManipulator* manip; 

static std::string _fileName;

void Visualize::initialize()
{
	GeoPoint3D min, max;
	d_surf->boundingbox(&min, &max);
	d_xcenter = (min.x+max.x) / 2;
	d_ycenter = (min.y+max.y) / 2;
	d_zcenter = (min.z+max.z) / 2;
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDisable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);
}

void display_cb(void)
{
	int winId = glutGetWindow();
	_windows[winId - 1]->display();
}


static void mouseWheel(int , int dir, int , int )
{
	int winId = glutGetWindow();
	Visualize *currWindow = _windows[winId - 1];
	if (dir > 0)
	{
		// Zoom in
		currWindow->d_angle--;
	}
	else
	{
		// Zoom out
		currWindow->d_angle++;
	}
	glutPostRedisplay();
}

static void SpecialInput(int key, int , int )
{
	int winId = glutGetWindow();
	Visualize *currWindow = _windows[winId - 1];
	switch(key)
	{
		case GLUT_KEY_LEFT:
			currWindow->d_xcenter -= 1;
		break;
		case GLUT_KEY_RIGHT:
			currWindow->d_xcenter += 1;
		break;
		case GLUT_KEY_UP:
			currWindow->d_ycenter -= 1;
		break;
		case GLUT_KEY_DOWN:
			currWindow->d_ycenter += 1;
		break;
	}
	glutPostRedisplay();
}

void mouse_cb(int , int ) 
{
	int winId = glutGetWindow();
	glutSetWindow(winId);
	display_cb();
}

static void keyboard( unsigned char key, int , int )
{
	int winId = glutGetWindow();
	Visualize *currWindow = _windows[winId - 1];

	bool signflag = GetSignFlag();
	bool fieldflag = GetFieldFlag();
	bool meshflag = GetMeshFlag();
	bool cellflag = GetCellFlag();
	bool gradflag = GetGradientFlag();
	bool borderflag = GetBorderFlag();
	switch ( key ) 
	{
		case '-': // Zoom out
			currWindow->d_angle++;
			break;      
		case '+': // Zoom in
			currWindow->d_angle--;
			break;
		case 'd': // Show/hide distance field
			if (fieldflag == true) 
				SetFieldFlag(false);
			else
				SetFieldFlag(true);
			break;
		case 's': // Toggle signed/unsigned distance field
			if (signflag == true) 
				SetSignFlag(false);
			else
				SetSignFlag(true);
			break;
		case 'm': // Show/hide Mesh
			if (meshflag == true) 
				SetMeshFlag(false);
			else
				SetMeshFlag(true);
			break;
		case 'c': // Show/hide cell centers
			if (cellflag == true) 
				SetCellFlag(false);
			else
				SetCellFlag(true);
			break;
		case 'g': // Show/hide gradients
			if (gradflag == true) 
				SetGradientFlag(false);
			else
				SetGradientFlag(true);
			break;
		case 'b': // Show/hide border flags
			if (borderflag == true) 
				SetBorderFlag(false);
			else
				SetBorderFlag(true);
			break;
		case 'w': // Display regenerated surface and save it into a file 
			if (newsurf != NULL) 
			{
				Visualize *window2 = new Visualize(newsurf);
				glutDisplayFunc(display_cb); // register Display Function
				glutKeyboardFunc(keyboard); // register Keyboard Handler
				glutMouseWheelFunc(mouseWheel);  // register scrollwheel handler
				glutSpecialFunc(SpecialInput); // register arrow keys
				glutMotionFunc(mouse_cb);

				// Save regenerated surface into a file
				std::string ext;
				DistIO::CutExt( _fileName, _fileName, ext );
				newsurf->name(_fileName + "_NET");
				newsurf->save(_fileName + "_NET" + ext);

				// Save Distance field into a file
				DistIO::GetInstance()->SaveDistField (distObj);
			}
			break;
		case 'f':
			// Save Distance field into a file
			DistIO::GetInstance()->SaveDistField (distObj);
			break;
		default:      
			break;
	}
	glutPostRedisplay();
}

void Visualize::display() 
{
	GeoPoint3D min, max;
	d_surf->boundingbox(&min, &max);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	int vp[4];
	glGetIntegerv(GL_VIEWPORT,vp);
	
	float zdist = 2*(max.z-min.z);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(d_angle, (float)vp[2]/vp[3], 100, 100000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	float pos[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	
	//VManipulator* manip = VManipulator::Instance();
	manip = VManipulator::Instance();
	manip->SetZCenter(10*zdist);
	manip->Load();

	gluLookAt(d_xcenter, d_ycenter, d_zcenter-10*zdist,
            d_xcenter, d_ycenter, d_zcenter,
            0.0f, 1.0f, 0.0f
           );

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	//float green[4] = {0.0f, 0.8f, 0.0f, 1.0f};

	CsiAttrib &tsattribs = d_surf->attrib();
	double red, green, blue;
	if (tsattribs.Find("RED"))
		UtlString::tonumber(tsattribs.Get("RED"), &red);
	else red = 0.0f;

	if (tsattribs.Find("GREEN"))
		UtlString::tonumber(tsattribs.Get("GREEN"), &green);
	else green = 0.8f;

	if (tsattribs.Find("BLUE"))
		UtlString::tonumber(tsattribs.Get("BLUE"), &blue);
	else blue = 0.0f;

	float colorvec[4];
	colorvec[0] = (float)red;
	colorvec[1] = (float)green;
	colorvec[2] = (float)blue;
	colorvec[3] = 1.0f;

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, colorvec);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 20.0f);
	CsiDraw(d_surf);

	glutSwapBuffers();
}

Visualize::Visualize(CsiTSurf *tsurf)
{
	// set window values
	d_width = 640;
	d_height = 480;

	// Load surface
	d_surf = tsurf;
	d_title = d_surf->name(); 

	glutInitWindowSize(d_width,d_height);				// set window size
	d_id = glutCreateWindow(d_title.c_str());		// create Window
	initialize();

	_windows.push_back(this);
}

void Visualize::InitVisu(int argc, char *argv[])
{

	_fileName = argv[1];

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	Visualize *window1 = new Visualize(tsurf);
	glutDisplayFunc(display_cb);						// register Display Function
	glutKeyboardFunc(keyboard);						// register Keyboard Handler
	glutMouseWheelFunc(mouseWheel);  // register scrollwheel handler
	glutSpecialFunc(SpecialInput); // register arrow keys
	glutMotionFunc(mouse_cb);

	glutMainLoop();
}

