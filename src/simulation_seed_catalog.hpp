#pragma once

struct SimulationSeed {
    std::string name;
    std::string description;
    Bytes capture;
};

class SimulationSeedCatalog {
public:
    std::vector<SimulationSeed> build(const SimulationParameters& parameters) const;
    std::vector<std::string> validate(const std::vector<SimulationSeed>& seeds) const;
    std::string manifest(const std::vector<SimulationSeed>& seeds) const;
};
