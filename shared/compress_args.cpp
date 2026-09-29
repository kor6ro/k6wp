#include "compress_args.hpp"

namespace k6wp {

std::vector<std::string> BuildCompressArgv(const CompressArgs& args) {
  std::vector<std::string> argv;
  argv.reserve(14);
  argv.push_back(kCompressFlagIn);
  argv.push_back(args.in);
  argv.push_back(kCompressFlagOut);
  argv.push_back(args.out);
  argv.push_back(kCompressFlagRes);
  argv.push_back(std::to_string(args.res_w) + "x" +
                 std::to_string(args.res_h));
  argv.push_back(kCompressFlagFps);
  argv.push_back(std::to_string(args.fps));
  argv.push_back(kCompressFlagCrf);
  argv.push_back(std::to_string(args.crf));
  argv.push_back(kCompressFlagEncoder);
  argv.push_back(args.encoder);
  if (args.force_long) {
    argv.push_back(kCompressFlagForceLong);
  }
  if (args.dry_run) {
    argv.push_back(kCompressFlagDryRun);
  }
  return argv;
}

bool IsKnownCompressorFlag(const std::string& flag) {
  return flag == kCompressFlagIn || flag == kCompressFlagOut ||
         flag == kCompressFlagRes || flag == kCompressFlagFps ||
         flag == kCompressFlagCrf || flag == kCompressFlagEncoder ||
         flag == kCompressFlagForceLong || flag == kCompressFlagDryRun ||
         flag == kCompressFlagProbeEncoders || flag == kCompressFlagHelp ||
         flag == kCompressFlagLockframe || flag == kCompressFlagOffsetS ||
         flag == kCompressFlagQuality;
}

}  // namespace k6wp
