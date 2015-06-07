#include "distio.h"
#include <sstream>

DistIO *DistIO::s_instance;

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
* _String2Double
* ------------------------------------------------------------------------
* converts a string into a double value
* @param[in] s - string to be converted 
* @return - double value 
*/ 
static double _String2Double( const std::string& s )
{
	std::istringstream i(s);
	double x;
	if (!(i >> x))
		return 0;
	 return x;
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
* PosLoad
* ------------------------------------------------------------------------
* Loads additional distance field attributes
* @param[in] distObj - distance field object to be loaded 
*/ 
void DistIO::PosLoad(DistCalc *distObj)
{
	// Load gradients and cell centers
	distObj->CalculateGradients();
	distObj->AssignVtx2Cell();	
}

/**
* SaveDistField
* ------------------------------------------------------------------------
* Saves distance field information into a .df file
* @param[in] distObj - distance field object to be saved 
*/ 
void DistIO::SaveDistField(DistCalc *distObj)
{
	// Get array to be saved to file
	std::vector<double> &voxels = distObj->GetVoxels();

	// Open file
	std::string ext;
	std::string cfilename;
	CutExt(distObj->d_filename, cfilename, ext);
	cfilename += ".df";
	d_file.open(cfilename.c_str(), ios::out);

	// Write size value
	d_file << "size = " << distObj->d_size << endl;

	// Write voxels array
	d_file << "# BEGIN VOXELS\n";
	for (std::vector<double>::iterator it = voxels.begin() ; it != voxels.end(); ++it)
		d_file << *it << endl;

	d_file << "# END VOXELS" << endl;

	d_file.close();
}

/**
* LoadDistField
* ------------------------------------------------------------------------
* Loads distance field information from .df file
* @param[in] surf - distance field object to be saved 
* @param[in] distObj - distance field object to be saved 
*/ 
DistCalc* DistIO::LoadDistField(CsiTSurf *surf, std::string filename)
{
	DistCalc *ret = new DistCalc(surf, filename);

	// Get array to be loaded from file
	std::vector<double> &voxels = ret->GetVoxels();


	unsigned int i = 0;
	std::string line;
	d_file.open(filename.c_str(), fstream::in);
	
	while ( getline(d_file, line) )
	{
		if( line.find("size = ") != std::string::npos ) // step size information 
		{
			ret->d_size = _String2Double( line.substr(7) ); 
			ret->d_nx = (int) ((ret->d_max.x - ret->d_min.x) / ret->d_size + 1);
			ret->d_ny = (unsigned int) ((ret->d_max.y - ret->d_min.y) / ret->d_size + 1);
			ret->d_nz = (unsigned int) ((ret->d_max.z - ret->d_min.z) / ret->d_size + 1);
			voxels.resize((ret->d_nx+1)*(ret->d_ny+1)*(ret->d_nz+1));
		}
		else if ( line.find("#") != std::string::npos ) // comment, ignore 
			continue;
		else
		{
			voxels[i] = _String2Double(line);
			i++;
		}
	}

	d_file.close();

	// Load gradients and cellcenter arrays
	PosLoad(ret);

	return ret;
}

/* extCut
 * ----------------------------------------------------------------------
 * Cuts the extension of a file name 
* @param[in] fname - original file name
* @param[out] name - file name without extension 
* @param[out] ext - file extension 
 */
void DistIO::CutExt( std::string fname, std::string &name, std::string &ext )
{
	/* Copy the original name */
	if(name != fname)
		name = fname;

	// Search backward for the '.'
	size_t i = name.find_last_of('.');

	// If we have an extension, save it
	if (i != std::string::npos) {
		ext = name.substr(i);
		name.resize(i); // Cut extension from file name
	} else ext = "";
}

