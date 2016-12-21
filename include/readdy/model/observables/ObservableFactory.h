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
 * This header file contains the definition of the ObservableFactory. Its purpose is to create observable of different
 * types in the form of unique_ptrs. The actual implementation of an observable can be changed by specializing the
 * dispatcher for its type and invoking a virtual (and then: overridden) method within the factory.
 *
 * @file ObservableFactory.h
 * @brief This header file contains the definition of the ObservableFactory.
 * @author clonker
 * @date 29.04.16
 */

#ifndef READDY_MAIN_OBSERVABLEFACTORY_H
#define READDY_MAIN_OBSERVABLEFACTORY_H

#include <string>
#include <unordered_map>
#include <readdy/model/observables/Observable.h>
#include <readdy/common/Utils.h>
#include <readdy/model/observables/Observables.h>
#include "Aggregators.h"

namespace readdy {
namespace model {
class Kernel;

namespace observables {

class ObservableFactory {
public:
    ObservableFactory(Kernel *const kernel) : kernel(kernel) {};

    template<typename T, typename Obs1, typename Obs2>
    inline std::unique_ptr<T> create(Obs1 *obs1, Obs2 *obs2, unsigned int stride = 1) const {
        return std::make_unique<T>(kernel, obs1, obs2, stride);
    };

    template<typename R, typename... Args>
    inline std::unique_ptr<R> create(unsigned int stride, Args &&... args) const {
        return std::unique_ptr<R>(
                ObservableFactory::get_dispatcher<R, Args...>::impl(this, stride, std::forward<Args>(args)...));
    }

    virtual HistogramAlongAxis *
    createHistogramAlongAxis(unsigned int stride, std::vector<double> binBorders,
                             std::vector<std::string> typesToCount, unsigned int axis) const {
        // todo: provide default impl
        throw std::runtime_error("Should be overridden (or todo: provide default impl)");
    }

    virtual NParticles *
    createNParticles(unsigned int stride, std::vector<std::string> typesToCount = {}) const {
        throw std::runtime_error("should be overridden (or todo: provide default impl)");
    }

    virtual Forces *
    createForces(unsigned int stride, std::vector<std::string> typesToCount = {}) const {
        throw std::runtime_error("should be overridden (or todo: provide default impl)");
    }

    virtual Positions *
    createPositions(unsigned int stride, std::vector<std::string> typesToCount = {}) const {
        throw std::runtime_error("should be overridden (or todo: provide default impl)");
    }

    virtual RadialDistribution *
    createRadialDistribution(unsigned int stride, std::vector<double> binBorders, std::vector<std::string> typeCountFrom,
                             std::vector<std::string> typeCountTo,
                             double particleDensity) const {
        throw std::runtime_error("should be overridden (or todo: provide default impl)");
    }

    virtual Particles *
    createParticles(unsigned int stride) const {
        throw std::runtime_error("should be overridden (or todo: provide default impl)");
    }

    virtual MeanSquaredDisplacement *
    createMeanSquaredDisplacement(unsigned int stride, std::vector<std::string> typesToCount, Particles *particlesObservable) const {
        throw std::runtime_error("should be overridden (or todo: provide default impl)");
    }

protected:
    Kernel *const kernel;

    template<typename T, typename... Args>
    struct get_dispatcher;

    template<typename T, typename... Args>
    struct get_dispatcher {
        static T *impl(const ObservableFactory *self, unsigned int stride, Args &&... args) {
            // this only invokes the normal constructor
            return new T(self->kernel, stride, std::forward<Args>(args)...);
        };
    };

    template<typename... Args>
    struct get_dispatcher<readdy::model::observables::HistogramAlongAxis, Args...> {
        static HistogramAlongAxis *impl(const ObservableFactory *self, unsigned int stride, Args &&... args) {
            return self->createHistogramAlongAxis(stride, std::forward<Args>(args)...);
        }
    };

    template<typename... Args>
    struct get_dispatcher<readdy::model::observables::NParticles, Args...> {
        static NParticles *impl(const ObservableFactory *self, unsigned int stride, Args &&... args) {
            return self->createNParticles(stride, std::forward<Args>(args)...);
        }
    };

    template<typename... Args>
    struct get_dispatcher<readdy::model::observables::Forces, Args...> {
        static Forces *impl(const ObservableFactory *self, unsigned int stride, Args &&... args) {
            return self->createForces(stride, std::forward<Args>(args)...);
        }
    };

    template<typename... Args>
    struct get_dispatcher<readdy::model::observables::Positions, Args...> {
        static Positions *impl(const ObservableFactory *self, unsigned int stride, Args &&... args) {
            return self->createPositions(stride, std::forward<Args>(args)...);
        }
    };

    template<typename... Args>
    struct get_dispatcher<readdy::model::observables::RadialDistribution, Args...> {
        static RadialDistribution *impl(const ObservableFactory *self, unsigned int stride, Args &&... args) {
            return self->createRadialDistribution(stride, std::forward<Args>(args)...);
        }
    };

    template<typename... Args>
    struct get_dispatcher<readdy::model::observables::Particles, Args...> {
        static Particles *impl(const ObservableFactory *self, unsigned int stride, Args &&... args) {
            return self->createParticles(stride, std::forward<Args>(args)...);
        }
    };

    template<typename... Args>
    struct get_dispatcher<readdy::model::observables::MeanSquaredDisplacement, Args...> {
        static MeanSquaredDisplacement *impl(const ObservableFactory *self, unsigned int stride, Args... args) {
            return self->createMeanSquaredDisplacement(stride, std::forward<Args>(args)...);
        }
    };
};

}
}
}
#endif //READDY_MAIN_OBSERVABLEFACTORY_H