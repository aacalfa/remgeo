#include <iostream>
#include <cstdio>
#include <cmath>
#include <limits>
#include <iomanip>
#include <utility>
#include <omp.h>

#include "distcalc.h"

#define NUM_THREADS 8
#ifdef WIN32
	#define USE_OPENMP
#else
	#ifdef USE_PTHREADS 
		#include <pthread.h>
	#endif	
#endif

#ifdef DBGTEST
bool reg0 = false;
bool reg1 = false;
bool reg2 = false;
bool reg3 = false;
bool reg4 = false;
bool reg5 = false;
bool reg6 = false;
#endif

using namespace std;

#ifdef USE_PTHREADS
typedef struct thread_data {
	int thread_id;
	int num_threads;
	double d;
	unsigned int idx;
	unsigned int progress;
	DistCalc *obj;
} Thread_Data;
#endif
/**
 * --------------------------------------------------------------------
 * Private functions:
 *  ___     _          _       
 * | _ \_ _(_)_ ____ _| |_ ___ 
 * |  _/ '_| \ V / _` |  _/ -_)
 * |_| |_| |_|\_/\__,_|\__\___|
 * --------------------------------------------------------------------
 */

/**
* _Loadbar
* ------------------------------------------------------------------------
* Prints a progress bar Process has done i out of n rounds, and we want a bar of width w and resolution r.
* @param[in] pt   - current loop iteration 
* @param[in] n - total number of loop iterations 
* @param[in] w - width of loadbar
*/ 
static inline void _Loadbar(unsigned int i, unsigned int n, unsigned int w = 50)
{
	if ( n < 1000 ) return;
	if ( (i != n) && (i % (n/100) != 0) ) return;
	
	float ratio = i/(float)n;
	unsigned int c = ratio * w;
	
	cerr << setw(3) << (int)(ratio*100) << "% [";
	for (unsigned int x=0; x<c; x++) cerr << "=";
	for (unsigned int x=c; x<w; x++) cerr << " ";
	cerr << "]\r" << flush;
}

#ifdef USE_PTHREADS
/**
* _ThreadWork
* ------------------------------------------------------------------------
* Performs the Grid2Mesh function (distance field calculation), using pthreads
* @param[in] threadarg - pthread data structure 
*/ 
static void* _ThreadWork(void *threadarg)
{
	Thread_Data *my_data = (Thread_Data*) threadarg;
	int thread_id              = my_data->thread_id;
	double d                   = my_data->d;
	unsigned int idx           = my_data->idx;
	unsigned int progress      = my_data->progress;
	DistCalc *obj              = my_data->obj;

	std::vector<double> &voxels = obj->GetVoxels();
	double s, t;

	int n = obj->d_nx/NUM_THREADS; // number of elements to handle    
  
	// Locate the starting index
	int start = thread_id * n;
  
	// Locate the ending index
	int end;
	if ( thread_id != (NUM_THREADS-1) )
		end = start + n;
	else
		end = obj->d_nx;
	
	for(int i = start; i <= end; ++i)
	{
		for(unsigned int j = 0; j <= obj->d_ny; ++j)
		{
			for(unsigned int k = 0; k <= obj->d_nz; ++k)
			{
				GeoPoint3D point(obj->d_min.x + i*obj->d_size, obj->d_min.y + j*obj->d_size, obj->d_min.z + k*obj->d_size);
				d = obj->Point2MeshDistance(point, s, t);
				idx = (obj->d_nx+1)*(obj->d_ny+1)*k + (obj->d_nx+1)*j +i;
				voxels[idx] = d;
				
				// Update progress bar
				progress++;
				if( thread_id == 0 ) _Loadbar(progress, voxels.size());
			}
		}
	}
 
	pthread_exit(NULL);
}
#endif

/**
* _AddVertexIntoSurf
* ------------------------------------------------------------------------
* Adds a vertex into a Triangle mesh 
* @param[in] surf - triangle mesh 
* @param[in] pt - vertex to be added
*/ 
static CsiTSurfVertex* _AddVertexIntoSurf( CsiTSurf *surf, GeoPoint3D pt)
{
	return surf->addVertex(pt.x, pt.y, pt.z, 1);
}

/**
* CheckNeighborVtxTriangles
* ------------------------------------------------------------------------
* isInBorder auxiliary function 
*/ 
static bool CheckNeighborVtxTriangles(CsiTSurf *surf, CsiTSurfVertex *oriVtx, CsiTSurfVertex *currVtx)
{
	// Get all surface vertices
	CsiTSurfVertexArray &vtxArray = surf->vertexArray();

	// Get current vertex triangles
	CsiTriangleItrList &triList = currVtx->triangleitrList();

	int count = 0; // Count how many times does oriVtx occur in currVtx triangles

	for (CsiTriangleItrItr titritr=triList.begin(); titritr!=triList.end(); ++titritr)
	{
		CsiTriangle *t = titritr->self();

		if( vtxArray[t->v1] == oriVtx ) count++;
		else if( vtxArray[t->v2] == oriVtx ) count++;
		else if( vtxArray[t->v3] == oriVtx ) count++;
	}

	if(count == 1) return true;
	else return false;
}

/**
* CheckNeighborVertices
* ------------------------------------------------------------------------
* isInBorder auxiliary function 
*/ 
static bool CheckNeighborVertices(CsiTSurf *surf, CsiTriangle *t, CsiTSurfVertex *oriVtx)
{
	// Get all surface vertices
	CsiTSurfVertexArray &vtxArray = surf->vertexArray();

	CsiTSurfVertex *vtx1 = vtxArray[t->v1];
	CsiTSurfVertex *vtx2 = vtxArray[t->v2];
	CsiTSurfVertex *vtx3 = vtxArray[t->v3];

	if( vtx1 != oriVtx )
	{
		if( CheckNeighborVtxTriangles(surf, oriVtx, vtx1) == true ) return true;
	}

	if( vtx2 != oriVtx )
	{
		if( CheckNeighborVtxTriangles(surf, oriVtx, vtx2) == true ) return true;
	}

	if( vtx3 != oriVtx )
	{
		if( CheckNeighborVtxTriangles(surf, oriVtx, vtx3) == true ) return true;
	}
	return false;
}

/**
* isInBorder
* ------------------------------------------------------------------------
* Informs if given surface vertex is located on the surface border 
* @param[in] surf - triangle mesh 
* @param[in] oriVtx - vertex to be inquired 
*/ 
bool DistCalc::isInBorder(CsiTSurf *surf, CsiTSurfVertex *oriVtx)
{
	// Get original vertex triangles
	CsiTriangleItrList &triList = oriVtx->triangleitrList();

	// Vertex belongs to only one triangle, thus it is on the border
	if(triList.size() == 1) return true;

	for (CsiTriangleItrItr titritr=triList.begin(); titritr!=triList.end(); ++titritr)
	{
		CsiTriangle *t = titritr->self();

		if( CheckNeighborVertices(surf, t, oriVtx) == true ) return true;
	}

	return false;
}

/**
* MountBorderMap
* ------------------------------------------------------------------------
* Identifies every border vertex of a surface 
*/ 
void DistCalc::MountBorderMap(void)
{
	CsiTSurfVertexArray& varray = d_surf->vertexArray();
	bool isBorder;

	cerr << "Mapping Surface Boundaries..." << endl;
	for (int i = 0; i < varray.size(); ++i) {
		isBorder = isInBorder(d_surf, varray[i]);
		varray[i]->setProp(0, (double)isBorder);
	}

	cerr << "Done." << endl;
}

/**
* CheckVertexPositions
* ------------------------------------------------------------------------
* Check if surface vertex coordinates are located in cell centers 
* @param[in] surf - triangle mesh 
*/ 
void DistCalc::CheckVertexPositions(CsiTSurf *surf)
{
	int i;
	CsiTSurfVertexArray &vtxArray = surf->vertexArray();
	CsiTSurfVertex *currPoint;
	int numVtx = vtxArray.size();
	
#ifdef USE_OPENMP
#pragma omp parallel for num_threads(NUM_THREADS) private (i,currPoint)
#endif
	for(i = 1; i < numVtx; i++)
	{
		bool found = false;
		int count = 0;
		currPoint = vtxArray[i];
		
		for( auto &pairPoint : d_surfcells )
		{
			if (pairPoint.first.x == currPoint->x && pairPoint.first.y == currPoint->y && pairPoint.first.z == currPoint->z ) {
				found = true;
				count++;
			}
		}
		
		if (found == false || count != 1) {
			std::cerr << "Ponto posicionado incorretamente!" << std::endl;
		}
	}
}

/**
* InterpolatePoint
* ------------------------------------------------------------------------
* Calculates the new coordinate of a given point of the regenerated surface,
* performing the attraction towards the original surface.
*/ 
GeoPoint3D DistCalc::InterpolatePoint(int i, unsigned int j, unsigned int k)
{
	unsigned int idxFrontDownLeft = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*j +i;
	unsigned int idxFrontDownRight = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*j +(i+1);
	unsigned int idxBackDownLeft = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*(j+1) +i;
	unsigned int idxFrontUpLeft = (d_nx+1)*(d_ny+1)*(k+1) + (d_nx+1)*j +i;
	unsigned int idxBackDownRight = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*(j+1) +(i+1);
	unsigned int idxFrontUpRight = (d_nx+1)*(d_ny+1)*(k+1) + (d_nx+1)*j +(i+1);
	unsigned int idxBackUpLeft = (d_nx+1)*(d_ny+1)*(k+1) + (d_nx+1)*(j+1) +i;
	unsigned int idxBackUpRight = (d_nx+1)*(d_ny+1)*(k+1) + (d_nx+1)*(j+1) +(i+1);

	std::vector<GeoPoint3D> points; 

	unsigned int idxC = (d_nx)*(d_ny)*k + (d_nx)*j +i;
	GeoPoint3D cellcenter = d_surfcells[idxC].first;

	// Get voxel coordinates 
	GeoPoint3D FrontDownLeft(cellcenter.x-d_size/2, cellcenter.y-d_size/2, cellcenter.z-d_size/2);
	GeoPoint3D FrontDownRight(cellcenter.x+d_size/2, cellcenter.y-d_size/2, cellcenter.z-d_size/2);
	GeoPoint3D BackDownLeft(cellcenter.x-d_size/2, cellcenter.y+d_size/2, cellcenter.z-d_size/2);
	GeoPoint3D BackDownRight(cellcenter.x+d_size/2, cellcenter.y+d_size/2, cellcenter.z-d_size/2);
	GeoPoint3D FrontUpLeft(cellcenter.x-d_size/2, cellcenter.y-d_size/2, cellcenter.z+d_size/2);
	GeoPoint3D FrontUpRight(cellcenter.x+d_size/2, cellcenter.y-d_size/2, cellcenter.z+d_size/2);
	GeoPoint3D BackUpLeft(cellcenter.x-d_size/2, cellcenter.y+d_size/2, cellcenter.z+d_size/2);
	GeoPoint3D BackUpRight(cellcenter.x+d_size/2, cellcenter.y+d_size/2, cellcenter.z+d_size/2);

	if(d_borders[idxFrontDownLeft] == true)
	{
		//attract grid point of cell  v -= dist * n
		FrontDownLeft  -= std::abs(d_voxels[idxFrontDownLeft]) * d_gradients[idxFrontDownLeft];
		points.push_back(FrontDownLeft);
	}

	if(d_borders[idxFrontDownRight] == true) 
	{
		//attract grid point of cell  v -= dist * n
		FrontDownRight -= std::abs(d_voxels[idxFrontDownRight]) * d_gradients[idxFrontDownRight];
		points.push_back(FrontDownRight);
	}

	if(d_borders[idxBackDownLeft] == true)
	{
		//attract grid point of cell  v -= dist * n
		BackDownLeft   -= std::abs(d_voxels[idxBackDownLeft]) * d_gradients[idxBackDownLeft];
		points.push_back(BackDownLeft);
	}

	if(d_borders[idxBackDownRight] == true)
	{
		//attract grid point of cell  v -= dist * n
		BackDownRight  -= std::abs(d_voxels[idxBackDownRight]) * d_gradients[idxBackDownRight];
		points.push_back(BackDownRight);
	}

	if(d_borders[idxFrontUpLeft] == true)
	{
		//attract grid point of cell  v -= dist * n
		FrontUpLeft    -= std::abs(d_voxels[idxFrontUpLeft]) * d_gradients[idxFrontUpLeft];
		points.push_back(FrontUpLeft);
	}

	if(d_borders[idxFrontUpRight] == true)
	{
		//attract grid point of cell  v -= dist * n
		FrontUpRight   -= std::abs(d_voxels[idxFrontUpRight]) * d_gradients[idxFrontUpRight];
		points.push_back(FrontUpRight);
	}

	if(d_borders[idxBackUpLeft] == true)
	{
		//attract grid point of cell  v -= dist * n
		BackUpLeft     -= std::abs(d_voxels[idxBackUpLeft]) * d_gradients[idxBackUpLeft];
		points.push_back(BackUpLeft);
	}

	if(d_borders[idxBackUpRight] == true)
	{
		//attract grid point of cell  v -= dist * n
		BackUpRight    -= std::abs(d_voxels[idxBackUpRight]) * d_gradients[idxBackUpRight];
		points.push_back(BackUpRight);
	}

	GeoPoint3D interp(0,0,0);

	int ptsize = points.size();
	if(ptsize == 0)
	{
		d_surfcells[idxC].second->setProp(0, 5);
		FrontDownLeft  -= std::abs(d_voxels[idxFrontDownLeft]) * d_gradients[idxFrontDownLeft];
		FrontDownRight -= std::abs(d_voxels[idxFrontDownRight]) * d_gradients[idxFrontDownRight];
		BackDownLeft   -= std::abs(d_voxels[idxBackDownLeft]) * d_gradients[idxBackDownLeft];
		BackDownRight  -= std::abs(d_voxels[idxBackDownRight]) * d_gradients[idxBackDownRight];
		FrontUpLeft    -= std::abs(d_voxels[idxFrontUpLeft]) * d_gradients[idxFrontUpLeft];
		FrontUpRight   -= std::abs(d_voxels[idxFrontUpRight]) * d_gradients[idxFrontUpRight];
		BackUpLeft     -= std::abs(d_voxels[idxBackUpLeft]) * d_gradients[idxBackUpLeft];
		BackUpRight    -= std::abs(d_voxels[idxBackUpRight]) * d_gradients[idxBackUpRight];

		points.push_back(FrontDownLeft);
		points.push_back(FrontDownRight);
		points.push_back(BackDownLeft);
		points.push_back(BackDownRight);
		points.push_back(FrontUpLeft);
		points.push_back(FrontUpRight);
		points.push_back(BackUpLeft);
		points.push_back(BackUpRight);
	}
	//////
	else
	{
		d_surfcells[idxC].second->setProp(0, 99);
	}

	// refresh value
	ptsize = points.size();

	for(int i = 0; i < ptsize; i++)
		interp += points[i];

	interp.x /= points.size();
	interp.y /= points.size();
	interp.z /= points.size();

	return interp;
}

/**
* RelaxSurfVertices
* ------------------------------------------------------------------------
* Atracts all the vertices of the regenerated surface to their correct positions according to the
* distance field
*/ 
void DistCalc::RelaxSurfVertices()
{
	// loop variables
	unsigned int j, k;
	unsigned int idx;
	int i;
	GeoPoint3D defaultpoint;
	GeoPoint3D interpPt;

	for(i = 0; i < d_nx; ++i)
	{
		for(j = 0; j < d_ny; ++j)
		{
			for(k = 0; k < d_nz; ++k)
			{
				idx = (d_nx)*(d_ny)*k + (d_nx)*j +i;

				// Not a vertex from the surface
				if(d_surfcells[idx].second == NULL) continue;

				interpPt = InterpolatePoint(i, j, k);
				d_surfcells[idx].second->x = interpPt.x; 
				d_surfcells[idx].second->y = interpPt.y;
				d_surfcells[idx].second->z = interpPt.z;
			}
		}
	}
}

/**
 * --------------------------------------------------------------------
 * Public functions:
 *  ___      _    _ _    
 * | _ \_  _| |__| (_)__ 
 * |  _/ || | '_ \ | / _|
 * |_|  \_,_|_.__/_|_\__|
 * --------------------------------------------------------------------
 */

/**
* Grid2Mesh
* ------------------------------------------------------------------------
* Calculates the distance field given the object's surface attribute 
*/ 
#ifndef USE_PTHREADS 
void DistCalc::Grid2Mesh()
{
	// loop variables
	int i;
	unsigned int j, k;

	if( d_nx < 1 || d_ny < 1 || d_nz < 1 )
		return ;

	d_voxels.resize((d_nx+1)*(d_ny+1)*(d_nz+1));
	d_borders.resize((d_nx+1)*(d_ny+1)*(d_nz+1));
	d_gradients.resize((d_nx+1)*(d_ny+1)*(d_nz+1));

	// Variable to used for printing progress bar
	cerr << "Loading Surface " << d_surf->name() << endl;
	cerr << "Number of triangles: "<< d_surf->trianglesList().size() << endl;
	cerr << "Number of vertices: "<< d_surf->vertexArray().size() << endl;
	cerr << "Step Size: " << d_size << endl;
	unsigned int progress = 0;

	double d; // distance from voxel to the surface
	unsigned int idx; // vector index

	double s, t;
	CsiTriangle *clostri;
	CsiTSurfVertexArray &vtxArray = d_surf->vertexArray();
	GeoPoint3D point;
	GeoPoint3D edge0;
	GeoPoint3D edge1;
	GeoPoint3D triangpoint;

	bool *isBorder = new bool; // Flag that indicates whether the closest point in the surface from the voxel is located on the surface border 
	*isBorder = false; 

#ifdef USE_OPENMP
	cerr << "Using OpenMP" << endl;
#endif

	// start timing measure
	double ctimeBegin = omp_get_wtime();

#ifdef USE_OPENMP
	#pragma omp parallel for num_threads(NUM_THREADS) private (d,idx,i,j,k,clostri,s,t,point,triangpoint,edge0,edge1)
#endif
	for(i = 0; i <= d_nx; ++i)
	{
		for(j = 0; j <= d_ny; ++j)
		{
			for(k = 0; k <= d_nz; ++k)
			{
			 	// Set voxel coordinate
				point = GeoPoint3D(d_min.x + i*d_size, d_min.y + j*d_size, d_min.z + k*d_size);

				// Calculate distance from voxel to surface
				clostri = NULL;
				d = Point2MeshDistance(point, s, t, isBorder, &clostri );

				// Store value in d_voxels
				idx = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*j +i;
				d_voxels[idx] = d;

				// Store field distance gradient
				// The triangle function is T(s; t) = B + sE0 + tE1
				edge0 = GeoPoint3D(*vtxArray[clostri->v2] - *vtxArray[clostri->v1]);
				edge1 = GeoPoint3D(*vtxArray[clostri->v3] - *vtxArray[clostri->v1]);
				triangpoint = GeoPoint3D(*vtxArray[clostri->v1] + s*edge0 + t*edge1);
				d_gradients[idx] = point - triangpoint;
				d_gradients[idx] = normalize(d_gradients[idx]); 

				// Store flag that indicates if closest point in the surface is on the border
				d_borders[idx] = *isBorder;

				// Update progress bar
				progress++;
				_Loadbar(progress, d_voxels.size());
			}
		}
	}
	
	// end timing measure
	double ctimeEnd = omp_get_wtime();
	cerr << endl << "Distance Field calculation time: "<< ctimeEnd - ctimeBegin << endl;
}
#else // PTHREADS
void DistCalc::Grid2Mesh()
{
	// loop variables
	int i;

	if( d_nx < 1 || d_ny < 1 || d_nz < 1 )
		return ;

	d_voxels.resize((d_nx+1)*(d_ny+1)*(d_nz+1));

	cerr << "Loading Surface " << d_surf->name() << endl;
	cerr << "Number of triangles: "<< d_surf->trianglesList().size() << endl;
	cerr << "Number of vertices: "<< d_surf->vertexArray().size() << endl;
	cerr << "Step Size: " << d_size << endl;
	unsigned int progress = 0;

	double d = 0.0;
	unsigned int idx = 0;
	
	pthread_t threads[NUM_THREADS];
	Thread_Data thread_data_array[NUM_THREADS];
	void *exit_status;
	cerr << "Using Pthreads: " << endl;

	// Measure Time
	double ctimeBegin = omp_get_wtime();

	// start timing measure
	for (i = 0 ; i < NUM_THREADS; i++)
	{
		thread_data_array[i].thread_id = i;
		thread_data_array[i].d = d;
		thread_data_array[i].idx = idx;
		thread_data_array[i].progress = progress;
		thread_data_array[i].obj = this;
		
		pthread_create(&threads[i], NULL, _ThreadWork, (void*) &thread_data_array[i]);
  }
 
  for(i = 0 ; i < NUM_THREADS; i++)
      pthread_join(threads[i], &exit_status);

	// end timing measure
	double ctimeEnd = omp_get_wtime();
	cerr << endl << "Distance Field calculation time: "<< ctimeEnd - ctimeBegin << endl;
}
#endif

/**
* Point2MeshDistance
* ------------------------------------------------------------------------
* Given a point pt, calculates the shortest distance between pt and the object's surface attribute 
* @param[in] pt   - point pt 
* @param[out] s  - s coordinate in T of the closest point from pt
* @param[out] t  - t coordinate in T of the closest point from pt
* @param[out] tri - closest triangle between pt and the surface
*/ 
double DistCalc::Point2MeshDistance(GeoPoint3D pt, double &s, double &t, bool *isBorder, CsiTriangle **clostri)
{
	CsiTriangleList &triangles = d_surf->trianglesList();
	double minDistance = std::numeric_limits<double>::max();

	CsiTSurfVertexArray &vtxArray = d_surf->vertexArray();

	double s0 = s, t0 = t;
	bool *regtemp	= new bool;
	*regtemp	= false; 
	for(CsiTriangleItr itr = triangles.begin(); itr != triangles.end(); ++itr) 
	{
		double curDistance = Point2TriangleDistance(pt, itr.self(), s0, t0, regtemp);
		
		// Update min values
		if( curDistance < minDistance )
		{
			s = s0; t = t0; *isBorder = *regtemp;
			minDistance = curDistance;
			*clostri = itr.self();
		}
	}

	// Account for distance field sign:
	// if this point is "above" or below" the triangle, comparing the orientation
	// between the point and the triangle

	// Find the triangle normal vector
	GeoPoint3D edge0 = *vtxArray[(*clostri)->v1] - *vtxArray[(*clostri)->v2];
	GeoPoint3D edge1 = *vtxArray[(*clostri)->v1] - *vtxArray[(*clostri)->v3];
	GeoPoint3D normal = cross(edge0, edge1);
	GeoPoint3D vecpt(pt - *vtxArray[(*clostri)->v1]);

	// inner product between point and triangle normal to check if the
	// point is above or below the triangle
	double orientation = inner(vecpt, normal);
	if (orientation < 0) // point is below the triangle, put negative sign
		minDistance *= -1;
	return minDistance;
	
}

/**
* Point2TriangleDistance
* ------------------------------------------------------------------------
* Given a point pt and a triangle tri, this function returns the shortest distance between them 
* The triangle function is T(s; t) = B + sE0 + tE1
* @param[in] pt   - point pt 
* @param[in] tri - triangle tri
* @param[out] s  - s coordinate in T of the closest point from pt
* @param[out] t  - t coordinate in T of the closest point from pt
*/ 
double DistCalc::Point2TriangleDistance(GeoPoint3D pt, CsiTriangle *tri, double &s, double &t, bool *isBorder)
{
	CsiTSurfVertexArray &vtxArray = d_surf->vertexArray();

	GeoPoint3D diff = *vtxArray[tri->v1] - pt; // B - P
	GeoPoint3D edge0 = *vtxArray[tri->v2] - *vtxArray[tri->v1];
	GeoPoint3D edge1 = *vtxArray[tri->v3] - *vtxArray[tri->v1];

	// Squared-distance function for pt to tri
	// Q(s; t) = as^2 + 2bst + ct^2 + 2ds + 2et + f;
	double a = inner(edge0, edge0); // E0 . E0 = a
	double b = inner(edge0, edge1); // E0 . E1 = b
	double c = inner(edge1, edge1); // E1 . E1 = c
	double d  = inner(edge0, diff); // E0 . (B - P) = d
	double e  = inner(edge1, diff); // E1 . (B - P) = e
	double f   = inner(diff, diff); // (B - P) . (B - P) = f
	double delta = fabs(a*c - b*b);
	double sqrDistance;
	s = b*e - c*d;
	t = b*d - a*e;
	if(isBorder != NULL) *isBorder = false;

	bool vtx1Border = (bool) vtxArray[tri->v1]->getProp(0);
	bool vtx2Border = (bool) vtxArray[tri->v2]->getProp(0);
	bool vtx3Border = (bool) vtxArray[tri->v3]->getProp(0);

	if (s + t <= delta)
	{
		if (s < 0)
		{
				if (t < 0) // region 4
				{
					// Grad(Q) = 2(as+bt+d,bs+ct+e)
					// (1,0)*Grad(Q(0,0)) = (1,0)*(d,e) = d
					// (0,1)*Grad(Q(0,0)) = (0,1)*(b+d,c+e) = e
#ifdef DBGTEST
	reg4 = true;
#endif

					if (d < 0)
					{
						t = 0;
						if (-d >= a)
						{
							s = 1;
							sqrDistance = a + 2*d + f;
							// Check if it is on surface border
							if(isBorder != NULL)
								if( vtx2Border == true) *isBorder = true;
						}
						else
						{
							s = -d/a;
							sqrDistance = d*s + f;
							// Check if it is on surface border
							if(isBorder != NULL)
								if(vtx1Border == true && vtx2Border == true) *isBorder = true;
						}
					}
					else
					{
						s = 0;
						if (e >= 0)
						{
							t = 0;
							sqrDistance = f;
							// Check if it is on surface border
							if(isBorder != NULL)
								if(vtx1Border == true) *isBorder = true;
						}
						else if (-e >= c)
						{
							t = 1;
							sqrDistance = c + 2*e + f;
							// Check if it is on surface border
							if(isBorder != NULL)
								if(vtx3Border == true) *isBorder = true;
						}
						else
						{
							t = -e/c;
							sqrDistance = e*t + f;
							// Check if it is on surface border
							if(isBorder != NULL)
								if(vtx1Border == true && vtx3Border == true) *isBorder = true;
						}
					}
				}
				else // region 3 (t edge)
				// F(t) = Q(0,t) = ct^2 + 2et + f
				// F'(t)/2 = ct+e
				// F'(T) = 0 when T = -e/c
				{
#ifdef DBGTEST
	reg3 = true;
#endif

					s = 0;
					if (e >= 0) // T < 0, minimum at 0, closest point is the lower left vertex
					{
						t = 0;
						sqrDistance = f;
						// Check if it is on surface border
						if(isBorder != NULL)
							if(vtx1Border == true) *isBorder = true;
					}
					else if (-e >= c) // (num >= denom): T > 0, minimum at 1, closest point is the upper left vertex
					{
						t = 1;
						sqrDistance = c + 2*e + f;
						// Check if it is on surface border
						if(isBorder != NULL)
							if(vtx3Border == true) *isBorder = true;
					}
					else // closest point is on t edge
					{
						t = -e/c;
						sqrDistance = e*t + f;
						// Check if it is on surface border
						if(isBorder != NULL)
							if(vtx1Border == true && vtx3Border == true) *isBorder = true;
					}
				}
			}
			else if (t < 0) // region 5 (s edge)
			{
				// F(s) = Q(s,0) = as^2 + 2ds + f
				// F'(s)/2 = as+d
				// F'(S) = 0 when S = -d/a
#ifdef DBGTEST
	reg5 = true;
#endif

				t = 0;
				if (d >= 0) // S < 0, minimum at 0, closest point is the lower left vertex
				{
					s = 0;
					sqrDistance = f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx1Border == true) *isBorder = true;
				}
				else if (-d >= a) // (num >= denom): S > 0, minimum at 1, closest point is the lower right vertex
				{
					s = 1;
					sqrDistance = a + 2*d + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx2Border == true) *isBorder = true;
				}
				else
				{
					s = -d/a;
					sqrDistance = d*s + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx1Border == true && vtx2Border == true) *isBorder = true;
				}
			}
			else // region 0
			{
				// minimum at interior point
				double invDet = 1/delta;
#ifdef DBGTEST
	reg0 = true;
#endif

				s *= invDet;
				t *= invDet;
				sqrDistance = s*(a*s + b*t + 2*d) +
				              t*(b*s + c*t + 2*e) + f;
			}
	}
	else
	{
		double tmp0, tmp1, numer, denom;

		if (s < 0) // region 2
		// Grad(Q) = 2(as+bt+d,bs+ct+e)
		// (0,-1)*Grad(Q(0,1)) = (0,-1)*(b+d,c+e) = -(c+e)
		// (1,-1)*Grad(Q(0,1)) = (1,-1)*(b+d,c+e) = (b+d)-(c+e)
		// min on edge s+t=1 if (1,-1)*Grad(Q(0,1)) < 0 )
		// min on edge s=0 otherwise
		{
#ifdef DBGTEST
	reg2 = true;
#endif
			tmp0 = b + d;
			tmp1 = c + e;
			if (tmp1 > tmp0) // minimum on edge s+t=1
			{
				numer = tmp1 - tmp0;
				denom = a - 2*b + c;
				if (numer >= denom)
				{
					s = 1;
					t = 0;
					sqrDistance = a + 2*d + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx2Border == true) *isBorder = true;
				}
				else
				{
					s = numer/denom;
					t = 1 - s;
					sqrDistance = s*(a*s + b*t + 2*d) +
					              t*(b*s + c*t + 2*e) + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx2Border == true && vtx3Border == true) *isBorder = true;
				}
			}
			else // minimum on edge s=0
			{
				s = 0;
				if (tmp1 <= 0)
				{
					t = 1;
					sqrDistance = c + 2*e + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx3Border == true) *isBorder = true;
				}
				else if (e >= 0)
				{
					t = 0;
					sqrDistance = f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx1Border == true) *isBorder = true;
				}
				else
				{
					t = -e/c;
					sqrDistance = e*t + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx1Border == true && vtx3Border == true) *isBorder = true;
				}
			}
		}
		else if (t < 0) // region 6
		{
#ifdef DBGTEST
	reg6 = true;
#endif

			tmp0 = b + e;
			tmp1 = a + d;
			if (tmp1 > tmp0) // minimum on edge s+t=1
			{
				numer = tmp1 - tmp0;
				denom = a - 2*b + c;
				if (numer >= denom)
				{
					t = 1;
					s = 0;
					sqrDistance = c + 2*e + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx3Border == true) *isBorder = true;
				}
				else
				{
					t = numer/denom;
					s = 1 - t;
					sqrDistance = s*(a*s + b*t + 2*d) +
					              t*(b*s + c*t + 2*e) + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx2Border == true && vtx3Border == true) *isBorder = true;
				}
			}
			else //minimum on edge t=0
			{
				t = 0;
				if (tmp1 <= 0)
				{
					s = 1;
					sqrDistance = a + 2*d + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx2Border == true) *isBorder = true;
				}
				else if (d >= 0)
				{
					s = 0;
					sqrDistance = f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx1Border == true) *isBorder = true;
				}
				else
				{
					s = -d/a;
					sqrDistance = d*s + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx1Border == true && vtx2Border == true) *isBorder = true;
				}
			}
		}
		else	// region 1
		// F(s) = Q(s,1-s) = (a-2b+c)s^2 + 2(b-c+d-e)s + (c+2e+f)
		// F'(s)/2 = (a-2b+c)s + (b-c+d-e)
		// F'(S) = 0 when S = (c+e-b-d)/(a-2b+c)
		// a-2b+c = |E0-E1|^2 > 0, so only sign of c+e-b-d need be considered
		{
#ifdef DBGTEST
	reg1 = true;
#endif

			numer = c + e - b - d; // c+e-b-d
			if (numer <= 0) // closest point is the upper left vertex (s = 0, t = 1)
			{
				s = 0;
				t = 1;
				sqrDistance = c + 2*e + f;
				// Check if it is on surface border
				if(isBorder != NULL)
					if(vtx3Border == true) *isBorder = true;
			}
			else
			{
				denom = a - 2*b + c; // positive quantity
				if (numer >= denom) // closest point is the lower right vertex (s = 1, t = 0)
				{
					s = 1;
					t = 0;
					sqrDistance = a + 2*d + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx2Border == true) *isBorder = true;
				}
				else
				{ // closest point is on the 1-s edge
					s = numer/denom;
					t = 1 - s;
					sqrDistance = s*(a*s + b*t + 2*d) +
					t*(b*s + c*t + 2*e) + f;
					// Check if it is on surface border
					if(isBorder != NULL)
						if(vtx2Border == true && vtx3Border == true) *isBorder = true;
				}
			}
		}
	}

	// Account for numerical round-off error.
	if (sqrDistance < 0)
	{
		sqrDistance = 0;
	}

	// return the calculate distance
#ifndef DBGTEST
	double distance = sqrt(sqrDistance);
	return distance;
#endif
	return sqrDistance;
}

/**
* AssignVtx2Cell
* ------------------------------------------------------------------------
* Sets the object's surcells vector, which contains every point pertaining to the
* grid cell's center
*/ 
void DistCalc::AssignVtx2Cell()
{
	if( d_voxels.size() == 0 ) return;

	// loop variables
	int i;
	unsigned int j, k;

	unsigned int idx;
	d_surfcells.resize((d_nx)*(d_ny)*(d_nz));

	for(i = 0; i < d_nx; ++i)
	{
		for(j = 0; j < d_ny; ++j)
		{
			for(k = 0; k < d_nz; ++k)
			{
				GeoPoint3D point(d_min.x + i*d_size, d_min.y + j*d_size, d_min.z + k*d_size);
				idx = (d_nx)*(d_ny)*k + (d_nx)*j +i;
				GeoPoint3D cellcenter(point.x + d_size/2, point.y + d_size/2, point.z + d_size/2);
				d_surfcells[idx].first = cellcenter;
				d_surfcells[idx].second = NULL; 
			}
		}
	}
}

/**
* CalculateGradients
* ------------------------------------------------------------------------
* Sets the object's gradients vector, which contains the distance field gradient
* for each voxel (uses finite difference)
*/ 
void DistCalc::CalculateGradients()
{
	if( d_voxels.size() == 0 ) return;

	// loop variables
	int i;
	unsigned int j, k;

	unsigned int idx;
	unsigned int idxFrontDownRight; 
	unsigned int idxBackDownLeft; 
	unsigned int idxFrontUpLeft; 

	d_gradients.resize((d_nx+1)*(d_ny+1)*(d_nz+1));

	double dx, dy, dz;

	unsigned int idxExtDown;
	unsigned int idxExtDownLeft;
	unsigned int idxExtDownFront;

	for(i = 0; i <= d_nx; ++i)
	{
		for(j = 0; j <= d_ny; ++j)
		{
			for(k = 0; k <= d_nz; ++k)
			{
				idx = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*j +i;
				idxFrontDownRight = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*j +(i+1);
				idxBackDownLeft = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*(j+1) +i;
				idxFrontUpLeft = (d_nx+1)*(d_ny+1)*(k+1) + (d_nx+1)*j +i;
				idxExtDown = (d_nx+1)*(d_ny+1)*(k-1) + (d_nx+1)*j +i;
				idxExtDownLeft = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*j +(i-1);
				idxExtDownFront = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*(j-1) +i;

				// df/dx = ( f(x+delta) - f(x-delta) ) / 2*delta
				if(i == 0)
					dx = (std::abs(d_voxels[idx+1]) - std::abs(d_voxels[idx]))/d_size;
				else if(i == d_nx)
					dx = (std::abs(d_voxels[idx]) - std::abs(d_voxels[idx-1]))/d_size;
				else
					dx = (std::abs(d_voxels[idxFrontDownRight]) - std::abs(d_voxels[idxExtDownLeft]))/(2*d_size);

				if(j == 0)
					dy = (std::abs(d_voxels[idxBackDownLeft]) - std::abs(d_voxels[idx]))/d_size;
				else if(j == d_ny)
					dy = (std::abs(d_voxels[idx]) - std::abs(d_voxels[idxExtDownFront]))/d_size;
				else
					dy = (std::abs(d_voxels[idxBackDownLeft]) - std::abs(d_voxels[idxExtDownFront]))/(2*d_size);

				if(k == 0)
					dz = (std::abs(d_voxels[idxFrontUpLeft]) - std::abs(d_voxels[idx]))/d_size;
				else if(k == d_nz)
					dz = (std::abs(d_voxels[idx]) - std::abs(d_voxels[idxExtDown]))/d_size;
				else
					dz = (std::abs(d_voxels[idxFrontUpLeft]) - std::abs(d_voxels[idxExtDown]))/(2*d_size);

				d_gradients[idx] = GeoPoint3D(dx, dy, dz);
			}
		}
	}
}


/**
* SurfaceNets
* ------------------------------------------------------------------------
* Reconstructs the original surface, using the data obtained from the distance field calculuation 
*/ 
CsiTSurf* DistCalc::SurfaceNets()
{
	// Mark cells for surface generation
	//CalculateGradients(); Not being used
	AssignVtx2Cell();

	// Generate new Surface from cell vertexes
	CsiTSurf *newsurf = new CsiTSurf(d_surf->name() + "_NET");

	// loop variables
	int i;
	unsigned int j, k;
	unsigned int idxC;
	unsigned int idxFrontDownLeft; 
	unsigned int idxFrontDownRight; 
	unsigned int idxBackDownLeft; 
	unsigned int idxFrontUpLeft; 
	unsigned int idxBackDownRight; 
	unsigned int idxFrontUpRight; 
	unsigned int idxBackUpLeft; 
	unsigned int idxBackUpRight; 

	unsigned int idxExtDownC; 
	unsigned int idxExtUpC; 
	unsigned int idxExtFrontC; 
	unsigned int idxExtBackC; 
	unsigned int idxExtRightC; 
	unsigned int idxExtLeftC; 

	GeoPoint3D cellcenter;

	for(i = 0; i < d_nx ; ++i)
	{
		for(j = 0; j < d_ny ; ++j)
		{
			for(k = 0; k < d_nz ; ++k)
			{
				idxC = (d_nx)*(d_ny)*k + (d_nx)*j +i;
				idxFrontDownLeft = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*j +i;
				idxFrontDownRight = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*j +(i+1);
				idxBackDownLeft = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*(j+1) +i;
				idxFrontUpLeft = (d_nx+1)*(d_ny+1)*(k+1) + (d_nx+1)*j +i;
				idxBackDownRight = (d_nx+1)*(d_ny+1)*k + (d_nx+1)*(j+1) +(i+1);
				idxFrontUpRight = (d_nx+1)*(d_ny+1)*(k+1) + (d_nx+1)*j +(i+1);
				idxBackUpLeft = (d_nx+1)*(d_ny+1)*(k+1) + (d_nx+1)*(j+1) +i;
				idxBackUpRight = (d_nx+1)*(d_ny+1)*(k+1) + (d_nx+1)*(j+1) +(i+1);

				idxExtDownC = (d_nx)*(d_ny)*(k-1) + (d_nx)*j +i;
				idxExtUpC = (d_nx)*(d_ny)*(k+1) + (d_nx)*j +i;
				idxExtFrontC = (d_nx)*(d_ny)*k + (d_nx)*(j-1) +i;
				idxExtBackC = (d_nx)*(d_ny)*k + (d_nx)*(j+1) +i;
				idxExtRightC = (d_nx)*(d_ny)*k + (d_nx)*j +(i+1);
				idxExtLeftC = (d_nx)*(d_ny)*k + (d_nx)*j +(i-1);

				cellcenter = d_surfcells[idxC].first;
	/* --------------------------------
	EdgeFace table:
	   edge       face1 face2
	   ---------- ----- -----
	   up-right     up  right
	   down-left   down left
	   up-front     up  front
	   down-back   down back
	   front-right front right 
	   back-left   back left
	---------------------------------*/
				// Create triangles 
				if ( inner(d_gradients[idxFrontUpRight], d_gradients[idxBackUpRight]) < 0 ) // up-right
				{
					if( d_surfcells[idxC].second == NULL )
						d_surfcells[idxC].second = _AddVertexIntoSurf(newsurf, cellcenter);

					d_surfcells[idxExtUpC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtUpC].first);

					d_surfcells[idxExtRightC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtRightC].first);

					newsurf->addTriangle(d_surfcells[idxC].second->pos,
					d_surfcells[idxExtUpC].second->pos, d_surfcells[idxExtRightC].second->pos);
				}
				if ( inner(d_gradients[idxFrontDownLeft], d_gradients[idxBackDownLeft]) < 0 ) // down-left
				{
					if( d_surfcells[idxC].second == NULL )
						d_surfcells[idxC].second = _AddVertexIntoSurf(newsurf, cellcenter);

					d_surfcells[idxExtDownC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtDownC].first);

					d_surfcells[idxExtLeftC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtLeftC].first);

					newsurf->addTriangle(d_surfcells[idxC].second->pos,
					d_surfcells[idxExtDownC].second->pos, d_surfcells[idxExtLeftC].second->pos);
				}
				if ( inner(d_gradients[idxFrontUpLeft], d_gradients[idxFrontUpRight]) < 0 ) // up-front
				{
					if( d_surfcells[idxC].second == NULL )
						d_surfcells[idxC].second = _AddVertexIntoSurf(newsurf, cellcenter);

					d_surfcells[idxExtUpC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtUpC].first);

					d_surfcells[idxExtFrontC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtFrontC].first);

					newsurf->addTriangle(d_surfcells[idxC].second->pos,
					d_surfcells[idxExtUpC].second->pos, d_surfcells[idxExtFrontC].second->pos);
				}
				if ( inner(d_gradients[idxBackDownLeft], d_gradients[idxBackDownRight]) < 0 ) // down-back
				{
					if( d_surfcells[idxC].second == NULL )
						d_surfcells[idxC].second = _AddVertexIntoSurf(newsurf, cellcenter);

					d_surfcells[idxExtDownC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtDownC].first);

					d_surfcells[idxExtBackC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtBackC].first);

					newsurf->addTriangle(d_surfcells[idxC].second->pos,
					d_surfcells[idxExtDownC].second->pos, d_surfcells[idxExtBackC].second->pos);
				}
				if ( inner(d_gradients[idxFrontUpRight], d_gradients[idxFrontDownRight]) < 0 ) // front-right
				{
					if( d_surfcells[idxC].second == NULL )
						d_surfcells[idxC].second = _AddVertexIntoSurf(newsurf, cellcenter);

					d_surfcells[idxExtFrontC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtFrontC].first);

					d_surfcells[idxExtRightC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtRightC].first);

					newsurf->addTriangle(d_surfcells[idxC].second->pos,
					d_surfcells[idxExtFrontC].second->pos, d_surfcells[idxExtRightC].second->pos);
				}
				if ( inner(d_gradients[idxBackDownLeft], d_gradients[idxBackUpLeft]) < 0 ) // back-left
				{
					if( d_surfcells[idxC].second == NULL )
						d_surfcells[idxC].second = _AddVertexIntoSurf(newsurf, cellcenter);

					d_surfcells[idxExtBackC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtBackC].first);

					d_surfcells[idxExtLeftC].second = _AddVertexIntoSurf(newsurf, d_surfcells[idxExtLeftC].first);

					newsurf->addTriangle(d_surfcells[idxC].second->pos,
					d_surfcells[idxExtBackC].second->pos, d_surfcells[idxExtLeftC].second->pos);
				}
			}
		}
	}
	cerr << "Number of triangles: "<< newsurf->trianglesList().size() << endl;
	cerr << "Number of vertices: "<< newsurf->vertexArray().size() << endl;
	
	RelaxSurfVertices();
	return newsurf;
}

