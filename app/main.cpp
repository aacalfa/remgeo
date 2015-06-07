#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

#define gmpuGridGlobais
#include "CsiGrid.h"
#undef gmpuGridGlobais

#include "visualize.h"
#include "testunit.h"
#include "distio.h"

using namespace std;

#define NUM_THREADS 8

CsiTSurf *tsurf = NULL;
CsiTSurf *newsurf = NULL;
DistCalc *distObj = NULL;
	
/*-----------------------*/
/* Main function.        */
/*-----------------------*/
int main(int argc, char* argv[]) 
{
	// A surface file must be given
	if (argc < 2)
	{
		cout << "Please inform the .ts surface file or the .df distance field file" << endl;
		return 0;
	}

	std::string surfname = argv[1];
	
	// NO_DF option: program does not calculate distance field, only displays original surface
	std::string optional;
	if(argc == 3) optional = argv[2];

#ifdef _WIN32
	surfname = "..\\..\\..\\app\\Relat_Testes\\teste_NET.ts";
#endif

	if( optional == "NO_DF" )
	{
		// Load Surface
		tsurf = CsiTSurf::Gocadload(surfname);
		tsurf->normalsCoerence();
	}
	else if( surfname.find(".ts") != std::string::npos ) // Loading a surface file
	{
		// Load Surface
		tsurf = CsiTSurf::Gocadload(surfname);
		tsurf->normalsCoerence();

		distObj = new DistCalc(tsurf, argv[1]);
		distObj->MountBorderMap();

		if( surfname.find("NET") == std::string::npos ) // Loading an original surface, calculate distance field 
		{
			distObj->Grid2Mesh(); // Load Grid
			// Regenerate surface using SurfaceNets Algorithm
			newsurf = distObj->SurfaceNets();
		}
	}
	else if( surfname.find(".df") != std::string::npos ) // Loading a distance field file
	{
		std::string ext;
		std::string surffile;
		DistIO::CutExt(surfname, surffile, ext);
		surffile += ".ts";
		tsurf = CsiTSurf::Gocadload(surffile); // Load surface file
		distObj = DistIO::GetInstance()->LoadDistField(tsurf, surfname); // Load distance field
	}
	else // wrong file type
	{
		cerr << "Unrecognized file type!" << endl;
		exit(1);
	}
	
#ifdef DBGTEST
	// Run test unit
	testTriangle(surfname);
#endif

	// Start Visualization
	Visualize::InitVisu(argc, argv);

	return 0; 
}

