####################################################################
#  CONFIG.MAK - ReMGeo
#  Created by Andre Calfa                                          #
####################################################################

#
# Definicao da Aplicacao
#
DBG = Yes
#NO_ECHO = Yes

PROJNAME = libremgeo
LIBNAME = remgeo

# Use Mult-Threads
USE_MT = Yes

#DEFINES += DBGTEST

#OPT = Yes
NO_DYNAMIC = Yes

DEFINES += USE_OPENMP
#DEFINES += USE_PTHREADS

#
# Definicoes para o TecMake
#
USE_STATIC = Yes
TEC_LIB = $(TCG_HOME)/lib
MGEO_LIB = $(TEC_LIB)/mgeo

ifeq ($(TEC_UNAME), vc12)
MGEO_LIB = ..
endif	
ifeq ($(TEC_UNAME), Linux38_64)
MGEO_LIB = ..
endif	

#
# Definicoes especificas para cada sistema
# Diretorio de bibliotecas do Tecgraf
#
TEC_LIB = $(TCG_HOME)/lib

DEFINES += _UNIX_
UNIX = Yes

CPPFLAGS += -fopenmp -std=c++11
ifeq ($(TEC_UNAME), vc11)
#CPPFLAGS += /openmp
else
CPPFLAGS += -fopenmp
endif

ifeq ($(TEC_UNAME), vc12)
TEC_LIB = ..
endif
#
# Define o ambiente baseado na localizacao
#
SIGEO = $(TEC_LIB)/g0ex
ifeq ($(TEC_UNAME), vc12)
SIGEO = ..
endif	

#
# Definicao do diretorio da aplicacao
#
GERECAD    = ..
GERECADINC = $(GERECAD)/include

#
# Definicao das libs do Tecgraf
#

#
# ZLIB
# ----------------------------------------------------------------------------
LIBZ = $(MGEO_LIB)/zlib-1.2.3
ZINC = $(LIBZ)/include
ZLIB = $(LIBZ)/lib/$(TEC_UNAME)

# SIGEO
# ----------------------------------------------------------------------------
SIGEOINC = $(SIGEO)/Fontes/Include

#
# HED
# ----------------------------------------------------------------------------
HED = $(TEC_LIB)/hed
ifeq ($(TEC_UNAME), Linux38_64)
HED = $(MGEO_LIB)/hed
endif

HEDLIB = $(HED)/lib/$(TEC_UNAME)
HEDINC = $(HED)/include

# ----------------------------------------------------------------------------
# CSI lib
# --------------------------------------------------------------------------
CSI = $(MGEO_LIB)/csi/2.1

ifeq ($(TEC_UNAME), vc12)
CSI = $(MGEO_LIB)/csi
endif
ifeq ($(TEC_UNAME), Linux38_64)
CSI = $(MGEO_LIB)/csi
endif
CSILIB = $(CSI)/lib/$(TEC_UNAME)
CSIINC = $(CSI)/include

# ----------------------------------------------------------------------------
# INTERSECT 
# --------------------------------------------------------------------------
INTERSECT = $(TEC_LIB)/intersect
ifeq ($(TEC_UNAME), Linux38_64)
INTERSECT = $(MGEO_LIB)/intersect
endif

INTERSECTLIB = $(INTERSECT)/lib/$(TEC_UNAME)
INTERSECTINC = $(INTERSECT)/include

# UtlUndo lib
# --------------------------------------------------------------------------
UNDO = $(MGEO_LIB)/UtlUndo
UNDOLIB = $(UNDO)/lib/$(TEC_UNAME)
UNDOINC = $(UNDO)/include

# OpenMP lib
# --------------------------------------------------------------------------
ifeq ($(TEC_UNAME), MacOS109)
OMPLIB = /opt/local/lib/gcc48
else
ifeq ($(TEC_UNAME), Linux38_64)
OMPLIB = /usr/lib/gcc/x86_64-linux-gnu/4.7
endif
endif

#
# Includes
# ----------------------------------------------------------------------------

INCLUDES += 	\
	$(ZINC) \
	$(HEDINC)	\
	$(INTERSECTINC)	\
	$(GEOTOOLINC) \
	$(OMPLIB) \
	$(CSIINC)	\
	$(GERECADINC)	

# Forca os includes da conta Tecgraf antes da g0eo
EXTRAINCS += $(SIGEOINC)

#
# Bibliotecas
# ----------------------------------------------------------------------------
ifeq ($(TEC_UNAME), Linux38_64)
LFLAGS += -L/usr/lib/x86_64-linux-gnu -L/usr/local/lib -L/usr/lib
endif

ifeq ($(TEC_UNAME), vc12)
SLIB +=	\
	$(ZLIB)/z.lib \
	$(CSILIB)/csi.lib \
	$(UNDOLIB)/UtlUndo.lib	\
	$(HEDLIB)/hed.lib	\
	$(INTERSECTLIB)/intersect.lib \
	#$(OMPLIB)/gomp.lib
else	
SLIB +=	\
	$(ZLIB)/libz.a \
	$(CSILIB)/libcsi.a \
	$(UNDOLIB)/libUtlUndo.a	\
	$(HEDLIB)/libhed.a	\
	$(INTERSECTLIB)/libintersect.a
ifeq ( $(USE_OPENMP), "Yes" )
	SLIB +=	\
	$(OMPLIB)/libgomp.a
endif	
endif

#LIBS += Xp z pthread dl
LIBS += openmp Xp z pthread dl

#
# Fontes
# ----------------------------------------------------------------------------

SRC +=	\
	distcalc.cpp \
	distio.cpp 
