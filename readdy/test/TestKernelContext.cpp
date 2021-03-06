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
 * Testfile containing tests for the KernelContext class.
 *
 * @file TestKernelContext.cpp
 * @brief Testfile for KernelContext.
 * @author clonker
 * @date 19.04.16
 */


#include <readdy/common/Utils.h>
#include <readdy/common/make_unique.h>
#include <readdy/model/Kernel.h>
#include <readdy/plugin/KernelProvider.h>
#include <readdy/testing/KernelTest.h>
#include <readdy/testing/Utils.h>
#include <readdy/testing/NOOPPotential.h>
#include <readdy/model/potentials/PotentialsOrder1.h>

namespace m = readdy::model;

namespace {

class TestKernelContext : public ::testing::Test {
protected:
    TestKernelContext() {}
};

class TestKernelContextWithKernels : public KernelTest {

};

TEST_F(TestKernelContext, SetGetKBT) {
    m::Context ctx;
    ctx.kBT() = 42;
    EXPECT_EQ(42, ctx.kBT());
}

TEST_F(TestKernelContext, PeriodicBoundary) {
    m::Context ctx;
    ctx.periodicBoundaryConditions() = {{true, false, true}};
    auto boundary = ctx.periodicBoundaryConditions();
    EXPECT_TRUE(boundary[0]);
    EXPECT_FALSE(boundary[1]);
    EXPECT_TRUE(boundary[2]);
}

TEST_F(TestKernelContext, DistanceFunctions) {
    m::Context ctx;
    ctx.periodicBoundaryConditions() = {{true, true, true}};
    ctx.boxSize() = {{2, 2, 2}};
    ctx.configure();
    ctx.boxSize() = {{4, 4, 4}};
    ctx.configure();
    auto distSquared = ctx.distSquaredFun();
    readdy::Vec3 v1(-1.5, -1.5, 0.);
    readdy::Vec3 v2(+1.5, +1.5, 0.);
    EXPECT_NEAR(distSquared(v1, v2), 2., 1e-9);
}

TEST_F(TestKernelContext, BoxSize) {
    m::Context ctx;
    ctx.boxSize() = {{10, 11, 12}};
    auto box_size = ctx.boxSize();
    EXPECT_EQ(box_size[0], 10);
    EXPECT_EQ(box_size[1], 11);
    EXPECT_EQ(box_size[2], 12);
}

TEST_F(TestKernelContext, Copyable) {
    m::Context context;
    m::Context copy(context);
}

TEST_F(TestKernelContext, PotentialOrder2Map) {
    m::Context ctx;
    ctx.particle_types().add("a", 1.);
    ctx.particle_types().add("b", 1.);
    auto noop = std::make_unique<readdy::testing::NOOPPotentialOrder2>(ctx.particle_types()("a"), ctx.particle_types()("b"));
    ctx.potentials().addUserDefined(noop.get());
    auto noop2 = std::make_unique<readdy::testing::NOOPPotentialOrder2>(ctx.particle_types()("b"),ctx.particle_types()( "a"));
    ctx.potentials().addUserDefined(noop2.get());
    ctx.configure();
    auto vector = ctx.potentials().potentialsOf("b", "a");
    EXPECT_EQ(vector.size(), 2);
}

TEST_P(TestKernelContextWithKernels, PotentialOrder1Map) {
    using vec_t = readdy::Vec3;
    auto kernel = readdy::plugin::KernelProvider::getInstance().create("SingleCPU");

    namespace rmp = readdy::model::potentials;

    auto &ctx = kernel->context();

    ctx.particle_types().add("A", 1.0);
    ctx.particle_types().add("B", 3.0);
    ctx.particle_types().add("C", 4.0);
    ctx.particle_types().add("D", 2.0);

    std::vector<short> idsToRemove;
    short uuid2_1, uuid2_2;
    {
        auto id1 = kernel->context().potentials().addBox("A", 0, vec_t{0, 0, 0}, vec_t{0, 0, 0});
        auto id2 = kernel->context().potentials().addBox("C", 0, vec_t{0, 0, 0}, vec_t{0, 0, 0});
        auto id3 = kernel->context().potentials().addBox("D", 0, vec_t{0, 0, 0}, vec_t{0, 0, 0});
        auto id4 = kernel->context().potentials().addBox("C", 0, vec_t{0, 0, 0}, vec_t{0, 0, 0});

        idsToRemove.push_back(id1);
        idsToRemove.push_back(id2);
        idsToRemove.push_back(id3);
        idsToRemove.push_back(id4);

        kernel->context().potentials().addBox("B", 0, vec_t{0, 0, 0}, vec_t{0, 0, 0});

        uuid2_1 = kernel->context().potentials().addHarmonicRepulsion("A", "C", 0, 4.0);
        uuid2_2 = kernel->context().potentials().addHarmonicRepulsion("B", "C", 0, 5.0);
        ctx.configure();
    }
    // test that order 1 potentials are set up correctly
    {
        {
            const auto &pot1_A = ctx.potentials().potentialsOf("A");
            EXPECT_EQ(pot1_A.size(), 1);
        }
        {
            const auto &pot1_B = ctx.potentials().potentialsOf("B");
            EXPECT_EQ(pot1_B.size(), 1);
        }
        {
            const auto &pot1_C = ctx.potentials().potentialsOf("C");
            EXPECT_EQ(pot1_C.size(), 2);
        }
        {
            const auto &pot1_D = ctx.potentials().potentialsOf("D");
            EXPECT_EQ(pot1_D.size(), 1);
        }
    }
    // test that order 2 potentials are set up correctly
    {
        EXPECT_EQ(ctx.potentials().potentialsOf("A", "A").size(), 0);
        EXPECT_EQ(ctx.potentials().potentialsOf("A", "B").size(), 0);
        EXPECT_EQ(ctx.potentials().potentialsOf("A", "D").size(), 0);
        EXPECT_EQ(ctx.potentials().potentialsOf("B", "B").size(), 0);
        EXPECT_EQ(ctx.potentials().potentialsOf("B", "D").size(), 0);

        EXPECT_EQ(ctx.potentials().potentialsOf("A", "C").size(), 1);
        EXPECT_EQ(ctx.potentials().potentialsOf("B", "C").size(), 1);
        {
            const auto &pot2_AC = ctx.potentials().potentialsOf("A", "C");
            EXPECT_EQ(pot2_AC[0]->getId(), uuid2_1);
        }
        {
            const auto &pot2_BC = ctx.potentials().potentialsOf("B", "C");
            EXPECT_EQ(pot2_BC[0]->getId(), uuid2_2);
        }
    }

    // now remove
    std::for_each(idsToRemove.begin(), idsToRemove.end(), [&](const short id) { ctx.potentials().remove(id); });
    {
        // only one potential for particle type B has a different uuid
        ctx.configure();
        EXPECT_EQ(ctx.potentials().potentialsOf("A").size(), 0);
        EXPECT_EQ(ctx.potentials().potentialsOf("B").size(), 1);
        EXPECT_EQ(ctx.potentials().potentialsOf("C").size(), 0);
        EXPECT_EQ(ctx.potentials().potentialsOf("D").size(), 0);

        // remove 2nd potential
        ctx.potentials().remove(uuid2_2);
        ctx.configure();
        EXPECT_EQ(ctx.potentials().potentialsOf("A", "C").size(), 1);
        EXPECT_EQ(ctx.potentials().potentialsOf("B", "C").size(), 0);
    }

}

TEST_F(TestKernelContext, ReactionDescriptorAddReactions) {
    const auto prepareCtx = [](m::Context &ctx, const std::string& descriptor, readdy::scalar rate){
        ctx.particle_types().add("A", 1.);
        ctx.particle_types().add("B", 1.);
        ctx.particle_types().add("C", 1.);
        ctx.reactions().add(descriptor, rate);
        ctx.configure();
    };
    {
        m::Context ctx;
        auto decay = "mydecay:A->";
        prepareCtx(ctx, decay, 2.);
        const auto &r = ctx.reactions().order1ByName("mydecay");
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->type(), m::reactions::ReactionType::Decay);
        EXPECT_EQ(r->nEducts(), 1);
        EXPECT_EQ(r->nProducts(), 0);
        EXPECT_EQ(r->educts()[0], ctx.particle_types().idOf("A"));
        EXPECT_EQ(r->rate(), 2.);
    }
    {
        m::Context ctx;
        auto conversion = "myconv: A -> B";
        prepareCtx(ctx, conversion, 3.);
        const auto &r = ctx.reactions().order1ByName("myconv");
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->type(), m::reactions::ReactionType::Conversion);
        EXPECT_EQ(r->nEducts(), 1);
        EXPECT_EQ(r->nProducts(), 1);
        EXPECT_EQ(r->educts()[0], ctx.particle_types().idOf("A"));
        EXPECT_EQ(r->products()[0], ctx.particle_types().idOf("B"));
        EXPECT_EQ(r->rate(), 3.);
    }
    {
        m::Context ctx;
        auto fusion = "myfus: B +(1.2) B -> C [0.5, 0.5]";
        prepareCtx(ctx, fusion, 4.);
        const auto &r = ctx.reactions().order2ByName("myfus");
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->type(), m::reactions::ReactionType::Fusion);
        EXPECT_EQ(r->nEducts(), 2);
        EXPECT_EQ(r->nProducts(), 1);
        EXPECT_EQ(r->educts()[0], ctx.particle_types().idOf("B"));
        EXPECT_EQ(r->educts()[1], ctx.particle_types().idOf("B"));
        EXPECT_EQ(r->products()[0], ctx.particle_types().idOf("C"));
        EXPECT_EQ(r->eductDistance(), 1.2);
        EXPECT_EQ(r->weight1(), 0.5);
        EXPECT_EQ(r->weight2(), 0.5);
        EXPECT_EQ(r->rate(), 4.);
    }
    {
        m::Context ctx;
        auto fission = "myfiss: B -> C +(3.0) B [0.1, 0.9]";
        prepareCtx(ctx, fission, 5.);
        const auto &r = ctx.reactions().order1ByName("myfiss");
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->type(), m::reactions::ReactionType::Fission);
        EXPECT_EQ(r->nEducts(), 1);
        EXPECT_EQ(r->nProducts(), 2);
        EXPECT_EQ(r->educts()[0], ctx.particle_types().idOf("B"));
        EXPECT_EQ(r->products()[0], ctx.particle_types().idOf("C"));
        EXPECT_EQ(r->products()[1], ctx.particle_types().idOf("B"));
        EXPECT_EQ(r->productDistance(), 3.0);
        EXPECT_EQ(r->weight1(), 0.1);
        EXPECT_EQ(r->weight2(), 0.9);
        EXPECT_EQ(r->rate(), 5.);
    }
    {
        m::Context ctx;
        auto enzymatic = "myenz:A +(1.5) C -> B + C";
        prepareCtx(ctx, enzymatic, 6.);
        const auto &r = ctx.reactions().order2ByName("myenz");
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->type(), m::reactions::ReactionType::Enzymatic);
        EXPECT_EQ(r->nEducts(), 2);
        EXPECT_EQ(r->nProducts(), 2);
        EXPECT_EQ(r->educts()[0], ctx.particle_types().idOf("A"));
        EXPECT_EQ(r->educts()[1], ctx.particle_types().idOf("C"));
        EXPECT_EQ(r->products()[0], ctx.particle_types().idOf("B"));
        EXPECT_EQ(r->products()[1], ctx.particle_types().idOf("C"));
        EXPECT_EQ(r->eductDistance(), 1.5);
        EXPECT_EQ(r->rate(), 6.);
    }
    {
        m::Context ctx;
        auto enzymatic = "eat:B +(1) A -> A + A";
        prepareCtx(ctx, enzymatic, 7.);
        const auto &r = ctx.reactions().order2ByName("eat");
        ASSERT_NE(r, nullptr);
        EXPECT_EQ(r->type(), m::reactions::ReactionType::Enzymatic);
        EXPECT_EQ(r->nEducts(), 2);
        EXPECT_EQ(r->nProducts(), 2);
        EXPECT_EQ(r->educts()[0], ctx.particle_types().idOf("B"));
        EXPECT_EQ(r->educts()[1], ctx.particle_types().idOf("A"));
        EXPECT_EQ(r->products()[0], ctx.particle_types().idOf("A"));
        EXPECT_EQ(r->products()[1], ctx.particle_types().idOf("A"));
        EXPECT_EQ(r->eductDistance(), 1.);
        EXPECT_EQ(r->rate(), 7.);
    }
}

TEST_F(TestKernelContext, ReactionDescriptorInvalidInputs) {
    m::Context ctx;
    ctx.particle_types().add("A", 1.);
    ctx.particle_types().add("B", 1.);
    std::vector<std::string> inv = {"myinvalid: + A -> B", "noarrow: A B", " : Noname ->", "weights: A + A -> A [0.1, ]", "blub: A (3)+ A -> B", "eat: A +(1) B -> A + A"};
    for (const auto &i : inv) {
        EXPECT_ANY_THROW(ctx.reactions().add(i, 42.));
    }
    ctx.configure();
    EXPECT_EQ(ctx.reactions().nOrder1(), 0);
    EXPECT_EQ(ctx.reactions().nOrder2(), 0);
}

TEST_F(TestKernelContext, ReactionNameExists) {
    m::Context ctx;
    ctx.particle_types().add("A", 1.);
    ctx.reactions().add("bla: A->", 1.);
    EXPECT_ANY_THROW(ctx.reactions().add("bla: A->", 1.));
}

TEST_F(TestKernelContext, ReactionNameAndId) {
    m::Context ctx;
    ctx.particle_types().add("A", 1.);
    ctx.reactions().add("foo: A->", 1.);
    ctx.reactions().add("bla: A+(1)A->A", 1.);
    const auto idFoo = ctx.reactions().idOf("foo");
    const auto idBla = ctx.reactions().idOf("bla");
    EXPECT_GE(idFoo, 0);
    EXPECT_GE(idBla, 0);
    EXPECT_EQ(ctx.reactions().nameOf(idFoo), "foo");
    EXPECT_EQ(ctx.reactions().nameOf(idBla), "bla");
}

TEST_F(TestKernelContext, GetAllReactions) {
    m::Context ctx;
    ctx.particle_types().add("A", 1.);
    ctx.reactions().add("foo: A->", 1.);
    ctx.reactions().add("bla: A+(1)A->A", 1.);
    ctx.reactions().addConversion("conv1", "A", "A", 1.);
    ctx.reactions().addConversion("conv2", "A", "A", 1.);
    ctx.reactions().addFusion("fusion", "A","A", "A", 1., 1.);
    ctx.configure();
    const auto &o1flat = ctx.reactions().order1Flat();
    const auto &o2flat = ctx.reactions().order2Flat();
    EXPECT_EQ(o1flat.size() + o2flat.size(), 5);
}


INSTANTIATE_TEST_CASE_P(TestKernelContext, TestKernelContextWithKernels,
                        ::testing::ValuesIn(readdy::testing::getKernelsToTest()));

}