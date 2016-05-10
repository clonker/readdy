/**
 * << detailed description >>
 *
 * @file gil_lock.h
 * @brief << brief description >>
 * @author clonker
 * @date 27.04.16
 */

#ifndef READDY2_MAIN_GIL_LOCK_H
#define READDY2_MAIN_GIL_LOCK_H

namespace readdy {
    namespace py {
        class gil_lock {
        public:
            gil_lock();
            ~gil_lock();
        private:
            PyGILState_STATE gilState;
        };
    }
}


#endif //READDY2_MAIN_GIL_LOCK_H