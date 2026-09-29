// First-run wizard implementation — moved verbatim from main_window.cpp
// (MED-5 part 1, pure refactor). See the header for the ownership split.

#include "first_run_wizard.hpp"

#include <QCheckBox>
#include <QCoreApplication>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWizard>

namespace k6wp {

// Dialog-builder strings used to be MainWindow::tr calls (context
// "MainWindow"); keep that context so behavior is identical.
namespace {

QString Ftr(const char* s) {
  return QCoreApplication::translate("MainWindow", s);
}

}  // namespace

FirstRunPickPage::FirstRunPickPage(QWidget* parent) : QWizardPage(parent) {
  setTitle(tr("Pilih video pertama Anda"));
  setSubTitle(tr("Pilih satu video dari komputer Anda. "
                 "File asli tidak dipindah atau diubah."));
  auto* layout = new QVBoxLayout(this);
  auto* hint = new QLabel(
      tr("Format yang didukung: mp4, webm, avi, mkv, mov, wmv."), this);
  hint->setWordWrap(true);
  layout->addWidget(hint);
  auto* pick_btn = new QPushButton(tr("Pilih video..."), this);
  pick_btn->setObjectName(QStringLiteral("firstrun_pick_btn"));
  pick_btn->setMinimumHeight(48);
  layout->addWidget(pick_btn);
  chosen_label_ = new QLabel(tr("Belum ada video dipilih."), this);
  chosen_label_->setObjectName(QStringLiteral("firstrun_chosen_label"));
  chosen_label_->setWordWrap(true);
  chosen_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(chosen_label_);
  layout->addStretch();
  connect(pick_btn, &QPushButton::clicked, this, [this] {
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Pilih Video"), QString(),
        tr("Video (*.mp4 *.webm *.avi *.mkv *.mov *.wmv);;Semua File (*)"));
    if (file.isEmpty()) {
      return;
    }
    chosen_ = file;
    chosen_label_->setText(file);
    emit completeChanged();
  });
}

bool FirstRunPickPage::isComplete() const { return !chosen_.isEmpty(); }

QString FirstRunPickPage::chosen() const { return chosen_; }

bool IsFirstRunCondition(bool has_settings_file, bool lib_empty,
                         bool no_current) {
  if (has_settings_file) {
    return false;
  }
  return lib_empty && no_current;
}

LibraryEntry BuildFirstRunEntry(const QString& file,
                                const QString& library_dst, bool probe_ok,
                                const VideoMetadata& meta,
                                const std::filesystem::path& thumb) {
  LibraryEntry entry;
  entry.src = std::filesystem::path(file.toStdWString());
  entry.dst = std::filesystem::path(library_dst.toStdWString());
  if (probe_ok) {
    entry.duration = meta.duration;
    entry.codec = meta.codec;
    entry.width = meta.width;
    entry.height = meta.height;
    entry.fps = static_cast<int>(meta.fps + 0.5);
    if (meta.width > 0 && meta.height > 0) {
      entry.res =
          std::to_string(meta.width) + "x" + std::to_string(meta.height);
    }
  }
  if (!thumb.empty()) {
    entry.thumb = thumb;
  }
  return entry;
}

bool RunFirstRunWizard(QWidget* parent, FirstRunChoices* out) {
  QWizard wizard(parent);
  wizard.setWindowTitle(Ftr("Selamat datang di K6WP"));
  wizard.setWizardStyle(QWizard::ModernStyle);
  wizard.setOption(QWizard::NoBackButtonOnStartPage, true);

  // A6 welcome page: layman intro, no settings (advanced or otherwise).
  // Flow stays welcome -> import (pick) -> auto-compress (options) -> apply
  // (the post-Finish chain in ApplyFirstRunResult).
  auto* welcome_page = new QWizardPage(&wizard);
  welcome_page->setObjectName(QStringLiteral("firstrun_welcome_page"));
  welcome_page->setTitle(Ftr("Selamat datang di K6WP"));
  welcome_page->setSubTitle(Ftr("Wallpaper video untuk Windows Anda."));
  auto* welcome_layout = new QVBoxLayout(welcome_page);
  auto* welcome_label = new QLabel(
      Ftr("K6WP menampilkan video sebagai wallpaper desktop Anda. Panduan "
          "singkat ini akan memilih video pertama, menyiapkan pengaturan "
          "awal, lalu menampilkan hasilnya di layar. Klik Lanjut untuk "
          "mulai — atau Nanti saja untuk melewati dan mengatur sendiri "
          "dari tab Wallpaper."),
      welcome_page);
  welcome_label->setWordWrap(true);
  welcome_layout->addWidget(welcome_label);
  welcome_layout->addStretch();
  wizard.addPage(welcome_page);

  auto* pick_page = new FirstRunPickPage(&wizard);
  pick_page->setObjectName(QStringLiteral("firstrun_pick_page"));
  wizard.addPage(pick_page);

  auto* opt_page = new QWizardPage(&wizard);
  opt_page->setObjectName(QStringLiteral("firstrun_opt_page"));
  opt_page->setTitle(Ftr("Pengaturan awal"));
  opt_page->setSubTitle(
      Ftr("Anda bisa mengubah semuanya nanti di tab Pengaturan."));
  auto* opt_layout = new QVBoxLayout(opt_page);
  auto* autostart_cb =
      new QCheckBox(Ftr("Jalankan saat Windows menyala"), opt_page);
  autostart_cb->setObjectName(QStringLiteral("firstrun_autostart"));
  autostart_cb->setToolTip(Ftr("K6WP menyala sendiri setiap masuk Windows"));
  autostart_cb->setChecked(false);
  auto* compress_cb = new QCheckBox(
      Ftr("Siapkan video otomatis (disarankan menyala)"), opt_page);
  compress_cb->setObjectName(QStringLiteral("firstrun_autocompress"));
  compress_cb->setToolTip(
      Ftr("Video disiapkan agar ringan dipakai sebagai wallpaper"));
  compress_cb->setChecked(true);
  opt_layout->addWidget(autostart_cb);
  opt_layout->addWidget(compress_cb);
  opt_layout->addStretch();
  wizard.addPage(opt_page);

  auto* tip_page = new QWizardPage(&wizard);
  tip_page->setObjectName(QStringLiteral("firstrun_tip_page"));
  tip_page->setTitle(Ftr("Saran"));
  tip_page->setSubTitle(Ftr("Satu hal sebelum mulai."));
  auto* tip_layout = new QVBoxLayout(tip_page);
  auto* tip_label = new QLabel(
      Ftr("Biarkan pilihan otomatis menyala. Video Anda akan disiapkan "
          "agar ringan dipakai sebagai wallpaper, lalu ditampilkan di "
          "layar. Klik Selesai untuk mulai — Anda bisa mengganti video "
          "kapan saja dari tab Wallpaper."),
      tip_page);
  tip_label->setWordWrap(true);
  tip_layout->addWidget(tip_label);
  tip_layout->addStretch();
  wizard.addPage(tip_page);

  wizard.setButtonText(QWizard::FinishButton, Ftr("Selesai"));
  wizard.setButtonText(QWizard::CancelButton, Ftr("Nanti saja"));
  wizard.setButtonText(QWizard::NextButton, Ftr("Lanjut"));
  wizard.setButtonText(QWizard::BackButton, Ftr("Kembali"));
  if (wizard.exec() != QDialog::Accepted) {
    return false;
  }
  if (out != nullptr) {
    out->file = pick_page->chosen();
    out->autostart = autostart_cb->isChecked();
    out->auto_compress = compress_cb->isChecked();
  }
  return true;
}

}  // namespace k6wp
