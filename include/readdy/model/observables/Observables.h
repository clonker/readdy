/********************************************************************
 * Copyright © 2016 Computational Molecular Biology Group,          *
 *                  Freie Universität Berlin (GER)                  *
 *                                                                  *
 * This file is part of ReaDDy.                                     *
 *                                                                  *
 * ReaDDy is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU Lesser General Public License as   *
 * published by the Free Software Foundation, either version 3 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU Lesser General Public License for more details.              *
 *                                                                  *
 * You should have received a copy of the GNU Lesser General        *
 * Public License along with this program. If not, see              *
 * <http://www.gnu.org/licenses/>.                                  *
 ********************************************************************/


/**
 * Header file containing definitions for various observables. Currently:
 *  - Positions,
 *  - Particles,
 *  - RadialDistribution,
 *  - CenterOfMass,
 *  - HistogramAlongAxis,
 *  - NParticles,
 *  - TestCombiner,
 *  - Forces
 *
 * @file Observables.h
 * @brief Header file containing definitions for various observables.
 * @author clonker
 * @date 26.04.16
 * @todo for demonstration purposes, add a more meaningful combiner observable, such as velocity
 */

#ifndef READDY_MAIN_OBSERVABLES_H
#define READDY_MAIN_OBSERVABLES_H

#include <readdy/model/observables/Observable.h>
#include <readdy/model/Vec3.h>
#include <vector>
#include <iostream>
#include <set>
#include <readdy/model/Particle.h>

namespace readdy {
namespace model {
namespace observables {

class KernelContext;

class Positions : public Observable<std::vector<Vec3>> {
public:
    Positions(Kernel *const kernel, unsigned int stride = 1) : Observable(kernel, stride) {}

    Positions(Kernel *const kernel, unsigned int stride, std::vector<std::string> typesToCount);

    Positions(Kernel *const kernel, unsigned int stride, std::vector<unsigned int> typesToCount);

    virtual void evaluate() = 0;

protected:
    std::vector<unsigned int> typesToCount;
};

class Particles
        : public Observable<std::tuple<std::vector<readdy::model::Particle::type_type>, std::vector<readdy::model::Particle::id_type>, std::vector<Vec3>>> {
public:
    Particles(Kernel *const kernel, unsigned int stride = 1) : Observable(kernel, stride) {}

    virtual void evaluate() = 0;
};

class RadialDistribution : public Observable<std::pair<std::vector<double>, std::vector<double>>> {
public:
    RadialDistribution(Kernel *const kernel, unsigned int stride, std::vector<double> binBorders,
                       std::vector<unsigned int> typeCountFrom, std::vector<unsigned int> typeCountTo, double particleToDensity);

    RadialDistribution(Kernel *const kernel, unsigned int stride, std::vector<double> binBorders,
                       const std::vector<std::string> &typeCountFrom, const std::vector<std::string> &typeCountTo,
                       double particleToDensity);

    virtual void evaluate() = 0;

    const std::vector<double> &getBinBorders() const;

    void setBinBorders(const std::vector<double> &binBorders);

protected:
    std::vector<double> binBorders;
    std::vector<double> counts;
    std::vector<unsigned int> typeCountFrom, typeCountTo;
    double particleToDensity;
};

class CenterOfMass : public Observable<readdy::model::Vec3> {

public:
    CenterOfMass(Kernel *const kernel, unsigned int stride, unsigned int particleType);

    CenterOfMass(Kernel *const kernel, unsigned int stride, const std::vector<unsigned int> &particleTypes);

    CenterOfMass(Kernel *const kernel, unsigned int stride, const std::string &particleType);

    CenterOfMass(Kernel *const kernel, unsigned int stride, const std::vector<std::string> &particleType);

    virtual void evaluate() override;

protected:
    std::set<unsigned int> particleTypes;
};

class HistogramAlongAxis : public Observable<std::vector<double>> {

public:
    HistogramAlongAxis(readdy::model::Kernel *const kernel, unsigned int stride,
                       std::vector<double> binBorders, std::set<unsigned int> typesToCount,
                       unsigned int axis);

    HistogramAlongAxis(Kernel *const kernel, unsigned int stride, std::vector<double> binBorders,
                       std::vector<std::string> typesToCount, unsigned int axis);

    virtual void evaluate() = 0;

protected:
    std::vector<double> binBorders;
    std::set<unsigned int> typesToCount;

    unsigned int axis;
};

class NParticles : public Observable<std::vector<unsigned long>> {

public:
    NParticles(Kernel *const kernel, unsigned int stride) : Observable(kernel, stride) {}

    NParticles(Kernel *const kernel, unsigned int stride, std::vector<std::string> typesToCount);

    NParticles(Kernel *const kernel, unsigned int stride, std::vector<unsigned int> typesToCount);

    virtual void evaluate() = 0;

protected:
    std::vector<unsigned int> typesToCount;
};

class TestCombiner
        : public Combiner<std::vector<double>, Positions, Positions> {
public:

    TestCombiner(Kernel *const kernel, Positions *obs1, Positions *obs2,
                 unsigned int stride)
            : Combiner(kernel, stride, obs1, obs2) {
    }

    virtual void evaluate() override;
};

class Forces : public Observable<std::vector<readdy::model::Vec3>> {

public:
    Forces(Kernel *const kernel, unsigned int stride) : Observable(kernel, stride), typesToCount({}) {}

    Forces(Kernel *const kernel, unsigned int stride, std::vector<std::string> typesToCount);

    Forces(Kernel *const kernel, unsigned int stride, std::vector<unsigned int> typesToCount);

    virtual void evaluate() = 0;

protected:
    std::vector<unsigned int> typesToCount;
};

}
}
}

#endif //READDY_MAIN_OBSERVABLES_H