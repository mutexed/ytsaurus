#include "bundle_controller_settings.h"

namespace NYT::NBundleControllerClient {

////////////////////////////////////////////////////////////////////////////////

void TCpuLimits::Register(TRegistrar registrar)
{
    registrar.Parameter("write_thread_pool_size", &TThis::WriteThreadPoolSize)
        .Optional();
    registrar.Parameter("lookup_thread_pool_size", &TThis::LookupThreadPoolSize)
        .Optional();
    registrar.Parameter("query_thread_pool_size", &TThis::QueryThreadPoolSize)
        .Optional();
}

void TMemoryLimits::Register(TRegistrar registrar)
{
    registrar.Parameter("tablet_static", &TThis::TabletStatic)
        .Optional();
    registrar.Parameter("tablet_dynamic", &TThis::TabletDynamic)
        .Optional();
    registrar.Parameter("compressed_block_cache", &TThis::CompressedBlockCache)
        .Optional();
    registrar.Parameter("uncompressed_block_cache", &TThis::UncompressedBlockCache)
        .Optional();
    registrar.Parameter("key_filter_block_cache", &TThis::KeyFilterBlockCache)
        .Optional();
    registrar.Parameter("versioned_chunk_meta", &TThis::VersionedChunkMeta)
        .Optional();
    registrar.Parameter("lookup_row_cache", &TThis::LookupRowCache)
        .Optional();
}

void TInstanceResources::Register(TRegistrar registrar)
{
    registrar.Parameter("vcpu", &TThis::Vcpu)
        .GreaterThanOrEqual(0)
        .Default(18000);
    registrar.Parameter("memory", &TThis::Memory)
        .GreaterThanOrEqual(0)
        .Default(120_GB);
    registrar.Parameter("net", &TThis::Net)
        .Optional();
    registrar.Parameter("type", &TThis::Type)
        .Default();
}

void TInstanceResources::Clear()
{
    Vcpu = 0;
    Memory = 0;
}

bool TInstanceResources::operator==(const TInstanceResources& other) const
{
    return std::tie(Vcpu, Memory, Net) == std::tie(other.Vcpu, other.Memory, other.Net);
}

void TBundleTargetConfig::Register(TRegistrar registrar)
{
    registrar.Parameter("cpu_limits", &TThis::CpuLimits)
        .DefaultNew();
    registrar.Parameter("memory_limits", &TThis::MemoryLimits)
        .DefaultNew();
    registrar.Parameter("rpc_proxy_count", &TThis::RpcProxyCount)
        .Optional();
    registrar.Parameter("rpc_proxy_resource_guarantee", &TThis::RpcProxyResourceGuarantee)
        .Default();
    registrar.Parameter("tablet_node_count", &TThis::TabletNodeCount)
        .Optional();
    registrar.Parameter("tablet_node_resource_guarantee", &TThis::TabletNodeResourceGuarantee)
        .Default();
}

////////////////////////////////////////////////////////////////////////////////

namespace NProto {

////////////////////////////////////////////////////////////////////////////////

#define YT_FROMPROTO_OPTIONAL_PTR(messagePtr, messageField, structPtr, structField) (((messagePtr)->has_##messageField()) ? (structPtr)->structField = (messagePtr)->messageField() : (structPtr)->structField)
#define YT_TOPROTO_OPTIONAL_PTR(messagePtr, messageField, structPtr, structField) (((structPtr)->structField.has_value()) ? (messagePtr)->set_##messageField((structPtr)->structField.value()) : void())

////////////////////////////////////////////////////////////////////////////////

void ToProto(NBundleController::NProto::TCpuLimits* protoCpuLimits, const NBundleControllerClient::TCpuLimitsPtr cpuLimits)
{
    YT_TOPROTO_OPTIONAL_PTR(protoCpuLimits, lookup_thread_pool_size, cpuLimits, LookupThreadPoolSize);
    YT_TOPROTO_OPTIONAL_PTR(protoCpuLimits, query_thread_pool_size, cpuLimits, QueryThreadPoolSize);
    YT_TOPROTO_OPTIONAL_PTR(protoCpuLimits, write_thread_pool_size, cpuLimits, WriteThreadPoolSize);
}

void FromProto(NBundleControllerClient::TCpuLimitsPtr cpuLimits, const NBundleController::NProto::TCpuLimits* protoCpuLimits)
{
    YT_FROMPROTO_OPTIONAL_PTR(protoCpuLimits, lookup_thread_pool_size, cpuLimits, LookupThreadPoolSize);
    YT_FROMPROTO_OPTIONAL_PTR(protoCpuLimits, query_thread_pool_size, cpuLimits, QueryThreadPoolSize);
    YT_FROMPROTO_OPTIONAL_PTR(protoCpuLimits, write_thread_pool_size, cpuLimits, WriteThreadPoolSize);
}

////////////////////////////////////////////////////////////////////////////////

void ToProto(NBundleController::NProto::TMemoryLimits* protoMemoryLimits, const NBundleControllerClient::TMemoryLimitsPtr memoryLimits)
{
    YT_TOPROTO_OPTIONAL_PTR(protoMemoryLimits, compressed_block_cache, memoryLimits, CompressedBlockCache);
    YT_TOPROTO_OPTIONAL_PTR(protoMemoryLimits, key_filter_block_cache, memoryLimits, KeyFilterBlockCache);
    YT_TOPROTO_OPTIONAL_PTR(protoMemoryLimits, lookup_row_cache, memoryLimits, LookupRowCache);

    YT_TOPROTO_OPTIONAL_PTR(protoMemoryLimits, tablet_dynamic, memoryLimits, TabletDynamic);
    YT_TOPROTO_OPTIONAL_PTR(protoMemoryLimits, tablet_static, memoryLimits, TabletStatic);

    YT_TOPROTO_OPTIONAL_PTR(protoMemoryLimits, uncompressed_block_cache, memoryLimits, UncompressedBlockCache);

    YT_TOPROTO_OPTIONAL_PTR(protoMemoryLimits, versioned_chunk_meta, memoryLimits, VersionedChunkMeta);
}

void FromProto(NBundleControllerClient::TMemoryLimitsPtr memoryLimits, const NBundleController::NProto::TMemoryLimits* protoMemoryLimits)
{
    YT_FROMPROTO_OPTIONAL_PTR(protoMemoryLimits, compressed_block_cache, memoryLimits, CompressedBlockCache);
    YT_FROMPROTO_OPTIONAL_PTR(protoMemoryLimits, key_filter_block_cache, memoryLimits, KeyFilterBlockCache);
    YT_FROMPROTO_OPTIONAL_PTR(protoMemoryLimits, lookup_row_cache, memoryLimits, LookupRowCache);

    YT_FROMPROTO_OPTIONAL_PTR(protoMemoryLimits, tablet_dynamic, memoryLimits, TabletDynamic);
    YT_FROMPROTO_OPTIONAL_PTR(protoMemoryLimits, tablet_static, memoryLimits, TabletStatic);

    YT_FROMPROTO_OPTIONAL_PTR(protoMemoryLimits, uncompressed_block_cache, memoryLimits, UncompressedBlockCache);

    YT_FROMPROTO_OPTIONAL_PTR(protoMemoryLimits, versioned_chunk_meta, memoryLimits, VersionedChunkMeta);
}

////////////////////////////////////////////////////////////////////////////////

void ToProto(NBundleController::NProto::TInstanceResources* protoInstanceResources, const NBundleControllerClient::TInstanceResourcesPtr instanceResources)
{
    if (instanceResources == nullptr) return;
    protoInstanceResources->set_memory(instanceResources->Memory);
    YT_TOPROTO_OPTIONAL_PTR(protoInstanceResources, net, instanceResources, Net);
    protoInstanceResources->set_type(instanceResources->Type);
    protoInstanceResources->set_vcpu(instanceResources->Vcpu);
}

void FromProto(NBundleControllerClient::TInstanceResourcesPtr instanceResources, const NBundleController::NProto::TInstanceResources* protoInstanceResources)
{
    YT_FROMPROTO_OPTIONAL_PTR(protoInstanceResources, memory, instanceResources, Memory);
    YT_FROMPROTO_OPTIONAL_PTR(protoInstanceResources, net, instanceResources, Net);
    YT_FROMPROTO_OPTIONAL_PTR(protoInstanceResources, type, instanceResources, Type);
    YT_FROMPROTO_OPTIONAL_PTR(protoInstanceResources, vcpu, instanceResources, Vcpu);
}

////////////////////////////////////////////////////////////////////////////////

void ToProto(NBundleController::NProto::TBundleConfig* protoBundleConfig, const NBundleControllerClient::TBundleTargetConfigPtr bundleConfig)
{
    YT_TOPROTO_OPTIONAL_PTR(protoBundleConfig, rpc_proxy_count, bundleConfig, RpcProxyCount);
    YT_TOPROTO_OPTIONAL_PTR(protoBundleConfig, tablet_node_count, bundleConfig, TabletNodeCount);
    ToProto(protoBundleConfig->mutable_cpu_limits(), bundleConfig->CpuLimits);
    ToProto(protoBundleConfig->mutable_memory_limits(), bundleConfig->MemoryLimits);
    ToProto(protoBundleConfig->mutable_rpc_proxy_resource_guarantee(), bundleConfig->RpcProxyResourceGuarantee);
    ToProto(protoBundleConfig->mutable_tablet_node_resource_guarantee(), bundleConfig->TabletNodeResourceGuarantee);
}

void FromProto(NBundleControllerClient::TBundleTargetConfigPtr bundleConfig, const NBundleController::NProto::TBundleConfig* protoBundleConfig)
{
    YT_FROMPROTO_OPTIONAL_PTR(protoBundleConfig, rpc_proxy_count, bundleConfig, RpcProxyCount);
    YT_FROMPROTO_OPTIONAL_PTR(protoBundleConfig, tablet_node_count, bundleConfig, TabletNodeCount);
    if (protoBundleConfig->has_cpu_limits())
        FromProto(bundleConfig->CpuLimits, &protoBundleConfig->cpu_limits());
    if (protoBundleConfig->has_memory_limits())
        FromProto(bundleConfig->MemoryLimits, &protoBundleConfig->memory_limits());
    if (protoBundleConfig->has_rpc_proxy_resource_guarantee())
        FromProto(bundleConfig->RpcProxyResourceGuarantee, &protoBundleConfig->rpc_proxy_resource_guarantee());
    if (protoBundleConfig->has_tablet_node_resource_guarantee())
        FromProto(bundleConfig->TabletNodeResourceGuarantee, &protoBundleConfig->tablet_node_resource_guarantee());
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NProto

} // namespace NYT::NBundleControllerClient
