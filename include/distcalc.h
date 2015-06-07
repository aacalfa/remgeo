#ifndef _distcalc_h_
#define _distcalc_h_

#include <vector>
#include <map>
#include "CsiTSurf.h"

class DistCalc
{
	std::vector<double> d_voxels; // grid points
	std::vector<bool> d_borders; // auxiliary vector to d_voxels, informs if the given voxel's closest point on the surface is on its border or not
	std::vector< std::pair< GeoPoint3D, CsiTSurfVertex* > > d_surfcells; // grid cells which are used to rebuild the original mesh
	std::vector<GeoPoint3D> d_gradients; // distance field gradient of each grid point 

	GeoPoint3D InterpolatePoint(int i, unsigned int j, unsigned int k);

	void CheckVertexPositions(CsiTSurf *surf);
	
	void RelaxSurfVertices();

public:
	std::string d_filename; // file containing surface
	CsiTSurf *d_surf; // triangle mesh
	double d_size;
	int d_nx;
	unsigned int d_ny, d_nz;
	GeoPoint3D d_min, d_max;

	///@name Construtores
	//@{
	/**
	* Construtor.
	*/
	DistCalc( CsiTSurf *surf, std::string filename )
	: d_voxels(), d_borders(), d_surfcells(), d_gradients(), d_surf(NULL)
	{
		d_surf = surf;
		d_filename = filename;
		d_size = d_surf->getResolutionEstimative();

		d_surf->boundingbox( &d_min, &d_max );
		//d_size = 80;
		d_size = 800;
		//d_size = 25;
		//d_size = 20;
		//d_size = 30;
		//d_size = 1.01;
		
		// Resize bounding box
		d_min.x -= d_size;
		d_min.y -= d_size;
		d_min.z -= d_size;

		d_max.x += d_size;
		d_max.y += d_size;
		d_max.z += d_size;
	
		d_nx = (int) ((d_max.x - d_min.x) / d_size + 1);
		d_ny = (unsigned int) ((d_max.y - d_min.y) / d_size + 1);
		d_nz = (unsigned int) ((d_max.z - d_min.z) / d_size + 1);
	}

	void Grid2Mesh( );
	
	double Point2MeshDistance(GeoPoint3D pt, double &s, double &t, bool *isBorder=NULL, CsiTriangle **clostri=NULL);
	
	double Point2TriangleDistance(GeoPoint3D p, CsiTriangle *tri, double &s, double &t, bool *isBorder=NULL );
	
	CsiTSurf* SurfaceNets();
	
	std::vector<double>& GetVoxels() { return d_voxels; }
	
	std::vector<bool>& GetBorders() { return d_borders; }

	std::vector<GeoPoint3D>& GetGradients() { return d_gradients; }
	
	std::vector< std::pair< GeoPoint3D, CsiTSurfVertex* > >& GetSurfCells() { return d_surfcells; }

	void AssignVtx2Cell();

	void CalculateGradients();

	static bool isInBorder(CsiTSurf *surf, CsiTSurfVertex *oriVtx);

	void MountBorderMap();
};
#endif // _distcalc_h_
