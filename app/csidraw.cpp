#ifdef WIN32
#define WINAPI __stdcall
#define DECLSPEC_IMPORT __declspec(dllimport)
#define WINGDIAPI DECLSPEC_IMPORT
#define APIENTRY WINAPI
#endif

#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <float.h>
#include <GL/gl.h>
#include <GL/freeglut.h>

#include "utl/UtlString.h"
#include "csidraw.h"
#include "distcalc.h"

using namespace std;

extern DistCalc *distObj;

#define ZSCALE 1.0

// false - draws distance field without sign (vertices closer to the surface have darker color than
// vertices away from the surface
// true - draws distance field with sign (vertices above the surface are drawn red and vertices
// below are drawn blue
static bool _fieldsign = true; 

// true - draws distance field 
// false - hides distance field
static bool _drawfield = false; 

// true - draws triangle mesh
// false - hides triangle mesh
static bool _drawmesh = true; 

// true - draws selected cells for mesh rebuild
// false - hides selected cells for mesh rebuild
static bool _drawcell = false; 

// true - draws voxels gradients 
// false - hides voxels gradients 
static bool _drawgrad = false;

// true - draws voxels which are closest to the surface borders 
// false - hides voxels which are closest to the surface borders
static bool _drawborder = false;

bool GetSignFlag() { return _fieldsign; }

bool GetFieldFlag() { return _drawfield; }

bool GetMeshFlag() { return _drawmesh; }

bool GetCellFlag() { return _drawcell; }

bool GetGradientFlag() { return _drawgrad; }

bool GetBorderFlag() { return _drawborder; }

void SetSignFlag(bool signflag) { _fieldsign = signflag; }

void SetFieldFlag(bool fieldflag) { _drawfield = fieldflag; }

void SetMeshFlag(bool meshflag) { _drawmesh = meshflag; }

void SetCellFlag(bool cellflag) { _drawcell = cellflag; }

void SetGradientFlag(bool gradflag) { _drawgrad = gradflag; }

void SetBorderFlag(bool borderflag) { _drawborder = borderflag; }

static bool ValidateVertex(CsiTSurfVertex *vtx1, CsiTSurfVertex *vtx2)
{
	// Get vertex triangles
	CsiTriangleItrList &triList1 = vtx1->triangleitrList();
	CsiTriangleItrList &triList2 = vtx2->triangleitrList();

	int cont = 0;

	for (CsiTriangleItrItr titritr1=triList1.begin(); titritr1!=triList1.end(); ++titritr1)
	{
		CsiTriangle *t1 = titritr1->self();
		for (CsiTriangleItrItr titritr2=triList2.begin(); titritr2!=triList2.end(); ++titritr2)
		{
			CsiTriangle *t2 = titritr2->self();
			if(t1 == t2)
			 cont++;
			if(cont > 1) return false;
		}
	}

	return true;
}

void CsiDraw(CsiTSurf *surf)
{
	// DRAW SURFACE
	if( _drawmesh == true )
	{
		CsiTriangleList& tlist = surf->trianglesList();
		CsiTSurfVertexArray& varray = surf->vertexArray();

		glBegin(GL_TRIANGLES);
		for (CsiTriangleItr t=tlist.begin(); t!=tlist.end(); ++t) {
			GeoPoint3D edge0 = *varray[t->v1] - *varray[t->v2];
			GeoPoint3D edge1 = *varray[t->v1] - *varray[t->v3];
			GeoPoint3D normal = cross(edge0, edge1);
			normal = normalize(normal);
			glNormal3d(normal.x, normal.y, normal.z);
			glVertex3d(varray[t->v1]->x, varray[t->v1]->y, varray[t->v1]->z*ZSCALE);
			glVertex3d(varray[t->v2]->x, varray[t->v2]->y, varray[t->v2]->z*ZSCALE);
			glVertex3d(varray[t->v3]->x, varray[t->v3]->y, varray[t->v3]->z*ZSCALE);
		}

		glEnd();
	}

	// DRAW BORDER TRIANGLES
	if(_drawborder == true)
	{
		CsiTriangleList& tlist = surf->trianglesList();
		CsiTSurfVertexArray& varray = surf->vertexArray();

		glDisable(GL_LIGHTING);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(10000,10);

		glBegin(GL_LINES);
		glColor3d(1,0,0);
		glLineWidth(4);
		for (CsiTriangleItr t=tlist.begin(); t!=tlist.end(); ++t) {
			bool vtx1 = DistCalc::isInBorder(surf, varray[t->v1]);
			bool vtx2 = DistCalc::isInBorder(surf, varray[t->v2]);
			bool vtx3 = DistCalc::isInBorder(surf, varray[t->v3]);
			int sum = vtx1 + vtx2 + vtx3; 
			if( sum <= 1 ) continue;
			if( sum < 3 )
			{
				if( vtx1 && vtx2 )
				{
					if( !ValidateVertex(varray[t->v1], varray[t->v2]) )
						continue;
				}
				if( vtx2 && vtx3 )
				{
					if( !ValidateVertex(varray[t->v2], varray[t->v3]) )
						continue;
				}
				if( vtx1 && vtx3 )
				{
					if( !ValidateVertex(varray[t->v1], varray[t->v3]) )
						continue;
				}
			}
			glVertex3d(varray[t->v1]->x, varray[t->v1]->y, varray[t->v1]->z*ZSCALE);
			glVertex3d(varray[t->v2]->x, varray[t->v2]->y, varray[t->v2]->z*ZSCALE);

			glVertex3d(varray[t->v3]->x, varray[t->v3]->y, varray[t->v3]->z*ZSCALE);
			glVertex3d(varray[t->v1]->x, varray[t->v1]->y, varray[t->v1]->z*ZSCALE);

			glVertex3d(varray[t->v2]->x, varray[t->v2]->y, varray[t->v2]->z*ZSCALE);
			glVertex3d(varray[t->v3]->x, varray[t->v3]->y, varray[t->v3]->z*ZSCALE);
		}

		glEnd();
		glLineWidth(1);
		glEnable(GL_LIGHTING);
		glDisable(GL_POLYGON_OFFSET_LINE);
	}
	// END DRAW SURFACE

	std::size_t found = surf->name().find("undefined");
	if( found != std::string::npos ) // Does not have distance field, quit
		return;

	if( distObj == NULL ) return;

	// loop variables
	int i;
	unsigned int j, k;

	if( distObj->d_nx < 1 || distObj->d_ny < 1 || distObj->d_nz < 1 )
		return;

	// DRAW DISTANCE FIELD
	std::vector<double> &voxels = distObj->GetVoxels();
	if( voxels.size() == 0 ) return;

	std::vector<GeoPoint3D> &gradients = distObj->GetGradients();
	std::vector< std::pair< GeoPoint3D, CsiTSurfVertex* > >& surfcells = distObj->GetSurfCells();
	double maxelm = *std::max_element(voxels.begin(), voxels.end());
	glDisable(GL_LIGHTING);
	//glPointSize(2.0);
	glBegin(GL_POINTS);
	for(i = 0; i <= distObj->d_nx; i++)
	{
		for(j = 0; j <= distObj->d_ny; j++)
		{
			for(k = 0; k <= distObj->d_nz; k++)
			{
				int idx = (distObj->d_nx+1)*(distObj->d_ny+1)*k + (distObj->d_nx+1)*j +i;
				int idxC = (distObj->d_nx)*(distObj->d_ny)*k + (distObj->d_nx)*j +i;
				// draw distance field
				if( _drawfield == true )
				{
					if( _fieldsign == true ) { // draw signed distance field
						if( voxels[idx] > 0.0 )
							glColor3d(1.0, 0.0, 0.0);
						else
							glColor3d(0.0, 0.0, 1.0);
					}
					else // draw unsigned distance field
						glColor3d(1.0, 2*fabs(voxels[idx]/maxelm), 1.0);

					GeoPoint3D point(distObj->d_min.x + i*distObj->d_size, distObj->d_min.y + j*distObj->d_size,
					distObj->d_min.z + k*distObj->d_size);
					glVertex3d(point.x, point.y, point.z);
				}

				// draw selected cells for mesh rebuild
				if (i == distObj->d_nx || j == distObj->d_ny || k == distObj->d_nz) continue;
				if( _drawcell == true && surfcells.size() > 0 )
				{
					glColor3d(0,1,1);
					glVertex3d(surfcells[idxC].first.x, surfcells[idxC].first.y, surfcells[idxC].first.z);
				}
			}
		}
	}
	glEnd();
	// END DRAW DISTANCE FIELD

	// DRAW VOXELS GRADIENTS
	if( _drawgrad == true && gradients.size() > 0 )
	{
		glBegin(GL_LINES);
		for(i = 0; i <= distObj->d_nx; i++)
		{
			for(j = 0; j <= distObj->d_ny; j++)
			{
				for(k = 0; k <= distObj->d_nz; k++)
				{
					int idx = (distObj->d_nx+1)*(distObj->d_ny+1)*k + (distObj->d_nx+1)*j +i;
					glColor3d(1.0, 0.647, 0.0);
					GeoPoint3D point(distObj->d_min.x + i*distObj->d_size, distObj->d_min.y + j*distObj->d_size,
					distObj->d_min.z + k*distObj->d_size);
					glVertex3d(point.x, point.y, point.z);
					glVertex3d(point.x+100*gradients[idx].x, point.y+100*gradients[idx].y, point.z+100*gradients[idx].z);
				}
			}
		}
		glEnd();
	}
	// END DRAW VOXELS GRADIENTS

	glEnable(GL_LIGHTING);
}

