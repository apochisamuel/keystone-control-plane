#include "internal.hpp"

namespace aegis::detail {

std::vector<SimulationSeed> SimulationSeedCatalog::build(const SimulationParameters& parameters) const {
    SimulationParameters base = parameters;
    base.rounds = 1U;
    base.seed = parameters.seed == 0U ? 1U : parameters.seed;
    ControlPlaneSimulator simulator;
    std::vector<SimulationSeed> seeds;
    for (const auto& [name, description, rounds, offset] : std::vector<std::tuple<std::string, std::string, std::uint32_t, std::uint32_t>>{
             {"heartbeat_lifecycle", "open, policy commit, data, and close", 1U, 0U},
             {"steady_data", "short steady-state data session", 4U, 11U},
             {"handover_path", "data session with handover and audit", 6U, 29U},
             {"long_window", "longer bounded data sequence", 12U, 47U}}) {
        auto config = base;
        config.rounds = rounds;
        config.seed += offset;
        seeds.push_back({name, description, simulator.capture(config)});
    }
    return seeds;
}

std::vector<std::string> SimulationSeedCatalog::validate(const std::vector<SimulationSeed>& seeds) const {
    std::vector<std::string> errors;
    std::set<std::string> names;
    for (const auto& seed : seeds) {
        if (!names.insert(seed.name).second) errors.push_back("duplicate seed name: " + seed.name);
        if (seed.capture.size() < 64U) errors.push_back("seed too small: " + seed.name);
        std::vector<std::string> parse_errors;
        const auto frames = parse_capture(seed.capture, parse_errors);
        if (!parse_errors.empty() || frames.empty()) errors.push_back("seed does not parse: " + seed.name);
        if (!frames.empty() && frames.front().operation != Operation::Open) errors.push_back("seed does not begin with open: " + seed.name);
    }
    return errors;
}

std::string SimulationSeedCatalog::manifest(const std::vector<SimulationSeed>& seeds) const {
    std::vector<std::vector<std::string>> rows{{"name", "bytes", "description"}};
    for (const auto& seed : seeds) rows.push_back({seed.name, std::to_string(seed.capture.size()), seed.description});
    return render_table(rows);
}

}  // namespace aegis::detail
