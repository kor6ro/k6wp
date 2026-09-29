#include "compress_controller.hpp"

#include <QDir>
#include <QStandardPaths>

#include <algorithm>
#include <cmath>

#include "config_schema.hpp"
#include "ffprobe_helper.hpp"
#include "monitor_util.hpp"

namespace k6wp {

bool CompressController::Enqueue(const CompressRequest& req, JobMeta& meta,
                                 QString* error_out) {
  return EnqueueWithStatus(req, meta, error_out) !=
         CompressService::EnqueueResult::kInvalid;
}

CompressService::EnqueueResult CompressController::EnqueueWithStatus(
    const CompressRequest& req, JobMeta& meta, QString* error_out) {
  const CompressService::EnqueueResult result =
      service_.EnqueueDetailed(req, meta, error_out);
  // Ledger on every accepted dispatch (including the rare start-failure
  // path: the queued Finished handler still resolves the request there).
  if (result != CompressService::EnqueueResult::kInvalid) {
    requests_[meta.job_id] = req;
  }
  return result;
}

bool CompressController::TakeRequest(int job_id, CompressRequest* out) {
  const auto it = requests_.find(job_id);
  if (it == requests_.end()) {
    if (out != nullptr) {
      *out = CompressRequest{};
    }
    return false;
  }
  if (out != nullptr) {
    *out = it->second;
  }
  requests_.erase(it);
  return true;
}

QString CompressController::DefaultWallpapersDir() {
  const QString local =
      QString::fromLocal8Bit(qgetenv("LOCALAPPDATA"));
  if (!local.isEmpty()) {
    return QDir(local).filePath(QStringLiteral("K6WP/wallpapers"));
  }
  const QString profile =
      QString::fromLocal8Bit(qgetenv("USERPROFILE"));
  if (!profile.isEmpty()) {
    return QDir(profile).filePath(QStringLiteral("AppData/Local/K6WP/wallpapers"));
  }
  return QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
         QStringLiteral("/K6WP/wallpapers");
}

QString CompressController::ResolveTabOutDir(const QString& override_dir,
                                             const QString& settings_dir) {
  QString out_dir =
      !override_dir.isEmpty() ? override_dir : settings_dir;
  if (out_dir.trimmed().isEmpty()) {
    out_dir = DefaultWallpapersDir();
  }
  QDir().mkpath(out_dir);
  return out_dir;
}

void CompressController::ResolveSimpleRes(const std::string& mode,
                                          const VideoMetadata* probed, int& w,
                                          int& h) {
  w = 1280;
  h = 720;
  if (mode == "720p") {
    return;
  }
  if (mode == "1080p") {
    w = 1920;
    h = 1080;
    return;
  }
  if (mode == "2160p") {
    w = 3840;
    h = 2160;
    return;
  }
  const bool meta_ok = probed != nullptr && probed->width > 0 &&
                       probed->height > 0;
  if (mode == "source") {
    if (meta_ok) {
      w = probed->width;
      h = probed->height;
    }
    return;
  }
  for (const MonitorInfo& m : ListMonitors()) {
    if (m.is_primary && m.width > 0 && m.height > 0) {
      w = m.width;
      h = m.height;
      return;
    }
  }
  if (meta_ok) {
    w = probed->width;
    h = probed->height;
  }
}

int CompressController::ResolveEngineFpsCap() {
  int cap = WallpaperConfig{}.fps_cap;
  try {
    cap = LoadConfig(DefaultConfigPath()).fps_cap;
  } catch (const ConfigError&) {
    try {
      cap = LoadConfig(DefaultConfigPath().wstring() + L".bak").fps_cap;
    } catch (const ConfigError&) {
    }
  }
  return cap;
}

int CompressController::ComputeEnqueueFps(double fps_source,
                                          int engine_fps_cap) {
  return fps_source > 0.0
             ? std::max(1, static_cast<int>(std::round(
                               std::min(fps_source,
                                        static_cast<double>(engine_fps_cap)))))
             : std::max(1, engine_fps_cap);
}

CompressRequest CompressController::BuildTabRequest(
    const TabRequestInputs& in) {
  // Output to the configured compress dir (default
  // %LOCALAPPDATA%/K6WP/wallpapers, single result location).
  // The compressor CLI requires an explicit positive WxH (it rejects 0x0),
  // so simple mode carries the StudioSettings resolution mode (match_monitor
  // by default) and Advanced keeps its explicit WxH override.
  QString out;
  const QString override_name = in.out_name_override.trimmed();
  if (!override_name.isEmpty()) {
    out = override_name;
    if (!out.endsWith(QStringLiteral(".mp4"), Qt::CaseInsensitive)) {
      out += QStringLiteral(".mp4");
    }
    out = QDir(in.out_dir).filePath(out);
  } else {
    out = UniqueOutPath(in.out_dir, in.src);
  }
  CompressRequest req;
  req.in_path = in.src;
  req.out_path = out;
  req.res_w = in.res_w;
  req.res_h = in.res_h;
  if (in.advanced) {
    req.fps = in.adv_fps;
    req.crf = in.adv_crf;
    req.encoder = in.adv_encoder;
    req.force = in.adv_force;
  } else {
    // Simple mode mirrors the Settings > Kompresor defaults (CRF 22,
    // FPS 30, encoder auto, H.264 MP4 + audio strip handled by the
    // compressor itself).
    int fps = in.simple_fps;
    if (fps < 1 || fps > 30) {
      fps = 30;
    }
    int crf = in.simple_crf;
    if (crf < 16 || crf > 28) {
      crf = 22;
    }
    req.fps = fps;
    req.crf = crf;
    req.encoder = QStringLiteral("auto");
    req.force = false;
  }
  return req;
}

}  // namespace k6wp
