//! @file Func1Factory.cpp

// This file is part of Cantera. See License.txt in the top-level directory or
// at https://cantera.org/license.txt for license and copyright information.

#include "cantera/numerics/Func1Factory.h"

namespace Cantera
{

Func1Factory* Func1Factory::s_factory = 0;
std::mutex Func1Factory::s_mutex;

Func1Factory::Func1Factory()
{
    // make_shared<Func1> is not allowed because the copy constructor is deleted
    reg("functor", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Func1());
    });
    reg("sin", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Sin1(params));
    });
    reg("cos", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Cos1(params));
    });
    reg("exp", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Exp1(params));
    });
    reg("log", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Log1(params));
    });
    reg("pow", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Pow1(params));
    });
    reg("constant", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Const1(params));
    });
    reg("polynomial3", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Poly1(params));
    });
    reg("Fourier", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Fourier1(params));
    });
    reg("Gaussian", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Gaussian1(params));
    });
    reg("Arrhenius", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Arrhenius1(params));
    });
    reg("tabulated-linear", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Tabulated1(params));
    });
    reg("tabulated-previous", [](const vector<double>& params) {
        return shared_ptr<Func1>(new Tabulated1(params, "previous"));
    });
}

Func1Factory* Func1Factory::factory()
{
    std::unique_lock<std::mutex> lock(s_mutex);
    if (!s_factory) {
        s_factory = new Func1Factory;
    }
    return s_factory;
}

void Func1Factory::deleteFactory()
{
    std::unique_lock<std::mutex> lock(s_mutex);
    delete s_factory;
    s_factory = 0;
}

Math1FactoryA* Math1FactoryA::s_factory = 0;
std::mutex Math1FactoryA::s_mutex;

Math1FactoryA::Math1FactoryA()
{
    reg("sum", [](const shared_ptr<Func1> f1, const shared_ptr<Func1> f2) {
        return shared_ptr<Func1>(new Sum1(f1, f2));
    });
    reg("diff", [](const shared_ptr<Func1> f1, const shared_ptr<Func1> f2) {
        return shared_ptr<Func1>(new Diff1(f1, f2));
    });
    reg("product", [](const shared_ptr<Func1> f1, const shared_ptr<Func1> f2) {
        return shared_ptr<Func1>(new Product1(f1, f2));
    });
    reg("ratio", [](const shared_ptr<Func1> f1, const shared_ptr<Func1> f2) {
        return shared_ptr<Func1>(new Ratio1(f1, f2));
    });
    reg("composite", [](const shared_ptr<Func1> f1, const shared_ptr<Func1> f2) {
        return shared_ptr<Func1>(new Composite1(f1, f2));
    });
}

Math1FactoryA* Math1FactoryA::factory()
{
    std::unique_lock<std::mutex> lock(s_mutex);
    if (!s_factory) {
        s_factory = new Math1FactoryA;
    }
    return s_factory;
}

void Math1FactoryA::deleteFactory()
{
    std::unique_lock<std::mutex> lock(s_mutex);
    delete s_factory;
    s_factory = 0;
}

Math1FactoryB* Math1FactoryB::s_factory = 0;
std::mutex Math1FactoryB::s_mutex;

Math1FactoryB::Math1FactoryB()
{
    reg("times-constant", [](const shared_ptr<Func1> f, double c) {
        return shared_ptr<Func1>(new TimesConstant1(f, c));
    });
    reg("plus-constant", [](const shared_ptr<Func1> f, double c) {
        return shared_ptr<Func1>(new PlusConstant1(f, c));
    });
    reg("periodic", [](const shared_ptr<Func1> f, double c) {
        return shared_ptr<Func1>(new Periodic1(f, c));
    });
}

Math1FactoryB* Math1FactoryB::factory()
{
    std::unique_lock<std::mutex> lock(s_mutex);
    if (!s_factory) {
        s_factory = new Math1FactoryB;
    }
    return s_factory;
}

void Math1FactoryB::deleteFactory()
{
    std::unique_lock<std::mutex> lock(s_mutex);
    delete s_factory;
    s_factory = 0;
}

shared_ptr<Func1> newFunc1(const string& func1Type, double coeff)
{
    return Func1Factory::factory()->create(func1Type, {coeff});
}

shared_ptr<Func1> newFunc1(const string& func1Type, const vector<double>& params)
{
    return Func1Factory::factory()->create(func1Type, params);
}

shared_ptr<Func1> newFunc1(const string& func1Type, const shared_ptr<Func1> f1,
                           const shared_ptr<Func1> f2)
{
    return Math1FactoryA::factory()->create(func1Type, f1, f2);
}

shared_ptr<Func1> newFunc1(const string& func1Type,
                           const shared_ptr<Func1> f, double coeff)
{
    return Math1FactoryB::factory()->create(func1Type, f, coeff);
}

string checkFunc1(const string& func1Type)
{
    if (Func1Factory::factory()->exists(func1Type)) {
        // standard functor
        return "standard";
    }
    if (Math1FactoryA::factory()->exists(func1Type)) {
        // compounding functor
        return "compound";
    }
    if (Math1FactoryB::factory()->exists(func1Type)) {
        // modifying functor
        return "modified";
    }
    return "undefined";
}

}
