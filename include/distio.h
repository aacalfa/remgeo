#ifndef _distio_h_
#define _distio_h_

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include "distcalc.h"

using namespace std;

class DistIO
{
	static DistIO *s_instance;

	DistIO()
	{
	}

	void PosLoad(DistCalc *distObj);

public:
	fstream d_file;

	static DistIO* GetInstance()
	{
		if( s_instance == NULL ) s_instance = new DistIO();
		return s_instance;
	}

	void SaveDistField(DistCalc *obj);

	DistCalc* LoadDistField(CsiTSurf *surf, std::string filename);

	static void CutExt( std::string fname, std::string &name, std::string &ext );
};
#endif
