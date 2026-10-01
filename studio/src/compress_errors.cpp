#include "compress_errors.hpp"

#include <QCoreApplication>

namespace k6wp {

QString FriendlyCompressError(const QString& technical) {
  const QString t = technical.toLower();
  if (t.contains(QStringLiteral("cancelled"))) {
    return QStringLiteral("Kompresi dibatalkan.");
  }
  if (t.contains(QStringLiteral("longer than 10 minutes")) ||
      t.contains(QStringLiteral("force-long"))) {
    return QStringLiteral(
        "Video lebih dari 10 menit. Izinkan video panjang lalu coba lagi, "
        "atau pilih video yang lebih pendek.");
  }
  if (t.contains(QStringLiteral("failed to read duration")) ||
      t.contains(QStringLiteral("corrupt"))) {
    return QStringLiteral(
        "Tidak bisa membaca video ini (file rusak atau format tak dikenal).");
  }
  if (t.contains(QStringLiteral("ffmpeg"))) {
    if (t.contains(QStringLiteral("tidak ditemukan")) ||
        t.contains(QStringLiteral("not found"))) {
      return QCoreApplication::translate(
          "k6wp::CompressError",
          "ffmpeg tidak ditemukan di folder aplikasi. Ekstrak ulang atau "
          "pasang ulang K6WP.");
    }
    if (t.contains(QStringLiteral("failed")) ||
        t.contains(QStringLiteral("gagal"))) {
      return QStringLiteral("Gagal menjalankan ffmpeg. Coba lagi.");
    }
  }
  if (t.contains(QStringLiteral("compressor"))) {
    if (t.contains(QStringLiteral("not found")) ||
        t.contains(QStringLiteral("tidak ditemukan"))) {
      return QCoreApplication::translate(
          "k6wp::CompressError",
          "Kompresor tidak ditemukan di folder aplikasi. Ekstrak ulang atau "
          "pasang ulang K6WP.");
    }
    if (t.contains(QStringLiteral("failed to start")) ||
        t.contains(QStringLiteral("gagal menjalankan")) ||
        t.contains(QStringLiteral("launch failed"))) {
      return QCoreApplication::translate(
          "k6wp::CompressError",
          "Gagal menjalankan kompresor. Coba lagi atau pasang ulang K6WP.");
    }
    if (t.contains(QStringLiteral("exited with code"))) {
      return QCoreApplication::translate(
          "k6wp::CompressError",
          "Kompresor berhenti tidak normal. Coba lagi atau cek log untuk "
          "detail teknis.");
    }
  }
  if (t.contains(QStringLiteral("input")) &&
      (t.contains(QStringLiteral("not found")) ||
       t.contains(QStringLiteral("not exist")) ||
       t.contains(QStringLiteral("tidak ditemukan")))) {
    return QStringLiteral("File input tidak ditemukan.");
  }
  if (t.contains(QStringLiteral("crf must be in range"))) {
    return QStringLiteral("Kualitas CRF harus antara 16 sampai 28.");
  }
  if (t.isEmpty()) {
    return QStringLiteral("Kompresi gagal tanpa keterangan.");
  }
  return QStringLiteral("Kompresi gagal. Lihat log untuk detail teknis.");
}

}  // namespace k6wp
