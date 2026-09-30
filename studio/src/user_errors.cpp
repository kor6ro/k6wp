#include "user_errors.hpp"

#include <QCoreApplication>

namespace k6wp {

QString FriendlyApplyError(const QString& technical) {
  const QString t = technical.toLower();
  if (t.contains(QStringLiteral("not running")) ||
      t.contains(QStringLiteral("tidak jalan")) ||
      t.contains(QStringLiteral("engine mati"))) {
    return QCoreApplication::translate(
        "k6wp::UserErrors",
        "Engine belum jalan. Klik Nyalakan Engine, lalu coba lagi.");
  }
  if (t.contains(QStringLiteral("busy")) ||
      t.contains(QStringLiteral("sibuk"))) {
    return QCoreApplication::translate(
        "k6wp::UserErrors",
        "Engine sedang sibuk. Tunggu proses selesai, lalu coba lagi.");
  }
  if (t.contains(QStringLiteral("cannot find")) ||
      t.contains(QStringLiteral("not found")) ||
      t.contains(QStringLiteral("tidak ditemukan"))) {
    return QCoreApplication::translate(
        "k6wp::UserErrors",
        "Engine tidak ditemukan di folder aplikasi. Ekstrak ulang atau "
        "pasang ulang K6WP.");
  }
  return QCoreApplication::translate(
      "k6wp::UserErrors",
      "Terapkan pengaturan gagal. Cek log untuk detail teknis, lalu coba lagi.");
}

QString FriendlyIpcError(const QString& technical) {
  const QString t = technical.toLower();
  if (t.contains(QStringLiteral("busy")) ||
      t.contains(QStringLiteral("sibuk"))) {
    return QCoreApplication::translate(
        "k6wp::UserErrors", "Engine sedang sibuk. Tunggu sebentar, lalu coba lagi.");
  }
  return QCoreApplication::translate(
      "k6wp::UserErrors",
      "Gagal menghubungi engine. Cek log untuk detail teknis, lalu coba lagi.");
}

QString FriendlyPreviewError(const QString& technical) {
  const QString t = technical.toLower();
  if (t.isEmpty()) {
    return QCoreApplication::translate(
        "k6wp::UserErrors",
        "Pratinjau tak tersedia. Cek log untuk detail teknis, lalu coba lagi.");
  }
  if (t.contains(QStringLiteral("no such file")) ||
      t.contains(QStringLiteral("not found")) ||
      t.contains(QStringLiteral("tidak ditemukan"))) {
    return QCoreApplication::translate(
        "k6wp::UserErrors",
        "Pratinjau tak tersedia. Pilih video lain, lalu coba lagi.");
  }
  return QCoreApplication::translate(
      "k6wp::UserErrors",
      "Pratinjau tak tersedia. Video mungkin rusak atau formatnya tidak "
      "didukung — coba lagi.");
}

}  // namespace k6wp
