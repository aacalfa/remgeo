####################################################################
#  CONFIG.MAK - App
#  Created by Andre Calfa                                          #
####################################################################

#
# Definicao da Aplicacao
#
PROJNAME = appd
APPNAME = appd

DBG = Yes
#NO_ECHO = Yes

# Para realizar o teste de calculo de distancia entre ponto e triangulo, descomentar a linha abaixo
#DEFINES += DBGTEST

DEFINES += USE_OPENMP
#DEFINES += USE_PTHREADS

#
# Definicoes para o TecMake
#
USE_IUP3 = Yes
USE_IUPCONTROLS = Yes
USE_STATIC = Yes
TEC_LIB = $(TCG_HOME)/lib
MGEO_LIB = $(TEC_LIB)/mgeo

ifeq ($(TEC_UNAME), Linux38_64)
LINUX = Yes
TEC_LIB = ..
endif

ifeq ($(TEC_UNAME), Linux38_64)
MGEO_LIB = ..
endif	
#
# Show the Layout Application for debug
IUP_DEBUG = Yes

#
# Definicoes especificas para cada sistema
# Diretorio de bibliotecas do Tecgraf
#
TEC_LIB = $(TCG_HOME)/lib
DEFINES += _UNIX_
UNIX = Yes

ifdef IUP_DEBUG
  DEFINES += IUP_DEBUG
endif

LCFLAGS += -L/opt/local/lib/ -L/opt/X11/lib/ 

CPPFLAGS += -fopenmp -std=c++11

#
# Define o ambiente baseado na localizacao
#
SIGEO = $(TEC_LIB)/g0ex
ifeq ($(TEC_UNAME), Linux38_64)
SIGEO = ..
endif	

#
# Definicao do diretorio da aplicacao
#
APPL    = ..
APPLINC = $(APPL)

#
# Definicao das libs do Tecgraf
#

#
# ZLIB
# ----------------------------------------------------------------------------
LIBZ = $(MGEO_LIB)/zlib-1.2.3
ZINC = $(LIBZ)/include
ZLIB = $(LIBZ)/lib/$(TEC_UNAME)

#
# GL
# ----------------------------------------------------------------------------
#GLINC = /usr/X11R6/include
ifeq ($(TEC_UNAME), MacOS1010)
GLINC = /opt/X11/include
else
GLINC = /usr/include
endif

# SIGEO
# ----------------------------------------------------------------------------
SIGEOINC = $(SIGEO)/Fontes/Include

#
# HED
# ----------------------------------------------------------------------------
HED = $(TEC_LIB)/hed
HEDLIB = $(HED)/lib/$(TEC_UNAME)
HEDINC = $(HED)/include

# ----------------------------------------------------------------------------
# CSI lib
# --------------------------------------------------------------------------
CSI = $(MGEO_LIB)/csi/2.1
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

# ReMGeo lib
# --------------------------------------------------------------------------
REMGEO = $(APPL)
REMGEOLIB = $(APPL)/lib/$(TEC_UNAME)
REMGEOINC = $(APPL)/include

# OpenMP lib
# --------------------------------------------------------------------------
ifeq ($(TEC_UNAME), MacOS1010)
OMPLIB = /opt/local/lib/gcc5/
else
OMPLIB = /usr/lib/gcc/x86_64-linux-gnu/4.7
endif

#
# Includes
# ----------------------------------------------------------------------------

INCLUDES += 	\
	$(ZINC) \
	$(GLINC) \
	$(HEDINC)	\
	$(INTERSECTINC)	\
	$(GEOTOOLINC) \
	$(OMPLIB) \
	$(CSIINC)	\
	$(REMGEOINC)	

# Forca os includes da conta Tecgraf antes da g0eo
EXTRAINCS += $(SIGEOINC)

#
# Bibliotecas
# ----------------------------------------------------------------------------
ifeq ($(TEC_UNAME), Linux38_64)
LFLAGS += -L/usr/lib/x86_64-linux-gnu -L/usr/local/lib -L/usr/lib
endif

LFLAGS += -L/opt/X11/lib/ -L/opt/local/lib

SLIB +=	\
	$(ZLIB)/libz.a \
	$(CSILIB)/libcsi.a \
	$(UNDOLIB)/libUtlUndo.a	\
	$(HEDLIB)/libhed.a	\
	$(INTERSECTLIB)/libintersect.a \
	$(REMGEOLIB)/libremgeo.a \
	$(OMPLIB)/libgomp.a

LIBS += z pthread dl glut GL GLU 

#
# Fontes
# ----------------------------------------------------------------------------

SRC +=	\
  main.cpp \
	visualize.cpp \
	csidraw.cpp \
	manipulator.cpp \
	testunit.cpp

