#pragma once

#include "public.h"

#include <yt/yt/core/logging/config.h>

#include <yt/yt/core/ytree/yson_serializable.h>

#include <yt/yt/server/lib/hydra/config.h>

#include <yt/yt/server/lib/election/config.h>

namespace NYT::NHydraStressTest {

//////////////////////////////////////////////////////////////////////////////////

class TConfig
    : public NYTree::TYsonSerializable
{
public:
    int PeerCount;
    int VotingPeerCount;

    int ClientCount;
    int ClientIncrement;
    TDuration ClientInterval;
    TDuration ClientWriteCasDelay;

    TDuration DefaultProxyTimeout;

    int MaxRandomPartitionIterations;
    TDuration RandomPartitionDelay;
    TDuration QuorumPartitionDelay;

    TDuration ClearStatePeriod;
    TDuration BuildSnapshotPeriod;
    TDuration LeaderSwitchPeriod;

    TDuration UnavailabilityTimeout;
    TDuration ResurrectionTimeout;

    NHydra::TDistributedHydraManagerConfigPtr HydraManager;
    NHydra::TFileChangelogStoreConfigPtr Changelogs;
    NHydra::TLocalSnapshotStoreConfigPtr Snapshots;
    NElection::TDistributedElectionManagerConfigPtr ElectionManager;
    NLogging::TLogManagerConfigPtr Logging;

    TConfig();
};

DEFINE_REFCOUNTED_TYPE(TConfig)

//////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NHydraStressTest
