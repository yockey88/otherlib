/**
 * \file plugins/manet_sample.cpp
 * set `networking.session-host = "manet-sample"` to run the toy MANET (one mesh, N
 *  actors, disc radio, no engine edits) instead of client-server; scenario shared with tests/network/manet_scenario.hpp
 **/
#include "plugin/plugin.hpp"

#include "network/manet_scenario.hpp"

/// the provide macro pastes the implementation type into the factory symbol name,
///  so it must be visible unqualified
using other::manet_director;

OTHER_PROVIDES(manet_director, other::peer_actor, "manet-sample");
OTHER_PLUGIN(manet_sample, "0.1.0", "N/A", "N/A")
