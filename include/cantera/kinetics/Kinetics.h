/**
 * @file Kinetics.h
 *  Base class for kinetics managers and also contains the kineticsmgr
 *  module documentation (see @ref  kineticsmgr and class
 *  @link Cantera::Kinetics Kinetics@endlink).
 */

// This file is part of Cantera. See License.txt in the top-level directory or
// at https://cantera.org/license.txt for license and copyright information.

#ifndef CT_KINETICS_H
#define CT_KINETICS_H

#include "StoichManager.h"
#include "cantera/base/ValueCache.h"
#include "MultiRate.h"

namespace Cantera
{

class ThermoPhase;
class Reaction;
class Solution;
class AnyMap;

//! @defgroup derivGroup Derivative Calculations
//! @details Methods for calculating analytical and/or numerical derivatives.

/**
 * @defgroup chemkinetics Chemical Kinetics
 */

//! @defgroup reactionGroup Reactions and Reaction Rates
//! Classes for handling reactions and reaction rates.
//! @ingroup chemkinetics

//! @defgroup kineticsmgr Kinetics Managers
//! Classes implementing models for chemical kinetics.
//! @section kinmodman Models and Managers
//!
//! A kinetics manager is a C++ class that implements a kinetics model; a
//! kinetics model is a set of mathematical equation describing how various
//! kinetic quantities are to be computed -- reaction rates, species production
//! rates, etc. Many different kinetics models might be defined to handle
//! different types of kinetic processes. For example, one kinetics model might
//! use expressions valid for elementary reactions in ideal gas mixtures. It
//! might, for example, require the reaction orders to be integral and equal to
//! the forward stoichiometric coefficients, require that each reaction be
//! reversible with a reverse rate satisfying detailed balance, include
//! pressure-dependent unimolecular reactions, etc. Another kinetics model might
//! be designed for heterogeneous chemistry at interfaces, and might allow
//! empirical reaction orders, coverage-dependent activation energies,
//! irreversible reactions, and include effects of potential differences across
//! the interface on reaction rates.
//!
//! A kinetics manager implements a kinetics model. Since the model equations
//! may be complex and expensive to evaluate, a kinetics manager may adopt
//! various strategies to 'manage' the computation and evaluate the expressions
//! efficiently. For example, if there are rate coefficients or other quantities
//! that depend only on temperature, a manager class may choose to store these
//! quantities internally, and re-evaluate them only when the temperature has
//! actually changed. Or a manager designed for use with reaction mechanisms
//! with a few repeated activation energies might precompute the terms @f$
//! \exp(-E/RT) @f$, instead of evaluating the exponential repeatedly for each
//! reaction. There are many other possible 'management styles', each of which
//! might be better suited to some reaction mechanisms than others.
//!
//! But however a manager structures the internal computation, the tasks the
//! manager class must perform are, for the most part, the same. It must be able
//! to compute reaction rates, species production rates, equilibrium constants,
//! etc. Therefore, all kinetics manager classes should have a common set of
//! public methods, but differ in how they implement these methods.
//!
//! A kinetics manager computes reaction rates of progress, species production
//! rates, equilibrium constants, and similar quantities for a reaction
//! mechanism. All kinetics manager classes derive from class Kinetics, which
//! defines a common public interface for all kinetics managers. Each derived
//! class overloads the virtual methods of Kinetics to implement a particular
//! kinetics model.
//!
//! For example, class BulkKinetics implements reaction rate expressions appropriate for
//! homogeneous reactions, and class InterfaceKinetics implements expressions
//! appropriate for heterogeneous mechanisms at interfaces, including how to handle
//! reactions involving charged species of phases with different electric potentials ---
//! something that class BulkKinetics doesn't deal with at all.
//!
//! Many of the methods of class Kinetics write into arrays the values of some
//! quantity for each species, for example the net production rate. These
//! methods always write the results into flat arrays, ordered by phase in the
//! order the phase was added, and within a phase in the order the species were
//! added to the phase (which is the same ordering as in the input file).
//! Example: suppose a heterogeneous mechanism involves three phases -- a bulk
//! phase 'a', another bulk phase 'b', and the surface phase 'a:b' at the a/b
//! interface. Phase 'a' contains 12 species, phase 'b' contains 3, and at the
//! interface there are 5 adsorbed species defined in phase 'a:b'. Then methods
//! like getNetProductionRates(double* net) will write and output array of
//! length 20, beginning at the location pointed to by 'net'. The first 12
//! values will be the net production rates for all 12 species of phase 'a'
//! (even if some do not participate in the reactions), the next 3 will be for
//! phase 'b', and finally the net production rates for the surface species will
//! occupy the last 5 locations.
//! @ingroup chemkinetics

//! @defgroup rateEvaluators Rate Evaluators
//! These classes are used to evaluate the rates of reactions.
//! @ingroup chemkinetics


//! Public interface for kinetics managers.
/*!
 * This class serves as a base class to derive 'kinetics managers', which are
 * classes that manage homogeneous chemistry within one phase, or heterogeneous
 * chemistry at one interface. The virtual methods of this class are meant to be
 * overloaded in subclasses. The non-virtual methods perform generic functions
 * and are implemented in Kinetics. They should not be overloaded. Only those
 * methods required by a subclass need to be overloaded; the rest will throw
 * exceptions if called.
 *
 * When the nomenclature "kinetics species index" is used below, this means that
 * the species index ranges over all species in all phases handled by the
 * kinetics manager.
 *
 *  @ingroup kineticsmgr
 */
class Kinetics
{
public:
    //! @name Constructors and General Information about Mechanism
    //! @{

    //! Default constructor.
    Kinetics() = default;

    virtual ~Kinetics() = default;

    //! Kinetics objects are not copyable or assignable
    Kinetics(const Kinetics&) = delete;
    Kinetics& operator=(const Kinetics&)= delete;

    //! Identifies the Kinetics manager type.
    //! Each class derived from Kinetics should override this method to return
    //! a meaningful identifier.
    //! @since Starting in %Cantera 3.0, the name returned by this method corresponds
    //!     to the canonical name used in the YAML input format.
    virtual string kineticsType() const {
        return "none";
    }

    //! Finalize Kinetics object and associated objects
    virtual void resizeReactions();

    //! Number of reactions in the reaction mechanism.
    size_t nReactions() const {
        return m_reactions.size();
    }

    //! Check that the specified reaction index is in range
    //! Throws an exception if i is greater than nReactions()
    void checkReactionIndex(size_t m) const;

    //! Check that an array size is at least nReactions()
    //! Throws an exception if ii is less than nReactions(). Used before calls
    //! which take an array pointer.
    void checkReactionArraySize(size_t ii) const;

    //! Check that the specified species index is in range
    //! Throws an exception if k is greater than nSpecies()-1
    void checkSpeciesIndex(size_t k) const;

    //! Check that an array size is at least nSpecies()
    //! Throws an exception if kk is less than nSpecies(). Used before calls
    //! which take an array pointer.
    void checkSpeciesArraySize(size_t mm) const;

    //! @}
    //! @name Information/Lookup Functions about Phases and Species
    //! @{

    /**
     * The number of phases participating in the reaction mechanism. For a
     * homogeneous reaction mechanism, this will always return 1, but for a
     * heterogeneous mechanism it will return the total number of phases in the
     * mechanism.
     */
    size_t nPhases() const {
        return m_thermo.size();
    }

    //! Check that the specified phase index is in range
    //! Throws an exception if m is greater than nPhases()
    void checkPhaseIndex(size_t m) const;

    //! Check that an array size is at least nPhases()
    //! Throws an exception if mm is less than nPhases(). Used before calls
    //! which take an array pointer.
    void checkPhaseArraySize(size_t mm) const;

    /**
     * Return the phase index of a phase in the list of phases defined within
     * the object.
     *
     *  @param ph string name of the phase
     *
     * If a -1 is returned, then the phase is not defined in the Kinetics
     * object.
     */
    size_t phaseIndex(const string& ph) const {
        if (m_phaseindex.find(ph) == m_phaseindex.end()) {
            return npos;
        } else {
            return m_phaseindex.at(ph) - 1;
        }
    }

    /**
     * Return pointer to phase where the reactions occur.
     * @since New in %Cantera 3.0
     */
    shared_ptr<ThermoPhase> reactionPhase() const;

    /**
     * Return pointer to phase associated with Kinetics by index.
     * @param n Index of the ThermoPhase being sought.
     * @since New in %Cantera 3.2.
     * @see thermo
     */
    shared_ptr<ThermoPhase> phase(size_t n=0) const {
        return m_thermo[n];
    }

    /**
     * This method returns a reference to the nth ThermoPhase object defined
     * in this kinetics mechanism. It is typically used so that member
     * functions of the ThermoPhase object may be called. For homogeneous
     * mechanisms, there is only one object, and this method can be called
     * without an argument to access it.
     *
     * @param n Index of the ThermoPhase being sought.
     */
    ThermoPhase& thermo(size_t n=0) {
        return *m_thermo[n];
    }
    const ThermoPhase& thermo(size_t n=0) const {
        return *m_thermo[n];
    }

    /**
     * The total number of species in all phases participating in the kinetics
     * mechanism. This is useful to dimension arrays for use in calls to
     * methods that return the species production rates, for example.
     */
    size_t nTotalSpecies() const {
        return m_kk;
    }

    /**
     * The location of species k of phase n in species arrays. Kinetics manager
     * classes return species production rates in flat arrays, with the species
     * of each phases following one another, in the order the phases were added.
     * This method is useful to find the value for a particular species of a
     * particular phase in arrays returned from methods like getCreationRates
     * that return an array of species-specific quantities.
     *
     * Example: suppose a heterogeneous mechanism involves three phases. The
     * first contains 12 species, the second 26, and the third 3. Then species
     * arrays must have size at least 41, and positions 0 - 11 are the values
     * for the species in the first phase, positions 12 - 37 are the values for
     * the species in the second phase, etc. Then kineticsSpeciesIndex(7, 0) =
     * 7, kineticsSpeciesIndex(4, 1) = 16, and kineticsSpeciesIndex(2, 2) = 40.
     *
     * @param k species index
     * @param n phase index for the species
     */
    size_t kineticsSpeciesIndex(size_t k, size_t n) const {
        return m_start[n] + k;
    }

    //! Return the name of the kth species in the kinetics manager.
    /*!
     *  k is an integer from 0 to ktot - 1, where ktot is the number of
     * species in the kinetics manager, which is the sum of the number of
     * species in all phases participating in the kinetics manager. If k is
     * out of bounds, the string "<unknown>" is returned.
     *
     * @param k species index
     */
    string kineticsSpeciesName(size_t k) const;

    /**
     * This routine will look up a species number based on the input
     * string nm. The lookup of species will occur for all phases
     * listed in the kinetics object.
     *
     *  return
     *   - If a match is found, the position in the species list is returned.
     *   - If no match is found, the value -1 is returned.
     *
     * @param nm   Input string name of the species
     */
    size_t kineticsSpeciesIndex(const string& nm) const;

    /**
     * This function looks up the name of a species and returns a
     * reference to the ThermoPhase object of the phase where the species
     * resides. Will throw an error if the species doesn't match.
     *
     * @param nm   String containing the name of the species.
     */
    ThermoPhase& speciesPhase(const string& nm);
    const ThermoPhase& speciesPhase(const string& nm) const;

    /**
     * This function takes as an argument the kineticsSpecies index
     * (that is, the list index in the list of species in the kinetics
     * manager) and returns the species' owning ThermoPhase object.
     *
     * @param k          Species index
     */
    ThermoPhase& speciesPhase(size_t k) {
        return thermo(speciesPhaseIndex(k));
    }

    /**
     * This function takes as an argument the kineticsSpecies index (that is, the
     * list index in the list of species in the kinetics manager) and returns
     * the index of the phase owning the species.
     *
     * @param k          Species index
     */
    size_t speciesPhaseIndex(size_t k) const;

    //! @}
    //! @name Reaction Rates Of Progress
    //! @{

    //!  Return the forward rates of progress of the reactions
    /*!
     * Forward rates of progress. Return the forward rates of
     * progress in array fwdROP, which must be dimensioned at
     * least as large as the total number of reactions.
     *
     * @param fwdROP  Output vector containing forward rates
     *                of progress of the reactions. Length: nReactions().
     */
    virtual void getFwdRatesOfProgress(double* fwdROP);

    //!  Return the Reverse rates of progress of the reactions
    /*!
     * Return the reverse rates of progress in array revROP, which must be
     * dimensioned at least as large as the total number of reactions.
     *
     * @param revROP  Output vector containing reverse rates
     *                of progress of the reactions. Length: nReactions().
     */
    virtual void getRevRatesOfProgress(double* revROP);

    /**
     * Net rates of progress. Return the net (forward - reverse) rates of
     * progress in array netROP, which must be dimensioned at least as large
     * as the total number of reactions.
     *
     * @param netROP  Output vector of the net ROP. Length: nReactions().
     */
    virtual void getNetRatesOfProgress(double* netROP);

    //! Return a vector of Equilibrium constants.
    /*!
     *  Return the equilibrium constants of the reactions in concentration
     *  units in array kc, which must be dimensioned at least as large as the
     *  total number of reactions.
     *
     * @f[
     *       Kc_i = \exp [ \Delta G_{ss,i} ] \prod(Cs_k) \exp(\sum_k \nu_{k,i} F \phi_n)
     * @f]
     *
     * @param kc   Output vector containing the equilibrium constants.
     *             Length: nReactions().
     */
    virtual void getEquilibriumConstants(double* kc) {
        throw NotImplementedError("Kinetics::getEquilibriumConstants");
    }

    /**
     * Change in species properties. Given an array of molar species property
     * values @f$ z_k, k = 1, \dots, K @f$, return the array of reaction values
     * @f[
     *    \Delta Z_i = \sum_k \nu_{k,i} z_k, i = 1, \dots, I.
     * @f]
     * For example, if this method is called with the array of standard-state
     * molar Gibbs free energies for the species, then the values returned in
     * array @c deltaProperty would be the standard-state Gibbs free energies of
     * reaction for each reaction.
     *
     * @param property Input vector of property value. Length: #m_kk.
     * @param deltaProperty Output vector of deltaRxn. Length: nReactions().
     */
    virtual void getReactionDelta(const double* property, double* deltaProperty) const;

    /**
     * Given an array of species properties 'g', return in array 'dg' the
     * change in this quantity in the reversible reactions. Array 'g' must
     * have a length at least as great as the number of species, and array
     * 'dg' must have a length as great as the total number of reactions.
     * This method only computes 'dg' for the reversible reactions, and the
     * entries of 'dg' for the irreversible reactions are unaltered. This is
     * primarily designed for use in calculating reverse rate coefficients
     * from thermochemistry for reversible reactions.
     */
    virtual void getRevReactionDelta(const double* g, double* dg) const;

    //! Return the vector of values for the reaction Gibbs free energy change.
    /*!
     * (virtual from Kinetics.h)
     * These values depend upon the concentration of the solution.
     *
     *  units = J kmol-1
     *
     * @param deltaG  Output vector of deltaG's for reactions Length:
     *     nReactions().
     */
    virtual void getDeltaGibbs(double* deltaG) {
        throw NotImplementedError("Kinetics::getDeltaGibbs");
    }

    //! Return the vector of values for the reaction electrochemical free
    //! energy change.
    /*!
     * These values depend upon the concentration of the solution and the
     * voltage of the phases
     *
     *  units = J kmol-1
     *
     * @param deltaM  Output vector of deltaM's for reactions Length:
     *     nReactions().
     */
    virtual void getDeltaElectrochemPotentials(double* deltaM) {
        throw NotImplementedError("Kinetics::getDeltaElectrochemPotentials");
    }

    /**
     * Return the vector of values for the reactions change in enthalpy.
     * These values depend upon the concentration of the solution.
     *
     *  units = J kmol-1
     *
     * @param deltaH  Output vector of deltaH's for reactions Length:
     *     nReactions().
     */
    virtual void getDeltaEnthalpy(double* deltaH) {
        throw NotImplementedError("Kinetics::getDeltaEnthalpy");
    }

    /**
     * Return the vector of values for the reactions change in entropy. These
     * values depend upon the concentration of the solution.
     *
     *  units = J kmol-1 Kelvin-1
     *
     * @param deltaS  Output vector of deltaS's for reactions Length:
     *     nReactions().
     */
    virtual void getDeltaEntropy(double* deltaS) {
        throw NotImplementedError("Kinetics::getDeltaEntropy");
    }

    /**
     * Return the vector of values for the reaction standard state Gibbs free
     * energy change. These values don't depend upon the concentration of the
     * solution.
     *
     *  units = J kmol-1
     *
     * @param deltaG  Output vector of ss deltaG's for reactions Length:
     *     nReactions().
     */
    virtual void getDeltaSSGibbs(double* deltaG) {
        throw NotImplementedError("Kinetics::getDeltaSSGibbs");
    }

    /**
     * Return the vector of values for the change in the standard state
     * enthalpies of reaction. These values don't depend upon the concentration
     * of the solution.
     *
     *  units = J kmol-1
     *
     * @param deltaH  Output vector of ss deltaH's for reactions Length:
     *     nReactions().
     */
    virtual void getDeltaSSEnthalpy(double* deltaH) {
        throw NotImplementedError("Kinetics::getDeltaSSEnthalpy");
    }

    /**
     * Return the vector of values for the change in the standard state
     * entropies for each reaction. These values don't depend upon the
     * concentration of the solution.
     *
     *  units = J kmol-1 Kelvin-1
     *
     * @param deltaS  Output vector of ss deltaS's for reactions Length:
     *     nReactions().
     */
    virtual void getDeltaSSEntropy(double* deltaS) {
        throw NotImplementedError("Kinetics::getDeltaSSEntropy");
    }

    /**
     * Return a vector of values of effective concentrations of third-body
     * collision partners of any reaction. Entries for reactions not involving
     * third-body collision partners are not defined and set to not-a-number.
     *
     * @param concm  Output vector of effective third-body concentrations.
     *      Length: nReactions().
     */
    virtual void getThirdBodyConcentrations(double* concm) {
        throw NotImplementedError("Kinetics::getThirdBodyConcentrations",
            "Not applicable/implemented for Kinetics object of type '{}'",
            kineticsType());
    }

    /**
     * Provide direct access to current third-body concentration values.
     * @see getThirdBodyConcentrations.
     */
    virtual const vector<double>& thirdBodyConcentrations() const {
        throw NotImplementedError("Kinetics::thirdBodyConcentrations",
            "Not applicable/implemented for Kinetics object of type '{}'",
            kineticsType());
    }

    //! @}
    //! @name Species Production Rates
    //! @{

    /**
     * Species creation rates [kmol/m^3/s or kmol/m^2/s]. Return the species
     * creation rates in array cdot, which must be dimensioned at least as
     * large as the total number of species in all phases. @see nTotalSpecies.
     *
     * @param cdot   Output vector of creation rates. Length: #m_kk.
     */
    virtual void getCreationRates(double* cdot);

    /**
     * Species destruction rates [kmol/m^3/s or kmol/m^2/s]. Return the species
     * destruction rates in array ddot, which must be dimensioned at least as
     * large as the total number of species. @see nTotalSpecies.
     *
     * @param ddot   Output vector of destruction rates. Length: #m_kk.
     */
    virtual void getDestructionRates(double* ddot);

    /**
     * Species net production rates [kmol/m^3/s or kmol/m^2/s]. Return the
     * species net production rates (creation - destruction) in array wdot,
     * which must be dimensioned at least as large as the total number of
     * species. @see nTotalSpecies.
     *
     * @param wdot   Output vector of net production rates. Length: #m_kk.
     */
    virtual void getNetProductionRates(double* wdot);

    //! @}

    //! @addtogroup derivGroup
    //! @{

    /**
     * @anchor kinDerivs
     * @par Routines to Calculate Kinetics Derivatives (Jacobians)
     * @name
     *
     * Kinetics derivatives are calculated with respect to temperature, pressure,
     * molar concentrations and species mole fractions for forward/reverse/net rates
     * of progress as well as creation/destruction and net production of species.
     *
     * The following suffixes are used to indicate derivatives:
     *  - `_ddT`: derivative with respect to temperature (a vector)
     *  - `_ddP`: derivative with respect to pressure (a vector)
     *  - `_ddC`: derivative with respect to molar concentration (a vector)
     *  - `_ddX`: derivative with respect to species mole fractions (a matrix)
     *  - `_ddCi`: derivative with respect to species concentrations (a matrix)
     *
     * @since New in Cantera 2.6
     *
     * @warning The calculation of kinetics derivatives is an experimental part of the
     *    %Cantera API and may be changed or removed without notice.
     *
     * Source term derivatives are based on a generic rate-of-progress expression
     * for the @f$ i @f$-th reaction @f$ R_i @f$, which is a function of temperature
     * @f$ T @f$, pressure @f$ P @f$ and molar concentrations @f$ C_j @f$:
     * @f[
     *     R_i = k_{f,i} C_M^{\nu_{M,i}} \prod_j C_j^{\nu_{ji}^\prime} -
     *           k_{r,i} C_M^{\nu_{M,i}} \prod_j C_j^{\nu_{ji}^{\prime\prime}}
     * @f]
     * Forward/reverse rate expressions @f$ k_{f,i} @f$ and @f$ k_{r,i} @f$ are
     * implemented by ReactionRate specializations; forward/reverse stoichiometric
     * coefficients are @f$ \nu_{ji}^\prime @f$ and @f$ \nu_{ji}^{\prime\prime} @f$.
     * Unless the reaction involves third-body colliders, @f$ \nu_{M,i} = 0 @f$.
     * For three-body reactions, effective ThirdBody collider concentrations @f$ C_M @f$
     * are considered with @f$ \nu_{M,i} = 1 @f$. For more detailed information on
     * relevant theory, see, for example, Perini, et al. @cite perini2012 or Niemeyer,
     * et al. @cite niemeyer2017, although specifics of %Cantera's implementation may
     * differ.
     *
     * Partial derivatives are obtained from the product rule, where resulting terms
     * consider reaction rate derivatives, derivatives of the concentration product
     * term, and, if applicable, third-body term derivatives. ReactionRate
     * specializations may implement exact derivatives (example:
     * ArrheniusRate::ddTScaledFromStruct) or approximate them numerically (examples:
     * ReactionData::perturbTemperature, PlogData::perturbPressure,
     * FalloffData::perturbThirdBodies). Derivatives of concentration and third-body
     * terms are based on analytic expressions.
     *
     * %Species creation and destruction rates are obtained by multiplying
     * rate-of-progress vectors by stoichiometric coefficient matrices. As this is a
     * linear operation, it is possible to calculate derivatives the same way.
     *
     * All derivatives are calculated for source terms while holding other properties
     * constant, independent of whether equation of state or @f$ \sum X_k = 1 @f$
     * constraints are satisfied. Thus, derivatives deviate from Jacobians and
     * numerical derivatives that implicitly enforce these constraints. Depending
     * on application and equation of state, derivatives can nevertheless be used to
     * obtain Jacobians, for example:
     *
     *  - The Jacobian of net production rates @f$ \dot{\omega}_{k,\mathrm{net}} @f$
     *    with respect to temperature at constant pressure needs to consider changes
     *    of molar density @f$ C @f$ due to temperature
     *    @f[
     *      \left.
     *          \frac{\partial \dot{\omega}_{k,\mathrm{net}}}{\partial T}
     *      \right|_{P=\mathrm{const}} =
     *      \frac{\partial \dot{\omega}_{k,\mathrm{net}}}{\partial T} +
     *      \frac{\partial \dot{\omega}_{k,\mathrm{net}}}{\partial C}
     *      \left. \frac{\partial C}{\partial T} \right|_{P=\mathrm{const}}
     *    @f]
     *    where for an ideal gas @f$ \partial C / \partial T = - C / T @f$. The
     *    remaining partial derivatives are obtained from getNetProductionRates_ddT()
     *    and getNetProductionRates_ddC(), respectively.
     *
     *  - The Jacobian of @f$ \dot{\omega}_{k,\mathrm{net}} @f$ with respect to
     *    temperature at constant volume needs to consider pressure changes due to
     *    temperature
     *    @f[
     *      \left.
     *          \frac{\partial \dot{\omega}_{k,\mathrm{net}}}{\partial T}
     *      \right|_{V=\mathrm{const}} =
     *      \frac{\partial \dot{\omega}_{k,\mathrm{net}}}{\partial T} +
     *      \frac{\partial \dot{\omega}_{k,\mathrm{net}}}{\partial P}
     *      \left. \frac{\partial P}{\partial T} \right|_{V=\mathrm{const}}
     *    @f]
     *    where for an ideal gas @f$ \partial P / \partial T = P / T @f$. The
     *    remaining partial derivatives are obtained from getNetProductionRates_ddT()
     *    and getNetProductionRates_ddP(), respectively.
     *
     *  - Similar expressions can be derived for other derivatives and source terms.
     *
     * While some applications require exact derivatives, others can tolerate
     * approximate derivatives that neglect terms to increase computational speed
     * and/or improve Jacobian sparsity (example: AdaptivePreconditioner).
     * Derivative evaluations settings are accessible by keyword/value pairs
     * using the methods getDerivativeSettings() and setDerivativeSettings().
     *
     * For BulkKinetics, the following keyword/value pairs are supported:
     *  - `skip-third-bodies` (boolean): if `false` (default), third body
     *    concentrations are considered for the evaluation of Jacobians
     *  - `skip-falloff` (boolean): if `false` (default), third-body effects
     *    on rate constants are considered for the evaluation of derivatives.
     *  - `rtol-delta` (double): relative tolerance used to perturb properties
     *    when calculating numerical derivatives. The default value is 1e-8.
     *
     * For InterfaceKinetics, the following keyword/value pairs are supported:
     *  - `skip-coverage-dependence` (boolean): if `false` (default), rate constant
     *    coverage dependence is not considered when evaluating derivatives.
     *  - `skip-electrochemistry` (boolean): if `false` (default), electrical charge
     *    is not considered in evaluating the derivatives and these reactions are
     *    treated as normal surface reactions.
     *  - `rtol-delta` (double): relative tolerance used to perturb properties
     *    when calculating numerical derivatives. The default value is 1e-8.
     *
     * @{
     */

    /**
     * Retrieve derivative settings.
     *
     * @param settings  AnyMap containing settings determining derivative evaluation.
     */
    virtual void getDerivativeSettings(AnyMap& settings) const
    {
        throw NotImplementedError("Kinetics::getDerivativeSettings",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Set/modify derivative settings.
     *
     * @param settings  AnyMap containing settings determining derivative evaluation.
     */
    virtual void setDerivativeSettings(const AnyMap& settings)
    {
        throw NotImplementedError("Kinetics::setDerivativeSettings",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for forward rate constants with respect to temperature
     * at constant pressure, molar concentration and mole fractions
     * @param[out] dkfwd  Output vector of derivatives. Length: nReactions().
     */
    virtual void getFwdRateConstants_ddT(double* dkfwd)
    {
        throw NotImplementedError("Kinetics::getFwdRateConstants_ddT",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for forward rate constants with respect to pressure
     * at constant temperature, molar concentration and mole fractions.
     * @param[out] dkfwd  Output vector of derivatives. Length: nReactions().
     */
    virtual void getFwdRateConstants_ddP(double* dkfwd)
    {
        throw NotImplementedError("Kinetics::getFwdRateConstants_ddP",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for forward rate constants with respect to molar
     * concentration at constant temperature, pressure and mole fractions.
     * @param[out] dkfwd  Output vector of derivatives. Length: nReactions().
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    virtual void getFwdRateConstants_ddC(double* dkfwd)
    {
        throw NotImplementedError("Kinetics::getFwdRateConstants_ddC",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for forward rates-of-progress with respect to temperature
     * at constant pressure, molar concentration and mole fractions.
     * @param[out] drop  Output vector of derivatives. Length: nReactions().
     */
    virtual void getFwdRatesOfProgress_ddT(double* drop)
    {
        throw NotImplementedError("Kinetics::getFwdRatesOfProgress_ddT",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for forward rates-of-progress with respect to pressure
     * at constant temperature, molar concentration and mole fractions.
     * @param[out] drop  Output vector of derivatives. Length: nReactions().
     */
    virtual void getFwdRatesOfProgress_ddP(double* drop)
    {
        throw NotImplementedError("Kinetics::getFwdRatesOfProgress_ddP",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for forward rates-of-progress with respect to molar
     * concentration at constant temperature, pressure and mole fractions.
     * @param[out] drop  Output vector of derivatives. Length: nReactions().
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    virtual void getFwdRatesOfProgress_ddC(double* drop)
    {
        throw NotImplementedError("Kinetics::getFwdRatesOfProgress_ddC",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for forward rates-of-progress with respect to species
     * mole fractions at constant temperature, pressure and molar concentration.
     *
     * The method returns a matrix with nReactions() rows and nTotalSpecies() columns.
     * For a derivative with respect to @f$ X_i @f$, all other @f$ X_j @f$ are held
     * constant, rather than enforcing @f$ \sum X_j = 1 @f$.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    virtual Eigen::SparseMatrix<double> fwdRatesOfProgress_ddX()
    {
        throw NotImplementedError("Kinetics::fwdRatesOfProgress_ddX",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for forward rates-of-progress with respect to species
     * concentration at constant temperature, pressure and remaining species
     * concentrations.
     *
     * The method returns a matrix with nReactions() rows and nTotalSpecies() columns.
     * For a derivative with respect to @f$ c_i @f$, all other @f$ c_j @f$ are held
     * constant.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     *
     * @since New in %Cantera 3.0.
     */
    virtual Eigen::SparseMatrix<double> fwdRatesOfProgress_ddCi()
    {
        throw NotImplementedError("Kinetics::fwdRatesOfProgress_ddCi",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for reverse rates-of-progress with respect to temperature
     * at constant pressure, molar concentration and mole fractions.
     * @param[out] drop  Output vector of derivatives. Length: nReactions().
     */
    virtual void getRevRatesOfProgress_ddT(double* drop)
    {
        throw NotImplementedError("Kinetics::getRevRatesOfProgress_ddT",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for reverse rates-of-progress with respect to pressure
     * at constant temperature, molar concentration and mole fractions.
     * @param[out] drop  Output vector of derivatives. Length: nReactions().
     */
    virtual void getRevRatesOfProgress_ddP(double* drop)
    {
        throw NotImplementedError("Kinetics::getRevRatesOfProgress_ddP",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for reverse rates-of-progress with respect to molar
     * concentration at constant temperature, pressure and mole fractions.
     * @param[out] drop  Output vector of derivatives. Length: nReactions().
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    virtual void getRevRatesOfProgress_ddC(double* drop)
    {
        throw NotImplementedError("Kinetics::getRevRatesOfProgress_ddC",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for reverse rates-of-progress with respect to species
     * mole fractions at constant temperature, pressure and molar concentration.
     *
     * The method returns a matrix with nReactions() rows and nTotalSpecies() columns.
     * For a derivative with respect to @f$ X_i @f$, all other @f$ X_j @f$ are held
     * constant, rather than enforcing @f$ \sum X_j = 1 @f$.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    virtual Eigen::SparseMatrix<double> revRatesOfProgress_ddX()
    {
        throw NotImplementedError("Kinetics::revRatesOfProgress_ddX",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for forward rates-of-progress with respect to species
     * concentration at constant temperature, pressure and remaining species
     * concentrations.
     *
     * The method returns a matrix with nReactions() rows and nTotalSpecies() columns.
     * For a derivative with respect to @f$ c_i @f$, all other @f$ c_j @f$ are held
     * constant.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     *
     * @since New in %Cantera 3.0.
     */
    virtual Eigen::SparseMatrix<double> revRatesOfProgress_ddCi()
    {
        throw NotImplementedError("Kinetics::revRatesOfProgress_ddCi",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for net rates-of-progress with respect to temperature
     * at constant pressure, molar concentration and mole fractions.
     * @param[out] drop  Output vector of derivatives. Length: nReactions().
     */
    virtual void getNetRatesOfProgress_ddT(double* drop)
    {
        throw NotImplementedError("Kinetics::getNetRatesOfProgress_ddT",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for net rates-of-progress with respect to pressure
     * at constant temperature, molar concentration and mole fractions.
     * @param[out] drop  Output vector of derivatives. Length: nReactions().
     */
    virtual void getNetRatesOfProgress_ddP(double* drop)
    {
        throw NotImplementedError("Kinetics::getNetRatesOfProgress_ddP",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for net rates-of-progress with respect to molar
     * concentration at constant temperature, pressure and mole fractions.
     * @param[out] drop  Output vector of derivatives. Length: nReactions().
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    virtual void getNetRatesOfProgress_ddC(double* drop)
    {
        throw NotImplementedError("Kinetics::getNetRatesOfProgress_ddC",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for net rates-of-progress with respect to species
     * mole fractions at constant temperature, pressure and molar concentration.
     *
     * The method returns a matrix with nReactions() rows and nTotalSpecies() columns.
     * For a derivative with respect to @f$ X_i @f$, all other @f$ X_j @f$ are held
     * constant, rather than enforcing @f$ \sum X_j = 1 @f$.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    virtual Eigen::SparseMatrix<double> netRatesOfProgress_ddX()
    {
        throw NotImplementedError("Kinetics::netRatesOfProgress_ddX",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for net rates-of-progress with respect to species
     * concentration at constant temperature, pressure, and remaining species
     * concentrations.
     *
     * The method returns a matrix with nReactions() rows and nTotalSpecies() columns.
     * For a derivative with respect to @f$ c_i @f$, all other @f$ c_j @f$ are held
     * constant.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     *
     * @since New in %Cantera 3.0.
     */
    virtual Eigen::SparseMatrix<double> netRatesOfProgress_ddCi()
    {
        throw NotImplementedError("Kinetics::netRatesOfProgress_ddCi",
            "Not implemented for kinetics type '{}'.", kineticsType());
    }

    /**
     * Calculate derivatives for species creation rates with respect to temperature
     * at constant pressure, molar concentration and mole fractions.
     * @param[out] dwdot  Output vector of derivatives. Length: #m_kk.
     */
    void getCreationRates_ddT(double* dwdot);

    /**
     * Calculate derivatives for species creation rates with respect to pressure
     * at constant temperature, molar concentration and mole fractions.
     * @param[out] dwdot  Output vector of derivatives. Length: #m_kk.
     */
    void getCreationRates_ddP(double* dwdot);

    /**
     * Calculate derivatives for species creation rates with respect to molar
     * concentration at constant temperature, pressure and mole fractions.
     * @param[out] dwdot  Output vector of derivatives. Length: #m_kk.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    void getCreationRates_ddC(double* dwdot);

    /**
     * Calculate derivatives for species creation rates with respect to species
     * mole fractions at constant temperature, pressure and molar concentration.
     *
     * The method returns a square matrix with nTotalSpecies() rows and columns.
     * For a derivative with respect to @f$ X_i @f$, all other @f$ X_j @f$ are held
     * constant, rather than enforcing @f$ \sum X_j = 1 @f$.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    Eigen::SparseMatrix<double> creationRates_ddX();

    /**
     * Calculate derivatives for species creation rates with respect to species
     * concentration at constant temperature, pressure, and concentration of all other
     * species.
     *
     * The method returns a square matrix with nTotalSpecies() rows and columns.
     * For a derivative with respect to @f$ c_i @f$, all other @f$ c_j @f$ are held
     * constant.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     *
     * @since New in %Cantera 3.0.
     */
    Eigen::SparseMatrix<double> creationRates_ddCi();

    /**
     * Calculate derivatives for species destruction rates with respect to temperature
     * at constant pressure, molar concentration and mole fractions.
     * @param[out] dwdot  Output vector of derivatives. Length: #m_kk.
     */
    void getDestructionRates_ddT(double* dwdot);

    /**
     * Calculate derivatives for species destruction rates with respect to pressure
     * at constant temperature, molar concentration and mole fractions.
     * @param[out] dwdot  Output vector of derivatives. Length: #m_kk.
     */
    void getDestructionRates_ddP(double* dwdot);

    /**
     * Calculate derivatives for species destruction rates with respect to molar
     * concentration at constant temperature, pressure and mole fractions.
     * @param[out] dwdot  Output vector of derivatives. Length: #m_kk.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    void getDestructionRates_ddC(double* dwdot);

    /**
     * Calculate derivatives for species destruction rates with respect to species
     * mole fractions at constant temperature, pressure and molar concentration.
     *
     * The method returns a square matrix with nTotalSpecies() rows and columns.
     * For a derivative with respect to @f$ X_i @f$, all other @f$ X_j @f$ are held
     * constant, rather than enforcing @f$ \sum X_j = 1 @f$.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    Eigen::SparseMatrix<double> destructionRates_ddX();

    /**
     * Calculate derivatives for species destruction rates with respect to species
     * concentration at constant temperature, pressure, and concentration of all other
     * species.
     *
     * The method returns a square matrix with nTotalSpecies() rows and columns.
     * For a derivative with respect to @f$ c_i @f$, all other @f$ c_j @f$ are held
     * constant.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     *
     * @since New in %Cantera 3.0.
     */
    Eigen::SparseMatrix<double> destructionRates_ddCi();

    /**
     * Calculate derivatives for species net production rates with respect to
     * temperature at constant pressure, molar concentration and mole fractions.
     * @param[out] dwdot  Output vector of derivatives. Length: #m_kk.
     */
    void getNetProductionRates_ddT(double* dwdot);

    /**
     * Calculate derivatives for species net production rates with respect to pressure
     * at constant temperature, molar concentration and mole fractions.
     * @param[out] dwdot  Output vector of derivatives. Length: #m_kk.
     */
    void getNetProductionRates_ddP(double* dwdot);

    /**
     * Calculate derivatives for species net production rates with respect to molar
     * concentration at constant temperature, pressure and mole fractions.
     * @param[out] dwdot  Output vector of derivatives. Length: #m_kk.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    void getNetProductionRates_ddC(double* dwdot);

    /**
     * Calculate derivatives for species net production rates with respect to species
     * mole fractions at constant temperature, pressure and molar concentration.
     *
     * The method returns a square matrix with nTotalSpecies() rows and columns.
     * For a derivative with respect to @f$ X_i @f$, all other @f$ X_j @f$ are held
     * constant, rather than enforcing @f$ \sum X_j = 1 @f$.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     */
    Eigen::SparseMatrix<double> netProductionRates_ddX();

    /**
     * Calculate derivatives for species net production rates with respect to species
     * concentration at constant temperature, pressure, and concentration of all other
     * species.
     *
     * The method returns a square matrix with nTotalSpecies() rows and columns.
     * For a derivative with respect to @f$ c_i @f$, all other @f$ c_j @f$ are held
     * constant.
     *
     * @warning  This method is an experimental part of the %Cantera API and
     *      may be changed or removed without notice.
     *
     * @since New in %Cantera 3.0.
     */
    Eigen::SparseMatrix<double> netProductionRates_ddCi();

    /** @} End of Kinetics Derivatives */
    //! @} End of addtogroup derivGroup

    //! @name Reaction Mechanism Informational Query Routines
    //! @{

    /**
     * Stoichiometric coefficient of species k as a reactant in reaction i.
     *
     * @param k   kinetic species index
     * @param i   reaction index
     */
    virtual double reactantStoichCoeff(size_t k, size_t i) const;

    /**
     * Stoichiometric coefficient matrix for reactants.
     */
    Eigen::SparseMatrix<double> reactantStoichCoeffs() const {
        return m_reactantStoich.stoichCoeffs();
    }

    /**
     * Stoichiometric coefficient of species k as a product in reaction i.
     *
     * @param k   kinetic species index
     * @param i   reaction index
     */
    virtual double productStoichCoeff(size_t k, size_t i) const;

    /**
     * Stoichiometric coefficient matrix for products.
     */
    Eigen::SparseMatrix<double> productStoichCoeffs() const {
        return m_productStoich.stoichCoeffs();
    }

    /**
     * Stoichiometric coefficient matrix for products of reversible reactions.
     */
    Eigen::SparseMatrix<double> revProductStoichCoeffs() const {
        return m_revProductStoich.stoichCoeffs();
    }

    //! Reactant order of species k in reaction i.
    /*!
     * This is the nominal order of the activity concentration in
     * determining the forward rate of progress of the reaction
     *
     * @param k   kinetic species index
     * @param i   reaction index
     */
    virtual double reactantOrder(size_t k, size_t i) const {
        throw NotImplementedError("Kinetics::reactantOrder");
    }

    //! product Order of species k in reaction i.
    /*!
     * This is the nominal order of the activity concentration of species k in
     * determining the reverse rate of progress of the reaction i
     *
     * For irreversible reactions, this will all be zero.
     *
     * @param k   kinetic species index
     * @param i   reaction index
     */
    virtual double productOrder(int k, int i) const {
        throw NotImplementedError("Kinetics::productOrder");
    }

    //! Get the vector of activity concentrations used in the kinetics object
    /*!
     *  @param[out] conc  Vector of activity concentrations. Length is equal
     *               to the number of species in the kinetics object
     */
    virtual void getActivityConcentrations(double* const conc) {
        throw NotImplementedError("Kinetics::getActivityConcentrations");
    }

    /**
     * True if reaction i has been declared to be reversible. If isReversible(i)
     * is false, then the reverse rate of progress for reaction i is always
     * zero.
     *
     * @param i   reaction index
     */
    bool isReversible(size_t i) const {
        return std::find(m_revindex.begin(), m_revindex.end(), i) < m_revindex.end();
    }

    /**
     * Return the forward rate constants
     *
     * The computed values include all temperature-dependent and pressure-dependent
     * contributions. By default, third-body concentrations are only considered if
     * they are part of the reaction rate definition; for a legacy implementation that
     * includes third-body concentrations see Cantera::use_legacy_rate_constants().
     * Length is the number of reactions. Units are a combination of kmol, m^3 and s,
     * that depend on the rate expression for the reaction.
     *
     * @param kfwd    Output vector containing the forward reaction rate
     *                constants. Length: nReactions().
     */
    virtual void getFwdRateConstants(double* kfwd) {
        throw NotImplementedError("Kinetics::getFwdRateConstants");
    }

    /**
     * Return the reverse rate constants.
     *
     * The computed values include all temperature-dependent and pressure-dependent
     * contributions. By default, third-body concentrations are only considered if
     * they are part of the reaction rate definition; for a legacy implementation that
     * includes third-body concentrations see Cantera::use_legacy_rate_constants().
     * Length is the number of reactions. Units are a combination of kmol, m^3 and s,
     * that depend on the rate expression for the reaction. Note, this routine will
     * return rate constants for irreversible reactions if the default for
     * `doIrreversible` is overridden.
     *
     * @param krev   Output vector of reverse rate constants
     * @param doIrreversible boolean indicating whether irreversible reactions
     *                       should be included.
     */
    virtual void getRevRateConstants(double* krev,
                                     bool doIrreversible = false) {
        throw NotImplementedError("Kinetics::getRevRateConstants");
    }

    //! @}
    //! @name Reaction Mechanism Construction
    //! @{

    //!  Add a phase to the kinetics manager object.
    /*!
     * This must be done before the function init() is called or before any
     * reactions are input. The following fields are updated:
     *
     *  - #m_start -> vector of integers, containing the starting position of
     *    the species for each phase in the kinetics mechanism.
     *  - #m_thermo -> vector of pointers to ThermoPhase phases that
     *    participate in the kinetics mechanism.
     *  - #m_phaseindex -> map containing the string id of each
     *    ThermoPhase phase as a key and the index of the phase within the
     *    kinetics manager object as the value.
     *
     * @param thermo    Reference to the ThermoPhase to be added.
     * @since New in %Cantera 3.0. Replaces addPhase.
     */
    virtual void addThermo(shared_ptr<ThermoPhase> thermo);

    /**
     * Prepare the class for the addition of reactions, after all phases have
     * been added. This method is called automatically when the first reaction
     * is added. It needs to be called directly only in the degenerate case
     * where there are no reactions. The base class method does nothing, but
     * derived classes may use this to perform any initialization (allocating
     * arrays, etc.) that requires knowing the phases.
     */
    virtual void init() {}

    //! Return the parameters for a phase definition which are needed to
    //! reconstruct an identical object using the newKinetics function. This
    //! excludes the reaction definitions, which are handled separately.
    AnyMap parameters();

    /**
     * Resize arrays with sizes that depend on the total number of species.
     * Automatically called before adding each Reaction and Phase.
     */
    virtual void resizeSpecies();

    /**
     * Add a single reaction to the mechanism. Derived classes should call the
     * base class method in addition to handling their own specialized behavior.
     *
     * @param r       Pointer to the Reaction object to be added.
     * @param resize  If `true`, resizeReactions is called after reaction is added.
     * @return `true` if the reaction is added or `false` if it was skipped
     */
    virtual bool addReaction(shared_ptr<Reaction> r, bool resize=true);

    /**
     * Modify the rate expression associated with a reaction. The
     * stoichiometric equation, type of the reaction, reaction orders, third
     * body efficiencies, reversibility, etc. must be unchanged.
     *
     * @param i    Index of the reaction to be modified
     * @param rNew Reaction with the new rate expressions
     */
    virtual void modifyReaction(size_t i, shared_ptr<Reaction> rNew);

    /**
     * Return the Reaction object for reaction *i*. Changes to this object do
     * not affect the Kinetics object until the #modifyReaction function is
     * called.
     */
    shared_ptr<Reaction> reaction(size_t i);

    shared_ptr<const Reaction> reaction(size_t i) const;

    //! Determine behavior when adding a new reaction that contains species not
    //! defined in any of the phases associated with this kinetics manager. If
    //! set to true, the reaction will silently be ignored. If false, (the
    //! default) an exception will be raised.
    void skipUndeclaredSpecies(bool skip) {
        m_skipUndeclaredSpecies = skip;
    }
    bool skipUndeclaredSpecies() const {
        return m_skipUndeclaredSpecies;
    }

    //! Determine behavior when adding a new reaction that contains third-body
    //! efficiencies for species not defined in any of the phases associated
    //! with this kinetics manager. If set to true, the given third-body
    //! efficiency will be ignored. If false, (the default) an exception will be
    //! raised.
    void skipUndeclaredThirdBodies(bool skip) {
        m_skipUndeclaredThirdBodies = skip;
    }
    bool skipUndeclaredThirdBodies() const {
        return m_skipUndeclaredThirdBodies;
    }

    //! Specify how to handle duplicate third body reactions where one reaction
    //! has an explicit third body and the other has the generic third body with a
    //! non-zero efficiency for the former third body. Options are "warn" (default),
    //! "error", "mark-duplicate", and "modify-efficiency".
    void setExplicitThirdBodyDuplicateHandling(const string& flag);
    string explicitThirdBodyDuplicateHandling() const {
        return m_explicit_third_body_duplicates;
    }

    //! @}
    //! @name Altering Reaction Rates
    //!
    //! These methods alter reaction rates. They are designed primarily for
    //! carrying out sensitivity analysis, but may be used for any purpose
    //! requiring dynamic alteration of rate constants. For each reaction, a
    //! real-valued multiplier may be defined that multiplies the reaction rate
    //! coefficient. The multiplier may be set to zero to completely remove a
    //! reaction from the mechanism.
    //! @{

    //! The current value of the multiplier for reaction i.
    /*!
     * @param i index of the reaction
     */
    double multiplier(size_t i) const {
        return m_perturb[i];
    }

    //! Set the multiplier for reaction i to f.
    /*!
     *  @param i  index of the reaction
     *  @param f  value of the multiplier.
     */
    virtual void setMultiplier(size_t i, double f) {
        m_perturb[i] = f;
    }

    virtual void invalidateCache() {
        m_cache.clear();
    };

    //! @}
    //! Check for unmarked duplicate reactions and unmatched marked duplicates
    //!
    //! @param throw_err  If `true`, raise an exception that identifies any unmarked
    //!     duplicate reactions and any reactions marked as duplicate that do not
    //!     actually have a matching reaction.
    //! @param fix  If `true` (and if `throw_err` is false), update the `duplicate`
    //!     flag on all reactions to correctly indicate whether or not they are
    //!     duplicates.
    //! @return  If `throw_err` and `fix` are `false`, the indices of the first pair
    //!     of duplicate reactions or the index of an unmatched duplicate as both
    //!     elements of the `pair`. Otherwise, `(npos, npos)` if no errors were detected
    //!     or if the errors were fixed.
    //! @since  The `fix` argument was added in %Cantera 3.2.
    virtual pair<size_t, size_t> checkDuplicates(bool throw_err=true, bool fix=false);

    //! Set root Solution holding all phase information
    virtual void setRoot(shared_ptr<Solution> root) {
        m_root = root;
    }

    //! Get the Solution object containing this Kinetics object and associated
    //! ThermoPhase objects
    shared_ptr<Solution> root() const {
        return m_root.lock();
    }

    //! Register a function to be called if reaction is added.
    //! @param id  A unique ID corresponding to the object affected by the callback.
    //!   Typically, this is a pointer to an object that also holds a reference to the
    //!   Kinetics object.
    //! @param callback  The callback function to be called after any reaction is added.
    //! When the callback becomes invalid (for example, the corresponding object is
    //! being deleted, the removeReactionAddedCallback() method must be invoked.
    //! @since New in %Cantera 3.1
    void registerReactionAddedCallback(void* id, const function<void()>& callback)
    {
        m_reactionAddedCallbacks[id] = callback;
    }

    //! Remove the reaction-changed callback function associated with the specified object.
    //! @since New in %Cantera 3.1
    void removeReactionAddedCallback(void* id)
    {
        m_reactionAddedCallbacks.erase(id);
    }

protected:
    //! Cache for saved calculations within each Kinetics object.
    ValueCache m_cache;

    // Update internal rate-of-progress variables #m_ropf and #m_ropr.
    virtual void updateROP() {
        throw NotImplementedError("Kinetics::updateROP");
    }

    //! Check whether `r1` and `r2` represent duplicate stoichiometries
    //! This function returns a ratio if two reactions are duplicates of
    //! one another, and 0.0 otherwise.
    /*!
     *  `r1` and `r2` are maps of species key to stoichiometric coefficient, one
     *  for each reaction, where the species key is `1+k` for reactants and
     *  `-1-k` for products and `k` is the species index.
     *
     *  @return 0.0 if the stoichiometries are not multiples of one another
     *    Otherwise, it returns the ratio of the stoichiometric coefficients.
     */
    double checkDuplicateStoich(map<int, double>& r1, map<int, double>& r2) const;

    //! Vector of rate handlers
    vector<unique_ptr<MultiRateBase>> m_rateHandlers;
    map<string, size_t> m_rateTypes; //!< Mapping of rate handlers

    //! @name Stoichiometry management
    //!
    //! These objects and functions handle turning reaction extents into species
    //! production rates and also handle turning thermo properties into reaction
    //! thermo properties.
    //! @{

    //! Stoichiometry manager for the reactants for each reaction
    StoichManagerN m_reactantStoich;

    //! Stoichiometry manager for the products for each reaction
    StoichManagerN m_productStoich;

    //! Stoichiometry manager for the products of reversible reactions
    StoichManagerN m_revProductStoich;

    //! Net stoichiometry (products - reactants)
    Eigen::SparseMatrix<double> m_stoichMatrix;
    //! @}

    //! Boolean indicating whether Kinetics object is fully configured
    bool m_ready = false;

    //! The number of species in all of the phases
    //! that participate in this kinetics mechanism.
    size_t m_kk = 0;

    //! Vector of perturbation factors for each reaction's rate of
    //! progress vector. It is initialized to one.
    vector<double> m_perturb;

    //! Vector of Reaction objects represented by this Kinetics manager
    vector<shared_ptr<Reaction>> m_reactions;

    //! m_thermo is a vector of pointers to ThermoPhase objects that are
    //! involved with this kinetics operator
    /*!
     * For homogeneous kinetics applications, this vector will only have one
     * entry. For interfacial reactions, this vector will consist of multiple
     * entries; some of them will be surface phases, and the other ones will be
     * bulk phases. The order that the objects are listed determines the order
     * in which the species comprising each phase are listed in the source term
     * vector, originating from the reaction mechanism.
     */
    vector<shared_ptr<ThermoPhase>> m_thermo;

    /**
     * m_start is a vector of integers specifying the beginning position for the
     * species vector for the n'th phase in the kinetics class.
     */
    vector<size_t> m_start;

    /**
     * Mapping of the phase name to the position of the phase within the
     * kinetics object. Positions start with the value of 1. The member
     * function, phaseIndex() decrements by one before returning the index
     * value, so that missing phases return -1.
     */
    map<string, size_t> m_phaseindex;

    //! number of spatial dimensions of lowest-dimensional phase.
    size_t m_mindim = 4;

    //! Forward rate constant for each reaction
    vector<double> m_rfn;

    //! Delta G^0 for all reactions
    vector<double> m_delta_gibbs0;

    //! Reciprocal of the equilibrium constant in concentration units
    vector<double> m_rkcn;

    //! Forward rate-of-progress for each reaction
    vector<double> m_ropf;

    //! Reverse rate-of-progress for each reaction
    vector<double> m_ropr;

    //! Net rate-of-progress for each reaction
    vector<double> m_ropnet;

    vector<size_t> m_revindex; //!< Indices of reversible reactions
    vector<size_t> m_irrev; //!< Indices of irreversible reactions

    //! The enthalpy change for each reaction to calculate Blowers-Masel rates
    vector<double> m_dH;

    //! Buffer used for storage of intermediate reaction-specific results
    vector<double> m_rbuf;

    //! See skipUndeclaredSpecies()
    bool m_skipUndeclaredSpecies = false;

    //! See skipUndeclaredThirdBodies()
    bool m_skipUndeclaredThirdBodies = false;

    //! Flag indicating whether reactions include undeclared third bodies
    bool m_hasUndeclaredThirdBodies = false;


    string m_explicit_third_body_duplicates = "warn";

    //! reference to Solution
    std::weak_ptr<Solution> m_root;

    //! Callback functions that are invoked when the reaction is added.
    map<void*, function<void()>> m_reactionAddedCallbacks;
};

}

#endif
