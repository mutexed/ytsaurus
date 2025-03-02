#ifndef TRANSACTION_MANAGER_DETAIL_INL_H_
#error "Direct inclusion of this file is not allowed, include transaction_manager_detail.h"
// For the sake of sane code completion.
#include "transaction_manager_detail.h"
#endif

#include <yt/yt/core/misc/collection_helpers.h>

namespace NYT::NTransactionSupervisor {

////////////////////////////////////////////////////////////////////////////////

template <class TTransaction>
void TTransactionManagerBase<TTransaction>::RegisterTransactionActionHandlers(
    TTransactionActionDescriptor<TTransaction> handlers)
{
    auto type = handlers.Type();
    EmplaceOrCrash(ActionHandlerMap_, type, std::move(handlers));
}

template <class TTransaction>
void TTransactionManagerBase<TTransaction>::RunPrepareTransactionActions(
    TTransaction* transaction,
    const TTransactionPrepareOptions& options)
{
    TTransactionActionGuard transactionActionGuard;
    for (const auto& action : transaction->Actions()) {
        try {
            auto it = ActionHandlerMap_.find(action.Type);
            if (it == ActionHandlerMap_.end()) {
                THROW_ERROR_EXCEPTION("Action %Qv is not registered",
                    action.Type);
            }
            it->second.Prepare(transaction, action.Value, options);
        } catch (const std::exception& ex) {
            YT_LOG_DEBUG(ex, "Prepare action failed (TransactionId: %v, ActionType: %v)",
                transaction->GetId(),
                action.Type);
            throw;
        }
    }
}

template <class TTransaction>
void TTransactionManagerBase<TTransaction>::RunCommitTransactionActions(
    TTransaction* transaction,
    const TTransactionCommitOptions& options)
{
    TTransactionActionGuard transactionActionGuard;
    for (const auto& action : transaction->Actions()) {
        try {
            auto it = ActionHandlerMap_.find(action.Type);
            if (it == ActionHandlerMap_.end()) {
                THROW_ERROR_EXCEPTION("Action %Qv is not registered",
                    action.Type);
            }
            it->second.Commit(transaction, action.Value, options);
        } catch (const std::exception& ex) {
            YT_LOG_ALERT(ex, "Commit action failed (TransactionId: %v, ActionType: %v)",
                transaction->GetId(),
                action.Type);
        }
    }
}

template <class TTransaction>
void TTransactionManagerBase<TTransaction>::RunAbortTransactionActions(
    TTransaction* transaction,
    const TTransactionAbortOptions& options)
{
    TTransactionActionGuard transactionActionGuard;
    for (const auto& action : transaction->Actions()) {
        try {
            auto it = ActionHandlerMap_.find(action.Type);
            if (it == ActionHandlerMap_.end()) {
                THROW_ERROR_EXCEPTION("Action %Qv is not registered",
                    action.Type);
            }
            it->second.Abort(transaction, action.Value, options);
        } catch (const std::exception& ex) {
            YT_LOG_ALERT(ex, "Abort action failed (TransactionId: %v, ActionType: %v)",
                transaction->GetId(),
                action.Type);
        }
    }
}

template <class TTransaction>
void TTransactionManagerBase<TTransaction>::RunSerializeTransactionActions(TTransaction* transaction)
{
    for (const auto& action : transaction->Actions()) {
        try {
            if (auto it = ActionHandlerMap_.find(action.Type); it != ActionHandlerMap_.end()) {
                it->second.Serialize(transaction, action.Value);
            }
        } catch (const std::exception& ex) {
            YT_LOG_ALERT(ex, "Serialize action failed (TransactionId: %v, ActionType: %v)",
                transaction->GetId(),
                action.Type);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT::NTransactionSupervisor
