/**
 * << detailed description >>
 *
 * @file SingleCPUDiffuseProgram.h
 * @brief << brief description >>
 * @author clonker
 * @date 19.04.16
 */

#ifndef READDY_MAIN_SINGLECPUDIFFUSEPROGRAM_H
#define READDY_MAIN_SINGLECPUDIFFUSEPROGRAM_H

#include <readdy/model/programs/Programs.h>
#include <readdy/kernel/singlecpu/SingleCPUKernel.h>

namespace readdy {
    namespace kernel {
        namespace singlecpu {

            namespace programs {
                class SingleCPUDiffuseProgram : public readdy::model::programs::DiffuseProgram{

                public:
                    SingleCPUDiffuseProgram(SingleCPUKernel *kernel);

                    virtual void execute() override;

                private:
                    SingleCPUKernel *kernel;
                };
            }
        }
    }
}


#endif //READDY_MAIN_SINGLECPUDIFFUSEPROGRAM_H