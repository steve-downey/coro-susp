#include <suspension.h>
#include <iostream>

using namespace suspension;

int f(int i)
{
    std::cout << "i:" << i << '\n';
  return i;
}

int f2(int i, int j)
{
  //  std::cout << "i:" << i << '\n';
  return i + j;
}

namespace {
struct NoDefault {
  int _i;

 public:
  NoDefault(int i) : _i(i)
  {
  }
  NoDefault() = delete;

};

std::ostream& operator<<(std::ostream& os, const NoDefault& v)
{
  return (os << "v._i=" << v._i);
}
}

NoDefault noDefault()
{
  return NoDefault{7};
}

suspension<int> getSusp(int i)
{
  //  std::cout << "12?\n";
  std::cout << "getSusp i: " << i << '\n';
  auto s = suspend(f, i);
  // std::cout << "force:" << s << '\n';
  return s;
}

int main(int, char**)
{
  std::cout << "main\n";
  std::cout << "about to create suspension\n";
  auto s = suspend(f, 2);
  std::cout << "after suspension\n";
  int i = s;
  std::cout << i << '\n';
  std::cout << s << '\n';

  std::cout << "about to create suspension3\n";
  auto s3 = suspend(f, 3);
  std::cout << "after suspension\n";

  std::cout << "about to create suspension4\n";
  auto s4 = suspend(noDefault);
  std::cout << "after suspension\n";
  std::cout << s4.get()._i << '\n';
  std::cout << s4.get() << '\n';

  auto s12 = getSusp(12);
  std::cout << s12 << '\n';
  int x = s12;
  std::cout << "x:" << x << '\n';
   auto s12a = s12;
  std::cout << "s12a:" << s12a << '\n';

  auto s17  = getSusp(17);
  auto s17a = s17;
  std::cout << s17 << '\n';
  std::cout << s17a << '\n';

  //return s + s4.get()._i;

  auto s9 = suspend(f2, 8, 1);
  std::cout << "after suspension\n";
  int i9 = s9;
  std::cout << "i9: " << i9 << '\n';
  std::cout << s9 << '\n';

  suspension<int> D5 = suspend([]() { return f(7); });

  suspension<int> D6 = suspend(f, 8);

  suspension<int> D7 = suspend(f2, 10, 1);

  std::cout << D5 << " = 7\n";
  std::cout << D6 << " = 8\n";
  std::cout << D7 << " = 11\n";
}
