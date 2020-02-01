/** @file gsNsTimeIntegrator.h

    @brief Provides time integration for incompressible Navier-Stokes equations.

    This file is part of the G+Smo library.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.

    Author(s):
        A.Shamanskiy (2016 - ...., TU Kaiserslautern)
*/

#pragma once

#include <gsElasticity/gsBaseAssembler.h>
#include <gsElasticity/gsBaseUtils.h>

namespace gismo
{

template <class T>
class gsNsAssembler;
template <class T>
class gsMassAssembler;

/** @brief Time integation for incompressible Navier-Stokes equations.
*/
template <class T>
class gsNsTimeIntegrator : public gsBaseAssembler<T>
{
public:
    typedef gsBaseAssembler<T> Base;
    /// constructor method. requires a gsNsAssembler for construction of the static linear system
    /// and a gsMassAssembler for the mass matrix
    gsNsTimeIntegrator(gsNsAssembler<T> & stiffAssembler_,
                       gsMassAssembler<T> & massAssembler_);

    /// @brief Returns the list of default options for assembly
    static gsOptionList defaultOptions();
    /// set intial conditions
    void setSolutionVector(const gsMatrix<T> & solutionVector)
    {
        GISMO_ENSURE(solutionVector.rows() == stiffAssembler.numDofs(),"Wrong size of the solution vector: " + util::to_string(solutionVector.rows()) +
                     ". Must be: " + util::to_string(stiffAssembler.numDofs()));
        solVector = solutionVector;
    }
    /// @brief Initialize the solver; execute before computing any time steps
    void initialize();
    /// make a time step according to a chosen scheme
    void makeTimeStep(T timeStep);
    /// make a IMEX time step in ALE formulation
    void makeTimeStepFSI(T timeStep,gsMatrix<T> & solutionVector, gsMatrix<T> & solutionVectorOld,
                         gsMultiPatch<T> & velocityALE,
                         std::vector<std::pair<index_t,index_t> > & patches,
                         gsSparseMatrix<T> & A_n, gsMatrix<T> & rhs_n);
    void makeTimeStepFSI2(T timeStep,gsMultiPatch<T> & velocityALE,
                          std::vector<std::pair<index_t,index_t> > & patches);

    /// assemble the linear system for the nonlinear solver
    virtual bool assemble(const gsMatrix<T> & solutionVector,
                          const std::vector<gsMatrix<T> > & fixedDoFs,
                          bool assembleMatrix = true);

    virtual int numDofs() const { return stiffAssembler.numDofs(); }
    /// returns  vector of displacement DoFs
    const gsMatrix<T> & solutionVector() const {return solVector;}
    const gsMatrix<T> & SolutionVectorOld() const {return oldSolVector;}

    /// time integraton schemes
    void implicitLinear();
    void implicitNonlinear();

    /// save solver state
    void saveState();
    /// recover solver state from saved state
    void recoverState();

protected:
    /// assembler object that generates the static system
    gsNsAssembler<T> & stiffAssembler;
    /// assembler object that generates the mass matrix
    gsMassAssembler<T> & massAssembler;
    /// Sparse matrix of the linear system to solve
    gsSparseMatrix<T> m_matrix;
    /// RHS vector of the linear system to solve
    gsMatrix<T> m_rhs;
    gsMatrix<T> constRHS;

    /// time step length
    T tStep, oldTimeStep;
    /// vector of displacement DoFs
    gsMatrix<T> solVector;
    gsMatrix<T> oldSolVector;
    using Base::m_system;
    using Base::m_options;
    using Base::m_ddof;

    gsMatrix<T> velVecSaved;
    gsMatrix<T> oldVecSaved;
    gsMatrix<T> massRhsSaved;
    gsMatrix<T> stiffRhsSaved;
    gsSparseMatrix<T> stiffMatrixSaved;
    std::vector<gsMatrix<T> > ddofsSaved;

};

}
