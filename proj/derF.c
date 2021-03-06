#include "derF.h"
#include "admMat.h"
#include <math.h>

//Sets
PetscErrorCode setupConstraints(PetscInt nb, Mat bus_data, Mat gen_data, Vec *x, Vec *xmin, Vec *xmax)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;

  PetscInt ng;
  ierr = MatGetSize(gen_data, &ng, NULL);CHKERRQ(ierr);

  PetscInt xSize = 2 * nb + 2 * ng;

  //Setting up max and min values (lines 97 - 114)
  Vec Va_max, Va_min, Vm_max, Vm_min, Pgmax, Pgmin, Qgmax, Qgmin, Pg, Qg, Vm, Va;
  ierr = MakeVector(&Pg, ng);CHKERRQ(ierr);
  ierr = MakeVector(&Qg, ng);CHKERRQ(ierr);
  ierr = MakeVector(&Vm, nb);CHKERRQ(ierr);
  ierr = MakeVector(&Va, nb);CHKERRQ(ierr);
  ierr = MakeVector(&Va_max, nb);CHKERRQ(ierr);
  ierr = MakeVector(&Va_min, nb);CHKERRQ(ierr);
  ierr = MakeVector(&Vm_max, nb);CHKERRQ(ierr);
  ierr = MakeVector(&Vm_min, nb);CHKERRQ(ierr);
  ierr = MakeVector(&Pgmax, ng);CHKERRQ(ierr);
  ierr = MakeVector(&Pgmin, ng);CHKERRQ(ierr);
  ierr = MakeVector(&Qgmax, ng);CHKERRQ(ierr);
  ierr = MakeVector(&Qgmin, ng);CHKERRQ(ierr);

  //Va_max = Inf(nb, 1);
  //Va_min = -Va_max;
  ierr = VecSet(Va_max, INFINITY);CHKERRQ(ierr);
  ierr = VecSet(Va_min, -1 * INFINITY);CHKERRQ(ierr);

  //Doing Va first so I can use the values for Va_max and Va_min
  //Va = bus_data(:, VA);
  ierr = MatGetColumnVector(bus_data, Va, VA);CHKERRQ(ierr);

  //Not creating refs as a vector
  //refs = bus_data(:, BUS_TYPE)==3;
  //Va_max(refs) = bus_data(refs, VA);
  //Va_min(refs) = bus_data(refs, VA);
  Vec busType;
  ierr = MakeVector(&busType, nb);CHKERRQ(ierr);
  PetscScalar const *btArr;
  PetscScalar const *VaArr;
  PetscInt max, min;
  ierr = MatGetColumnVector(bus_data, busType, BUS_TYPE);CHKERRQ(ierr);
  ierr = VecGetArrayRead(busType, &btArr);CHKERRQ(ierr);
  ierr = VecGetArrayRead(Va, &VaArr);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(busType, &min, &max);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {
    if(btArr[i - min] == 3)
    {
      ierr = VecSetValue(Va_max, i, VaArr[i - min], INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecSetValue(Va_min, i, VaArr[i - min], INSERT_VALUES);CHKERRQ(ierr);
    }
  }
  ierr = VecRestoreArrayRead(busType, &btArr);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(Va, &VaArr);CHKERRQ(ierr);
  ierr = VecDestroy(&busType);CHKERRQ(ierr);

  ierr = VecAssemblyBegin(Va_max);CHKERRQ(ierr);
  ierr = VecAssemblyBegin(Va_min);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(Va_max);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(Va_min);CHKERRQ(ierr);


  //Vm_max = 1.1 * ones(nb, 1);
  //Vm_min = 0.9 * ones(nb, 1);
  ierr = VecSet(Vm_max, 1.1);CHKERRQ(ierr);
  ierr = VecSet(Vm_min, 0.9);CHKERRQ(ierr);


  //Vm = bus_data(:, VM);
  ierr = MatGetColumnVector(bus_data, Vm, VM);CHKERRQ(ierr);


  //Pgmax = gen_data(:, PMAX)/100;
  //Pgmin = gen_data(:, PMIN)/100;
  ierr = MatGetColumnVector(gen_data, Pgmax, PMAX);CHKERRQ(ierr);
  ierr = MatGetColumnVector(gen_data, Pgmin, PMIN);CHKERRQ(ierr);
  ierr = VecScale(Pgmax, 0.01);CHKERRQ(ierr);
  ierr = VecScale(Pgmin, 0.01);CHKERRQ(ierr);


  //Pg = (Pgmax+Pgmin)/2;
  ierr = VecWAXPY(Pg, 1, Pgmax, Pgmin);CHKERRQ(ierr);
  ierr = VecScale(Pg, 0.5);CHKERRQ(ierr);


  //Qgmax = gen_data(:, QMAX)/100;
  //Qgmin = gen_data(:, QMIN)/100;
  ierr = MatGetColumnVector(gen_data, Qgmax, QMAX);CHKERRQ(ierr);
  ierr = MatGetColumnVector(gen_data, Qgmin, QMIN);CHKERRQ(ierr);
  ierr = VecScale(Qgmax, 0.01);CHKERRQ(ierr);
  ierr = VecScale(Qgmin, 0.01);CHKERRQ(ierr);


  //Qg = (Qgmax+Qgmin)/2;
  ierr = VecWAXPY(Qg, 1, Qgmax, Qgmin);CHKERRQ(ierr);
  ierr = VecScale(Qg, 0.5);CHKERRQ(ierr);


  //x = [Va; Vm; Pg; Qg];
  //xmin = [Va_min; Vm_min; Pgmin; Qgmin];
  //xmax = [Va_max; Vm_max; Pgmax; Qgmax];
  Vec xVecs[4] = {Va, Vm, Pg, Qg};
  Vec xmaxVecs[4] = {Va_max, Vm_max, Pgmax, Qgmax};
  Vec xminVecs[4] = {Va_min, Vm_min, Pgmin, Qgmin};
  ierr = stackNVectors(x, xVecs, 4, xSize);CHKERRQ(ierr);
  ierr = stackNVectors(xmax, xmaxVecs, 4, xSize);CHKERRQ(ierr);
  ierr = stackNVectors(xmin, xminVecs, 4, xSize);CHKERRQ(ierr);


  //Clean up memory
  ierr = VecDestroy(&Va_max);CHKERRQ(ierr);
  ierr = VecDestroy(&Va_min);CHKERRQ(ierr);
  ierr = VecDestroy(&Vm_max);CHKERRQ(ierr);
  ierr = VecDestroy(&Vm_min);CHKERRQ(ierr);
  ierr = VecDestroy(&Pgmax);CHKERRQ(ierr);
  ierr = VecDestroy(&Pgmin);CHKERRQ(ierr);
  ierr = VecDestroy(&Qgmax);CHKERRQ(ierr);
  ierr = VecDestroy(&Qgmin);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


PetscErrorCode getLimitedLines(Mat branch_data, IS *il, PetscInt *nl2)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;

  PetscInt nl;
  ierr = MatGetSize(branch_data, &nl, NULL);CHKERRQ(ierr);

  IS ilTemp;

  Vec rateA;
  ierr = MakeVector(&rateA, nl);CHKERRQ(ierr);
  ierr = MatGetColumnVector(branch_data, rateA, RATE_A);CHKERRQ(ierr);

  PetscScalar const *aArr;
  ierr = VecGetArrayRead(rateA, &aArr);CHKERRQ(ierr);
  PetscInt min, max, n;
  ierr = VecGetOwnershipRange(rateA, &min, &max);CHKERRQ(ierr);
  ierr = VecGetLocalSize(rateA, &n);CHKERRQ(ierr);
  PetscInt *vals;
  ierr = PetscMalloc1(n, &vals);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {
    if(aArr[i - min] == 0)
      vals[i - min] = max;
    else
      vals[i - min] = i;
  }
  ierr = VecRestoreArrayRead(rateA, &aArr);CHKERRQ(ierr);
  ierr = VecDestroy(&rateA);CHKERRQ(ierr);

  ierr = ISCreateGeneral(PETSC_COMM_WORLD, n, vals, PETSC_COPY_VALUES, il);CHKERRQ(ierr);
  PetscFree(vals);

  ierr = ISCreateGeneral(PETSC_COMM_WORLD, 1, &max, PETSC_COPY_VALUES, &ilTemp);CHKERRQ(ierr);
  ierr = indexDifference(*il, ilTemp, il);CHKERRQ(ierr);

  ierr = ISGetSize(*il, nl2);CHKERRQ(ierr);

  ierr = ISDestroy(&ilTemp);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

//Comparable to gh_fcn1 in matlab
PetscErrorCode calcFirstDerivative(Vec x, Mat Ybus, Mat bus_data, Mat gen_data,
  Mat branch_data, IS il, Mat Yf, Mat Yt, PetscInt nl2, PetscInt nl, PetscScalar baseMVA, Vec xmax, Vec xmin,
  Vec *h, Vec *g, Mat *dh, Mat *dg, Vec *gn, Vec *hn,
  Mat * dSf_dVa, Mat *dSf_dVm, Mat *dSt_dVm, Mat *dSt_dVa, Vec *Sf, Vec *St)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;

  PetscInt nb, ng;
  ierr = MatGetSize(bus_data, &nb, NULL);CHKERRQ(ierr);
  ierr = MatGetSize(gen_data, &ng, NULL);CHKERRQ(ierr);

  PetscInt xSize = 2 * nb + 2 * ng;


  Vec Pg, Qg, Vm, Va;
  ierr = getVecIndices(x, 0, nb, &Va);CHKERRQ(ierr);
  ierr = getVecIndices(x, nb, nb * 2, &Vm);CHKERRQ(ierr);
  ierr = getVecIndices(x, nb * 2, xSize - ng, &Pg);CHKERRQ(ierr);
  ierr = getVecIndices(x, xSize - ng, xSize, &Qg);CHKERRQ(ierr);


  //gen_buses = gen_data(:, GEN_BUS);
  Vec gen_buses;
  ierr = MakeVector(&gen_buses, ng);CHKERRQ(ierr);
  ierr = MatGetColumnVector(gen_data, gen_buses, GEN_BUS);CHKERRQ(ierr);


  //Cg - sparse(gen_buses, (1:ng)', 1, nb, ng);
  //Using transpose here for the matrix preallocation (one value per column)
  Mat Cg, CgT;
  ierr = makeSparse(&CgT, ng, nb, 1, 1);

  PetscScalar const *genBusArr;
  ierr = VecGetArrayRead(gen_buses, &genBusArr);CHKERRQ(ierr);
  PetscInt min, max;
  ierr = VecGetOwnershipRange(gen_buses, &min, &max);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {
    ierr = MatSetValue(CgT, i, genBusArr[i - min] - 1, 1, INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(CgT, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(CgT, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(gen_buses, &genBusArr);CHKERRQ(ierr);

  ierr = MatTranspose(CgT, MAT_INITIAL_MATRIX, &Cg);CHKERRQ(ierr);
  ierr = MatDestroy(&CgT);CHKERRQ(ierr);


  //Sbusg = Cg * (Pg + 1j * Qg);
  Vec Sbusg, SbusgWork;
  ierr = MakeVector(&Sbusg, nb);CHKERRQ(ierr);
  ierr = MakeVector(&SbusgWork, ng);CHKERRQ(ierr);

  ierr = VecWAXPY(SbusgWork, PETSC_i, Qg, Pg);CHKERRQ(ierr);
  ierr = MatMult(Cg, SbusgWork, Sbusg);CHKERRQ(ierr);
  ierr = VecDestroy(&SbusgWork);CHKERRQ(ierr);


  //Sload = (bus_data(:, PD) + 1j * bus_data(:,QD)) / baseMVA;
  Vec Sload, pd, qd;
  ierr = MakeVector(&Sload, nb);CHKERRQ(ierr);
  ierr = MakeVector(&pd, nb);CHKERRQ(ierr);
  ierr = MakeVector(&qd, nb);CHKERRQ(ierr);

  ierr = MatGetColumnVector(bus_data, pd, PD);CHKERRQ(ierr);
  ierr = MatGetColumnVector(bus_data, qd, QD);CHKERRQ(ierr);
  ierr = VecWAXPY(Sload, PETSC_i, qd, pd);CHKERRQ(ierr);
  ierr = VecScale(Sload, 1 / baseMVA);CHKERRQ(ierr);

  ierr = VecDestroy(&pd);CHKERRQ(ierr);
  ierr = VecDestroy(&qd);CHKERRQ(ierr);

  //Sbus = Sbusg - Sload;
  Vec Sbus;
  ierr = MakeVector(&Sbus, nb);CHKERRQ(ierr);
  ierr = VecWAXPY(Sbus, -1, Sload, Sbusg);CHKERRQ(ierr);


  //V = Vm .* exp(1j * Va);
  Vec V, VaWork;
  ierr = MakeVector(&V, nb);CHKERRQ(ierr);
  ierr = MakeVector(&VaWork, nb);CHKERRQ(ierr);
  ierr = VecCopy(Va, VaWork);CHKERRQ(ierr);

  ierr = VecScale(VaWork, PETSC_i);CHKERRQ(ierr);
  ierr = VecExp(VaWork);CHKERRQ(ierr);
  ierr = VecPointwiseMult(V, Vm, VaWork);CHKERRQ(ierr);
  ierr = VecDestroy(&VaWork);CHKERRQ(ierr);


  //mis = V .* conj(Ybus * V) - Sbus;
  Vec mis, conjYbusV;
  ierr = MakeVector(&mis, nb);CHKERRQ(ierr);
  ierr = MakeVector(&conjYbusV, nb);CHKERRQ(ierr);

  ierr = MatMult(Ybus, V, conjYbusV);CHKERRQ(ierr);
  ierr = VecConjugate(conjYbusV);CHKERRQ(ierr);

  ierr = VecPointwiseMult(mis, V, conjYbusV);CHKERRQ(ierr);
  ierr = VecAXPY(mis, -1, Sbus);CHKERRQ(ierr);
  ierr = VecDestroy(&conjYbusV);CHKERRQ(ierr);


  //gn = [ real(mis); imag(mis)];
  ierr = MakeVector(gn, nb * 2);CHKERRQ(ierr);
  PetscScalar const *misArr;
  ierr = VecGetArrayRead(mis, &misArr);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(mis, &min, &max);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {
    VecSetValue(*gn, i,      PetscRealPart(misArr[i - min]), INSERT_VALUES);CHKERRQ(ierr);
    VecSetValue(*gn, i + nb, PetscImaginaryPart(misArr[i - min]), INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = VecRestoreArrayRead(mis, &misArr);CHKERRQ(ierr);
  ierr = VecDestroy(&mis);CHKERRQ(ierr);

  ierr = VecAssemblyBegin(*gn);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(*gn);CHKERRQ(ierr);


  //flow_max = (branch_data(il, RATE_A) / baseMVA) .^ 2;
  Vec flow_max, branchRateA, flowMaxTemp;
  ierr = MakeVector(&branchRateA, nl);CHKERRQ(ierr);
  ierr = MatGetColumnVector(branch_data, branchRateA, RATE_A);CHKERRQ(ierr);

  ierr = VecScale(branchRateA, 1 / baseMVA);CHKERRQ(ierr);
  ierr = VecPow(branchRateA, 2);CHKERRQ(ierr);

  ierr = VecGetSubVector(branchRateA, il, &flowMaxTemp);CHKERRQ(ierr);
  ierr = VecDuplicate(flowMaxTemp, &flow_max);CHKERRQ(ierr);
  ierr = VecCopy(flowMaxTemp, flow_max);CHKERRQ(ierr);
  ierr = VecRestoreSubVector(branchRateA, il, &flowMaxTemp);CHKERRQ(ierr);

  ierr = VecDestroy(&branchRateA);CHKERRQ(ierr);
  ierr = VecDestroy(&flowMaxTemp);CHKERRQ(ierr);


  //Sf = V(branch_data(il, F_BUS)) .* conj(Yf(il, :) * V);
  Vec ilFVals;
  Mat YfIl;
  ierr = getSubMatVector(&ilFVals, branch_data, il, F_BUS, nl);CHKERRQ(ierr);
  ierr = MatCreateSubMatrix(Yf, il, NULL, MAT_INITIAL_MATRIX, &YfIl);CHKERRQ(ierr);

  Vec YfV, VfRight;
  ierr = MatCreateVecs(YfIl, &VfRight, &YfV);CHKERRQ(ierr);
  ierr = VecCopy(V, VfRight);CHKERRQ(ierr);

  ierr = MatMult(YfIl, VfRight, YfV);CHKERRQ(ierr);
  ierr = VecDestroy(&VfRight);CHKERRQ(ierr);
  ierr = VecConjugate(YfV);CHKERRQ(ierr);

  PetscScalar const *ilFVArr;
  ierr = VecGetArrayRead(ilFVals, &ilFVArr);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(ilFVals, &min, &max);CHKERRQ(ierr);
  PetscInt n;
  ierr = VecGetLocalSize(ilFVals, &n);CHKERRQ(ierr);
  PetscInt *SfVals;
  ierr = PetscMalloc1(n, &SfVals);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {
    SfVals[i - min] = ilFVArr[i - min]-1;
  }
  ierr = VecRestoreArrayRead(ilFVals, &ilFVArr);CHKERRQ(ierr);

  IS isFV;
  ierr = ISCreateGeneral(PETSC_COMM_WORLD, n, SfVals, PETSC_COPY_VALUES, &isFV);CHKERRQ(ierr);
  PetscFree(SfVals);

  Vec VfInd;
  ierr = VecGetSubVector(V, isFV, &VfInd);CHKERRQ(ierr);
  ierr = VecDuplicate(VfInd, Sf);CHKERRQ(ierr);
  ierr = VecPointwiseMult(*Sf, VfInd, YfV);CHKERRQ(ierr);
  ierr = VecRestoreSubVector(V, isFV, &VfInd);CHKERRQ(ierr);

  ierr = VecDestroy(&ilFVals);CHKERRQ(ierr);


  //St = V(branch_data(il, T_BUS)) .* conj(Yt(il, :) * V);
  Vec ilTVals;
  Mat YtIl;
  ierr = getSubMatVector(&ilTVals, branch_data, il, T_BUS, nl);CHKERRQ(ierr);
  ierr = MatCreateSubMatrix(Yt, il, NULL, MAT_INITIAL_MATRIX, &YtIl);CHKERRQ(ierr);

  Vec YtV, VtRight;
  ierr = MatCreateVecs(YtIl, &VtRight, &YtV);CHKERRQ(ierr);
  ierr = VecCopy(V, VtRight);CHKERRQ(ierr);

  ierr = MatMult(YtIl, VtRight, YtV);CHKERRQ(ierr);
  ierr = VecDestroy(&VtRight);CHKERRQ(ierr);
  ierr = VecConjugate(YtV);CHKERRQ(ierr);

  PetscScalar const *ilTVArr;
  ierr = VecGetArrayRead(ilTVals, &ilTVArr);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(ilTVals, &min, &max);CHKERRQ(ierr);
  ierr = VecGetLocalSize(ilTVals, &n);CHKERRQ(ierr);
  PetscInt *StVals;
  ierr = PetscMalloc1(n, &StVals);
  for(PetscInt i = min; i < max; i++)
  {
    StVals[i - min] = ilTVArr[i - min]-1;
  }
  ierr = VecRestoreArrayRead(ilTVals, &ilTVArr);CHKERRQ(ierr);

  IS isTV;
  ierr = ISCreateGeneral(PETSC_COMM_WORLD, n, StVals, PETSC_COPY_VALUES, &isTV);CHKERRQ(ierr);
  PetscFree(StVals);

  Vec VtInd;
  ierr = VecGetSubVector(V, isTV, &VtInd);CHKERRQ(ierr);
  ierr = VecDuplicate(VtInd, St);CHKERRQ(ierr);
  ierr = VecPointwiseMult(*St, VtInd, YtV);CHKERRQ(ierr);
  ierr = VecRestoreSubVector(V, isTV, &VtInd);CHKERRQ(ierr);

  ierr = VecDestroy(&ilTVals);CHKERRQ(ierr);


  //hn = [Sf .* conj(Sf) - flow_max;
  //      St .* conj(St) - flow_max ];
  ierr = MakeVector(hn, nl2 * 2);CHKERRQ(ierr);
  Vec SfConj, StConj;
  ierr = VecDuplicate(*Sf, &SfConj);CHKERRQ(ierr);
  ierr = VecDuplicate(*St, &StConj);CHKERRQ(ierr);
  ierr = VecCopy(*Sf, SfConj);CHKERRQ(ierr);
  ierr = VecCopy(*St, StConj);CHKERRQ(ierr);
  ierr = VecConjugate(SfConj);CHKERRQ(ierr);
  ierr = VecConjugate(StConj);CHKERRQ(ierr);

  ierr = VecPointwiseMult(SfConj, *Sf, SfConj);CHKERRQ(ierr);
  ierr = VecPointwiseMult(StConj, *St, StConj);CHKERRQ(ierr);

  ierr = VecAXPY(SfConj, -1, flow_max);CHKERRQ(ierr);
  ierr = VecAXPY(StConj, -1, flow_max);CHKERRQ(ierr);

  PetscScalar const *SfArr;
  PetscScalar const *StArr;
  ierr = VecGetArrayRead(SfConj, &SfArr);CHKERRQ(ierr);
  ierr = VecGetArrayRead(StConj, &StArr);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(SfConj, &min, &max);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {
    VecSetValue(*hn, i,       SfArr[i - min], INSERT_VALUES);CHKERRQ(ierr);
    VecSetValue(*hn, i + nl2, StArr[i - min], INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = VecRestoreArrayRead(SfConj, &SfArr);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(StConj, &StArr);CHKERRQ(ierr);

  ierr = VecDestroy(&SfConj);CHKERRQ(ierr);
  ierr = VecDestroy(&StConj);CHKERRQ(ierr);

  ierr = VecAssemblyBegin(*hn);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(*hn);CHKERRQ(ierr);


  //n = length(V);            same as nb
  //ng = size(gen_data, 1);   hasn't changed
  //n_var = length(x);        same as xSize


  //Ibus = Ybus * V
  Vec Ibus;
  ierr = MatCreateVecs(Ybus, NULL, &Ibus);CHKERRQ(ierr);
  ierr = MatMult(Ybus, V, Ibus);CHKERRQ(ierr);


  //diagV = sparse(1:n, 1:n, V, n, n);
  Mat diagV;
  ierr = makeDiagonalMat(&diagV, V, nb);CHKERRQ(ierr);


  //diagIbus = sparse(1:n, 1:n, Ibus, n, n);
  Mat diagIbus;
  ierr = makeDiagonalMat(&diagIbus, Ibus, nb);CHKERRQ(ierr);


  //diagVnorm = sparse(1:n, 1:n, V ./ abs(V), n, n);
  Mat diagVnorm;
  Vec Vnorm;
  ierr = VecDuplicate(V, &Vnorm);CHKERRQ(ierr);
  ierr = VecCopy(V, Vnorm);CHKERRQ(ierr);

  ierr = VecAbs(Vnorm);CHKERRQ(ierr);
  ierr = VecReciprocal(Vnorm);CHKERRQ(ierr);
  ierr = VecPointwiseMult(Vnorm, V, Vnorm);
  ierr = makeDiagonalMat(&diagVnorm, Vnorm, nb);CHKERRQ(ierr);


  //dSbus_dVm = diagV * conj(Ybus * diagVnorm) + conj(diagIbus) * diagVnorm;
  Mat dSbus_dVm, dSbus_dVmWork1, dSbus_dVmWork2, diagIbusConj;
  ierr = MatDuplicate(diagIbus, MAT_COPY_VALUES, &diagIbusConj);CHKERRQ(ierr);
  ierr = MatConjugate(diagIbusConj);CHKERRQ(ierr);

  ierr = MatMatMult(diagIbusConj, diagVnorm, MAT_INITIAL_MATRIX, PETSC_DEFAULT, &dSbus_dVmWork1);CHKERRQ(ierr);
  ierr = MatMatMult(Ybus, diagVnorm, MAT_INITIAL_MATRIX, PETSC_DEFAULT, &dSbus_dVmWork2);CHKERRQ(ierr);
  ierr = MatConjugate(dSbus_dVmWork2);CHKERRQ(ierr);
  ierr = MatMatMult(diagV, dSbus_dVmWork2, MAT_INITIAL_MATRIX, PETSC_DEFAULT, &dSbus_dVm);CHKERRQ(ierr);
  ierr = MatAXPY(dSbus_dVm, 1, dSbus_dVmWork1, DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);


  //dSbus_dVa = 1j * diagV * conj(diagIbus - Ybus * diagV);
  Mat dSbus_dVa, dSbus_dVaWork;
  ierr = MatMatMult(Ybus, diagV, MAT_INITIAL_MATRIX, PETSC_DEFAULT, &dSbus_dVaWork);CHKERRQ(ierr);
  ierr = MatAYPX(dSbus_dVaWork, -1, diagIbus, DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
  ierr = MatConjugate(dSbus_dVaWork);CHKERRQ(ierr);

  ierr = MatMatMult(diagV, dSbus_dVaWork, MAT_INITIAL_MATRIX, PETSC_DEFAULT, &dSbus_dVa);CHKERRQ(ierr);
  ierr = MatScale(dSbus_dVa, PETSC_i);CHKERRQ(ierr);


  //neg_Cg = sparse(gen_data(:, GEN_BUS), 1:ng, -1, nb, ng);
  Mat neg_Cg;
  ierr = makeSparse(&neg_Cg, nb, ng, ng, ng);CHKERRQ(ierr);//Come back to this and change it later

  Vec genBus;
  ierr = MakeVector(&genBus, ng);CHKERRQ(ierr);
  ierr = MatGetColumnVector(gen_data, genBus, GEN_BUS);CHKERRQ(ierr);

  PetscScalar const *negCgArr;
  ierr = VecGetArrayRead(genBus, &negCgArr);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(genBus, &min, &max);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {
    ierr = MatSetValue(neg_Cg, negCgArr[i - min] - 1, i, -1, INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(neg_Cg, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(neg_Cg, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(genBus, &negCgArr);CHKERRQ(ierr);


  //dgn = sparse(2*nb, n_var);
  //dgn(:, [1:5 6:10 11:15 16:20]) = [
  //    real([dSbus_dVa dSbus_dVm]) neg_Cg sparse(nb, ng);  %% P mismatch w.r.t Va, Vm, Pg, Qg
  //    imag([dSbus_dVa dSbus_dVm]) sparse(nb, ng) neg_Cg;  %% Q mismatch w.r.t Va, Vm, Pg, Qg
  //];
  //dgn = dgn';
  //Potentially come back to this and remove zeros
  Mat dgnT, dgn;
  ierr = makeSparse(&dgnT, 2 * nb, xSize, xSize, xSize);CHKERRQ(ierr);

  Mat realVa, imagVa, realVm, imagVm;
  ierr = MatDuplicate(dSbus_dVa, MAT_COPY_VALUES, &realVa);CHKERRQ(ierr);
  ierr = MatDuplicate(dSbus_dVa, MAT_COPY_VALUES, &imagVa);CHKERRQ(ierr);
  ierr = MatDuplicate(dSbus_dVm, MAT_COPY_VALUES, &realVm);CHKERRQ(ierr);
  ierr = MatDuplicate(dSbus_dVm, MAT_COPY_VALUES, &imagVm);CHKERRQ(ierr);

  ierr = MatRealPart(realVa);CHKERRQ(ierr);
  ierr = MatImaginaryPart(imagVa);CHKERRQ(ierr);
  ierr = MatRealPart(realVm);CHKERRQ(ierr);
  ierr = MatImaginaryPart(imagVm);CHKERRQ(ierr);

  ierr = MatGetOwnershipRange(realVa, &min, &max);CHKERRQ(ierr);
  PetscScalar *realVaArr;
  ierr = PetscMalloc1((max - min) * nb, &realVaArr);CHKERRQ(ierr);
  PetscScalar *imagVaArr;
  ierr = PetscMalloc1((max - min) * nb, &imagVaArr);CHKERRQ(ierr);
  PetscScalar *realVmArr;
  ierr = PetscMalloc1((max - min) * nb, &realVmArr);CHKERRQ(ierr);
  PetscScalar *imagVmArr;
  ierr = PetscMalloc1((max - min) * nb, &imagVmArr);CHKERRQ(ierr);
  PetscScalar *negCgArr2;
  ierr = PetscMalloc1((max - min) * nb, &negCgArr2);CHKERRQ(ierr);
  PetscInt *nbArr = intArray(nb);
  PetscInt *nbArr2 = intArray2(nb, nb * 2);
  PetscInt *ngArr = intArray(ng);
  PetscInt *ngArr3 = intArray2(nb * 2, nb * 2 + ng);
  PetscInt *ngArr4 = intArray2(nb * 2 + ng, nb * 2 + ng * 2);
  PetscInt *rowsArr = intArray2(min, max);
  PetscInt *rowsArr2 = intArray2(min + nb, max + nb);
  ierr = MatGetValues(realVa, max - min, rowsArr, nb, nbArr, realVaArr);CHKERRQ(ierr);
  ierr = MatGetValues(imagVa, max - min, rowsArr, nb, nbArr, imagVaArr);CHKERRQ(ierr);
  ierr = MatGetValues(realVm, max - min, rowsArr, nb, nbArr, realVmArr);CHKERRQ(ierr);
  ierr = MatGetValues(imagVm, max - min, rowsArr, nb, nbArr, imagVmArr);CHKERRQ(ierr);
  ierr = MatGetValues(neg_Cg, max - min, rowsArr, ng, ngArr, negCgArr2);CHKERRQ(ierr);
  //Try going through this and only inserting the nonzero values into the matrix instead of all of them
  ierr = addNonzeros(dgnT, max - min, rowsArr, nb, nbArr, realVaArr);CHKERRQ(ierr);
  ierr = addNonzeros(dgnT, max - min, rowsArr, nb, nbArr2, realVmArr);CHKERRQ(ierr);
  ierr = addNonzeros(dgnT, max - min, rowsArr2, nb, nbArr, imagVaArr);CHKERRQ(ierr);
  ierr = addNonzeros(dgnT, max - min, rowsArr2, nb, nbArr2, imagVmArr);CHKERRQ(ierr);
  ierr = addNonzeros(dgnT, max - min, rowsArr, ng, ngArr3, negCgArr2);CHKERRQ(ierr);
  ierr = addNonzeros(dgnT, max - min, rowsArr2, ng, ngArr4, negCgArr2);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(dgnT, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(dgnT, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFree(ngArr);
  PetscFree(ngArr3);
  PetscFree(ngArr4);
  PetscFree(nbArr);
  PetscFree(nbArr2);
  PetscFree(rowsArr);
  PetscFree(rowsArr2);
  PetscFree(realVaArr);
  PetscFree(imagVaArr);
  PetscFree(realVmArr);
  PetscFree(imagVmArr);
  PetscFree(negCgArr2);
  ierr = MatTranspose(dgnT, MAT_INITIAL_MATRIX, &dgn);CHKERRQ(ierr);

  ierr = MatDestroy(&realVa);CHKERRQ(ierr);
  ierr = MatDestroy(&imagVa);CHKERRQ(ierr);
  ierr = MatDestroy(&realVm);CHKERRQ(ierr);
  ierr = MatDestroy(&imagVm);CHKERRQ(ierr);
  ierr = MatDestroy(&dgnT);CHKERRQ(ierr);
  ierr = MatDestroy(&neg_Cg);CHKERRQ(ierr);


  //f1 = [1,4];
  //t1 = [2,5];
  //Values were computed earlier and are store in isFV and isTV respectively


  //nl1 = length(f1); same as nl2


  //If = Yf(il, :) * V;
  //It = Yt(il, :) * V;
  Vec If, It;
  ierr = VecDuplicate(YfV, &If);CHKERRQ(ierr);
  ierr = VecDuplicate(YtV, &It);CHKERRQ(ierr);
  ierr = VecCopy(YfV, If);CHKERRQ(ierr);
  ierr = VecCopy(YtV, It);CHKERRQ(ierr);

  ierr = VecConjugate(If);CHKERRQ(ierr);
  ierr = VecConjugate(It);CHKERRQ(ierr);


  //Vnorm = V ./ abs(V);
  //Computed earlier


  //diagVf = sparse(1:nl1, 1:nl1, V(f1), nl1, nl1);
  Mat diagVf;
  Vec Vf1;
  ierr = VecGetSubVector(V, isFV, &Vf1);CHKERRQ(ierr);
  ierr = makeDiagonalMat(&diagVf, Vf1, nl2);CHKERRQ(ierr);
  ierr = VecRestoreSubVector(V, isFV, &Vf1);


  //diagIf = sparse(1:nl1, 1:nl1, If, nl1, nl1);
  Mat diagIf;
  ierr = makeDiagonalMat(&diagIf, If, nl2);


  //diagVt = sparse(1:nl1, 1:nl1, V(t1), nl1, nl1);
  Mat diagVt;
  Vec Vt1;
  ierr = VecGetSubVector(V, isTV, &Vt1);CHKERRQ(ierr);
  ierr = makeDiagonalMat(&diagVt, Vt1, nl2);CHKERRQ(ierr);
  ierr = VecRestoreSubVector(V, isTV, &Vt1);


  //diagIf = sparse(1:nl1, 1:nl1, If, nl1, nl1);
  Mat diagIt;
  ierr = makeDiagonalMat(&diagIt, It, nl2);


  //diagV = sparse(1:nb, 1:nb, V, nb, nb);
  //diagVnorm = sparse(1:nb, 1:nb, Vnorm, nb, nb);
  //Both matrices computed above


  //dSf_dVa = 1j * (conj(diagIf) * sparse(1:nl1, f1, V(f1), nl1, nb) - diagVf * conj(Yf(il, :) * diagV));
  //dSf_dVm = conj(diagIf) * sparse(1:nl1, f1, Vnorm(f1), nl1, nb) + diagVf * conj(Yf(il, :) * diagVnorm;
  //dSt_dVa = 1j * (conj(diagIt) * sparse(1:nl1, t1, V(t1), nl1, nb) - diagVt * conj(Yt(il, :) * diagV));
  //dSt_dVm = conj(diagIt) * sparse(1:nl1, t1, Vnorm(t1), nl1, nb) + diagVt * conj(Yt(il, :) * diagVnorm;
  ierr = dSMat(dSf_dVa, PETSC_i, -1, nl2, nb, diagIf, diagVf, V,     YfIl, isFV, diagV);
  ierr = dSMat(dSf_dVm, 1,        1, nl2, nb, diagIf, diagVf, Vnorm, YfIl, isFV, diagVnorm);
  ierr = dSMat(dSt_dVa, PETSC_i, -1, nl2, nb, diagIt, diagVt, V,     YtIl, isTV, diagV);
  ierr = dSMat(dSt_dVm, 1,        1, nl2, nb, diagIt, diagVt, Vnorm, YtIl, isTV, diagVnorm);


  //Sf = V(f1) .* conj(If);
  //St = V(t1) .* conj(It);
  ierr = VecDestroy(Sf);CHKERRQ(ierr);
  ierr = VecDestroy(St);CHKERRQ(ierr);

  ierr = VecConjugate(If);CHKERRQ(ierr);
  ierr = VecConjugate(It);CHKERRQ(ierr);

  Vec VisF, VisT;
  ierr = VecGetSubVector(V, isFV, &VisF);CHKERRQ(ierr);
  ierr = VecGetSubVector(V, isTV, &VisT);CHKERRQ(ierr);
  ierr = VecDuplicate(VisF, Sf);CHKERRQ(ierr);
  ierr = VecDuplicate(VisT, St);CHKERRQ(ierr);

  ierr = VecPointwiseMult(*Sf, VisF, If);CHKERRQ(ierr);
  ierr = VecPointwiseMult(*St, VisT, It);CHKERRQ(ierr);

  ierr = VecRestoreSubVector(V, isFV, &VisF);CHKERRQ(ierr);
  ierr = VecRestoreSubVector(V, isTV, &VisT);CHKERRQ(ierr);


  //nl_Sf = length(Sf)
  //Same as nl2


  //dAf_dPf = sparse(1:nl_Sf, 1:nl_Sf, 2 * real(Sf), nl_Sf, nl_Sf);
  //dAf_dQf = sparse(1:nl_Sf, 1:nl_Sf, 2 * imag(Sf), nl_Sf, nl_Sf);
  //dAt_dPt = sparse(1:nl_Sf, 1:nl_Sf, 2 * real(St), nl_Sf, nl_Sf);
  //dAt_dQt = sparse(1:nl_Sf, 1:nl_Sf, 2 * imag(St), nl_Sf, nl_Sf);
  Mat dAf_dPf, dAf_dQf, dAt_dPt, dAt_dQt;
  ierr = makeDiagonalMatRI(&dAf_dPf, *Sf, nl2, 'r', 2);CHKERRQ(ierr);
  ierr = makeDiagonalMatRI(&dAf_dQf, *Sf, nl2, 'i', 2);CHKERRQ(ierr);
  ierr = makeDiagonalMatRI(&dAt_dPt, *St, nl2, 'r', 2);CHKERRQ(ierr);
  ierr = makeDiagonalMatRI(&dAt_dQt, *St, nl2, 'i', 2);CHKERRQ(ierr);


  //dAf_dVm = dAf_dPf * real(dSf_dVm) + dAf_dQf * imag(dSf_dVm);
  //dAf_dVa = dAf_dPf * real(dSf_dVa) + dAf_dQf * imag(dSf_dVa);
  //dAt_dVm = dAt_dPt * real(dSt_dVm) + dAt_dQt * imag(dSt_dVm);
  //dAt_dVa = dAt_dPt * real(dSt_dVa) + dAt_dQt * imag(dSt_dVa);
  Mat dAf_dVm, dAf_dVa, dAt_dVm, dAt_dVa;
  ierr = matRealPMatImag(&dAf_dVm, dAf_dPf, dAf_dQf, *dSf_dVm);CHKERRQ(ierr);
  ierr = matRealPMatImag(&dAf_dVa, dAf_dPf, dAf_dQf, *dSf_dVa);CHKERRQ(ierr);
  ierr = matRealPMatImag(&dAt_dVm, dAt_dPt, dAt_dQt, *dSt_dVm);CHKERRQ(ierr);
  ierr = matRealPMatImag(&dAt_dVa, dAt_dPt, dAt_dQt, *dSt_dVa);CHKERRQ(ierr);


  //dhn = sparse(2*nl2, n_var);
  //dhn(:, [1:5 6:10]) = [
  //  dAf_dVa, dAf_dVm;
  //  dAt_dVa, dAt_dVm;
  //];
  //Potentially come back to and remove zeros
  Mat dhnT, dhn;
  ierr = makeSparse(&dhnT, 2 * nl2, xSize, xSize, xSize);CHKERRQ(ierr);

  ierr = MatGetOwnershipRange(dAf_dVa, &min, &max);CHKERRQ(ierr);
  PetscScalar *faArr;
  ierr = PetscMalloc1((max - min) * nb, &faArr);CHKERRQ(ierr);
  PetscScalar *fmArr;
  ierr = PetscMalloc1((max - min) * nb, &fmArr);CHKERRQ(ierr);
  PetscScalar *taArr;
  ierr = PetscMalloc1((max - min) * nb, &taArr);CHKERRQ(ierr);
  PetscScalar *tmArr;
  ierr = PetscMalloc1((max - min) * nb, &tmArr);CHKERRQ(ierr);
  nbArr = intArray(nb);
  nbArr2 = intArray2(nb, nb * 2);
  rowsArr = intArray2(min, max);
  rowsArr2 = intArray2(min + nl2, max + nl2);
  ierr = MatGetValues(dAf_dVa, max - min, rowsArr, nb, nbArr, faArr);CHKERRQ(ierr);
  ierr = MatGetValues(dAf_dVm, max - min, rowsArr, nb, nbArr, fmArr);CHKERRQ(ierr);
  ierr = MatGetValues(dAt_dVa, max - min, rowsArr, nb, nbArr, taArr);CHKERRQ(ierr);
  ierr = MatGetValues(dAt_dVm, max - min, rowsArr, nb, nbArr, tmArr);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {

    ierr = MatSetValues(dhnT, max - min, rowsArr, nb, nbArr, faArr, INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatSetValues(dhnT, max - min, rowsArr, nb, nbArr2, fmArr, INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatSetValues(dhnT, max - min, rowsArr2, nb, nbArr, taArr, INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatSetValues(dhnT, max - min, rowsArr2, nb, nbArr2, tmArr, INSERT_VALUES);CHKERRQ(ierr);
  }
  PetscFree(nbArr);
  PetscFree(nbArr2);
  PetscFree(rowsArr);
  PetscFree(rowsArr2);
  PetscFree(faArr);
  PetscFree(fmArr);
  PetscFree(taArr);
  PetscFree(tmArr);
  ierr = MatAssemblyBegin(dhnT, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(dhnT, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatTranspose(dhnT, MAT_INITIAL_MATRIX, &dhn);CHKERRQ(ierr);
  ierr = MatDestroy(&dAf_dVa);CHKERRQ(ierr);
  ierr = MatDestroy(&dAf_dVm);CHKERRQ(ierr);
  ierr = MatDestroy(&dAt_dVa);CHKERRQ(ierr);
  ierr = MatDestroy(&dAt_dVm);CHKERRQ(ierr);
  ierr = MatDestroy(&dhnT);CHKERRQ(ierr);


  //AA = speye(length(x));
  Mat AA;
  Vec vec4nb;
  ierr = MakeVector(&vec4nb, xSize);CHKERRQ(ierr);
  ierr = VecSet(vec4nb, 1);CHKERRQ(ierr);
  ierr = makeDiagonalMat(&AA, vec4nb, xSize);CHKERRQ(ierr);
  ierr = VecDestroy(&vec4nb);CHKERRQ(ierr);


  //ieq = find( abs(xmax-xmin) <= eps );        %% equality constraints
  IS ieq;
  Vec absDiff;
  PetscScalar eps = 0.000000000000001;
  ierr = MakeVector(&absDiff, xSize);CHKERRQ(ierr);
  ierr = VecWAXPY(absDiff, -1, xmin, xmax);CHKERRQ(ierr);
  ierr = VecAbs(absDiff);CHKERRQ(ierr);
  ierr = find(&ieq, lessEqual, &absDiff, &eps, 1);


  //igt = find( xmax >=  1e10 & xmin > -1e10 );     %% greater than, unbounded above
  IS igt;
  Vec igtVec[2] = {xmax, xmin};
  PetscScalar igtVal[2] = {10000000000, -10000000000};
  ierr = find(&igt, greaterEqualgreater, igtVec, igtVal, 2);


  //ilt = find( xmin <= -1e10 & xmax <  1e10 );     %% less than, unbounded below
  IS ilt;
  Vec iltVec[2] = {xmin, xmax};
  PetscScalar iltVal[2] = {-10000000000, 10000000000};
  ierr = find(&ilt, lessEqualless, iltVec, iltVal, 2);


  //ibx = find( (abs(xmax-xmin) > eps) & (xmax < 1e10) & (xmin > -1e10) );
  IS ibx;
  Vec ibxVec[3] = {absDiff, xmax, xmin};
  PetscScalar ibxVal[3] = {eps, 10000000000, -10000000000};
  ierr = find(&ibx, greaterLessGreater, ibxVec, ibxVal, 3);


  //Ae = AA(ieq, :);
  Mat Ae;
  ierr = MatCreateSubMatrix(AA, ieq, NULL, MAT_INITIAL_MATRIX, &Ae);CHKERRQ(ierr);


  //be = xmax(ieq, 1);
  Vec be;
  ierr = getSubVector(xmax, ieq, &be);CHKERRQ(ierr);


  //Ai  = [ AA(ilt, :); -AA(igt, :); AA(ibx, :); -AA(ibx, :) ];
  Mat Ai, AAilt, AAigt, AAibx, AAibxN;
  ierr = MatCreateSubMatrix(AA, ilt, NULL, MAT_INITIAL_MATRIX, &AAilt);CHKERRQ(ierr);
  ierr = MatCreateSubMatrix(AA, igt, NULL, MAT_INITIAL_MATRIX, &AAigt);CHKERRQ(ierr);
  ierr = MatCreateSubMatrix(AA, ibx, NULL, MAT_INITIAL_MATRIX, &AAibx);CHKERRQ(ierr);
  ierr = MatDestroy(&AA);CHKERRQ(ierr);

  ierr = MatScale(AAigt, -1);CHKERRQ(ierr);
  ierr = MatDuplicate(AAibx, MAT_COPY_VALUES, &AAibxN);CHKERRQ(ierr);
  ierr = MatScale(AAibxN, -1);CHKERRQ(ierr);

  PetscInt iltN, igtN, ibxN, ieqN;
  ierr = ISGetSize(ilt, &iltN);CHKERRQ(ierr);
  ierr = ISGetSize(igt, &igtN);CHKERRQ(ierr);
  ierr = ISGetSize(ibx, &ibxN);CHKERRQ(ierr);
  ierr = ISGetSize(ieq, &ieqN);CHKERRQ(ierr);

  PetscInt iltlocN, igtlocN, ibxlocN, ieqlocN;
  ierr = ISGetLocalSize(ilt, &iltlocN);CHKERRQ(ierr);
  ierr = ISGetLocalSize(igt, &igtlocN);CHKERRQ(ierr);
  ierr = ISGetLocalSize(ibx, &ibxlocN);CHKERRQ(ierr);
  ierr = ISGetLocalSize(ieq, &ieqlocN);CHKERRQ(ierr);

  ierr = makeSparse(&Ai, iltN + igtN + 2 * ibxN, xSize, xSize, xSize);CHKERRQ(ierr);

  PetscScalar *iltArr;
  ierr = PetscMalloc1(iltlocN * xSize, &iltArr);CHKERRQ(ierr);
  PetscScalar *igtArr;
  ierr = PetscMalloc1(igtlocN * xSize, &igtArr);CHKERRQ(ierr);
  PetscScalar *ibxArr;
  ierr = PetscMalloc1(ibxlocN * xSize, &ibxArr);CHKERRQ(ierr);
  PetscScalar *ibxNArr;
  ierr = PetscMalloc1(ibxlocN * xSize, &ibxNArr);CHKERRQ(ierr);
  nbArr = intArray(xSize);
  ierr = MatGetOwnershipRange(AAilt, &min, &max);CHKERRQ(ierr);
  rowsArr = intArray2(min, max);
  rowsArr2 = intArray2(min, max);
  ierr = MatGetValues(AAilt, max - min, rowsArr, xSize, nbArr, iltArr);CHKERRQ(ierr);
  ierr = addNonzeros(Ai, max - min, rowsArr2, xSize, nbArr, iltArr);CHKERRQ(ierr);
  PetscFree(rowsArr);
  PetscFree(rowsArr2);

  ierr = MatGetOwnershipRange(AAigt, &min, &max);CHKERRQ(ierr);
  rowsArr = intArray2(min, max);
  rowsArr2 = intArray2(min + iltN, max + iltN);
  ierr = MatGetValues(AAigt, max - min, rowsArr, xSize, nbArr, igtArr);CHKERRQ(ierr);
  ierr = addNonzeros(Ai, max - min, rowsArr2, xSize, nbArr, igtArr);CHKERRQ(ierr);
  PetscFree(rowsArr);
  PetscFree(rowsArr2);

  ierr = MatGetOwnershipRange(AAibx, &min, &max);CHKERRQ(ierr);
  rowsArr = intArray2(min, max);
  rowsArr2 = intArray2(min + iltN + igtN, max + iltN + igtN);

  ierr = MatGetValues(AAibx, max - min, rowsArr, xSize, nbArr, ibxArr);CHKERRQ(ierr);
  ierr = addNonzeros(Ai, max - min, rowsArr2, xSize, nbArr, ibxArr);CHKERRQ(ierr);
  PetscFree(rowsArr);
  PetscFree(rowsArr2);

  ierr = MatGetOwnershipRange(AAibxN, &min, &max);CHKERRQ(ierr);
  rowsArr = intArray2(min, max);
  rowsArr2 = intArray2(min + iltN + igtN + ibxN, max + iltN + igtN + ibxN);
  ierr = MatGetValues(AAibxN, max - min, rowsArr, xSize, nbArr, ibxNArr);CHKERRQ(ierr);
  ierr = addNonzeros(Ai, max - min, rowsArr2, xSize, nbArr, ibxNArr);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(Ai, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Ai, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  ierr = MatDestroy(&AAilt);CHKERRQ(ierr);
  ierr = MatDestroy(&AAigt);CHKERRQ(ierr);
  ierr = MatDestroy(&AAibx);CHKERRQ(ierr);
  ierr = MatDestroy(&AAibxN);CHKERRQ(ierr);

  PetscFree(nbArr);
  PetscFree(rowsArr2);
  PetscFree(rowsArr);
  PetscFree(iltArr);
  PetscFree(igtArr);
  PetscFree(ibxArr);
  PetscFree(ibxNArr);


  //bi  = [ xmax(ilt, 1); -xmin(igt, 1); xmax(ibx, 1); -xmin(ibx, 1) ];
  Vec bi, maxIlt, maxIbx, minIgt, minIbx;
  ierr = getSubVector(xmax, ilt, &maxIlt);CHKERRQ(ierr);
  ierr = getSubVector(xmax, ibx, &maxIbx);CHKERRQ(ierr);
  ierr = getSubVector(xmin, igt, &minIgt);CHKERRQ(ierr);
  ierr = getSubVector(xmin, ibx, &minIbx);CHKERRQ(ierr);

  ierr = VecScale(minIgt, -1);CHKERRQ(ierr);
  ierr = VecScale(minIbx, -1);CHKERRQ(ierr);

  Vec biVecs[4] = {maxIlt, minIgt, maxIbx, minIbx};
  ierr = stackNVectors(&bi, biVecs, 4, iltN + igtN + 2 * ibxN);CHKERRQ(ierr);

  ierr = VecDestroy(&maxIlt);CHKERRQ(ierr);
  ierr = VecDestroy(&maxIbx);CHKERRQ(ierr);
  ierr = VecDestroy(&minIgt);CHKERRQ(ierr);
  ierr = VecDestroy(&minIbx);CHKERRQ(ierr);


  //h = [hn; Ai * x - bi];          %% inequality constraints
  Vec Aix;
  ierr = MatCreateVecs(Ai, NULL, &Aix);CHKERRQ(ierr);
  ierr = MatMult(Ai, x, Aix);CHKERRQ(ierr);
  ierr = VecAXPY(Aix, -1, bi);CHKERRQ(ierr);

  Vec hVecs[2] = {*hn, Aix};
  PetscInt hnN;
  ierr = VecGetSize(*hn, &hnN);CHKERRQ(ierr);
  ierr = stackNVectors(h, hVecs, 2, hnN + iltN + igtN + 2 * ibxN);CHKERRQ(ierr);

  ierr = VecDestroy(&Aix);CHKERRQ(ierr);


  //g = [gn; Ae * x - be];          %% equality constraints
  Vec Aex;
  ierr = MatCreateVecs(Ae, NULL, &Aex);CHKERRQ(ierr);
  ierr = MatMult(Ae, x, Aex);CHKERRQ(ierr);
  ierr = VecAXPY(Aex, -1, be);CHKERRQ(ierr);

  Vec gVecs[2] = {*gn, Aex};
  PetscInt gnN;
  ierr = VecGetSize(*gn, &gnN);CHKERRQ(ierr);
  ierr = stackNVectors(g, gVecs, 2, gnN + ieqN);CHKERRQ(ierr);

  ierr = VecDestroy(&Aex);CHKERRQ(ierr);


  //dh = [dhn' Ai'];                 %% 1st derivative of inequalities
  //took transpose of dhn above
  Mat AiT;
  ierr = MatTranspose(Ai, MAT_INITIAL_MATRIX, &AiT);CHKERRQ(ierr);
  ierr = matJoinMatWidth(dh, dhn, AiT);CHKERRQ(ierr);
  ierr = MatDestroy(&AiT);CHKERRQ(ierr);
  ierr = MatDestroy(&dhn);CHKERRQ(ierr);

  ierr = remZeros(dh);CHKERRQ(ierr);


  //dg = [dgn Ae'];                 %% 1st derivative of equalities
  Mat AeT;
  ierr = MatTranspose(Ae, MAT_INITIAL_MATRIX, &AeT);CHKERRQ(ierr);
  ierr = matJoinMatWidth(dg, dgn, AeT);CHKERRQ(ierr);
  ierr = MatDestroy(&AeT);CHKERRQ(ierr);
  ierr = MatDestroy(&dgn);CHKERRQ(ierr);

  ierr = remZeros(dg);CHKERRQ(ierr);


  //Cleanup
  ierr = MatDestroy(&diagIbus);CHKERRQ(ierr);
  ierr = MatDestroy(&dSbus_dVm);CHKERRQ(ierr);
  ierr = MatDestroy(&dSbus_dVmWork1);CHKERRQ(ierr);
  ierr = MatDestroy(&dSbus_dVmWork2);CHKERRQ(ierr);
  ierr = MatDestroy(&dSbus_dVa);CHKERRQ(ierr);
  ierr = MatDestroy(&dSbus_dVaWork);CHKERRQ(ierr);
  ierr = MatDestroy(&diagIbusConj);CHKERRQ(ierr);
  ierr = MatDestroy(&dSbus_dVa);CHKERRQ(ierr);
  ierr = MatDestroy(&diagV);CHKERRQ(ierr);
  ierr = MatDestroy(&diagVnorm);CHKERRQ(ierr);
  ierr = MatDestroy(&Cg);CHKERRQ(ierr);
  ierr = MatDestroy(&dgn);CHKERRQ(ierr);
  ierr = MatDestroy(&diagVf);CHKERRQ(ierr);
  ierr = MatDestroy(&diagVt);CHKERRQ(ierr);
  ierr = MatDestroy(&diagIf);CHKERRQ(ierr);
  ierr = MatDestroy(&diagIt);CHKERRQ(ierr);
  ierr = MatDestroy(&YfIl);CHKERRQ(ierr);
  ierr = MatDestroy(&YtIl);CHKERRQ(ierr);
  ierr = MatDestroy(&dAf_dPf);CHKERRQ(ierr);
  ierr = MatDestroy(&dAf_dQf);CHKERRQ(ierr);
  ierr = MatDestroy(&dAt_dPt);CHKERRQ(ierr);
  ierr = MatDestroy(&dAt_dQt);CHKERRQ(ierr);
  ierr = MatDestroy(&Ae);CHKERRQ(ierr);
  ierr = MatDestroy(&Ai);CHKERRQ(ierr);
  ierr = VecDestroy(&bi);CHKERRQ(ierr);
  ierr = VecDestroy(&be);CHKERRQ(ierr);
  ierr = VecDestroy(&gen_buses);CHKERRQ(ierr);
  ierr = VecDestroy(&Sbusg);CHKERRQ(ierr);
  ierr = VecDestroy(&Sload);CHKERRQ(ierr);
  ierr = VecDestroy(&Sbus);CHKERRQ(ierr);
  ierr = VecDestroy(&V);CHKERRQ(ierr);
  ierr = VecDestroy(&flow_max);CHKERRQ(ierr);
  ierr = VecDestroy(&Vnorm);CHKERRQ(ierr);
  ierr = VecDestroy(&Ibus);CHKERRQ(ierr);
  ierr = VecDestroy(&genBus);CHKERRQ(ierr);
  ierr = VecDestroy(&YfV);CHKERRQ(ierr);
  ierr = VecDestroy(&YtV);CHKERRQ(ierr);
  ierr = VecDestroy(&If);CHKERRQ(ierr);
  ierr = VecDestroy(&It);CHKERRQ(ierr);
  ierr = VecDestroy(&VisF);CHKERRQ(ierr);
  ierr = VecDestroy(&VisT);CHKERRQ(ierr);
  ierr = VecDestroy(&absDiff);CHKERRQ(ierr);
  ierr = VecDestroy(&Pg);CHKERRQ(ierr);
  ierr = VecDestroy(&Qg);CHKERRQ(ierr);
  ierr = VecDestroy(&Va);CHKERRQ(ierr);
  ierr = VecDestroy(&Vm);CHKERRQ(ierr);
  ierr = ISDestroy(&isFV);CHKERRQ(ierr);
  ierr = ISDestroy(&isTV);CHKERRQ(ierr);
  ierr = ISDestroy(&ieq);CHKERRQ(ierr);
  ierr = ISDestroy(&igt);CHKERRQ(ierr);
  ierr = ISDestroy(&ilt);CHKERRQ(ierr);
  ierr = ISDestroy(&ibx);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}


PetscErrorCode matJoinMatWidth(Mat *out, Mat left, Mat right)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;

  PetscInt rows, lCol, rCol, min, max;
  ierr = MatGetSize(left, &rows, &lCol);CHKERRQ(ierr);
  ierr = MatGetSize(right, NULL, &rCol);CHKERRQ(ierr);

  ierr = makeSparse(out, rows, lCol + rCol, lCol + rCol, lCol + rCol);CHKERRQ(ierr);

  ierr = MatGetOwnershipRange(left, &min, &max);CHKERRQ(ierr);
  PetscScalar *leftVals;
  ierr = PetscMalloc1((max - min) * lCol, &leftVals);CHKERRQ(ierr);
  PetscScalar *rightVals;
  ierr = PetscMalloc1((max - min) * rCol, &rightVals);CHKERRQ(ierr);

  PetscInt *rowArr = intArray2(min, max);
  PetscInt *colArr = intArray2(0, lCol);
  PetscInt *colArrR = intArray2(0, rCol);
  PetscInt *colArr2 = intArray2(lCol, lCol + rCol);

  ierr = MatGetValues(left, max - min, rowArr, lCol, colArr, leftVals);CHKERRQ(ierr);
  ierr = MatGetValues(right, max - min, rowArr, rCol, colArrR, rightVals);CHKERRQ(ierr);

  ierr = addNonzeros(*out, max - min, rowArr, lCol, colArr, leftVals);CHKERRQ(ierr);
  ierr = addNonzeros(*out, max - min, rowArr, rCol, colArr2, rightVals);CHKERRQ(ierr);

  PetscFree(leftVals);
  PetscFree(rightVals);

  ierr = MatAssemblyBegin(*out, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*out, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  PetscFree(rowArr);
  PetscFree(colArr);
  PetscFree(colArrR);
  PetscFree(colArr2);

  PetscFunctionReturn(0);
}



PetscErrorCode makeDiagonalMatRI(Mat *m, Vec vals, PetscInt dim, char r, PetscScalar scale)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;

  ierr = makeSparse(m, dim, dim, 1, 0);CHKERRQ(ierr);

  PetscScalar const *mArr;
  ierr = VecGetArrayRead(vals, &mArr);CHKERRQ(ierr);
  PetscInt min, max;
  ierr = VecGetOwnershipRange(vals, &min, &max);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {
    if(r == 'r')
    {
      ierr = MatSetValue(*m, i, i, scale * PetscRealPart(mArr[i - min]), INSERT_VALUES);CHKERRQ(ierr);
    }
    else
    {
      ierr = MatSetValue(*m, i, i, scale * PetscImaginaryPart(mArr[i - min]), INSERT_VALUES);CHKERRQ(ierr);
    }
  }
  ierr = MatAssemblyBegin(*m, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*m, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(vals, &mArr);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}



PetscErrorCode dSMat(Mat *dSf_dVa, PetscScalar scale, PetscInt op, PetscInt nl2, PetscInt nb,
  Mat diagIf, Mat diagVf, Vec V, Mat YfIl, IS isFV, Mat diagV)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;

  Mat conjDiagIf;
  ierr = MatDuplicate(diagIf, MAT_COPY_VALUES, &conjDiagIf);CHKERRQ(ierr);
  ierr = MatConjugate(conjDiagIf);CHKERRQ(ierr);

  Mat Vf1Mat;
  Vec VisF;
  ierr = VecGetSubVector(V, isFV, &VisF);CHKERRQ(ierr);
  ierr = makeSparse(&Vf1Mat, nl2, nb, 1, 1);
  PetscScalar const *vArr;
  PetscInt const *f1Arr;
  PetscInt max, min;
  ierr = VecGetArrayRead(VisF, &vArr);CHKERRQ(ierr);
  ierr = ISGetIndices(isFV, &f1Arr);CHKERRQ(ierr);ierr = VecGetOwnershipRange(VisF, &min, &max);CHKERRQ(ierr);
  for(PetscInt i = min; i < max; i++)
  {
    MatSetValue(Vf1Mat, i, f1Arr[i - min], vArr[i - min], INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = VecRestoreArrayRead(VisF, &vArr);CHKERRQ(ierr);
  ierr = ISRestoreIndices(isFV, &f1Arr);CHKERRQ(ierr);
  ierr = VecRestoreSubVector(V, isFV, &VisF);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(Vf1Mat, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(Vf1Mat, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  ierr = MatMatMult(conjDiagIf, Vf1Mat, MAT_INITIAL_MATRIX, PETSC_DEFAULT, dSf_dVa);CHKERRQ(ierr);

  Mat conjYfIlDiagV, YfIl2;
  PetscInt ilR, ilC;
  ierr = MatGetSize(YfIl, &ilR, &ilC);CHKERRQ(ierr);
  ierr = makeSparse(&YfIl2, ilR, ilC, ilC, ilC);
  ierr = MatCopy(YfIl, YfIl2, DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
  ierr = MatMatMult(YfIl2, diagV, MAT_INITIAL_MATRIX, PETSC_DEFAULT, &conjYfIlDiagV);CHKERRQ(ierr);
  ierr = MatConjugate(conjYfIlDiagV);CHKERRQ(ierr);

  Mat dSf_dVaWork;
  ierr = MatMatMult(diagVf, conjYfIlDiagV, MAT_INITIAL_MATRIX, PETSC_DEFAULT, &dSf_dVaWork);CHKERRQ(ierr);

  ierr = MatAXPY(*dSf_dVa, op, dSf_dVaWork, DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
  ierr = MatScale(*dSf_dVa, scale);CHKERRQ(ierr);

  ierr = MatDestroy(&YfIl2);CHKERRQ(ierr);
  ierr = MatDestroy(&conjYfIlDiagV);CHKERRQ(ierr);
  ierr = MatDestroy(&conjDiagIf);CHKERRQ(ierr);
  ierr = MatDestroy(&Vf1Mat);CHKERRQ(ierr);
  ierr = MatDestroy(&dSf_dVaWork);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}


PetscErrorCode matRealPMatImag(Mat *result, Mat mat1, Mat mat2, Mat matCom)
{
  PetscErrorCode ierr;
  PetscFunctionBegin;

  Mat real, imag;
  ierr = MatDuplicate(matCom, MAT_COPY_VALUES, &real);CHKERRQ(ierr);
  ierr = MatDuplicate(matCom, MAT_COPY_VALUES, &imag);CHKERRQ(ierr);

  ierr = MatRealPart(real);CHKERRQ(ierr);
  ierr = MatImaginaryPart(imag);CHKERRQ(ierr);

  Mat work;
  ierr = MatMatMult(mat1, real, MAT_INITIAL_MATRIX, PETSC_DEFAULT, result);CHKERRQ(ierr);
  ierr = MatMatMult(mat2, imag, MAT_INITIAL_MATRIX, PETSC_DEFAULT, &work);CHKERRQ(ierr);

  ierr = MatAXPY(*result, 1, work, DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);

  ierr = MatDestroy(&real);CHKERRQ(ierr);
  ierr = MatDestroy(&imag);CHKERRQ(ierr);
  ierr = MatDestroy(&work);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}
