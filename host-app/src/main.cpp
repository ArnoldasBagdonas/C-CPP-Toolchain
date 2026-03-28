
// Simple Hello World C++ example
#include <iostream>
#include <example/add.h>

int main() {
	std::cout << "Hello, world!" << std::endl;
	int result = add(2, 3);
	std::cout << "add(2, 3) = " << result << std::endl;
	return 0;
}
