//
// Created by clonker on 07.03.16.
//

#include <readdy/kernel/singlecpu/SingleCPUKernel.h>
#include <readdy/kernel/singlecpu/programs/SingleCPUProgramFactory.h>
#include <readdy/kernel/singlecpu/programs/SingleCPUTestProgram.h>
#include <readdy/kernel/singlecpu/potentials/SingleCPUPotentialFactory.h>
#include <readdy/kernel/singlecpu/reactions/SingleCPUReactionFactory.h>


namespace readdy {
    namespace kernel {
        namespace singlecpu {
            const std::string SingleCPUKernel::name = "SingleCPU";
            struct SingleCPUKernel::Impl {
                std::unique_ptr<readdy::model::RandomProvider> rand = std::make_unique<readdy::model::RandomProvider>();
                std::unique_ptr<readdy::model::KernelContext> context;
                std::unique_ptr<SingleCPUKernelStateModel> model;
                std::unique_ptr<potentials::SingleCPUPotentialFactory> potentials;
                std::unique_ptr<programs::SingleCPUProgramFactory> programs;
                std::unique_ptr<reactions::SingleCPUReactionFactory> reactions;
            };

            SingleCPUKernel::SingleCPUKernel() : readdy::model::Kernel(name), pimpl(std::make_unique<SingleCPUKernel::Impl>()) {
                pimpl->programs = std::make_unique<programs::SingleCPUProgramFactory>(this);
                pimpl->potentials = std::make_unique<potentials::SingleCPUPotentialFactory>(this);
                pimpl->reactions = std::make_unique<reactions::SingleCPUReactionFactory>(this);
                pimpl->context = std::make_unique<readdy::model::KernelContext>(pimpl->reactions.get());
                pimpl->model = std::make_unique<SingleCPUKernelStateModel>(pimpl->context.get());
            }

            /**
             * factory method
             */
            std::unique_ptr<SingleCPUKernel> SingleCPUKernel::create() {
                return std::make_unique<SingleCPUKernel>();
            }

            /**
             * Destructor: default
             */
            SingleCPUKernel::~SingleCPUKernel() = default;

            readdy::model::KernelStateModel &SingleCPUKernel::getKernelStateModel() const {
                return *pimpl->model;
            }

            readdy::model::RandomProvider &SingleCPUKernel::getRandomProvider() const {
                return *pimpl->rand;
            }

            readdy::model::KernelContext &SingleCPUKernel::getKernelContext() const {
                return *pimpl->context;
            }

            SingleCPUKernelStateModel &SingleCPUKernel::getKernelStateModelSingleCPU() const {
                return *pimpl->model;
            }

            std::vector<std::string> SingleCPUKernel::getAvailablePotentials() const {
                return pimpl->potentials->getAvailablePotentials();
            }

            std::unique_ptr<readdy::model::potentials::Potential> SingleCPUKernel::createPotential(std::string &name) const {
                return pimpl->potentials->createPotential(name);
            }

            readdy::model::potentials::PotentialFactory &SingleCPUKernel::getPotentialFactory() const {
                return *pimpl->potentials;
            }

            readdy::model::programs::ProgramFactory &SingleCPUKernel::getProgramFactory() const {
                return *pimpl->programs;
            }

            readdy::model::reactions::ReactionFactory &SingleCPUKernel::getReactionFactory() const {
                return *pimpl->reactions;
            }


            SingleCPUKernel &SingleCPUKernel::operator=(SingleCPUKernel &&rhs) = default;

            SingleCPUKernel::SingleCPUKernel(SingleCPUKernel &&rhs) = default;

        }
    }
}



