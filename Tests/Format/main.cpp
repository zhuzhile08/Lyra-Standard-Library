#include <LSD/Format.h>
#include <format>

int main() {
	std::printf("%s\n", lsd::format("Ha{:f<5}a\nLo{0:l^10}", 'h', 'l').data());
	std::print("{},\n{:#},\n{},\n{}\n", true, false, true, false);
}
