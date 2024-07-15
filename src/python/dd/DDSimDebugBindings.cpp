#include "python/dd/DDSimDebugBindings.hpp"

#include "backend/dd/DDSimDebug.hpp"

void bindBackend(pybind11::module& m) {

  m.def("create_ddsim_simulation_state", []() {
    auto* state = new DDSimulationState();
    createDDSimulationState(state);
    return &state->interface;
  });

  m.def("destroy_ddsim_simulation_state", [](SimulationState* state) {
    destroyDDSimulationState(reinterpret_cast<DDSimulationState*>(state));
  });
}
