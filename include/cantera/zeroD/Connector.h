//! @file Connector.h

// This file is part of Cantera. See License.txt in the top-level directory or
// at https://cantera.org/license.txt for license and copyright information.

#ifndef CT_CONNECTOR_H
#define CT_CONNECTOR_H

#include "cantera/base/ct_defs.h"
#include "cantera/base/global.h"

namespace Cantera
{
class ReactorBase;

/**
 * Base class for walls and flow devices connecting reactors.
 * In a reactor network, walls and flow devices (valves, pressure regulators, etc.)
 * form edges of a directed graph that connect reactors that form nodes.
 *
 * @since New in %Cantera 3.1.
 *
 * @ingroup connectorGroup
 */
class Connector
{
public:
    //! Transitional constructor.
    //! @todo  Implement deprecation warning.
    Connector(const string& name="(none)") : m_name(name) {}

    //! Instantiate a Connector object with associated ReactorBase objects.
    //! @param r0  First reactor.
    //! @param r1  Second reactor.
    //! @param name  Name of the connector.
    Connector(shared_ptr<ReactorBase> r0, shared_ptr<ReactorBase> r1,
              const string& name="(none)") : m_nodes({r0, r1}), m_name(name) {}

    virtual ~Connector() = default;
    Connector(const Connector&) = delete;
    Connector& operator=(const Connector&) = delete;

    //! String indicating the connector implemented. Usually
    //! corresponds to the name of the derived class.
    virtual string type() const {
        return "Connector";
    }

    //! Retrieve connector name.
    string name() const {
        return m_name;
    }

    //! Set connector name.
    void setName(const string& name) {
        m_name = name;
    }

    //! Set the default name of a connector. Returns `false` if it was previously set.
    void setDefaultName(map<string, int>& counts);

protected:
    //! Pair of reactors forming end points of the connector.
    pair<shared_ptr<ReactorBase>, shared_ptr<ReactorBase>> m_nodes;

    string m_name;  //!< Connector name.
    bool m_defaultNameSet = false;  //!< `true` if default name has been previously set.
};

}

#endif
