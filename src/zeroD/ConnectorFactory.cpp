//! @file ConnectorFactory.cpp

// This file is part of Cantera. See License.txt in the top-level directory or
// at https://cantera.org/license.txt for license and copyright information.

#include "cantera/zeroD/ConnectorFactory.h"
#include "cantera/zeroD/FlowDevice.h"
#include "cantera/zeroD/flowControllers.h"
#include "cantera/zeroD/Wall.h"

namespace Cantera
{

ConnectorFactory* ConnectorFactory::s_factory = 0;
std::mutex ConnectorFactory::connector_mutex;

ConnectorFactory::ConnectorFactory()
{
    reg("MassFlowController",
        [](shared_ptr<ReactorNode> r0, shared_ptr<ReactorNode> r1, const string& name)
        { return new MassFlowController(r0, r1, name); });
    reg("PressureController",
        [](shared_ptr<ReactorNode> r0, shared_ptr<ReactorNode> r1, const string& name)
        { return new PressureController(r0, r1, name); });
    reg("Valve",
        [](shared_ptr<ReactorNode> r0, shared_ptr<ReactorNode> r1, const string& name)
        { return new Valve(r0, r1, name); });
    reg("Wall",
        [](shared_ptr<ReactorNode> r0, shared_ptr<ReactorNode> r1, const string& name)
        { return new Wall(r0, r1, name); });
}

ConnectorFactory* ConnectorFactory::factory() {
    std::unique_lock<std::mutex> lock(connector_mutex);
    if (!s_factory) {
        s_factory = new ConnectorFactory;
    }
    return s_factory;
}

void ConnectorFactory::deleteFactory() {
    std::unique_lock<std::mutex> lock(connector_mutex);
    delete s_factory;
    s_factory = 0;
}

shared_ptr<Connector> newConnector(
    const string& model,
    shared_ptr<ReactorNode> r0, shared_ptr<ReactorNode> r1, const string& name)
{
    return shared_ptr<Connector>(
        ConnectorFactory::factory()->create(model, r0, r1, name));
}

shared_ptr<FlowDevice> newFlowDevice(const string& model, const string& name)
{
    auto dev = std::dynamic_pointer_cast<FlowDevice>(
        newConnector(model, nullptr, nullptr, name));
    if (!dev) {
        throw CanteraError("newFlowDevice",
            "Detected incompatible Connector type '{}'", model);
    }
    return dev;
}

shared_ptr<FlowDevice> newFlowDevice3(const string& model)
{
    warn_deprecated("newFlowDevice3",
        "Use newFlowDevice instead; to be removed after Cantera 3.1.");
    return newFlowDevice(model);
}

shared_ptr<WallBase> newWall(const string& model, const string& name)
{
    auto wall = std::dynamic_pointer_cast<WallBase>(
        newConnector(model, nullptr, nullptr, name));
    if (!wall) {
        throw CanteraError("newWall",
            "Detected incompatible Connector type '{}'", model);
    }
    return wall;
}

shared_ptr<WallBase> newWall3(const string& model)
{
    warn_deprecated("newWall3",
        "Use newWall instead; to be removed after Cantera 3.1.");
    return newWall(model);
}

}