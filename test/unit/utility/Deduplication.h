#include <utility>
#include <memory>
#include <bitset>
#include <tuple>

std::pair<char, bool> p = {'a', true};
std::tuple<bool, bool, bool> t = {true, true, false};

std::bitset <8> b = 0b10101010;
std::unique_ptr<int> up = std::make_unique<int>(5);
std::shared_ptr<int> sp = std::make_shared<int>(5);
std::weak_ptr<int> wp = sp;