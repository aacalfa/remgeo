#include <cstdio>
#include <cmath>
#include "distcalc.h"
#include "testunit.h"

using namespace std;

#define NUM_ITR 1E5
#define TOL 1E-3

extern DistCalc *distObj;

// Static variables
static double errorCount = 0;

static void InsideRegTest(CsiTSurf *tsurf)
{
	double d;
	double s, t;
	CsiTriangle *tri = tsurf->trianglesList().begin().self();

	GeoPoint3D pt(2.5, 2, 1); 
	d = distObj->Point2TriangleDistance(pt, tri, s, t);
cout << d << endl;
	pt = GeoPoint3D(2.5, 2, -1); 
	d = distObj->Point2TriangleDistance(pt, tri, s, t);
cout << d << endl;
}

static void vertexTest(CsiTSurf *tsurf)
{
	//double s, t;
	double sqrt2 = sqrt(2);
	double sqrt5 = sqrt(5);
	double sqrt10 = sqrt(10);

	CsiTriangle *tri = tsurf->trianglesList().begin().self();
	CsiTSurfVertexArray &vtxArray = tsurf->vertexArray();

	GeoPoint3D v1 = *vtxArray[tri->v1]; // vertex 1 of triangle
	GeoPoint3D v2 = *vtxArray[tri->v2]; // vertex 2 of triangle
	GeoPoint3D v3 = *vtxArray[tri->v3]; // vertex 3 of triangle

	double d; // Distance calculated by algorithm, that will be verified
	double res; // Real Distance between the point and the triangle, using euclidian distance

	double s, t;

	// Test vertex 1 going along x coordinate
	for(double i = v1.x, cnt = 0; cnt < NUM_ITR; i--, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(i, v1.y, v1.z);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		if ( d != (i-v1.x)*(i-v1.x) )
		{
			cout << "Error: #0" << endl;
			cout << d << "\t" << i*i << endl;
			errorCount++;
		}
	}

	// Test vertex 1 going along x and y coordinates
	for(double i = v1.x, j = v1.y, cnt = 0; cnt < NUM_ITR; i--, j--, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(i, j, 0);
		d = distObj->Point2TriangleDistance(pt, tri , s, t);
		res = ((i-v1.x)*sqrt2)*((i-v1.x)*sqrt2);
		if ( abs(d - res) > TOL )
		{
			cout << "Error: #1" << endl;
			cout << d << "\t" << res << endl;
			errorCount++;
		}
	}

	// Test vertex 1 going positively along z coordinate
	for(double i = v1.z, cnt = 0; cnt < NUM_ITR; i++, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(v1.x ,v1.y, i);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		if ( d != (i-v1.z)*(i-v1.z) )
		{
			cout << "Error: #2a" << endl;
			cout << d << "\t" << i*i << endl;
			errorCount++;
		}
	}

	// Test vertex 1 going negatively along z coordinate
	for(double i = v1.z, cnt = 0; cnt < NUM_ITR; i--, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(v1.x, v1.y, i);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		if ( d != (i-v1.z)*(i-v1.z) )
		{
			cout << "Error: #2b" << endl;
			cout << d << "\t" << i*i << endl;
			errorCount++;
		}
	}

	// Additional test for region 4, going along the bisector of s and t 
	for(double i = v1.x, j = v1.y, cnt = 0; cnt < NUM_ITR; i--, j = (sqrt5-1.0)/2.0*(i-1)+1, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(i,j,0);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		res = (v1.x-i)*(v1.x-i) + (v1.y-j)*(v1.y-j);
		if ( abs(d - res) > TOL )
		{
			cout << "Error: #2c" << endl;
			cout << d << "\t" << res << endl;
			errorCount++;
		}
	}

	// Test vertex 2 going along x coordinate
	for(double i = v2.x, cnt = 0; cnt < NUM_ITR; i++, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(i, v2.y, v2.z);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		if ( d != (i-v2.x)*(i-v2.x) )
		{
			cout << "Error: #3" << endl;
			cout << d << "\t" << i*i << endl;
			errorCount++;
		}
	}

	// Test vertex 2 going along x and y coordinates
	for(double i = v2.x, j = v2.y, cnt = 0; cnt < NUM_ITR; i++, j--, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(i,j,0);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		res = ((i-v2.x)*sqrt2)*((i-v2.x)*sqrt2);
		if ( abs(d - res) > TOL )
		{
			cout << "Error: #4" << endl;
			cout << d << "\t" << res << endl;
			errorCount++;
		}
	}

// Additional test for region 6, going along the bisector of s and 1-s 
	for(double i = v2.x, j = v2.y, cnt = 0; cnt < NUM_ITR; i++, j = -0.5*i+3, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(i,j,0);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		res = (cnt*sqrt5/2)*(cnt*sqrt5/2);
		if ( abs(d - res) > TOL )
		{
			cout << "Error: #4a" << endl;
			cout << d << "\t" << res << endl;
			errorCount++;
		}
	}

	// Test vertex 2 going positively along z coordinate
	for(double i = v2.z, cnt = 0; cnt < NUM_ITR; i++, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(v2.x, v2.y, i);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		if ( d != (i-v2.z)*(i-v2.z) )
		{
			cout << "Error: #5a" << endl;
			cout << d << "\t" << i*i << endl;
			errorCount++;
		}
	}

	// Test vertex 2 going negatively along z coordinate
	for(double i = v2.z, cnt = 0; cnt < NUM_ITR; i--, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(v2.x, v2.y, i);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		if ( d != (i-v2.z)*(i-v2.z) )
		{
			cout << "Error: #5b" << endl;
			cout << d << "\t" << i*i << endl;
			errorCount++;
		}
	}

	// Test vertex 3 going along x and y coordinates on positive direction
	for(double i = v3.x, j = v3.y; i < NUM_ITR; i++, j++)
	{
		GeoPoint3D pt = GeoPoint3D(i,j,0);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		res = ((i-v3.x)*sqrt2)*((i-v3.x)*sqrt2);
		if ( abs(d - res) > TOL )
		{
			cout << "Error: #6" << endl;
			cout << d << "\t" << res << endl;
			errorCount++;
		}
	}

	// Test vertex 3 going along x and y coordinates on negative direction
	for(double i = 2, j = 3; j < NUM_ITR; i--, j++)
	{
		GeoPoint3D pt = GeoPoint3D(i,j,0);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		res = ((i-v3.x)*sqrt2)*((i-v3.x)*sqrt2);
		if ( abs(d - res) > TOL )
		{
			cout << "Error: #7" << endl;
			cout << d << "\t" << res << endl;
			errorCount++;
		}
	}

	// Test vertex 3 going positively along z coordinate
	for(double i = v3.z, cnt = 0; cnt < NUM_ITR; i++, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(v3.x, v3.y, i);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		if ( d != (i-v3.z)*(i-v3.z) )
		{
			cout << "Error: #8a" << endl;
			cout << d << "\t" << i*i << endl;
			errorCount++;
		}
	}

	// Test vertex 3 going negatively along z coordinate
	for(double i = v3.z, cnt = 0; cnt < NUM_ITR; i--, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(v3.x, v3.y, i);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		if ( d != (i-v3.z)*(i-v3.z) )
		{
			cout << "Error: #8a" << endl;
			cout << d << "\t" << i*i << endl;
			errorCount++;
		}
	}

	// Additional test for region 2 going along the bisector of 1-s and t 
	for(double i = v3.x, j = v3.y, cnt = 0; cnt < NUM_ITR; j++, i = 3*j-j*sqrt10-7+3*sqrt10, cnt++)
	{
		GeoPoint3D pt = GeoPoint3D(i,j,0);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		res = (v3.x-i)*(v3.x-i) + (v3.y-j)*(v3.y-j);
		if ( abs(d - res) > TOL )
		{
			cout << "Error: #8b" << endl;
			cout << d << "\t" << res << endl;
			errorCount++;
		}
	}
}

static void edgeTest(CsiTSurf *tsurf)
{
	//double s, t;
	double sqrt2 = sqrt(2);
	double sqrt5 = sqrt(5);

	CsiTriangle *tri = tsurf->trianglesList().begin().self();
	CsiTSurfVertexArray &vtxArray = tsurf->vertexArray();

	GeoPoint3D v1 = *vtxArray[tri->v1]; // vertex 1 of triangle
	GeoPoint3D v2 = *vtxArray[tri->v2]; // vertex 2 of triangle
	GeoPoint3D v3 = *vtxArray[tri->v3]; // vertex 3 of triangle

	double d; // Distance calculated by algorithm, that will be verified
	double res; // Real Distance between the point and the triangle, using euclidian distance
	double s, t;

	// Test a point going perpendicularly along 's' edge
	for(double j = (v1.y+v2.y)/2, cnt = 0; cnt < NUM_ITR; cnt++, j--)
	{
		GeoPoint3D pt = GeoPoint3D((v1.x+v2.x)/2, j, (v1.z+v2.z)/2);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		if ( d != (j-1)*(j-1) )
		{
			cout << "Error: #9" << endl;
			cout << d << "\t" << j*j << endl;
			errorCount++;
		}
	}

	// Test a point going perpendicularly along 't' edge
	for(double i = (v1.x+v3.x)/2, j = (v1.y+v3.y)/2, k = 0, cnt = 0; cnt < NUM_ITR; cnt++, i--,	j=j+0.5,k++)
	{
		GeoPoint3D pt = GeoPoint3D(i,j,0);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		res = ((k/2)*sqrt5)*((k/2)*sqrt5);
		if ( abs(d - res) > TOL )
		{
			cout << "Error: #10" << endl;
			cout << d << "\t" << res << endl;
			errorCount++;
		}
	}

	// Test a point going perpendicularly along '1-s' edge
	for(double i = (v2.x+v3.x)/2, j = (v2.y+v3.y)/2, cnt = 0; cnt < NUM_ITR; cnt++, i++, j++)
	{
		GeoPoint3D pt = GeoPoint3D(i,j,0);
		d = distObj->Point2TriangleDistance(pt, tri, s, t);
		res = ((j-2)*sqrt2)*((j-2)*sqrt2);
		if ( abs(d - res) > TOL )
		{
			cout << "Error: #11" << endl;
			cout << d << "\t" << res << endl;
			errorCount++;
		}
	}

	// Test the distance field sign
	GeoPoint3D pt = GeoPoint3D(2,2,2);
	d = distObj->Point2MeshDistance(pt, s, t);
	if ( d <= 0 )
	{
		cout << "Error: #12" << endl;
		cout << d << "\t" << res << endl;
		errorCount++;
	}
	pt = GeoPoint3D(2,2,-2);
	d = distObj->Point2MeshDistance(pt, s, t);
	if ( d >= 0 )
	{
		cout << "Error: #13" << endl;
		cout << d << "\t" << res << endl;
		errorCount++;
	}
}

//////////////////////
// GLOBAL FUNCTIONS
//////////////////////

#ifdef DBGTEST
extern bool reg0;
extern bool reg1;
extern bool reg2;
extern bool reg3;
extern bool reg4;
extern bool reg5;
extern bool reg6;
#endif

void testTriangle(std::string surfname)
{
	// Load Surface with only one triangle
	CsiSurfaceList surfacesList;
	CsiTSurf *tsurf = CsiTSurf::Gocadload(surfname);

	cout << "Checking distance from point to triangle calculation..." << endl;

	// Reg 0 test
	InsideRegTest(tsurf);

	//VERTEXES
	vertexTest(tsurf);

	//EDGES
	edgeTest(tsurf);

	if ( errorCount == 0 ) cout << "No errors found." << endl;

#ifdef DBGTEST
	if ( reg0 == false )cout << "reg0." << endl;
	if ( reg1 == false )cout << "reg1." << endl;
	if ( reg2 == false )cout << "reg2." << endl;
	if ( reg3 == false )cout << "reg3." << endl;
	if ( reg4 == false )cout << "reg4." << endl;
	if ( reg5 == false )cout << "reg5." << endl;
	if ( reg6 == false )cout << "reg6." << endl;
#endif
}

