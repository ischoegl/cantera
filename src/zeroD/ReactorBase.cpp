
//! @file ReactorBase.cpp

// This file is part of Cantera. See License.txt in the top-level directory or
// at https://cantera.org/license.txt for license and copyright information.

#include "cantera/zeroD/ReactorBase.h"
#include "cantera/zeroD/FlowDevice.h"
#include "cantera/zeroD/ReactorNet.h"
#include "cantera/zeroD/ReactorSurface.h"
#include "cantera/thermo/SurfPhase.h"
#include "cantera/base/yaml.h"

using namespace std;
namespace Cantera
{

ReactorBase::ReactorBase(const string& name) :
    m_nsp(0),
    m_thermo(0),
    m_kin(0),
    m_vol(1.0),
    m_enthalpy(0.0),
    m_intEnergy(0.0),
    m_pressure(0.0),
    m_net(0)
{
    m_name = name;
}

void ReactorBase::setThermoMgr(ThermoPhase& thermo)
{
    m_thermo = &thermo;
    m_nsp = m_thermo->nSpecies();
    m_thermo->saveState(m_state);
    m_enthalpy = m_thermo->enthalpy_mass();
    m_intEnergy = m_thermo->intEnergy_mass();
    m_pressure = m_thermo->pressure();
}

void ReactorBase::syncState()
{
    m_thermo->saveState(m_state);
    m_enthalpy = m_thermo->enthalpy_mass();
    m_intEnergy = m_thermo->intEnergy_mass();
    m_pressure = m_thermo->pressure();
    if (m_net) {
        m_net->setNeedsReinit();
    }
}

std::string ReactorBase::toYAML() const
{
    YAML::Emitter yml;
    std::stringstream out;

    yml << YAML::BeginMap;
    yml << YAML::Key << name();
    yml << YAML::BeginMap;
    yml << YAML::Key << "type";
    yml << YAML::Value << typeStr();
    yml << YAML::Key << "phases" << YAML::Flow;
    yml << YAML::BeginSeq;
    yml << m_thermo->name();
    for (const auto& s : m_surfaces) {
        yml << s->thermo()->name();
    }
    yml << YAML::EndSeq;
    yml << YAML::EndMap;
    yml << YAML::EndMap;

    out << yml.c_str();
    return out.str();
}

void ReactorBase::addInlet(FlowDevice& inlet)
{
    m_inlet.push_back(&inlet);
}

void ReactorBase::addOutlet(FlowDevice& outlet)
{
    m_outlet.push_back(&outlet);
}

void ReactorBase::addWall(WallBase& w, int lr)
{
    m_wall.push_back(&w);
    if (lr == 0) {
        m_lr.push_back(0);
    } else {
        m_lr.push_back(1);
    }
}

WallBase& ReactorBase::wall(size_t n)
{
    return *m_wall[n];
}

void ReactorBase::addSurface(ReactorSurface* surf)
{
    if (find(m_surfaces.begin(), m_surfaces.end(), surf) == m_surfaces.end()) {
        m_surfaces.push_back(surf);
        surf->setReactor(this);
    }
}

ReactorSurface* ReactorBase::surface(size_t n)
{
    return m_surfaces[n];
}

ReactorNet& ReactorBase::network()
{
    if (m_net) {
        return *m_net;
    } else {
        throw CanteraError("ReactorBase::network",
                           "Reactor is not part of a ReactorNet");
    }
}

void ReactorBase::setNetwork(ReactorNet* net)
{
    m_net = net;
}

doublereal ReactorBase::residenceTime()
{
    doublereal mout = 0.0;
    for (size_t i = 0; i < m_outlet.size(); i++) {
        mout += m_outlet[i]->massFlowRate();
    }
    return mass()/mout;
}

FlowDevice& ReactorBase::inlet(size_t n)
{
    return *m_inlet[n];
}
FlowDevice& ReactorBase::outlet(size_t n)
{
    return *m_outlet[n];
}

}
