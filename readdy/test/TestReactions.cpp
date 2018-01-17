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
 * << detailed description >>
 *
 * @file TestReactions.cpp
 * @brief << brief description >>
 * @author clonker
 * @date 21.06.16
 */

#include <readdy/plugin/KernelProvider.h>
#include <readdy/api/SimulationScheme.h>
#include <readdy/testing/KernelTest.h>
#include <readdy/testing/Utils.h>

namespace {

struct TestReactions : KernelTest {

};

TEST_P(TestReactions, TestConstantNumberOfParticleType) {
    // scenario: two particle types A and B, which can form a complex AB which after a time is going to dissolve back
    // into A and B. Therefore, the numbers #(A) + #(AB) and #(B) + #(AB) must remain constant.

    using n_particles_obs = readdy::model::observables::NParticles;

    auto stdRand = [](readdy::scalar lower = 0.0, readdy::scalar upper = 1.0) -> readdy::scalar {
        return static_cast <readdy::scalar> (std::rand()) / (RAND_MAX / (upper - lower)) + lower;
    };

    kernel->context().particle_types().add("A", 1.0);
    kernel->context().particle_types().add("B", 1.0);
    kernel->context().particle_types().add("AB", 0.0);
    kernel->context().periodicBoundaryConditions() = {{true, true, true}};
    kernel->context().boxSize() = {{5, 5, 5}};
    kernel->context().reactions().addFusion("Form complex", "A", "B", "AB", .5, 1.0);
    kernel->context().reactions().addFission("Dissolve", "AB", "A", "B", .5, 1.0);

    unsigned long n_A = 50;
    unsigned long n_B = n_A;

    for (unsigned long i = 0; i < n_A; ++i) {
        kernel->addParticle("A", {stdRand(-2.5, 2.5), stdRand(-2.5, 2.5), stdRand(-2.5, 2.5)});
        kernel->addParticle("B", {stdRand(-2.5, 2.5), stdRand(-2.5, 2.5), stdRand(-2.5, 2.5)});
    }


    auto obs = kernel->observe().nParticles(1, std::vector<std::string>({"A", "B", "AB"}));
    auto conn = kernel->connectObservable(obs.get());
    obs->setCallback([&n_A, &n_B](const n_particles_obs::result_type &result) {
        if (result.size() == 2) {
            EXPECT_EQ(n_A, result[0] + result[2])
                                << "Expected #(A)+#(AB)==" << n_A << ", but #(A)=" << result[0] << ", #(AB)="
                                << result[2];
            EXPECT_EQ(n_B, result[1] + result[2])
                                << "Expected #(B)+#(AB)==" << n_B << ", but #(B)=" << result[1] << ", #(AB)="
                                << result[2];
        }
    });

    {
        readdy::util::PerformanceNode pn("", false);
        auto conf = readdy::api::SchemeConfigurator<readdy::api::ReaDDyScheme>(kernel.get(), pn);
        const auto progs = kernel->getAvailableActions();
        conf.configureAndRun(10, 1);
    }

}

// Compare two vectors via their distance to the (1,1,1) plane in 3d space. This is quite arbitrary and only used to construct an ordered set.
struct Vec3ProjectedLess {
    bool operator()(const readdy::Vec3& lhs, const readdy::Vec3& rhs) const {
        return (lhs.x + lhs.y + lhs.z) < (rhs.x + rhs.y + rhs.z);
    }
};

TEST_P(TestReactions, FusionFissionWeights) {
    /* A diffuses, F is fixed
     * Also during reactions, weights are chosen such that F does not move.
     * Idea: position F particles and remember their positions (ordered set), do ONE time-step and check if current positions are still the same.
     */
    auto &context = kernel->context();
    context.particle_types().add("A", 0.5);
    context.particle_types().add("F", 0.0);
    context.periodicBoundaryConditions() = {{true, true, true}};
    context.boxSize() = {{20, 20, 20}};
    context.configure();

    const readdy::scalar weightF {static_cast<readdy::scalar>(0)};
    const readdy::scalar weightA  {static_cast<readdy::scalar>(1.)};
    kernel->context().reactions().addFusion("F+A->F", "F", "A", "F", 1.0, 2.0, weightF, weightA);
    kernel->context().reactions().addFission("F->F+A", "F", "F", "A", 1.0, 2.0, weightF, weightA);

    std::set<readdy::Vec3, Vec3ProjectedLess> fPositions;
    auto n3 = readdy::model::rnd::normal3<readdy::scalar>;
    for (std::size_t i = 0; i < 15; ++i) {
        auto fPos = n3(static_cast<readdy::scalar>(0.), static_cast<readdy::scalar>(0.8));
        kernel->addParticle("F", fPos);
        // fPos
        fPositions.emplace(kernel->stateModel().getParticles().back().getPos());
        kernel->addParticle("A", n3(static_cast<readdy::scalar>(0.), static_cast<readdy::scalar>(1.)));
    }

    auto obs = kernel->observe().positions(1, std::vector<std::string>({"F"}));
    obs->setCallback(
            [&fPositions, this](const readdy::model::observables::Positions::result_type &result) {
                std::set<readdy::Vec3, Vec3ProjectedLess> checklist;
                for (const auto &pos : result) {
                    checklist.emplace(pos);
                }
                EXPECT_EQ(fPositions.size(), checklist.size());
                auto itPos = fPositions.begin();
                auto itCheck = checklist.begin();
                while (itPos != fPositions.end()) {
                    EXPECT_VEC3_NEAR(*itPos, *itCheck, kernel->doublePrecision() ? 1e-8 : 1e-6);
                    ++itPos;
                    ++itCheck;
                }
            }
    );
    auto connection = kernel->connectObservable(obs.get());

    {
        readdy::util::PerformanceNode pn("", false);
        readdy::api::SchemeConfigurator<readdy::api::ReaDDyScheme>(kernel.get(), pn).configureAndRun(1, .5);
    }
}

TEST_P(TestReactions, FusionThroughBoundary) {
    auto &ctx = kernel->context();
    ctx.boxSize() = {{10, 10, 10}};
    ctx.periodicBoundaryConditions() = {true, true, true};
    ctx.particle_types().add("A", 0.);
    ctx.reactions().add("fus: A +(2) A -> A", 1e16);

    auto &&neighborList = kernel->actions().updateNeighborList();
    auto &&reactions = kernel->actions().uncontrolledApproximation(1);
    // resulting particle should be at 4.9
    kernel->stateModel().addParticle({4.5, 4.5, 4.5, ctx.particle_types().idOf("A")});
    kernel->stateModel().addParticle({-4.7, -4.7, -4.7, ctx.particle_types().idOf("A")});
    kernel->context().configure();
    neighborList->perform();
    reactions->perform();

    const auto particles = kernel->stateModel().getParticles();
    EXPECT_EQ(particles.size(), 1);
    const auto &pos = particles[0].getPos();
    EXPECT_NEAR(pos.x, 4.9, 0.00001);
    EXPECT_NEAR(pos.y, 4.9, 0.00001);
    EXPECT_NEAR(pos.z, 4.9, 0.00001);
}

TEST_P(TestReactions, FissionNearBoundary) {
    auto &ctx = kernel->context();
    ctx.boxSize() = {{10, 10, 10}};
    ctx.periodicBoundaryConditions() = {true, true, true};
    ctx.particle_types().add("A", 0.);
    ctx.reactions().add("fus: A -> A +(2) A", 1e16);

    auto &&neighborList = kernel->actions().updateNeighborList();
    auto &&reactions = kernel->actions().uncontrolledApproximation(1);
    // one product will be in negative-x halfspace, the other in positive-x halfspace
    kernel->stateModel().addParticle({-4.9999999, 0, 0, ctx.particle_types().idOf("A")});
    kernel->context().configure();
    neighborList->perform();
    reactions->perform();

    const auto particles = kernel->stateModel().getParticles();
    EXPECT_EQ(particles.size(), 2);
    const auto &pos1 = particles[0].getPos();
    const auto &pos2 = particles[1].getPos();
    if (pos1.x <= 0.) {
        EXPECT_GT(pos2.x, 0.);
    } else if (pos1.x >= 0.) {
        EXPECT_LT(pos2.x, 0.);
    }
}

/*
 * @todo this is rather an integration test that should be separated from the rest
 * TEST_P(TestReactions, ConstantNumberOfParticles) {
 *
    using namespace readdy;
    // A is absorbed and created by F, while the number of F stays constant, this test spans multiple timesteps
    kernel->getKernelContext().particle_types().add("A", 0.5, 1.0);
    kernel->getKernelContext().particle_types().add("F", 0.0, 1.0);
    kernel->context().setPeriodicBoundary(true, true, true);
    kernel->context().setBoxSize(10, 10, 10);

    const readdy::scalar weightF = 0.;
    const readdy::scalar weightA = 1.;
    kernel->registerReaction<readdy::model::reactions::Fusion>("F+A->F", "F", "A", "F", .1, 2.0, weightF, weightA);

    auto n3 = readdy::model::rnd::normal3<>;
    for (std::size_t i = 0; i < 200; ++i) {
        kernel->addParticle("F", n3(0., 1.));
        kernel->addParticle("A", n3(0., 1.));
    }

    auto obs = kernel->createObservable<readdy::model::observables::NParticles>(1, std::vector<std::string>({"F"}));
    obs->setCallback(
            [](const readdy::model::observables::NParticles::result_type &result) {
                EXPECT_EQ(result[0], 200);
                log::error("::: {}", result[0]);
            }
    );
    auto connection = kernel->connectObservable(obs.get());

    {
        auto conf = readdy::api::SchemeConfigurator<readdy::api::ReaDDyScheme>(kernel.get(), true);
        conf.withReactionScheduler<readdy::model::actions::reactions::GillespieParallel>();
        conf.withSkinSize(5.);
        conf.configureAndRun(400, 0.01);
    }
}*/

INSTANTIATE_TEST_CASE_P(TestReactionsCore, TestReactions,
                        ::testing::ValuesIn(readdy::testing::getKernelsToTest()));
}
