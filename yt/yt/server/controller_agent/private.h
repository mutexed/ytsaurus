#pragma once

#include "public.h"

#include <yt/yt/server/lib/controller_agent/persistence.h>

#include <yt/yt/library/profiling/sensor.h>

#include <yt/yt/core/logging/log.h>

#include <yt/yt/core/tracing/trace_context.h>


namespace NYT::NControllerAgent {

////////////////////////////////////////////////////////////////////////////////

DEFINE_ENUM(ELegacyLivePreviewMode,
    (ExplicitlyEnabled)
    (ExplicitlyDisabled)
    (DoNotCare)
    (NotSupported)
);

////////////////////////////////////////////////////////////////////////////////

constexpr auto OperationIdTag = "operation_id";

////////////////////////////////////////////////////////////////////////////////

DECLARE_REFCOUNTED_STRUCT(IOperationController)
using IOperationControllerWeakPtr = TWeakPtr<IOperationController>;

DECLARE_REFCOUNTED_STRUCT(TSnapshotJob)

DECLARE_REFCOUNTED_CLASS(TSnapshotBuilder)
DECLARE_REFCOUNTED_CLASS(TSnapshotDownloader)

DECLARE_REFCOUNTED_CLASS(TChunkListPool)

DECLARE_REFCOUNTED_CLASS(TZombieOperationOrchids)

DECLARE_REFCOUNTED_CLASS(TJobProfiler)

DECLARE_REFCOUNTED_CLASS(TJobTracker)
DECLARE_REFCOUNTED_CLASS(TJobTrackerOperationHandler)
DECLARE_REFCOUNTED_CLASS(TJobTrackerConfig)

DECLARE_REFCOUNTED_CLASS(TDockerRegistryConfig)

struct TSettleJobRequest
{
    TOperationId OperationId;
    TAllocationId AllocationId;
};

struct TJobStartInfo
{
    TJobId JobId;
    TSharedRef JobSpecBlob;
};

struct TStartedJobInfo
{
    TJobId JobId;
    TString NodeAddress;
};

struct TJobMonitoringDescriptor
{
    TIncarnationId IncarnationId;
    int Index;
};

void FormatValue(TStringBuilderBase* builder, const TJobMonitoringDescriptor& descriptor, TStringBuf /*format*/);

TString ToString(const TJobMonitoringDescriptor& descriptor);

struct TLivePreviewTableBase;

////////////////////////////////////////////////////////////////////////////////

inline const NLogging::TLogger ControllerLogger("Controller");
inline const NLogging::TLogger ControllerAgentLogger("ControllerAgent");
inline const NLogging::TLogger ControllerEventLogger("ControllerEventLog");
inline const NLogging::TLogger ControllerFeatureStructuredLogger("ControllerFeatureStructuredLog");

inline const NProfiling::TProfiler ControllerAgentProfiler("/controller_agent");

////////////////////////////////////////////////////////////////////////////////

using TOperationIdToControllerMap = THashMap<TOperationId, IOperationControllerPtr>;
using TOperationIdToWeakControllerMap = THashMap<TOperationId, IOperationControllerWeakPtr>;

////////////////////////////////////////////////////////////////////////////////

//! Creates a child trace context for operation and set allocation tags to it.
//! Returns TraceContextGuard with the trace context.

NTracing::TTraceContextGuard CreateOperationTraceContextGuard(
    TString spanName,
    TOperationId operationId);

////////////////////////////////////////////////////////////////////////////////

struct TCompositePendingJobCount
{
    int DefaultCount = 0;
    THashMap<TString, int> CountByPoolTree = {};

    int GetJobCountFor(const TString& tree) const;
    bool IsZero() const;

    void Persist(const TStreamPersistenceContext& context);
};

void Serialize(const TCompositePendingJobCount& jobCount, NYson::IYsonConsumer* consumer);

void FormatValue(TStringBuilderBase* builder, const TCompositePendingJobCount& jobCount, TStringBuf /*format*/);

bool operator == (const TCompositePendingJobCount& lhs, const TCompositePendingJobCount& rhs);
bool operator != (const TCompositePendingJobCount& lhs, const TCompositePendingJobCount& rhs);

TCompositePendingJobCount operator + (const TCompositePendingJobCount& lhs, const TCompositePendingJobCount& rhs);
TCompositePendingJobCount operator - (const TCompositePendingJobCount& lhs, const TCompositePendingJobCount& rhs);
TCompositePendingJobCount operator - (const TCompositePendingJobCount& count);

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NControllerAgent

