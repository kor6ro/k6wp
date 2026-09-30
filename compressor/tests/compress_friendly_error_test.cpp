// compress_friendly_error_test.cpp — the compressor's USER-FACING error
// payload must be actionable, and must be recognised by Studio's mapping.
//
// compressor.exe is a Qt-free CLI with no i18n machinery, so its
// `{"friendly":...,"technical":...}` string IS the text a user eventually
// reads. Studio then maps that whole payload through
// k6wp::FriendlyCompressError (studio/src/compress_errors.cpp) and shows the
// mapped sentence. Two sides, one message: this suite locks the contract that
// keeps them agreeing.
//
// What it locks:
//
//   1. `friendly` for ffmpeg_not_found names something the user can ACTUALLY
//      do. The old text ("Pastikan ffmpeg terinstall di vendor/ffmpeg/ atau
//      atur K6WP_FFMPEG") told an ordinary Windows user about a source-tree
//      folder they do not have and an environment variable they will never
//      set. The shipped app installs flat, so a missing ffmpeg means the
//      installed folder is incomplete -> re-extract / reinstall.
//   2. `friendly` carries no path, no directory, no .exe name and no
//      developer knob — for EVERY friendly id, not just this one.
//   3. `technical` is untouched. That is the string that reaches Studio's log
//      pane, so losing it would make the log useless.
//   4. The JSON shape is unchanged. compress_argv_contract already asserts the
//      {"error":...} envelope reaches the real binary; this asserts the object
//      inside it still has both keys, so the mapping stays a pure text
//      operation on Studio's side.
//   5. CROSS-SIDE AGREEMENT: Studio's rule is
//      `contains("ffmpeg") && (contains("tidak ditemukan") || contains("not found"))`
//      (compress_errors.cpp, case-folded). Step 6 replays exactly that rule
//      over the real string, so a reworded message that Studio would no longer
//      recognise turns this suite RED instead of silently degrading to the
//      generic fallback at runtime.
//
// Failure drill: restoring the old developer text for ffmpeg_not_found turns
// steps 1, 2 and 6 red.
//
// src/ffmpeg_job.cpp is #included (thumbnailer_test / probe_json_test pattern)
// so the anonymous-namespace FriendlyId/FormatError are reachable; only
// encoder_detect.cpp is compiled separately.
//
// Exit 0 = the payload is safe to show a user; nonzero = it is not.

#include <cctype>
#include <cstdio>
#include <string>

#include "../src/ffmpeg_job.cpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const char* what) {
  ++g_checks;
  std::printf("%s %s\n", cond ? "PASS" : "FAIL", what);
  if (!cond) ++g_failures;
}

std::string Lower(std::string s) {
  for (char& c : s) {
    c = static_cast<char>(
        std::tolower(static_cast<unsigned char>(c)));
  }
  return s;
}

bool Contains(const std::string& hay, const std::string& needle) {
  return hay.find(needle) != std::string::npos;
}

// Value of a top-level string field in the {"friendly":"..","technical":".."}
// object FormatError emits. Returns false when the field is absent, so a shape
// change reads as a failed check instead of an empty string that passes a
// "non-empty" assertion.
bool JsonField(const std::string& json, const std::string& field,
               std::string* out) {
  const std::string key = "\"" + field + "\":\"";
  const std::size_t beg = json.find(key);
  if (beg == std::string::npos) return false;
  const std::size_t start = beg + key.size();
  const std::size_t end = json.find('"', start);
  if (end == std::string::npos) return false;
  *out = json.substr(start, end - start);
  return true;
}

// Everything a user-visible sentence must never contain: a directory layout, an
// absolute path, an environment variable, or an executable filename. The
// compressed message has to survive a user who has never opened a terminal.
bool HasDeveloperDetail(const std::string& s) {
  const std::string t = Lower(s);
  return Contains(t, "vendor") || Contains(t, "k6wp_ffmpeg") ||
         Contains(t, ".exe") || Contains(t, ":/") || Contains(t, "\\") ||
         Contains(t, "environment") || Contains(t, "env var");
}

}  // namespace

int main() {
  // ---- 1/2/5/6: the ffmpeg-missing message ----
  // The exact technical string BuildFfmpegCmdline pairs with this id
  // (ffmpeg_job.cpp). Copied, not re-derived: the point is that the friendly
  // half is judged against the payload that actually ships.
  const std::string ffmpeg_technical =
      "ffmpeg.exe not found (K6WP_FFMPEG or vendor/ffmpeg/ffmpeg.exe)";
  const std::string payload =
      k6wp::compressor::FormatError("ffmpeg_not_found", ffmpeg_technical);

  std::string friendly;
  Check(JsonField(payload, "friendly", &friendly),
        "1. payload carries a friendly field");
  Check(!friendly.empty(), "1. friendly message is not empty");
  Check(!HasDeveloperDetail(friendly),
        "1. friendly message has no path / vendor dir / env var / .exe");
  // Actionable = it names the app and the remedy. An Indonesian user who lost
  // a shipped file must be told to re-extract or reinstall, nothing else.
  Check(Contains(Lower(friendly), "k6wp"),
        "1. friendly message names the app to re-extract/reinstall");
  Check(Contains(Lower(friendly), "ekstrak ulang") ||
            Contains(Lower(friendly), "pasang ulang"),
        "1. friendly message names an action the user can take");

  // ---- 3: the technical half survives (it is the log) ----
  std::string technical;
  Check(JsonField(payload, "technical", &technical),
        "3. payload carries a technical field");
  Check(technical == ffmpeg_technical,
        "3. technical detail is passed through verbatim (log keeps it)");

  // ---- 4: the wire shape is unchanged ----
  Check(payload.rfind("{\"friendly\":\"", 0) == 0,
        "4. payload opens with the friendly field");
  Check(Contains(payload, "\",\"technical\":\""),
        "4. payload still has exactly the friendly+technical object shape");

  // ---- 5/6: every friendly id is user-safe ----
  const char* ids[] = {"ffmpeg_not_found", "input_not_found",
                       "launch_failed",    "cancelled",
                       "timeout",          "ffmpeg_failed",
                       "an_id_that_does_not_exist"};
  for (const char* id : ids) {
    const std::string p =
        k6wp::compressor::FormatError(id, "technical detail for the log");
    std::string f;
    const bool has_field = JsonField(p, "friendly", &f);
    std::printf("     [%s] -> %s\n", id,
                has_field ? f.c_str() : "(no friendly field)");
    Check(has_field, "5. friendly field present");
    Check(!f.empty(), "5. friendly message is not empty");
    Check(!HasDeveloperDetail(f), "5. friendly message is user-safe");
    Check(f.find('\n') == std::string::npos && f.find('\r') == std::string::npos,
          "5. friendly message is a single line");
  }

  // ---- 6: Studio's ffmpeg-not-found rule still matches this message ----
  // Mirrors k6wp::FriendlyCompressError (studio/src/compress_errors.cpp): the
  // case-folded payload must satisfy `contains("ffmpeg")` AND
  // `contains("tidak ditemukan") || contains("not found")`, and must NOT be
  // swallowed earlier by the cancelled / too-long / corrupt / input-missing
  // rules that precede it.
  const std::string t = Lower(payload);
  const bool matched_by_studio =
      !Contains(t, "cancelled") && !Contains(t, "longer than 10 minutes") &&
      !Contains(t, "force-long") &&
      !(Contains(t, "failed to read duration") || Contains(t, "corrupt")) &&
      Contains(t, "ffmpeg") &&
      (Contains(t, "tidak ditemukan") || Contains(t, "not found"));
  Check(matched_by_studio,
        "6. Studio's ffmpeg-not-found rule recognises this payload");

  std::printf("RESULT: %d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
