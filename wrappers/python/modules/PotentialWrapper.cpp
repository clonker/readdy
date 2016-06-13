/**
 * << detailed description >>
 *
 * @file PotentialWrapper.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 10.06.16
 */

#include "PotentialWrapper.h"
#include "interpreter_lock.h"
#include <readdy/model/Vec3.h>
#include <iostream>
#include <boost/python/extract.hpp>

namespace readdy {

    namespace py {
        PotentialOrder2Wrapper::PotentialOrder2Wrapper(const std::string &name, boost::python::object o1, boost::python::object o2)
                : PotentialOrder2(name),
                  calcEnergyFun(new boost::python::object(o1), [](boost::python::object *o) {
                      interpreter_lock lock;
                      delete o;
                  }),
                  calcForceFun(new boost::python::object(o2), [](boost::python::object *o) {
                      interpreter_lock lock;
                      delete o;
                  })
                  { };

        double PotentialOrder2Wrapper::calculateEnergy(const model::Vec3 &x_i, const model::Vec3 &x_j) {
            interpreter_lock lock;
            std::cout << "calc energy" << std::endl;
            return boost::python::extract<double>((*calcEnergyFun)(x_i, x_j));
        }

        void PotentialOrder2Wrapper::calculateForce(model::Vec3 &force, const model::Vec3 &x_i, const model::Vec3 &x_j) {
            interpreter_lock lock;
            std::cout << "calc force" << std::endl;
            force += boost::python::extract<readdy::model::Vec3>((*calcForceFun)(x_i, x_j));
        }

        void PotentialOrder2Wrapper::calculateForceAndEnergy(model::Vec3 &force, double &energy, const model::Vec3 &x_i, const model::Vec3 &x_j) {
            interpreter_lock lock;
            std::cout << "calc force & energy" << std::endl;
            energy += boost::python::extract<double>((*calcEnergyFun)(x_i, x_j));
            force += boost::python::extract<readdy::model::Vec3>((*calcForceFun)(x_i, x_j));
        }
    }


}

