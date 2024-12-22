#include <LSD/Format.h>
#include <format>

int main() {
	std::printf("%s\n", lsd::format("Ha{:a<5}\nLo{0:l^10}\n{2}\n", 'h', 'l', 12345.34567897f).data());
	std::printf("%s\n", lsd::format("Hello {}\n", "world").data());
	//std::print("{},\n{:A},\n{},\n{}\n", true, 0.0f, true, false);
}
