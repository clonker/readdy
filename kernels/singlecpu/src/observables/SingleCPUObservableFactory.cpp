/**
 * << detailed description >>
 *
 * @file SingleCPUObservableFactory.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 30.06.16
 */


#include <readdy/model/Kernel.h>
#include <readdy/kernel/singlecpu/observables/SingleCPUObservableFactory.h>
#include <readdy/kernel/singlecpu/observables/SingleCPUObservables.h>

namespace readdy {
    namespace kernel {
        namespace singlecpu {
            namespace observables {
                SingleCPUObservableFactory::SingleCPUObservableFactory(readdy::kernel::singlecpu::SingleCPUKernel *const kernel) : ObservableFactory(kernel), kernel(kernel) {
                }

                readdy::model::HistogramAlongAxisObservable *SingleCPUObservableFactory::createAxisHistogramObservable(unsigned int stride, std::vector<double> binBorders,
                                                                                                               std::vector<std::string> typesToCount, unsigned int axis) const {
                    return new HistogramAlongAxisObservable<>(kernel, stride, binBorders, typesToCount, axis);
                }

                readdy::model::NParticlesObservable *SingleCPUObservableFactory::createNParticlesObservable(
                        unsigned int stride, std::vector<std::string> typesToCount) const {
                    return new NParticlesObservable<>(kernel, stride, typesToCount);
                }

                readdy::model::ForcesObservable *
                SingleCPUObservableFactory::createForcesObservable(unsigned int stride, std::vector<std::string> typesToCount) const {
                    return new ForcesObservable<>(kernel, stride, typesToCount);
                }
            }
        }
    }
}