#include "aegis/aegis.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: aegis-inspect FILE\n"; return 2; }
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) { std::cerr << "cannot open input\n"; return 1; }
    aegis::Bytes data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    aegis::Engine engine;
    const auto result = engine.ingest_capture(data);
    std::cout << "accepted=" << result.accepted_frames << " rejected=" << result.rejected_frames << "\n";
    std::cout << engine.inspect();
    return result.rejected_frames == 0 ? 0 : 1;
}
