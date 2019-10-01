/// This is an example of using the thermal expansion solver on a 2D multi-patch geometry
#include <gismo.h>
#include <gsElasticity/gsThermoAssembler.h>
#include <gsElasticity/gsWriteParaviewMultiPhysics.h>

using namespace gismo;

int main(int argc, char* argv[]){

    gsInfo << "Testing the thermal expansion solver in 2D.\n";

    //=====================================//
                // Input //
    //=====================================//

    std::string filename = ELAST_DATA_DIR"/rotor_2D.xml";
    index_t numUniRef = 1; // number of h-refinements
    index_t numKRef = 0; // number of k-refinements
    index_t numPlotPoints = 10000;
    real_t fluxValue = 100.;

    // minimalistic user interface for terminal
    gsCmdLine cmd("Testing the thermal expansion solver in 2D.");
    cmd.addInt("r","refine","Number of uniform refinement application",numUniRef);
    cmd.addInt("k","krefine","Number of degree elevation application",numKRef);
    cmd.addInt("p","points","Number of points to plot to Paraview",numPlotPoints);
    try { cmd.getValues(argc,argv); } catch (int rv) { return rv; }

    // material parameters
    real_t thExpCoef = 2e-4;
    real_t initTemp = 20.;

    // heat source function, rhs for the heat equation
    gsConstantFunction<> heatSource(0.,2);
    // boundary temperature, dirichlet BC for the heat equation
    gsConstantFunction<> bTemp(initTemp,2);
    // boundary flux, nuemann BC for the heat equation
    gsConstantFunction<> heatFlux(fluxValue,2);
    // boundary conditions for the heat equation
    gsBoundaryConditions<> bcTemp;
    bcTemp.addCondition(0,boundary::south,condition_type::dirichlet,&bTemp);
    bcTemp.addCondition(0,boundary::north,condition_type::neumann,&heatFlux);

    // gravity, rhs for the linear elasticity equation
    gsConstantFunction<> gravity(0.,0.,2);
    // boundary conditions for the linear elasticity equation
    gsBoundaryConditions<> bcElast;
    // Dirichlet BC are imposed separately for every component (coordinate)
    for (index_t d = 0; d < 2; ++d)
    {   // 0 refers to a patch number, nullptr means that the dirichlet BC is homogeneous, d stats for the displacement component
        bcElast.addCondition(0,boundary::south,condition_type::dirichlet,nullptr,d);
        bcElast.addCondition(0,boundary::west,condition_type::dirichlet,nullptr,d);
        bcElast.addCondition(0,boundary::east,condition_type::dirichlet,nullptr,d);
    }

    //=============================================//
         // Scanning geometry and creating bases //
    //=============================================//

    // scanning geometry
    gsMultiPatch<> geometry;
    gsReadFile<>(filename, geometry);
    // creating basis
    gsMultiBasis<> basis(geometry);
    for (index_t i = 0; i < numKRef; ++i)
    {
        basis.degreeElevate();
        basis.uniformRefine();
    }
    for (index_t i = 0; i < numUniRef; ++i)
        basis.uniformRefine();

    //=============================================//
                // Solving temperature //
    //=============================================//

    gsStopwatch clock;

    // creating an assembler for the heat equation
    gsPoissonAssembler<> heatAssembler(geometry,basis,bcTemp,heatSource);
    clock.restart();
    gsInfo<<"Assembling heat...\n";
    heatAssembler.assemble();
    gsInfo << "Assembled the heat equation system (matrices and a load vector) with "
           << heatAssembler.numDofs() << " dofs in " << clock.stop() << "s.\n";

    clock.restart();
    gsInfo << "Solving heat...\n";
#ifdef GISMO_WITH_PARDISO
    gsSparseSolver<>::PardisoLDLT solverHeat(heatAssembler.matrix());
    gsVector<> solVectorHeat = solverHeat.solve(heatAssembler.rhs());
    gsInfo << "Solved the heat system with PardisoLDLT solver in " << clock.stop() <<"s.\n";
#else
    gsSparseSolver<>::SimplicialLDLT solverHeat(heatAssembler.matrix());
    solVectorHeat = solverHeat.solve(heatAssembler.rhs());
    gsInfo << "Solved the heat system with EigenLDLT solver in " << clock.stop() <<"s.\n";
#endif

    // temperature as an IGA field
    gsMultiPatch<> temperature;
    heatAssembler.constructSolution(solVectorHeat,temperature);

    //=============================================//
             // Solving thermal expansion //
    //=============================================//

    // creating assembler
    gsThermoAssembler<real_t> assembler(geometry,basis,bcElast,gravity,temperature);
    assembler.options().setReal("InitTemp",initTemp);
    assembler.options().setReal("ThExpCoef",thExpCoef);

    gsInfo<<"Assembling elasticity...\n";
    clock.restart();
    assembler.assemble();
    gsInfo << "Assembled the elasticity system (matrix and load vector) with "
           << assembler.numDofs() << " dofs in " << clock.stop() << "s.\n";


    clock.restart();
    gsInfo << "Solving elasticity...\n";
#ifdef GISMO_WITH_PARDISO
    gsSparseSolver<>::PardisoLDLT solver(assembler.matrix());
    gsVector<> solVector = solver.solve(assembler.rhs());
    gsInfo << "Solved the elasticity system with PardisoLDLT solver in " << clock.stop() <<"s.\n";
#else
    gsSparseSolver<>::SimplicialLDLT solver(assembler.matrix());
    gsVector<> solVector = solver.solve(assembler.rhs());
    gsInfo << "Solved the elasticity system with EigenLDLT solver in " << clock.stop() <<"s.\n";
#endif

    // constructing solution as an IGA function
    gsMultiPatch<> solution;
    assembler.constructSolution(solVector,solution);

    // constructing an IGA field (geometry + solution)
    gsField<> solutionField(assembler.patches(),solution);
    gsField<> heatField(assembler.patches(),temperature);

    //=============================================//
                  // Visualization //
    //=============================================//

    // creating a container to plot all fields to one Paraview file
    if (numPlotPoints > 0)
    {
        std::map<std::string,const gsField<> *> fields;
        fields["Deformation"] = &solutionField;
        fields["Temperature"] = &heatField;
        gsWriteParaviewMultiPhysics(fields,"rotor",numPlotPoints);
        gsInfo << "Open \"rotor.pvd\" in Paraview for visualization.\n";
    }

    return 0;
}
