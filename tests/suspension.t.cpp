#include <suspension.h>

#include "gtest/gtest.h"
using ::testing::Test;
using namespace suspension;

namespace testing {
namespace {
int func_called;
int func2_called;
int func3_called;
}
class SuspensionTest : public Test {
  protected:
    SuspensionTest() {
    }
    ~SuspensionTest() {
    }

    virtual void SetUp() {
        func_called = 0;
        func2_called = 0;
        func3_called = 0;
    }

    virtual void TearDown() {
    }

  public:
};

namespace {
int func() {
    func_called++;
    return 5;
}

int func2(int i) {
    func2_called++;
    return i;
}

int func3(int i, int j) {
    func3_called++;
    return i + j;
}

std::string stringTest(const char* str) {
    return str;
}

suspension<int> get_susp(int i)
{
    auto s = suspend(func2, i);
    return s;
}

}


TEST_F(SuspensionTest, breathingTest) {
    suspension<int> D1(1);

    int j{D1};

    EXPECT_EQ(1, j);

    suspension<int> D2 = suspend(func);
    EXPECT_EQ(0, func_called);

    int k = D2;
    EXPECT_EQ(1, func_called);
    EXPECT_EQ(5, k);

    suspension<int> D3 = suspend(func);

    suspension<int> D4 = suspend(func2, 3);
    EXPECT_EQ(0, func2_called);

    suspension<int> D5 = suspend([]() { return func2(7); });

    suspension<int> D6 = suspend(func2, 8);

    suspension<int> D7 = suspend(func3, 10, 1);

    EXPECT_EQ(1, func_called);
    EXPECT_EQ(0, func2_called);
    EXPECT_EQ(0, func3_called);

    EXPECT_EQ(5, D3.get());
    EXPECT_EQ(2, func_called);

    EXPECT_EQ(3, D4.get());
    EXPECT_EQ(1, func2_called);

    EXPECT_EQ(7, D5.get());
    EXPECT_EQ(2, func2_called);

    EXPECT_EQ(8, D6.get());
    EXPECT_EQ(3, func2_called);

    EXPECT_EQ(11, D7.get());
    EXPECT_EQ(1, func3_called);

    EXPECT_EQ(2, func_called);
    EXPECT_EQ(3, func2_called);
    EXPECT_EQ(1, func3_called);

    EXPECT_EQ(1, static_cast<int>(D1));
    EXPECT_EQ(5, static_cast<int>(D2));
    EXPECT_EQ(5, static_cast<int>(D3));
    EXPECT_EQ(3, static_cast<int>(D4));
    EXPECT_EQ(7, static_cast<int>(D5));
    EXPECT_EQ(8, static_cast<int>(D6));
    EXPECT_EQ(11, force(D7));

    EXPECT_EQ(2, func_called);
    EXPECT_EQ(3, func2_called);
    EXPECT_EQ(1, func3_called);
}

TEST_F(SuspensionTest, moveTest) {
    std::string str;
    suspension<std::string> d1(str);
    suspension<std::string> d2("test");
    suspension<std::string> d3 = suspend(stringTest, "this is a test");
    suspension<std::string> d4 = suspend([](){return stringTest("another test");});

    EXPECT_TRUE(d1.is_forced());
    EXPECT_TRUE(d2.is_forced());
    EXPECT_FALSE(d3.is_forced());
    EXPECT_FALSE(d4.is_forced());

    EXPECT_EQ(std::string("this is a test"), force(d3));
    EXPECT_EQ(std::string("another test"), force(d4));
}

TEST_F(SuspensionTest, copySuspTest) {
    auto s = get_susp(11);
    EXPECT_EQ(11, s.get());
}

namespace {
struct NoDefault
{
    int i_;
  public:
    NoDefault(int i) : i_(i) {};
    NoDefault() = delete;
};

struct NoMove
{
    int i_;
  public:
    NoMove(int i) : i_(i) {};
    NoMove(NoMove const& r) : i_(r.i_) {};
    ~NoMove() {} // No implicit move
};

struct NoCopy
{
    int i_;
  public:
    NoCopy(int i) : i_(i) {}
    NoCopy(NoCopy&&) = default;
    NoCopy(NoCopy const&) = delete;
};
}

TEST_F(SuspensionTest, oddTypes) {
    suspension<NoDefault> d1 = suspend([](){return NoDefault{1};});
    suspension<NoMove> d2 = suspend([](){return NoMove{2};});
    suspension<NoCopy> d3 = suspend([](){return NoCopy{3};});

    EXPECT_EQ(1, force(d1).i_);
    EXPECT_EQ(2, force(d2).i_);
    EXPECT_EQ(3, force(d3).i_);
}

}
