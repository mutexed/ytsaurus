#include "parallel_file_writer.h"

#include <yt/cpp/mapreduce/common/helpers.h>

#include <yt/cpp/mapreduce/interface/config.h>

#include <library/cpp/iterator/functools.h>

#include <library/cpp/threading/blocking_queue/blocking_queue.h>
#include <library/cpp/threading/future/future.h>
#include <library/cpp/threading/future/async.h>

#include <util/generic/guid.h>

#include <util/string/builder.h>

#include <util/system/fstat.h>
#include <util/system/info.h>
#include <util/system/mutex.h>
#include <util/system/thread.h>

#include <util/thread/pool.h>

namespace NYT {
namespace NDetail {

using ::ToString;

////////////////////////////////////////////////////////////////////////////////

class TParallelFileWriter
    : public IParallelFileWriter
{
public:
    /// @brief Write file in parallel.
    /// @param client       Client which used for write file on server.
    /// @param fileName     Source path to file in local storage.
    /// @param path         Dist path to file on server.
    TParallelFileWriter(
        const IClientBasePtr& client,
        const TRichYPath& path,
        const std::shared_ptr<IThreadPool>& threadPool,
        const TParallelFileWriterOptions& options);

    ~TParallelFileWriter();

    void Write(TSharedRef blob) override;

    void WriteFile(const TString& fileName) override;

    void Finish() override;

private:
    struct IWriteTask;

    NThreading::TFuture<void> StartWriteTask(
        std::unique_ptr<IWriteTask> task,
        TRichYPath filePath
    );

    TRichYPath CreateFilePath(const std::pair<size_t, size_t>& taskId);

    static void* ThreadWrite(void* opaque);

    void ThreadWrite(int index, i64 start, i64 length);

    void ThrowIfDead();

private:
    struct IWriteTask
    {
        virtual ~IWriteTask() = default;
        virtual void Write(const IFileWriterPtr& writer, std::atomic<bool>& hasException) = 0;
        virtual size_t GetDataSize() const = 0;
    };

    struct TBlobWriteTask
        : public IWriteTask
    {
        TBlobWriteTask(TSharedRef blob)
            : Blob(std::move(blob))
        { }

        void Write(const IFileWriterPtr& writer, std::atomic<bool>& /*hasException*/) override
        {
            writer->Write(Blob.begin(), Blob.size());
        }

        size_t GetDataSize() const override
        {
            return Blob.Size();
        }

        TSharedRef Blob;
    };

    class TFileWriteTask
        : public IWriteTask
    {
    public:
        TFileWriteTask(TString fileName, i64 startPosition, i64 length)
            : FileName_(std::move(fileName))
            , StartPosition_(startPosition)
            , Length_(length)
        { }

        void Write(const IFileWriterPtr& writer, std::atomic<bool>& hasException) override
        {
            TFile file(FileName_, RdOnly);
            file.Seek(StartPosition_, SeekDir::sSet);

            TFileInput input(file);
            while (Length_ > 0 && !hasException) {
                void *buffer;
                i64 bufSize = input.Next(&buffer);
                if (Length_ < bufSize) {
                    bufSize = Length_;
                }
                writer->Write(buffer, bufSize);
                Length_ -= bufSize;
            }
        }

        size_t GetDataSize() const override
        {
            return Length_;
        }

    private:
        TString FileName_;
        i64 StartPosition_;
        i64 Length_;
    };

    struct TTaskDescription
    {
        TRichYPath Path;
        // First value is original blobId, second - split inside this blob.
        std::pair<size_t, size_t> Order;
    };

private:
    std::shared_ptr<IThreadPool> ThreadPool_;
    ITransactionPtr Transaction_;
    TRichYPath Path_;
    TParallelFileWriterOptions Options_;
    IResourceLimiterPtr RamLimiter_;
    bool AcquireRamForBuffers_;
    TMutex MutexForException_;
    std::exception_ptr Exception_ = nullptr;
    std::atomic<bool> HasException_ = false;
    std::atomic<bool> Finished_ = false;
    std::vector<TTaskDescription> Tasks_;
    std::vector<NThreading::TFuture<void>> Futures_;
    TString TmpPathPrefix_;
    size_t NextBlobId_ = 0;
};

////////////////////////////////////////////////////////////////////////////////

TParallelFileWriter::TParallelFileWriter(
    const IClientBasePtr &client,
    const TRichYPath& path,
    const std::shared_ptr<IThreadPool>& threadPool,
    const TParallelFileWriterOptions& options)
    : ThreadPool_(threadPool)
    , Transaction_(client->StartTransaction())
    , Path_(path)
    , Options_(options)
    , RamLimiter_(options.RamLimiter_)
    , AcquireRamForBuffers_(options.AcquireRamForBuffers_)
    , TmpPathPrefix_(Options_.TmpDirectory_
        ? *Options_.TmpDirectory_ + "/" + CreateGuidAsString()
        : Path_.Path_)
{
    static constexpr size_t DefaultRamLimit = 2ull * 1ull << 30ull;  // 2 GiB

    if (RamLimiter_ == nullptr) {
        RamLimiter_ = MakeIntrusive<TResourceLimiter>(DefaultRamLimit, ::TStringBuilder() << "TParallelFileWriter[" << NodeToYsonString(PathToNode(Path_)) << "]");
    }

    // Otherwise we will deadlock on trying to assign job.
    Y_ENSURE(Options_.MaxBlobSize_ <= RamLimiter_->GetLimit());
}

TParallelFileWriter::~TParallelFileWriter()
{
    NDetail::FinishOrDie(this, "TParallelFileWriter");
}

NThreading::TFuture<void> TParallelFileWriter::StartWriteTask(
    std::unique_ptr<IWriteTask> task,
    TRichYPath filePath
) {
    TResourceGuard memoryGuard(RamLimiter_, task->GetDataSize());

    return NThreading::Async([
        this,
        task=std::move(task),
        filePath=std::move(filePath),
        memoryGuard=std::move(memoryGuard)
    ] () mutable {
        if (HasException_) {
            return;
        }

        try {
            IFileWriterPtr writer = Transaction_->CreateFileWriter(filePath, TFileWriterOptions().WriterOptions(Options_.WriterOptions_.GetOrElse({})));
            size_t lockSize = AcquireRamForBuffers_ ? writer->GetBufferMemoryUsage() : 0;
            // Hard lock since it's acquired inside thread => may deadlock if not enough memory.
            TResourceGuard writerGuard(RamLimiter_, lockSize, EResourceLimiterLockType::HARD);
            task->Write(writer, HasException_);
            writer->Finish();
        } catch (const std::exception& e) {
            auto guard = Guard(MutexForException_);
            HasException_ = true;
            Exception_ = std::current_exception();
            try {
                Transaction_->Abort();
            } catch (...) {
                // Never mind if tx is already dead - we won't commit it anyway => no data will be written.
            }
            Finished_ = true;
        }

        return;
    }, *ThreadPool_);
}

TRichYPath TParallelFileWriter::CreateFilePath(const std::pair<size_t, size_t>& taskId)
{
    TString filePathStr = TmpPathPrefix_ + "__ParallelFileWriter__" + ToString(taskId.first) + "_" + ToString(taskId.second);
    auto filePath = Path_;
    filePath.Path(filePathStr).Append(false);
    return filePath;
}

void TParallelFileWriter::Write(TSharedRef blob)
{
    ThrowIfDead();
    Y_ENSURE(!Finished_, "Tried to push blob to already finished writer");

    auto blobId = NextBlobId_++;

    for (const auto& [subBlobId, subBlob] : Enumerate(blob.Split(Options_.MaxBlobSize_))) {
        auto taskId = std::pair(blobId, subBlobId);
        auto filePath = CreateFilePath(taskId);

        auto future = StartWriteTask(std::make_unique<TBlobWriteTask>(std::move(subBlob)), filePath);

        Tasks_.emplace_back(TTaskDescription{
            .Path = std::move(filePath),
            .Order = std::move(taskId)
        });
        Futures_.emplace_back(std::move(future));
    }
}

void TParallelFileWriter::WriteFile(const TString& fileName)
{
    ThrowIfDead();
    Y_ENSURE(!Finished_, "Tried to push file to already finished writer");

    auto blobId = NextBlobId_++;

    auto length = GetFileLength(fileName);
    size_t subBlobId = 0;
    for (i64 pos = 0; pos < length; pos += Options_.MaxBlobSize_, ++subBlobId) {
        auto taskId = std::pair(blobId, subBlobId);
        auto filePath = CreateFilePath(taskId);

        i64 begin = pos;
        i64 end = std::min(begin + static_cast<i64>(Options_.MaxBlobSize_), length);

        auto future = StartWriteTask(std::make_unique<TFileWriteTask>(fileName, begin, end - begin), filePath);

        Tasks_.emplace_back(TTaskDescription{
            .Path = std::move(filePath),
            .Order = std::move(taskId)
        });
        Futures_.emplace_back(std::move(future));
    }
}

void TParallelFileWriter::ThrowIfDead() {
    if (HasException_) {
        auto guard = Guard(MutexForException_);
        std::rethrow_exception(Exception_);
    }
}

void TParallelFileWriter::Finish()
{
    if (Finished_) {
        return;
    }
    Finished_ = true;

    NThreading::WaitAll(Futures_).GetValueSync();

    if (Exception_) {
        Transaction_->Abort();
        std::rethrow_exception(Exception_);
    }
    auto createOptions = TCreateOptions();
    auto concatenateOptions = TConcatenateOptions();
    if (Path_.Append_.GetOrElse(false)) {
        createOptions.IgnoreExisting(true);
        concatenateOptions.Append(true);
    } else {
        createOptions.Force(true);
        concatenateOptions.Append(false);
    }
    Transaction_->Create(Path_.Path_, NT_FILE, createOptions);

    Sort(Tasks_, [](const TTaskDescription& lhs, const TTaskDescription& rhs) {
        return lhs.Order < rhs.Order;
    });

    TVector<TYPath> tempPaths;
    tempPaths.reserve(Tasks_.size());
    for (const auto& task : Tasks_) {
        tempPaths.emplace_back(task.Path.Path_);
    }

    Transaction_->Concatenate(tempPaths, Path_.Path_, concatenateOptions);
    for (const auto& path : tempPaths) {
        Transaction_->Remove(path);
    }
    Transaction_->Commit();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NDetail

////////////////////////////////////////////////////////////////////////////////

void WriteFileParallel(
    const IClientBasePtr &client,
    const TString& fileName,
    const TRichYPath& path,
    const TParallelFileWriterOptions& options)
{
    auto writer = CreateParallelFileWriter(client, path, options);
    writer->WriteFile(fileName);
    writer->Finish();
}

////////////////////////////////////////////////////////////////////////////////
::TIntrusivePtr<IParallelFileWriter> CreateParallelFileWriter(
    const IClientBasePtr& client,
    const TRichYPath& path,
    const TParallelFileWriterOptions& options)
{
    auto threadPool = std::make_shared<TSimpleThreadPool>();
    threadPool->Start(options.ThreadCount_);
    return CreateParallelFileWriter(client, path, threadPool, options);
}

////////////////////////////////////////////////////////////////////////////////

::TIntrusivePtr<IParallelFileWriter> CreateParallelFileWriter(
    const IClientBasePtr& client,
    const TRichYPath& path,
    const std::shared_ptr<IThreadPool>& threadPool,
    const TParallelFileWriterOptions& options)
{
    return ::MakeIntrusive<NDetail::TParallelFileWriter>(client, path, threadPool, options);
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NYT
