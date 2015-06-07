#ifndef _CSIDRAW_H_
#define _CSIDRAW_H_

#include "CsiTSurf.h"

bool GetFieldFlag();
void SetFieldFlag(bool fieldflag);

bool GetSignFlag();
void SetSignFlag(bool signflag);

bool GetMeshFlag();
void SetMeshFlag(bool meshflag);

bool GetCellFlag();
void SetCellFlag(bool cellflag);

bool GetGradientFlag();
void SetGradientFlag(bool gradflag);

bool GetBorderFlag();
void SetBorderFlag(bool borderflag);

void CsiDraw(CsiTSurf *tsurf);

#endif
