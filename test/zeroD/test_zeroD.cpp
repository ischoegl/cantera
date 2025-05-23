#include "gtest/gtest.h"
#include "cantera/thermo.h"
#include "cantera/kinetics.h"
#include "cantera/zerodim.h"
#include "cantera/base/Interface.h"
#include "cantera/numerics/eigen_sparse.h"
#include "cantera/numerics/SystemJacobianFactory.h"
#include "cantera/numerics/AdaptivePreconditioner.h"

using namespace Cantera;

// This test is an (almost) exact equivalent of a clib test
// (clib::test_ctreactor.cpp::ctreactor::reactor_simple)
TEST(zerodim, simple)
{
    double T = 1050;
    double P = 5 * 101325;
    string X = "CH4:1.0, O2:2.0, N2:7.52";

    auto sol = newSolution("gri30.yaml", "gri30", "none");
    sol->thermo()->setState_TPX(T, P, X);
    auto reactor = newReactor4("IdealGasReactor", sol, "simple");
    ASSERT_EQ(reactor->name(), "simple");
    reactor->initialize();
    ReactorNet network(reactor);
    network.initialize();

    double t = 0.0;
    while (t < 0.1) {
        ASSERT_GE(reactor->temperature(), T);
        t = network.time() + 5e-3;
        network.advance(t);
    }
}

// Test guards preventing segfaults for uninitialized zerodim objects
TEST(zerodim, test_guards)
{
    // Reactor with no contents
    Reactor reactor;
    EXPECT_THROW(reactor.temperature(), CanteraError);
    EXPECT_THROW(reactor.density(), CanteraError);
    EXPECT_THROW(reactor.massFractions(), CanteraError);
    EXPECT_THROW(reactor.massFraction(0), CanteraError);

    // Wall with no adjacent reactors
    Wall wall;
    EXPECT_THROW(wall.heatRate(), CanteraError);
    EXPECT_THROW(wall.expansionRate(), CanteraError);

    // FlowDevice with no adjacent reactors
    EXPECT_THROW(FlowDevice().massFlowRate(), CanteraError);
    EXPECT_THROW(MassFlowController().updateMassFlowRate(0.), CanteraError);
    EXPECT_THROW(PressureController().updateMassFlowRate(0.), CanteraError);
    EXPECT_THROW(Valve().updateMassFlowRate(0.), CanteraError);
}

TEST(zerodim, reservoir)
{
    auto gas = newSolution("gri30.yaml", "gri30", "none");
    auto res = newReservoir(gas, "my-reservoir");
    ASSERT_EQ(res->type(), "Reservoir");
    ASSERT_EQ(res->name(), "my-reservoir");
}

TEST(zerodim, flowdevice)
{
    auto gas = newSolution("gri30.yaml", "gri30", "none");

    auto node0 = newReactor4("IdealGasReactor", gas, "upstream");
    auto node1 = newReactor4("IdealGasReactor", gas, "downstream");

    auto valve = newFlowDevice("Valve", node0, node1, "valve");
    ASSERT_EQ(valve->name(), "valve");
    ASSERT_EQ(valve->in().name(), "upstream");
    ASSERT_EQ(valve->out().name(), "downstream");

    ASSERT_EQ(node0->nInlets(), 0);
    ASSERT_EQ(node0->nOutlets(), 1);
    ASSERT_EQ(node1->nInlets(), 1);
    ASSERT_EQ(node1->nOutlets(), 0);
}

TEST(zerodim, wall)
{
    auto gas = newSolution("gri30.yaml", "gri30", "none");

    auto node0 = newReactor4("IdealGasReactor", gas, "left");
    auto node1 = newReactor4("IdealGasReactor", gas, "right");

    auto wall = newWall("Wall", node0, node1, "wall");
    ASSERT_EQ(wall->name(), "wall");
    ASSERT_EQ(wall->left().name(), "left");
    ASSERT_EQ(wall->right().name(), "right");

    ASSERT_EQ(node0->nWalls(), 1);
    ASSERT_EQ(node1->nWalls(), 1);
}

TEST(zerodim, mole_reactor)
{
    // simplified version of continuous_reactor.py
    auto gas = newSolution("h2o2.yaml", "ohmech", "none");

    auto tank = make_shared<Reservoir>(gas, "fuel-air-tank");
    auto exhaust = make_shared<Reservoir>(gas, "exhaust");

    auto stirred = make_shared<IdealGasMoleReactor>(gas, "stirred-reactor");
    stirred->setEnergy(0);
    stirred->setInitialVolume(30.5 * 1e-6);

    auto mfc = make_shared<MassFlowController>(tank, stirred, "mass-flow-controller");
    double residenceTime = 2.;
    double mass = stirred->mass();
    mfc->setMassFlowRate(mass/residenceTime);

    auto preg = make_shared<PressureController>(stirred, exhaust, "pressure-regulator");
    preg->setPrimary(mfc);
    preg->setDeviceCoefficient(1e-3);

    auto net = ReactorNet(stirred);
    net.initialize();
}

TEST(zerodim, mole_reactor_2)
{
    // simplified version of continuous_reactor.py
    auto gas = newSolution("h2o2.yaml", "ohmech", "none");

    auto tank = newReservoir(gas, "fuel-air-tank");
    auto exhaust = newReservoir(gas, "exhaust");

    auto stirred = newReactor4(
        "IdealGasMoleReactor", gas, "stirred-reactor");
    stirred->setEnergy(0);
    stirred->setInitialVolume(30.5 * 1e-6);

    auto mfc = newFlowDevice(
        "MassFlowController", tank, stirred, "mass-flow-controller");
    double residenceTime = 2.;
    double mass = stirred->mass();
    mfc->setMassFlowRate(mass/residenceTime);

    auto preg = newFlowDevice(
        "PressureController", stirred, exhaust, "pressure-regulator");
    preg->setPrimary(mfc);
    preg->setDeviceCoefficient(1e-3);

    vector<shared_ptr<ReactorBase>> reactors{stirred};
    auto net = newReactorNet(reactors);
    net->initialize();
}

// This test ensures that prior reactor initialization of a reactor does
// not affect later integration within a network. This example was
// adapted from test_reactor.py::test_equilibrium_HP.
TEST(zerodim, test_individual_reactor_initialization)
{
    // initial conditions
    double T0 = 1100.0;
    double P0 = 10 * OneAtm;
    double tol = 1e-7;
    string X0 = "H2:1.0, O2:0.5, AR:8.0";
    // reactor solution, phase, and kinetics objects
    shared_ptr<Solution> sol1 = newSolution("h2o2.yaml");
    sol1->thermo()->setState_TPX(T0, P0, X0);
    // set up reactor object
    auto reactor1 = newReactor4("Reactor", sol1);
    // initialize reactor prior to integration to ensure no impact
    reactor1->initialize();
    // setup reactor network and integrate
    ReactorNet network(reactor1);
    network.initialize();
    network.advance(1.0);
    // secondary gas for comparison
    shared_ptr<Solution> sol2 = newSolution("h2o2.yaml");
    sol2->thermo()->setState_TPX(T0, P0, X0);
    sol2->thermo()->equilibrate("UV");
    // secondary reactor for comparison
    auto reactor2 = newReactor4("Reactor", sol2);
    reactor2->initialize(0.0);
    // get state of reactors
    vector<double> state1(reactor1->neq());
    vector<double> state2(reactor2->neq());
    reactor1->getState(state1.data());
    reactor2->getState(state2.data());
    // compare the reactors.
    EXPECT_EQ(reactor1->neq(), reactor2->neq());
    for (size_t i = 0; i < reactor1->neq(); i++) {
        EXPECT_NEAR(state1[i], state2[i], tol);
    }
}

TEST(MoleReactorTestSet, test_mole_reactor_get_state)
{
    // setting up solution object and thermo/kinetics pointers
    double tol = 1e-8;
    auto sol = newSolution("h2o2.yaml");
    sol->thermo()->setState_TPY(1000.0, OneAtm, "H2:0.5, O2:0.5");
    IdealGasConstPressureMoleReactor reactor(sol);
    reactor.setInitialVolume(0.5);
    reactor.setEnergy(false);
    reactor.initialize();
    vector<double> state(reactor.neq());
    // test get state
    const ThermoPhase& thermo = reactor.contents();
    const vector<double>& imw = thermo.inverseMolecularWeights();
    // prescribed state
    double mass = reactor.volume() * thermo.density();
    size_t H2I = reactor.componentIndex("H2")-1;
    size_t O2I = reactor.componentIndex("O2")-1;
    double O2_Moles = imw[O2I] * 0.5 * mass;
    double  H2_Moles = imw[H2I] * 0.5 * mass;
    // test getState
    reactor.getState(state.data());
    EXPECT_NEAR(state[reactor.componentIndex("H2")], H2_Moles, tol);
    EXPECT_NEAR(state[reactor.componentIndex("O2")], O2_Moles, tol);
    EXPECT_NEAR(reactor.volume(), 0.5, tol);
    EXPECT_NEAR(reactor.pressure(), OneAtm, tol);
}

TEST(AdaptivePreconditionerTests, test_adaptive_precon_utils)
{
    // setting the tolerance
    double tol = 1e-8;
    size_t testSize = 4;
    AdaptivePreconditioner precon;
    precon.initialize(testSize);
    // test get and set utils
    double droptol = 1e-4;
    precon.setIlutDropTol(droptol);
    EXPECT_NEAR(precon.ilutDropTol(), droptol, tol);

    int fillfactor = static_cast<int>(testSize) / 2;
    precon.setIlutFillFactor(fillfactor);
    EXPECT_NEAR(precon.ilutFillFactor(), fillfactor, tol);

    double gamma = 1;
    precon.setGamma(gamma);
    EXPECT_NEAR(precon.gamma(), gamma, tol);
    // test setup and getting the matrix
    precon.updatePreconditioner();
    Eigen::SparseMatrix<double> identity(testSize, testSize);
    identity.setIdentity();
    EXPECT_TRUE(precon.matrix().isApprox(identity));
    // test solve
    vector<double> output(testSize, 0);
    vector<double> rhs_vector(testSize, 10);
    precon.solve(testSize, rhs_vector.data(), output.data());
    for (size_t i = 0; i < testSize; i++) {
        EXPECT_NEAR(rhs_vector[i], output[i], tol);
    }
    // test prune preconditioner and threshold
    double thresh = 0.5;
    precon.setThreshold(thresh);
    EXPECT_NEAR(precon.threshold(), thresh, tol);
    for (size_t i = 0; i < testSize; i++) {
        for (size_t j = 0; j < testSize; j++) {
            precon.setValue(i, j, thresh * 0.9);
        }
    }
    Eigen::MatrixXd testMat(testSize, testSize);
    testMat.setIdentity();
    testMat.fill(thresh * 0.9);
    EXPECT_TRUE(precon.jacobian().isApprox(testMat));
    precon.updatePreconditioner();
    EXPECT_TRUE(precon.matrix().isApprox(identity * (thresh * 1.1)));
    // reset and setup then test again
    precon.reset();
    precon.updatePreconditioner();
    EXPECT_TRUE(precon.matrix().isApprox(identity));
}

TEST(AdaptivePreconditionerTests, test_precon_solver_stats)
{
    // setting up solution object and thermo/kinetics pointers
    auto sol = newSolution("h2o2.yaml");
    sol->thermo()->setState_TPY(1000.0, OneAtm, "H2:0.5, O2:0.5");
    auto reactor = newReactor4("IdealGasMoleReactor", sol);
    reactor->setInitialVolume(0.5);
    // setup reactor network and integrate
    ReactorNet network(reactor);
    // setup preconditioner
    shared_ptr<SystemJacobian> precon_ptr = newSystemJacobian("Adaptive");
    network.setPreconditioner(precon_ptr);
    EXPECT_THROW(network.step(), CanteraError);
    // take a step
    network.setLinearSolverType("GMRES");
    // get solver stats
    network.step();
    AnyMap stats = network.solverStats();
    EXPECT_GE(stats["jac_evals"].asInt(), 0);
    EXPECT_GE(stats["lin_rhs_evals"].asInt(), 0);
    EXPECT_GE(stats["lin_iters"].asInt(), 0);
    EXPECT_GE(stats["lin_conv_fails"].asInt(), 0);
    EXPECT_GE(stats["prec_evals"].asInt(), 0);
    EXPECT_GE(stats["prec_solves"].asInt(), 0);
    EXPECT_GE(stats["jt_vec_setup_evals"].asInt(), 0);
    EXPECT_GE(stats["jt_vec_prod_evals"].asInt(), 0);
    EXPECT_GE(stats["nonlinear_iters"].asInt(), 0);
    EXPECT_GE(stats["nonlinear_conv_fails"].asInt(), 0);
}

int main(int argc, char** argv)
{
    printf("Running main() from test_zeroD.cpp\n");
    testing::InitGoogleTest(&argc, argv);
    Cantera::make_deprecation_warnings_fatal();
    Cantera::CanteraError::setStackTraceDepth(20);
    printStackTraceOnSegfault();
    int result = RUN_ALL_TESTS();
    Cantera::appdelete();
    return result;
}
