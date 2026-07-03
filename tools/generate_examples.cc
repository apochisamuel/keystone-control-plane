#include "aegis/aegis.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    const char* output = argc > 1 ? argv[1] : "example.aeg";
    aegis::SimulationConfig config;
    config.rounds = 6U;
    config.seed = 41U;
    const auto stream = aegis::make_synthetic_capture(config);
    std::ofstream file(output, std::ios::binary);
    file.write(reinterpret_cast<const char*>(stream.data()), static_cast<std::streamsize>(stream.size()));
    std::cout << "wrote " << stream.size() << " bytes\n";
    return file ? 0 : 1;
}
