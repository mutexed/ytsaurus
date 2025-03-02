#pragma once

#include "periodic_executor.h"
#include "public.h"

#include <yt/yt/core/actions/callback.h>
#include <yt/yt/core/actions/future.h>

#include <yt/yt/core/misc/backoff_strategy_config.h>

namespace NYT::NConcurrency {

////////////////////////////////////////////////////////////////////////////////

namespace NDetail {

class TRetryingInvocationTimePolicy
    : private TDefaultInvocationTimePolicy
{
public:
    using TCallbackResult = TError;
    using TOptions = TRetryingPeriodicExecutorOptions;

    explicit TRetryingInvocationTimePolicy(const TOptions& options);

    void ProcessResult(TCallbackResult result);

    using TDefaultInvocationTimePolicy::KickstartDeadline;

    using TDefaultInvocationTimePolicy::IsEnabled;

    bool ShouldKickstart(const TOptions& newOptions);

    void SetOptions(TOptions newOptions);

    bool ShouldKickstart(
        const std::optional<NConcurrency::TPeriodicExecutorOptions>& periodicOptions,
        const std::optional<TExponentialBackoffOptions>& backofOptions);

    void SetOptions(
        std::optional<NConcurrency::TPeriodicExecutorOptions> periodicOptions,
        std::optional<TExponentialBackoffOptions> backofOptions);

    TInstant NextDeadline();

    bool IsOutOfBandProhibited();

    void Reset();

    TDuration GetBackoffTimeEstimate() const;

private:
    //! Used for backoff time estimation
    std::atomic<TDuration> CachedBackoffDuration_;
    std::atomic<double> CachedBackoffMultiplier_;

    TBackoffStrategy Backoff_;

    bool IsInBackoffMode() const;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace NDetail

////////////////////////////////////////////////////////////////////////////////

class TRetryingPeriodicExecutor
    : public NDetail::TPeriodicExecutorBase<NDetail::TRetryingInvocationTimePolicy>
{
public:
    TRetryingPeriodicExecutor(
        IInvokerPtr invoker,
        TPeriodicCallback callback,
        TRetryingPeriodicExecutorOptions options);

    TRetryingPeriodicExecutor(
        IInvokerPtr invoker,
        TPeriodicCallback callback,
        NConcurrency::TPeriodicExecutorOptions periodicOptions,
        TExponentialBackoffOptions backoffOptions);

    TRetryingPeriodicExecutor(
        IInvokerPtr invoker,
        TPeriodicCallback callback,
        TExponentialBackoffOptions backoffOptions,
        std::optional<TDuration> period = {});

    TDuration GetBackoffTimeEstimate() const;

private:
    using TBase = NDetail::TPeriodicExecutorBase<NDetail::TRetryingInvocationTimePolicy>;
};

DEFINE_REFCOUNTED_TYPE(TRetryingPeriodicExecutor);

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NConcurrency
