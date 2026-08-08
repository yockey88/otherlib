/**
 * \file plugins/manet_sample.cpp
 *
 * the Mode-4 exemplar as shipped content: set `networking.session-host =
 * "manet-sample"` and the driver mesh runs the toy MANET instead of the default
 * client-server session — one mesh, N member actors, disc radio over the memory
 * fabric, no engine edits. the scenario itself lives in tests/network/
 * manet_scenario.hpp so the deterministic test and this plugin share one source.
 **/
#include "plugin/plugin.hpp"

#include "network/manet_scenario.hpp"

/// the provide macro pastes the implementation type into the factory symbol name,
///  so it must be visible unqualified
using other::manet_director;

OTHER_PROVIDES(manet_director, other::peer_mesh_actor, "manet-sample");
OTHER_PLUGIN(manet_sample, "0.1.0", "N/A", "N/A")
