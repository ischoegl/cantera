#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>

#include "cantera/core.h"
#include "cantera/clib/clib_defs.h"
#include "cantera/clib_experimental/ct3.h"
#include "cantera/clib_experimental/ctsol3.h"
#include "cantera/clib_experimental/ctthermo3.h"
#include "cantera/clib_experimental/ctkin3.h"
#include "cantera/clib_experimental/cttrans3.h"

using namespace Cantera;
using ::testing::HasSubstr;

string reportError()
{
    vector<char> output_buf;
    int buflen = ct3_getCanteraError(0, output_buf.data());
    output_buf.resize(buflen);
    ct3_getCanteraError(buflen, output_buf.data());
    return string(output_buf.data());
}

TEST(ct3, cabinet_exceptions)
{
    sol3_newSolution("h2o2.yaml", "ohmech", "default");
    sol3_name(999, 0, 0);

    string err = reportError();
    EXPECT_THAT(err, HasSubstr("Index 999 out of range."));

    sol3_thermo(998);
    err = reportError();
    EXPECT_THAT(err, HasSubstr("Index 998 out of range."));

    int ret = sol3_del(997);
    ASSERT_EQ(ret, -1);
    err = reportError();
    EXPECT_THAT(err, HasSubstr("Index 997 out of range."));

    int ref = sol3_newSolution("h2o2.yaml", "ohmech", "default");
    sol3_del(ref);
    int thermo = sol3_thermo(ref);
    EXPECT_EQ(thermo, -2);
    err = reportError();
    EXPECT_THAT(err, HasSubstr("has been deleted."));

    ct3_resetStorage();
    ret = sol3_del(0);
    ASSERT_EQ(ret, -1);
    err = reportError();
    EXPECT_THAT(err, HasSubstr("Index 0 out of range."));
}

TEST(ct3, new_solution)
{
    ct3_resetStorage();

    string name = "ohmech";
    int ref = sol3_newSolution("h2o2.yaml", name.c_str(), "default");
    ASSERT_EQ(ref, 0);

    ASSERT_EQ(sol3_cabinetSize(), 1);
    ASSERT_EQ(thermo3_cabinetSize(), 1);
    ASSERT_EQ(kin3_cabinetSize(), 1);

    int buflen = sol3_name(ref, 0, 0); // includes \0
    ASSERT_EQ(buflen, int(name.size() + 1));

    int thermo = sol3_thermo(ref);
    ASSERT_EQ(thermo3_parentHandle(thermo), ref);

    vector<char> buf(buflen);
    sol3_name(ref, buflen, buf.data());
    string solName(buf.data());
    ASSERT_EQ(solName, name);
}

TEST(ct3, sol3_objects)
{
    ct3_resetStorage();

    int ref = sol3_newSolution("gri30.yaml", "gri30", "none");
    ASSERT_EQ(ref, 0);
    ASSERT_EQ(thermo3_cabinetSize(), 1); // one ThermoPhase object

    int ref2 = sol3_newSolution("h2o2.yaml", "ohmech", "default");
    ASSERT_EQ(ref2, 1);
    ASSERT_EQ(thermo3_cabinetSize(), 2); // two ThermoPhase objects

    int thermo = sol3_thermo(ref);
    ASSERT_EQ(thermo3_parentHandle(thermo), ref);

    int thermo2 = sol3_thermo(ref2);
    ASSERT_EQ(thermo2, 1); // references stored object with index '1'
    ASSERT_EQ(thermo3_nSpecies(thermo2), 10u);
    ASSERT_EQ(thermo3_parentHandle(thermo2), ref2);

    int kin = sol3_kinetics(ref);

    int kin2 = sol3_kinetics(ref2);
    ASSERT_EQ(kin2, 1);
    ASSERT_EQ(kin3_nReactions(kin2), 29u);
    ASSERT_EQ(kin3_parentHandle(kin2), ref2);
    ASSERT_EQ(kin3_parentHandle(kin), ref);

    int trans = sol3_transport(ref);
    ASSERT_EQ(trans3_parentHandle(trans), ref);

    int trans2 = sol3_transport(ref2);
    ASSERT_EQ(trans2, 1);
    int buflen = trans3_transportModel(trans2, 0, 0);
    vector<char> buf(buflen);
    trans3_transportModel(trans2, buflen, buf.data());
    string trName(buf.data());
    ASSERT_EQ(trName, "mixture-averaged");
    ASSERT_EQ(trans3_parentHandle(trans2), ref2);

    sol3_del(ref2);
    int nsp = thermo3_nSpecies(thermo2);
    ASSERT_EQ(nsp, ERR);
    string err = reportError();
    EXPECT_THAT(err, HasSubstr("has been deleted."));

    nsp = thermo3_nSpecies(thermo2);
    ASSERT_EQ(nsp, ERR);
    err = reportError();
    EXPECT_THAT(err, HasSubstr("has been deleted."));

    // trans2 = sol3_setTransportModel(ref, "mixture-averaged");
    // ASSERT_EQ(trans2, 2);
    // buflen = trans3_transportModel(trans2, 0, 0);
    // buf.resize(buflen);
    // trans3_transportModel(trans2, buflen, buf.data());
    // trName = buf.data();
    // ASSERT_EQ(trName, "mixture-averaged");
}

TEST(ct3, new_interface)
{
    ct3_resetStorage();

    int sol = sol3_newSolution("ptcombust.yaml", "gas", "none");
    ASSERT_EQ(sol, 0);

    vector<int> adj{sol};
    int surf = sol3_newInterface("ptcombust.yaml", "Pt_surf", 1, adj.data());
    ASSERT_EQ(surf, 1);

    int ph_surf = sol3_thermo(surf);
    int buflen = sol3_name(ph_surf, 0, 0) + 1; // include \0
    vector<char> buf(buflen);
    sol3_name(ph_surf, buflen, buf.data());
    string solName(buf.data());
    ASSERT_EQ(solName, "Pt_surf");

    int kin3_surf = sol3_kinetics(surf);
    buflen = kin3_kineticsType(kin3_surf, 0, 0) + 1; // include \0
    buf.resize(buflen);
    kin3_kineticsType(ph_surf, buflen, buf.data());
    string kinType(buf.data());
    ASSERT_EQ(kinType, "surface");
}

TEST(ct3, new_interface_auto)
{
    ct3_resetStorage();

    vector<int> adj;
    int surf = sol3_newInterface("ptcombust.yaml", "Pt_surf", 0, adj.data());
    ASSERT_EQ(surf, 0);

    ASSERT_EQ(sol3_nAdjacent(surf), 1u);
    int gas = sol3_adjacent(surf, 0);
    ASSERT_EQ(gas, 1);

    int buflen = sol3_name(gas, 0, 0) + 1; // include \0
    vector<char> buf(buflen);
    sol3_name(gas, buflen, buf.data());
    string solName(buf.data());
    ASSERT_EQ(solName, "gas");
}

TEST(ct3, thermo)
{
    int ret;
    int sol = sol3_newSolution("gri30.yaml", "gri30", "none");
    int thermo = sol3_thermo(sol);
    ASSERT_GE(thermo, 0);
    size_t nsp = thermo3_nSpecies(thermo);
    ASSERT_EQ(nsp, 53u);

    ret = thermo3_setTemperature(thermo, 500);
    ASSERT_EQ(ret, 0);
    ret = thermo3_setPressure(thermo, 5 * 101325);
    ASSERT_EQ(ret, 0);
    ret = thermo3_setMoleFractionsByName(thermo, "CH4:1.0, O2:2.0, N2:7.52");
    ASSERT_EQ(ret, 0);

    ret = thermo3_equilibrate(thermo, "HP", "auto", 1e-9, 50000, 1000, 0);
    ASSERT_EQ(ret, 0);
    double T = thermo3_temperature(thermo);
    ASSERT_GT(T, 2200);
    ASSERT_LT(T, 2300);

    size_t ns = thermo3_nSpecies(thermo);
    vector<double> work(ns);
    vector<double> X(ns);
    thermo3_getMoleFractions(thermo, ns, X.data());

    thermo3_getPartialMolarEnthalpies(thermo, ns, work.data());
    double prod = std::inner_product(X.begin(), X.end(), work.begin(), 0.0);
    ASSERT_NEAR(prod, thermo3_enthalpy_mole(thermo), 1e-6);

    thermo3_getPartialMolarEntropies(thermo, ns, work.data());
    prod = std::inner_product(X.begin(), X.end(), work.begin(), 0.0);
    ASSERT_NEAR(prod, thermo3_entropy_mole(thermo), 1e-6);

    thermo3_getPartialMolarIntEnergies(thermo, ns, work.data());
    prod = std::inner_product(X.begin(), X.end(), work.begin(), 0.0);
    ASSERT_NEAR(prod, thermo3_intEnergy_mole(thermo), 1e-6);

    thermo3_getPartialMolarCp(thermo, ns, work.data());
    prod = std::inner_product(X.begin(), X.end(), work.begin(), 0.0);
    ASSERT_NEAR(prod, thermo3_cp_mole(thermo), 1e-6);

    thermo3_getPartialMolarVolumes(thermo, ns, work.data());
    prod = std::inner_product(X.begin(), X.end(), work.begin(), 0.0);
    ASSERT_NEAR(prod, 1./thermo3_molarDensity(thermo), 1e-6);
}

TEST(ct3, kinetics)
{
    int sol0 = sol3_newSolution("gri30.yaml", "gri30", "none");
    int thermo = sol3_thermo(sol0);
    int kin = sol3_kinetics(sol0);
    ASSERT_GE(kin, 0);

    size_t nr = kin3_nReactions(kin);
    ASSERT_EQ(nr, 325u);

    thermo3_equilibrate(thermo, "HP", "auto", 1e-9, 50000, 1000, 0);
    double T = thermo3_temperature(thermo);
    thermo3_setTemperature(thermo, T - 200);

    auto sol = newSolution("gri30.yaml", "gri30", "none");
    auto phase = sol->thermo();
    auto kinetics = sol->kinetics();

    phase->equilibrate("HP");
    ASSERT_NEAR(T, phase->temperature(), 1e-2);
    phase->setTemperature(T - 200);

    vector<double> c_ropf(nr);
    kin3_getFwdRatesOfProgress(kin, 325, c_ropf.data());
    vector<double> cpp_ropf(nr);
    kinetics->getFwdRatesOfProgress(cpp_ropf.data());

    for (size_t n = 0; n < nr; n++) {
        ASSERT_NEAR(cpp_ropf[n], c_ropf[n], 1e-6);
    }
}

TEST(ct3, transport)
{
    int sol0 = sol3_newSolution("gri30.yaml", "gri30", "default");
    int thermo = sol3_thermo(sol0);
    int tran = sol3_transport(sol0);

    size_t nsp = thermo3_nSpecies(thermo);
    vector<double> c_dkm(nsp);
    int ret = trans3_getMixDiffCoeffs(tran, 53, c_dkm.data());
    ASSERT_EQ(ret, 0);

    vector<double> cpp_dkm(nsp);
    auto sol = newSolution("gri30.yaml", "gri30");
    auto transport = sol->transport();
    transport->getMixDiffCoeffs(cpp_dkm.data());

    for (size_t n = 0; n < nsp; n++) {
        ASSERT_NEAR(cpp_dkm[n], c_dkm[n], 1e-10);
    }
}


int main(int argc, char** argv)
{
    printf("Running main() from test_clib3.cpp\n");
    testing::InitGoogleTest(&argc, argv);
    make_deprecation_warnings_fatal();
    printStackTraceOnSegfault();
    Cantera::CanteraError::setStackTraceDepth(20);
    // vector<string> fileNames = {"gtest-freeflame.yaml", "gtest-freeflame.h5"};
    // for (const auto& fileName : fileNames) {
    //     if (std::ifstream(fileName).good()) {
    //         std::remove(fileName.c_str());
    //     }
    // }
    int result = RUN_ALL_TESTS();
    appdelete();
    return result;
}