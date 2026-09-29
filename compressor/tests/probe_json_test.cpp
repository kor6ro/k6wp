// probe_json_test.cpp — QA harness for MED-9 (single JSON ffprobe probe).
//
// Tests the PURE parser k6wp::compressor::ParseProbeJson (declared in
// src/video_probe.hpp, implemented in src/video_probe.cpp) against canned
// ffprobe -of json samples. No ffprobe binary needed for the unit checks.
// A `--probe <video>` QA mode runs the real ProbeVideoProps (single
// ffprobe spawn) and prints the parsed values for before/after comparison.
//
// Exit 0 = all assertions held; nonzero = an assertion failed.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "../src/video_probe.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

void Check(bool cond, const char* what) {
  ++g_checks;
  std::printf("%s %s\n", cond ? "PASS" : "FAIL", what);
  if (!cond) ++g_failures;
}

bool Near(double a, double b, double eps = 1e-9) {
  return std::fabs(a - b) <= eps;
}

// Canonical ffprobe sample: format.duration (string) + video stream +
// audio stream. Key order matches ffprobe's own output.
const char kCanonical[] = R"({
  "streams": [
    {
      "codec_name": "h264",
      "codec_type": "video",
      "width": 1280,
      "height": 720,
      "r_frame_rate": "30/1"
    },
    {
      "codec_name": "aac",
      "codec_type": "audio"
    }
  ],
  "format": {
    "duration": "12.500000"
  }
})";

// Same facts, shuffled: streams AFTER format, keys within each object in
// reverse order, audio stream first. Must parse identically (JSON is
// order-independent, unlike the old line-order -of default parsing).
const char kShuffled[] = R"({
  "format": {
    "duration": "12.500000"
  },
  "streams": [
    {
      "codec_type": "audio",
      "codec_name": "aac"
    },
    {
      "r_frame_rate": "30/1",
      "height": 720,
      "width": 1280,
      "codec_type": "video",
      "codec_name": "h264"
    }
  ]
})";

void ExpectCanonical(const k6wp::compressor::VideoProps& p, const char* tag) {
  char buf[256];
  std::snprintf(buf, sizeof(buf), "[%s] valid", tag);
  Check(p.valid, buf);
  std::snprintf(buf, sizeof(buf), "[%s] codec h264", tag);
  Check(p.video_codec == "h264", buf);
  std::snprintf(buf, sizeof(buf), "[%s] width 1280", tag);
  Check(p.width == 1280, buf);
  std::snprintf(buf, sizeof(buf), "[%s] height 720", tag);
  Check(p.height == 720, buf);
  std::snprintf(buf, sizeof(buf), "[%s] fps 30", tag);
  Check(Near(p.fps, 30.0), buf);
  std::snprintf(buf, sizeof(buf), "[%s] duration 12.5", tag);
  Check(Near(p.duration, 12.5), buf);
  std::snprintf(buf, sizeof(buf), "[%s] has_audio", tag);
  Check(p.has_audio, buf);
}

void TestCanonicalAndShuffled() {
  k6wp::compressor::VideoProps a;
  Check(k6wp::compressor::ParseProbeJson(kCanonical, a),
        "canonical JSON parses ok");
  ExpectCanonical(a, "canonical");

  k6wp::compressor::VideoProps b;
  Check(k6wp::compressor::ParseProbeJson(kShuffled, b),
        "shuffled JSON parses ok");
  ExpectCanonical(b, "shuffled");

  // Order-independence: every field identical across key orders.
  Check(a.video_codec == b.video_codec && a.width == b.width &&
            a.height == b.height && Near(a.fps, b.fps) &&
            Near(a.duration, b.duration) && a.has_audio == b.has_audio &&
            a.valid == b.valid,
        "shuffled == canonical on all fields (order-independent)");
}

void TestNoAudio() {
  const char* json = R"({
    "streams": [
      {"codec_name": "h264", "codec_type": "video",
       "width": 640, "height": 480, "r_frame_rate": "30000/1001"}
    ],
    "format": {"duration": "5.0"}
  })";
  k6wp::compressor::VideoProps p;
  Check(k6wp::compressor::ParseProbeJson(json, p), "no-audio JSON parses ok");
  Check(p.valid, "no-audio valid");
  Check(!p.has_audio, "no-audio -> has_audio false");
  Check(Near(p.fps, 30000.0 / 1001.0), "no-audio fps 30000/1001");
  Check(Near(p.duration, 5.0), "no-audio duration 5.0");
}

void TestDurationAsNumber() {
  // ffprobe usually emits duration as a string, but accept a bare number.
  const char* json = R"({
    "streams": [
      {"codec_name": "h264", "codec_type": "video",
       "width": 320, "height": 240, "r_frame_rate": "25/1"}
    ],
    "format": {"duration": 7.25}
  })";
  k6wp::compressor::VideoProps p;
  Check(k6wp::compressor::ParseProbeJson(json, p),
        "numeric-duration JSON parses ok");
  Check(Near(p.duration, 7.25), "numeric duration 7.25 accepted");
}

void TestMissingDuration() {
  // No format section at all: video facts still valid, duration 0.
  const char* json = R"({
    "streams": [
      {"codec_name": "hevc", "codec_type": "video",
       "width": 1920, "height": 1080, "r_frame_rate": "60/1"}
    ]
  })";
  k6wp::compressor::VideoProps p;
  Check(k6wp::compressor::ParseProbeJson(json, p),
        "missing-format JSON parses ok");
  Check(p.valid, "missing-format still valid (video stream present)");
  Check(Near(p.duration, 0.0), "missing duration -> 0.0");
  Check(p.video_codec == "hevc", "missing-format codec hevc");
}

void TestCorrupt() {
  // Every corrupt/truncated input must yield invalid props, never a crash.
  const char* cases[] = {
      "",
      "{",
      "{\"streams\": [",
      "{\"streams\": [{\"codec_name\": \"h264\",",
      "not json at all",
      "{\"format\": {\"duration\": \"1.0\"}}",  // valid JSON, no streams
      "{\"streams\": []}",
      "{\"streams\": [{\"codec_type\": \"audio\", \"codec_name\": "
      "\"aac\"}]}",  // audio only
      "{\"streams\": [{\"codec_type\": \"video\"}]}",  // video, no dims
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    k6wp::compressor::VideoProps p;
    char buf[128];
    std::snprintf(buf, sizeof(buf), "corrupt case %zu returns false", i);
    Check(!k6wp::compressor::ParseProbeJson(cases[i], p), buf);
    std::snprintf(buf, sizeof(buf), "corrupt case %zu props invalid", i);
    Check(!p.valid, buf);
  }
}

void TestBadFps() {
  // r_frame_rate "0/0" or garbage: no div-by-zero, fps 0, still valid.
  const char* json = R"({
    "streams": [
      {"codec_name": "h264", "codec_type": "video",
       "width": 640, "height": 480, "r_frame_rate": "0/0"}
    ],
    "format": {"duration": "1.0"}
  })";
  k6wp::compressor::VideoProps p;
  Check(k6wp::compressor::ParseProbeJson(json, p), "zero-denominator parses");
  Check(p.valid, "zero-denominator still valid");
  Check(Near(p.fps, 0.0), "zero-denominator fps 0 (no div-by-zero)");
}

int RunUnit() {
  std::printf("=== MED-9 unit: single-JSON probe parsing (order-independent) ===\n");
  TestCanonicalAndShuffled();
  TestNoAudio();
  TestDurationAsNumber();
  TestMissingDuration();
  TestCorrupt();
  TestBadFps();
  std::printf(g_failures == 0 ? "RESULT: ALL %d CHECKS PASSED\n"
                              : "RESULT: %d/%d CHECK(S) FAILED\n",
              g_failures == 0 ? g_checks : g_failures, g_checks);
  return g_failures == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  // QA mode: --parse-file <json> runs the pure parser on file bytes.
  // Prints VALID/INVALID; exit 0 in both cases (INVALID is the structured
  // reject — the point is no crash). Exit 2 only if the file is unreadable.
  if (argc == 3 && std::strcmp(argv[1], "--parse-file") == 0) {
    std::ifstream in(argv[2], std::ios::binary);
    if (!in) {
      std::printf("parse-file: cannot open %s\n", argv[2]);
      return 2;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    k6wp::compressor::VideoProps p;
    const bool ok = k6wp::compressor::ParseProbeJson(ss.str(), p);
    std::printf("parse-file %s: %s valid=%d (no crash)\n", argv[2],
                ok ? "VALID" : "INVALID", p.valid ? 1 : 0);
    return 0;
  }
  // QA mode: --probe <video> runs the REAL single-spawn ProbeVideoProps.
  if (argc == 3 && std::strcmp(argv[1], "--probe") == 0) {
    const k6wp::compressor::VideoProps p =
        k6wp::compressor::ProbeVideoProps(argv[2]);
    std::printf(
        "probe codec=%s width=%d height=%d fps=%.6f duration=%.6f "
        "has_audio=%d valid=%d\n",
        p.video_codec.c_str(), p.width, p.height, p.fps, p.duration,
        p.has_audio ? 1 : 0, p.valid ? 1 : 0);
    return p.valid ? 0 : 1;
  }
  if (argc > 1) {
    std::printf("usage: %s [--probe <video>] [--parse-file <json>]\n", argv[0]);
    return 2;
  }
  return RunUnit();
}
