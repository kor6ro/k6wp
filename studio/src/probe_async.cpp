#include "probe_async.hpp"

#include <QFutureWatcher>
#include <QtConcurrent>

#include <utility>

namespace k6wp {

void ProbeVideoAsync(const std::filesystem::path& path,
                     std::function<void(bool ok, VideoMetadata meta)> callback,
                     QObject* context) {
  using Result = std::pair<bool, VideoMetadata>;
  auto* watcher = new QFutureWatcher<Result>(context);
  QObject::connect(watcher, &QFutureWatcher<Result>::finished, context,
                   [watcher, callback = std::move(callback)]() {
                     const Result result = watcher->result();
                     watcher->deleteLater();
                     callback(result.first, result.second);
                   });
  watcher->setFuture(QtConcurrent::run([path]() -> Result {
    VideoMetadata meta;
    const bool ok = FfprobeHelper().Probe(path, meta);
    return {ok, meta};
  }));
}

}  // namespace k6wp
