#include "lxqtfiledialoghelper.h"

#include <libfm-qt/libfmqt.h>
#include <libfm-qt/filedialog.h>

#include <QWindow>
#include <QDebug>
#include <QTimer>

#include <memory>

static std::unique_ptr<Fm::LibFmQt> libfmQtContext_;

LXQtFileDialogHelper::LXQtFileDialogHelper() {
    if(!libfmQtContext_) {
        // initialize libfm-qt only once
        libfmQtContext_ = std::unique_ptr<Fm::LibFmQt>{new Fm::LibFmQt()};
    }

    // can only be used after libfm-qt initialization
    dlg_ = std::unique_ptr<Fm::FileDialog>(new Fm::FileDialog());
    connect(dlg_.get(), &Fm::FileDialog::accepted, this, &LXQtFileDialogHelper::accept);
    connect(dlg_.get(), &Fm::FileDialog::rejected, this, &LXQtFileDialogHelper::reject);

    connect(dlg_.get(), &Fm::FileDialog::fileSelected, this, &LXQtFileDialogHelper::fileSelected);
    connect(dlg_.get(), &Fm::FileDialog::filesSelected, this, &LXQtFileDialogHelper::filesSelected);
    connect(dlg_.get(), &Fm::FileDialog::currentChanged, this, &LXQtFileDialogHelper::currentChanged);
    connect(dlg_.get(), &Fm::FileDialog::directoryEntered, this, &LXQtFileDialogHelper::directoryEntered);
    connect(dlg_.get(), &Fm::FileDialog::filterSelected, this, &LXQtFileDialogHelper::filterSelected);
}

LXQtFileDialogHelper::~LXQtFileDialogHelper() {
}

void LXQtFileDialogHelper::exec() {
    dlg_->exec();
}

bool LXQtFileDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow* parent) {
    dlg_->setAttribute(Qt::WA_NativeWindow, true); // without this, sometimes windowHandle() will return nullptr

    dlg_->setWindowFlags(windowFlags);
    dlg_->setWindowModality(windowModality);

    // Reference: KDE implementation
    // https://github.com/KDE/plasma-integration/blob/master/src/platformtheme/kdeplatformfiledialoghelper.cpp
    dlg_->windowHandle()->setTransientParent(parent);

    // NOTE: the timer here is required as a workaround borrowed from KDE. Without this, the dialog UI will be blocked.
    // QFileDialog calls our platform plugin to show our own native file dialog instead of showing its widget.
    // However, it still creates a hidden dialog internally, and then make it modal.
    // So user input from all other windows that are not the children of the QFileDialog widget will be blocked.
    // This includes our own dialog. After the return of this show() method, QFileDialog creates its own window and
    // then make it modal, which blocks our UI. The timer schedule a delayed popup of our file dialog, so we can
    // show again after QFileDialog and override the modal state. Then our UI can be unblocked.
    QTimer::singleShot(0, dlg_.get(), &QDialog::show);
    dlg_->setFocus();
    return true;
}

void LXQtFileDialogHelper::hide() {
    dlg_->hide();
}

bool LXQtFileDialogHelper::defaultNameFilterDisables() const {
    return false;
}

void LXQtFileDialogHelper::setDirectory(const QUrl& directory) {
    dlg_->setDirectory(directory);
}

QUrl LXQtFileDialogHelper::directory() const {
    return dlg_->directory();
}

void LXQtFileDialogHelper::selectFile(const QUrl& filename) {
    dlg_->selectFile(filename);
}

QList<QUrl> LXQtFileDialogHelper::selectedFiles() const {
    return dlg_->selectedFiles();
}

void LXQtFileDialogHelper::setFilter() {
    // FIXME: what's this?
}

void LXQtFileDialogHelper::selectMimeTypeFilter(const QString& filter) {
    // TODO
}

void LXQtFileDialogHelper::selectNameFilter(const QString& filter) {
    dlg_->selectNameFilter(filter);
}

QString LXQtFileDialogHelper::selectedMimeTypeFilter() const {
    // TODO
}

QString LXQtFileDialogHelper::selectedNameFilter() const {
    return dlg_->selectedNameFilter();
}

bool LXQtFileDialogHelper::isSupportedUrl(const QUrl& url) const {
    return dlg_->isSupportedUrl(url);
}

void LXQtFileDialogHelper::initDialog() {
    auto& opt = options();
    dlg_->setFilter(opt->filter());
    dlg_->setViewMode(opt->viewMode() == QFileDialog::Detail ? Fm::FolderView::DetailedListMode : Fm::FolderView::CompactMode);
    dlg_->setFileMode(QFileDialog::FileMode(opt->fileMode()));
    dlg_->setAcceptMode(QFileDialog::AcceptMode(opt->acceptMode()));
    // bool useDefaultNameFilters() const;
    dlg_->setNameFilters(opt->nameFilters());
    if(!opt->mimeTypeFilters().empty()) {
        dlg_->setMimeTypeFilters(opt->mimeTypeFilters());
    }

    dlg_->setDefaultSuffix(opt->defaultSuffix());
    // QStringList history() const;

    /*
    void setLabelText(DialogLabel label, const QString &text);
    QString labelText(DialogLabel label) const;
    bool isLabelExplicitlySet(DialogLabel label);
    */
    auto url = opt->initialDirectory();
    if(url.isValid()) {
        dlg_->setDirectory(url);
    }

    auto filter = opt->initiallySelectedMimeTypeFilter();
    if(!filter.isEmpty()) {
        selectMimeTypeFilter(filter);
    }

    filter = opt->initiallySelectedNameFilter();
    if(!filter.isEmpty()) {
        selectNameFilter(filter);
    }

    auto selectedFiles = opt->initiallySelectedFiles();
    for(const auto& selectedFile: selectedFiles) {
        selectFile(selectedFile);
    }
    // QStringList supportedSchemes() const;
}

/*
FileDialogPlugin::FileDialogPlugin() {

}

QPlatformFileDialogHelper *FileDialogPlugin::createHelper() {
    return new LXQtFileDialogHelper();
}
*/
