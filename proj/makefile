#
CFLAGS		 = -std=c11
FFLAGS		 =
CPPFLAGS         =
FPPFLAGS         =
MANSEC           = Vec
LOCDIR		 = src/vec/vec/examples/tutorials/

include ${PETSC_DIR}/lib/petsc/conf/variables
include ${PETSC_DIR}/lib/petsc/conf/rules

pwrGrid: pwrGrid.o admMat.o  loadMat.o derF.o calcCost.o derS.o util.o chkopts
	-${CLINKER} -o pwrGrid pwrGrid.o admMat.o loadMat.o derF.o calcCost.o derS.o util.o ${PETSC_VEC_LIB}
	${RM} -f pwrGrid.o admMat.o loadMat.o derF.o calcCost.o derS.o util.o

include ${PETSC_DIR}/lib/petsc/conf/test
