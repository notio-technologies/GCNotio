/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)  *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "BlankState.h"
#include <QtGui>
#include "MainWindow.h"
#include "Context.h"
#include "Colors.h"
#include "Athlete.h"
#include "AddCloudWizard.h"
#include "CloudService.h"

//
// Replace home window when no ride
//
BlankStatePage::BlankStatePage(Context *context) : GcWindow(context), context(context), canShow_(true)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch();
    QHBoxLayout *homeLayout = new QHBoxLayout;
    mainLayout->addLayout(homeLayout);
    homeLayout->setAlignment(Qt::AlignCenter);
    homeLayout->addSpacing(20); // left margin
    setAutoFillBackground(true);
    setProperty("color", QColor(Qt::white));
    setProperty("nomenu", true);

    // left part
    QWidget *left = new QWidget(this);
    leftLayout = new QVBoxLayout(left);
    leftLayout->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    left->setLayout(leftLayout);

    welcomeTitle = new QLabel(left);
    welcomeTitle->setFont(QFont("Helvetica", 30, QFont::Bold, false));
    leftLayout->addWidget(welcomeTitle);

    welcomeText = new QLabel(left);
    welcomeText->setFont(QFont("Helvetica", 16, QFont::Light, false));
    leftLayout->addWidget(welcomeText);

    leftLayout->addSpacing(10);

    homeLayout->addWidget(left);
    homeLayout->addSpacing(50);

    QWidget *right = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(right);
    rightLayout->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    right->setLayout(rightLayout);

    img = new QToolButton(this);
    img->setFocusPolicy(Qt::NoFocus);
    img->setToolButtonStyle(Qt::ToolButtonIconOnly);
    img->setStyleSheet("QToolButton {text-align: left;color : blue;background: transparent}");
    rightLayout->addWidget(img);

    homeLayout->addWidget(right);
    // right margin
    homeLayout->addSpacing(20);

    // control if shown or not in future
    QHBoxLayout *bottomRow = new QHBoxLayout;
    mainLayout->addSpacing(20);
    mainLayout->addLayout(bottomRow);

    dontShow = new QCheckBox(tr("Don't show this next time."), this);
    dontShow->setFocusPolicy(Qt::NoFocus);
    closeButton = new QPushButton(tr("Close"), this);
    closeButton->setFocusPolicy(Qt::NoFocus);
    bottomRow->addWidget(dontShow);
    bottomRow->addStretch();
    bottomRow->addWidget(closeButton);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(setCanShow()));
    connect(closeButton, SIGNAL(clicked()), this, SIGNAL(closeClicked()));
}

void
BlankStatePage::setCanShow()
{
    // the view was closed, so set canShow_ off
    canShow_ = false;
    saveState();
}

QPushButton*
BlankStatePage::addToShortCuts(ShortCut shortCut)
{
    //
    // Separator
    //
    if (shortCuts.count()>0) {
        leftLayout->addSpacing(20);
        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        leftLayout->addWidget(line);
    }

    // append to the list of shortcuts
    shortCuts.append(shortCut);

    //
    // Create text and button
    //
    QLabel *shortCutLabel = new QLabel(this);
    shortCutLabel->setWordWrap(true);
    shortCutLabel->setText(shortCut.label);
    shortCutLabel->setFont(QFont("Helvetica", 14, QFont::Light, false));
    leftLayout->addWidget(shortCutLabel);

    QPushButton *shortCutButton = new QPushButton(this);
    shortCutButton->setFocusPolicy(Qt::NoFocus);
    shortCutButton->setText(shortCut.buttonLabel);
    shortCutButton->setIcon(QPixmap(shortCut.buttonIconPath));
    shortCutButton->setIconSize(QSize(40,40));
    //importButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    //importButton->setStyleSheet("QToolButton {text-align: left;color : blue;background: transparent}");
    shortCutButton->setStyleSheet("QPushButton {border-radius: 10px;border-style: outset; background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #DDDDDD, stop: 1 #BBBBBB); border-width: 1px; border-color: #555555;} QPushButton:pressed {background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #BBBBBB, stop: 1 #999999);}");
    shortCutButton->setFixedSize(200*dpiXFactor, 60*dpiYFactor);
    leftLayout->addWidget(shortCutButton);

    return shortCutButton;
}

//
// Replace analysis window when no ride
//
BlankStateAnalysisPage::BlankStateAnalysisPage(Context *context) : BlankStatePage(context)
{  
    dontShow->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_ANALYSIS, false).toBool());
    welcomeTitle->setText(tr("Activities"));
    welcomeText->setText(tr("No files ?\nLet's start with some data."));

    img->setIcon(QPixmap(":images/analysis.png"));
    img->setIconSize(QSize(800,330));

    ShortCut scImport;
    scImport.label = tr("Import files from your disk or usb device");
    scImport.buttonLabel = tr("Import data");
    scImport.buttonIconPath = ":images/mac/download.png";
    QPushButton *importButton = addToShortCuts(scImport);
    connect(importButton, SIGNAL(clicked()), context->mainWindow, SLOT(importFile()));

    ShortCut scDownload;
    scDownload.label = tr("Download from serial device.");
    scDownload.buttonLabel = tr("Download from device");
    scDownload.buttonIconPath = ":images/mac/download.png";
    QPushButton *downloadButton = addToShortCuts(scDownload);
    connect(downloadButton, SIGNAL(clicked()), context->mainWindow, SLOT(downloadRide()));

    canShow_ = !appsettings->cvalue(context->athlete->cyclist, GC_BLANK_ANALYSIS).toBool();
}

// Notio buttons stylesheet.
QString BlankStateNotioAnalysisPage::m_buttonStyle =
        QString::fromUtf8("QPushButton {"
                          "   background-color: black;"
                          "   border-width: 0px;"
                          "   border-radius: %1px;"
                          "   color: #fff200;"
                          "   font: bold 14px \"AvenirNext\";"
                          "}"
                          "QPushButton:hover:pressed {"
                          "   background-color: #706800;"
                          "   color: black;"
                          "   border-style: outset;"
                          "   border-width: 0px;"
                          "   border-radius: %1px;"
                          "   font: bold 14px \"AvenirNext\";"
                          "}"
                          "QPushButton:hover {"
                          "   background-color: #fff200;"
                          "   border-width: 0px;"
                          "   border-radius: %1px;"
                          "   color: black;"
                          "   font: bold 14px \"AvenirNext\";"
                          "}"
                          "QPushButton:disabled {"
                          "   background-color: #606060;"
                          "}"
                          "QPushButton:checked {"
                          "   background-color: #3fa93a;"
                          "   color: white;"
                          "}"
                          "QPushButton:checked:hover:pressed {"
                          "   background-color: #1d4c1a;"
                          "   color: white;"
                          "}"
                          "QPushButton:checked:hover {"
                          "   background-color: #337f2c;;"
                          "   color: white;"
                          "}");

///////////////////////////////////////////////////////////////////////////////
/// \brief BlankStateNotioAnalysisPage::BlankStateNotioAnalysisPage
///        Constructor.
///
/// \param[in/out] context Context object pointer.
///////////////////////////////////////////////////////////////////////////////
BlankStateNotioAnalysisPage::BlankStateNotioAnalysisPage(Context *iContext) : BlankStatePage(iContext)
{
    // Set background color.
    setProperty("color", QColor("#f3f3f3"));
    dontShow->setChecked(appsettings->cvalue(iContext->athlete->cyclist, GC_BLANK_ANALYSIS, false).toBool());

    // Set Notio logo.
    m_notioLogo.load(":images/services/notio.png");
    welcomeTitle->setPixmap(m_notioLogo.scaled(kLogoSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // Set text.
    welcomeText->setTextFormat(Qt::RichText);
    welcomeText->setText(QString("<font size=5 face=Helvetica><b>%1</b></font><br>%2").arg(tr("Quick Steps"), tr("Follow these steps to start analyzing your data.")));

    // Set image.
    img->setIcon(QPixmap(":images/notioAnalysis.png"));
    img->setIconSize(QSize(800, 330));

    // Add Cloud Wizard shortcut.
    ShortCut wConnectSc;
    wConnectSc.label = tr("1. Connect to your Notio Account.");
    wConnectSc.buttonLabel = tr("ADD ACCOUNT");        
    m_connectButton = addToShortCuts(wConnectSc);
    m_connectButton->setWhatsThis(tr("Opens the cloud wizard."));
    m_connectButton->setCheckable(true);
    connect(m_connectButton, SIGNAL(clicked()), SLOT(addNotioAccount()));

    // Notio Sync dialog shortcut.
    ShortCut wSyncSc;
    wSyncSc.label = tr("2. Synchronize with Notio Cloud.");
    wSyncSc.buttonLabel = tr("SYNC");
    m_syncButton = addToShortCuts(wSyncSc);
    m_syncButton->setWhatsThis(tr("Opens the Notio synchronization dialog window."));
    connect(m_syncButton, SIGNAL(clicked()), SLOT(syncNotio()));

    // Import activity from file shortcut.
    ShortCut wImportSc;
    wImportSc.label = tr("Or, import files saved locally.");
    wImportSc.buttonLabel = tr("IMPORT DATA");
    m_importButton = addToShortCuts(wImportSc);
    m_importButton->setWhatsThis(tr("Opens the filesystem browser to import an activity file."));
    connect(m_importButton, SIGNAL(clicked()), iContext->mainWindow, SLOT(importFile()));

    // User Guide shortcut.
    ShortCut wHelpSc;
    wHelpSc.label = tr("Need help?");
    wHelpSc.buttonLabel = tr("USER GUIDE");
    m_helpButton = addToShortCuts(wHelpSc);
    m_helpButton->setWhatsThis(tr("Opens the Notio's user guide into your web browser."));
    connect(m_helpButton, SIGNAL(clicked()), iContext->mainWindow, SLOT(notioHelpView()));

    // Set buttons stylesheet and resize buttons.
    QFontMetrics wFontMetric(QApplication::font());
    double wButtonWidth = kButtonPadding + std::max({ wFontMetric.width(m_connectButton->text()), wFontMetric.width(m_syncButton->text()),
                                                      wFontMetric.width(m_importButton->text()), wFontMetric.width(m_helpButton->text()) });

    // Calculate button height based on longest button text.
    double wButtonHeight = wButtonWidth / kButtonRatio;

    // Determine button border radius.
    int wButtonRadius = static_cast<int>(wButtonHeight / 2);

    // Update base stylesheet.
    QString wStyleSheet = m_buttonStyle.arg(wButtonRadius);

    // Resize and apply stylesheet.
    m_connectButton->setFixedSize(static_cast<int>(wButtonWidth), static_cast<int>(wButtonHeight));
    m_connectButton->setStyleSheet(wStyleSheet);
    m_syncButton->setFixedSize(static_cast<int>(wButtonWidth), static_cast<int>(wButtonHeight));
    m_syncButton->setStyleSheet(wStyleSheet);
    m_importButton->setFixedSize(static_cast<int>(wButtonWidth), static_cast<int>(wButtonHeight));
    m_importButton->setStyleSheet(wStyleSheet);
    m_helpButton->setFixedSize(static_cast<int>(wButtonWidth), static_cast<int>(wButtonHeight));
    m_helpButton->setStyleSheet(wStyleSheet);

    // Verify that Notio cloud account has been configured and it is active.
    updateConnectButton();
    connect(context, SIGNAL(configChanged(qint32)), SLOT(updateConnectButton()));

    canShow_ = !appsettings->cvalue(iContext->athlete->cyclist, GC_BLANK_ANALYSIS).toBool();
}

///////////////////////////////////////////////////////////////////////////////
/// \brief BlankStateNotioAnalysisPage::saveState
///        This method is called when the blank page is closed.
///////////////////////////////////////////////////////////////////////////////
void BlankStateNotioAnalysisPage::saveState()
{
    // Save "don't show" option for the analysis view.
    appsettings->setCValue(context->athlete->cyclist, GC_BLANK_ANALYSIS, dontShow->isChecked());
}

///////////////////////////////////////////////////////////////////////////////
/// \brief BlankStateNotioAnalysisPage::event
///        This method is called when an event occurs. It filter for a "Paint
///        Event" and resize logo and buttons widgets.
///
/// \param[in] iEvent   Event that occured.
///
/// \return The input event.
///////////////////////////////////////////////////////////////////////////////
bool BlankStateNotioAnalysisPage::event(QEvent *iEvent)
{
    if ((iEvent->type() == QEvent::Paint))
    {
        // Resize Notio logo.
        QSize wNewSize = geometry().size() / 8;

        // Check for maximum size.
        if (wNewSize.height() > kLogoSize.height())
            wNewSize = kLogoSize;

        // Set logo.
        welcomeTitle->setPixmap(m_notioLogo.scaled(wNewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    return QWidget::event(iEvent);
}

///////////////////////////////////////////////////////////////////////////////
/// \brief BlankStateNotioAnalysisPage::syncNotio
///        This method calls the sync dialog for Notio cloud service.
///////////////////////////////////////////////////////////////////////////////
void BlankStateNotioAnalysisPage::syncNotio()
{
    qDebug() << Q_FUNC_INFO;

    // Update connect button status.
    updateConnectButton();

    // Get cloud service with name. Skip if not Notio and not active.
    const CloudService *wNotioService = CloudServiceFactory::instance().service("NotioCloud");

    // Verify that Notio cloud account has been configured and it is active.
    if (wNotioService && (appsettings->cvalue(context->athlete->cyclist, wNotioService->activeSettingName(), "false").toString() == "true"))
    {
        // Verify if service accept queries.
        if (wNotioService->capabilities() & CloudService::Query)
        {
            // Create service action for sync dialog.
            QAction *wServiceAction = new QAction(nullptr);
            wServiceAction->setData(wNotioService->id());

            // Block main window to avoid calling multiple sync dialog from welcome screen.
            context->mainWindow->setEnabled(false);

            // Open sync dialog.
            context->mainWindow->syncCloud(wServiceAction);
            context->mainWindow->setEnabled(true);
        }
    }
    // Need to add account, so ask user.
    else
    {
        QMessageBox *wMsgBox = new QMessageBox(this);

        wMsgBox->setWindowTitle(tr("Notio Synchronization"));
        wMsgBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        wMsgBox->setIcon(QMessageBox::Warning);
        wMsgBox->setWindowModality(Qt::WindowModality::WindowModal);

        // Show a message to the user.
        wMsgBox->setText(tr("You need to connect to your Notio cloud account first."));
        if (wMsgBox->exec() & QMessageBox::Ok)
        {
            addNotioAccount();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief BlankStateNotioAnalysisPage::addNotioAccount
///        This method is called when user click on the button to add a cloud
///        account.
///////////////////////////////////////////////////////////////////////////////
void BlankStateNotioAnalysisPage::addNotioAccount()
{
    qDebug() << Q_FUNC_INFO;

    if (context)
    {
        // Set connect button checked status.
        const CloudService *wNotioService = CloudServiceFactory::instance().service("NotioCloud");
        bool wNotioCloudActive = (wNotioService && (appsettings->cvalue(context->athlete->cyclist, wNotioService->activeSettingName(), "false").toString() == "true"));

        m_connectButton->setChecked(wNotioCloudActive);

        // Call the cloud wizard.
        AddCloudWizard *wCloudWizard = new AddCloudWizard(context);

        connect(wCloudWizard, SIGNAL(finished(int)), this, SLOT(updateConnectButton()));
        wCloudWizard->exec();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \brief BlankStateNotioAnalysisPage::updateConnectButton
///        This method updates the connect button checked status to indicate to
///        the user that a Notio cloud account is active.
///////////////////////////////////////////////////////////////////////////////
void BlankStateNotioAnalysisPage::updateConnectButton()
{
    if (m_connectButton)
    {
        const CloudService *wNotioService = CloudServiceFactory::instance().service("NotioCloud");
        bool wNotioCloudActive = (wNotioService && (appsettings->cvalue(context->athlete->cyclist, wNotioService->activeSettingName(), "false").toString() == "true"));

        m_connectButton->setChecked(wNotioCloudActive);
    }
}

//
// Replace home window when no ride
//
BlankStateHomePage::BlankStateHomePage(Context *context) : BlankStatePage(context)
{
    dontShow->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_HOME, false).toBool());
    welcomeTitle->setText(tr("Trends"));
    welcomeText->setText(tr("No ride ?\nLet's start with some data."));

    img->setIcon(QPixmap(":images/home.png"));
    img->setIconSize(QSize(800,330));

    ShortCut scImport;
    scImport.label = tr("Import files from your disk or usb device");
    scImport.buttonLabel = tr("Import data");
    scImport.buttonIconPath = ":images/mac/download.png";
    QPushButton *importButton = addToShortCuts(scImport);
    connect(importButton, SIGNAL(clicked()), context->mainWindow, SLOT(importFile()));

    ShortCut scDownload;
    scDownload.label = tr("Download from serial device.");
    scDownload.buttonLabel = tr("Download from device");
    scDownload.buttonIconPath = ":images/mac/download.png";
    QPushButton *downloadButton = addToShortCuts(scDownload);
    connect(downloadButton, SIGNAL(clicked()), context->mainWindow, SLOT(downloadRide()));

    canShow_ = !appsettings->cvalue(context->athlete->cyclist, GC_BLANK_HOME).toBool();
}

//
// Replace diary window when no ride
//
BlankStateDiaryPage::BlankStateDiaryPage(Context *context) : BlankStatePage(context)
{
    dontShow->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_DIARY, false).toBool());
    welcomeTitle->setText(tr("Diary"));
    welcomeText->setText(tr("No ride ?\nLet's start with some data."));

    img->setIcon(QPixmap(":images/diary.png"));
    img->setIconSize(QSize(800,330));

    ShortCut scImport;
    scImport.label = tr("Import files from your disk or usb device");
    scImport.buttonLabel = tr("Import data");
    scImport.buttonIconPath = ":images/mac/download.png";
    QPushButton *importButton = addToShortCuts(scImport);
    connect(importButton, SIGNAL(clicked()), context->mainWindow, SLOT(importFile()));

    ShortCut scDownload;
    scDownload.label = tr("Download from serial device.");
    scDownload.buttonLabel = tr("Download from device");
    scDownload.buttonIconPath = ":images/mac/download.png";
    QPushButton *downloadButton = addToShortCuts(scDownload);
    connect(downloadButton, SIGNAL(clicked()), context->mainWindow, SLOT(downloadRide()));

    canShow_ = !appsettings->cvalue(context->athlete->cyclist, GC_BLANK_DIARY).toBool();
}

//
// Replace train window when no ride
//
BlankStateTrainPage::BlankStateTrainPage(Context *context) : BlankStatePage(context)
{
    dontShow->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_BLANK_TRAIN, false).toBool());
    welcomeTitle->setText(tr("Train"));
    welcomeText->setText(tr("No devices or workouts ?\nLet's get you setup."));

    img->setIcon(QPixmap(":images/train.png"));
    img->setIconSize(QSize(800,330));

    ShortCut scAddDevice;
    // - add a realtime device
    // - find video and workouts
    scAddDevice.label = tr("Find and add training devices.");
    scAddDevice.buttonLabel = tr("Add device");
    scAddDevice.buttonIconPath = ":images/devices/kickr.png";
    QPushButton *addDeviceButton = addToShortCuts(scAddDevice);
    connect(addDeviceButton, SIGNAL(clicked()), context->mainWindow, SLOT(addDevice()));


    ShortCut scImportWorkout;
    scImportWorkout.label = tr("Find and Import your videos and workouts.");
    scImportWorkout.buttonLabel = tr("Scan hard drives");
    scImportWorkout.buttonIconPath = ":images/toolbar/Disk.png";
    QPushButton *importWorkoutButton = addToShortCuts(scImportWorkout);
    connect(importWorkoutButton, SIGNAL(clicked()), context->mainWindow, SLOT(manageLibrary()));

    ShortCut scDownloadWorkout;
    scDownloadWorkout.label = tr("Download workout files from the Erg DB.");
    scDownloadWorkout.buttonLabel = tr("Download workouts");
    scDownloadWorkout.buttonIconPath = ":images/mac/download.png";
    QPushButton *downloadWorkoutButton = addToShortCuts(scDownloadWorkout);
    connect(downloadWorkoutButton, SIGNAL(clicked()), context->mainWindow, SLOT(downloadErgDB()));

    canShow_ = !appsettings->cvalue(context->athlete->cyclist, GC_BLANK_TRAIN).toBool();
}

// save away the don't show stuff
void
BlankStateAnalysisPage::saveState()
{
    appsettings->setCValue(context->athlete->cyclist, GC_BLANK_ANALYSIS, dontShow->isChecked());
}
void
BlankStateDiaryPage::saveState()
{
    appsettings->setCValue(context->athlete->cyclist, GC_BLANK_DIARY, dontShow->isChecked());
}
void
BlankStateHomePage::saveState()
{
    appsettings->setCValue(context->athlete->cyclist, GC_BLANK_HOME, dontShow->isChecked());
}
void
BlankStateTrainPage::saveState()
{
    appsettings->setCValue(context->athlete->cyclist, GC_BLANK_TRAIN, dontShow->isChecked());
}

