#include <iostream>
#include <limits>
#include <sstream>
#include <fstream>

#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QTextEdit>
#include <QDebug>

#include "MainWindow.h"
#include "./ui_mainwindow.h"

#include "TextViewDock.h"
#include "PlotViewDock.h"
#include "GeneralConfigDlg.h"

#include "renderer_qcp.h"

#include "AnalysisDlg/AnalysisSettingDlg.h"
#include "AnalysisDlg/Layout2dDlg.h"
#include "AnalysisDlg/TransverseRayFanDlg.h"
#include "AnalysisDlg/SingleRayTraceDlg.h"
#include "AnalysisDlg/LongitudinalDlg.h"
#include "AnalysisDlg/ParaxialTraceDlg.h"
#include "AnalysisDlg/PrescriptionDlg.h"
#include "AnalysisDlg/FieldCurvatureDlg.h"
#include "AnalysisDlg/ChromaticFocusShiftDlg.h"
#include "AnalysisDlg/SpotDiagramDlg.h"

using namespace ads;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("Geopter");

    // File menu
    QObject::connect(ui->actionNew,  SIGNAL(triggered()), this, SLOT(newFile()));
    QObject::connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(saveAs()));
    QObject::connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    QObject::connect(ui->actionPreference, SIGNAL(triggered()), this, SLOT(showPreference()));

    // Edit menu
    QObject::connect(ui->actionSetVigFactors, SIGNAL(triggered()), this, SLOT(setVignettingFactors()));

    // Analysis menu
    QObject::connect(ui->actionPrescription,           SIGNAL(triggered()), this, SLOT(showPrescription()));
    QObject::connect(ui->action2DLayout,               SIGNAL(triggered()), this, SLOT(showLayout()));
    QObject::connect(ui->actionSingleRayTrace,         SIGNAL(triggered()), this, SLOT(showSingleRayTrace()));
    QObject::connect(ui->actionParaxialRayTrace,       SIGNAL(triggered()), this, SLOT(showParaxialRayTrace()));
    QObject::connect(ui->actionRayAberration ,         SIGNAL(triggered()), this, SLOT(showTransverseRayFan()));
    QObject::connect(ui->actionLongitudinalAberration, SIGNAL(triggered()), this, SLOT(showLongitudinal()));
    QObject::connect(ui->actionFieldCurvature,         SIGNAL(triggered()), this, SLOT(showFieldCurvature()));
    QObject::connect(ui->actionChromaticFocusShift,    SIGNAL(triggered()), this, SLOT(showChromaticFocusShift()));
    QObject::connect(ui->actionSpotDiagram,            SIGNAL(triggered()), this, SLOT(showSpotDiagram()));

    // Help menu
    QObject::connect(ui->actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));


    CDockManager::setConfigFlag(CDockManager::OpaqueSplitterResize, true);
    CDockManager::setConfigFlag(CDockManager::XmlCompressionEnabled, false);
    CDockManager::setConfigFlag(CDockManager::FocusHighlighting, true);

    // create optical system
    opt_sys_ = std::make_shared<OpticalSystem>();
    opt_sys_->create_minimum_system();

    // set system editor as central dock
    m_dockManager = new CDockManager(this);

    m_systemEditorDock = new SystemEditorDock(opt_sys_, "System Editor");
    auto* CentralDockArea = m_dockManager->setCentralWidget(m_systemEditorDock );
    CentralDockArea->setAllowedAreas(DockWidgetArea::OuterDockAreas);

    // create python console
    QTabWidget *consoleTab = new QTabWidget;
    m_pyConsole = new PythonQtScriptingConsole(NULL, PythonQt::self()->getMainModule());
    m_stdoutText = new QTextEdit;
    m_stdoutText->setReadOnly(true);
    m_stderrText = new QTextEdit;
    m_stderrText->setReadOnly(true);
    consoleTab->addTab(m_pyConsole, tr("PyConsole"));
    consoleTab->addTab(m_stdoutText, tr("Output"));
    consoleTab->addTab(m_stderrText, tr("Error"));
    m_qout = new QDebugStream(std::cout, m_stdoutText);
    m_qerr = new QDebugStream(std::cerr, m_stderrText);

    CDockWidget* ConsoleDock = new CDockWidget("Console");

    ConsoleDock->setWidget(consoleTab);
    ConsoleDock->setFeature(CDockWidget::DockWidgetClosable, false);
    m_dockManager->addDockWidget(DockWidgetArea::BottomDockWidgetArea, ConsoleDock);
    ui->menuView->addAction(ConsoleDock->toggleViewAction());

    QString agfDir = QApplication::applicationDirPath() + "/AGF";
    loadAgfsFromDir(agfDir);

    m_systemEditorDock->syncUiWithSystem();

}

MainWindow::~MainWindow()
{
    opt_sys_.reset();
    delete m_qout;
    delete m_qerr;

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // Delete dock manager here to delete all floating widgets. This ensures
    // that all top level windows of the dock manager are properly closed
    m_dockManager->deleteLater();
    QMainWindow::closeEvent(event);
}


void MainWindow::loadAgfsFromDir(QString agfDir)
{
    std::vector< std::string > agf_paths;

    QStringList nameFilters;
    nameFilters.append("*.agf");
    nameFilters.append("*.AGF");

    QDir dir(agfDir);
    QStringList entry = dir.entryList(nameFilters, QDir::Files);
    for (QString file : entry) {
        qDebug() << dir.filePath(file);
        agf_paths.push_back(dir.filePath(file).toStdString());
    }

    bool ret = opt_sys_->material_lib()->load_agf_files(agf_paths);
    if(!ret){
        QMessageBox::warning(this,tr("Error") ,tr("AGF load error"));
    }

}


/*********************************************************************************************************************************
 *
 * File menu
 *
 * ********************************************************************************************************************************/
void MainWindow::newFile()
{
    opt_sys_->create_minimum_system();
    //opt_sys_->update_model();

    m_systemEditorDock->setOpticalSystem(opt_sys_);
    m_systemEditorDock->syncUiWithSystem();

    QMessageBox::information(this,tr("Info"), tr("Created new lens"));
}

void MainWindow::saveAs()
{
    // open file selection dialog
    QString filePath = QFileDialog::getSaveFileName(this, tr("select JSON"), QApplication::applicationDirPath(), tr("JSON files(*.json);;All Files(*.*)"));
    if(filePath.isEmpty()){
        return;
    }

    std::string json_path = filePath.toStdString();
    FileIO::save_to_json(*opt_sys_, json_path);

    QMessageBox::information(this,tr("Info"), tr("Saved to JSON file"));
}

void MainWindow::openFile()
{
    QStringList filePaths = QFileDialog::getOpenFileNames(this, tr("Open JSON"), QApplication::applicationDirPath(),tr("JSON files(*.json)"));
    if(filePaths.empty()){
        //QMessageBox::warning(this,tr("Canceled"), tr("Canceled"));
        return;
    }

    std::string json_path = filePaths.first().toStdString();
    FileIO::load_from_json(*opt_sys_,json_path);

    opt_sys_->update_model();

    m_systemEditorDock->setOpticalSystem(opt_sys_);
    m_systemEditorDock->syncUiWithSystem();

    QMessageBox::information(this,tr("Info"), tr("OpticalSystem newly loaded"));
}

void MainWindow::showPreference()
{
    GeneralConfigDlg* dlg = new GeneralConfigDlg(opt_sys_, this);
    dlg->exec();
    delete dlg;
}

/*********************************************************************************************************************************
 *
 * Edit menu
 *
 * ********************************************************************************************************************************/
void MainWindow::setVignettingFactors()
{
    opt_sys_->set_vignetting_factors();
    opt_sys_->update_model();
    m_systemEditorDock->syncUiWithSystem();
}


/*********************************************************************************************************************************
 *
 * Analysis menu
 *
 * ********************************************************************************************************************************/
void MainWindow::showPrescription()
{
    TextViewDock *dock = new TextViewDock("Prescription", opt_sys_.get());
    dock->createSettingDialog<PrescriptionDlg>();
    m_dockManager->addDockWidgetFloating(dock);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->resize(300,200);
    dock->updateText();
}


void MainWindow::showLayout()
{
    PlotViewDock *dock = new PlotViewDock("2D layout", opt_sys_.get());
    dock->createSettingDialog<Layout2dDlg>();
    m_dockManager->addDockWidgetFloating(dock);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->resize(300,200);
    dock->updatePlot();
}


void MainWindow::showParaxialRayTrace()
{
    TextViewDock *dock = new TextViewDock("Paraxial Ray Trace", opt_sys_.get());
    dock->createSettingDialog<ParaxialTraceDlg>();
    m_dockManager->addDockWidgetFloating(dock);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->resize(300,200);
    dock->updateText();
}

void MainWindow::showSingleRayTrace()
{
    TextViewDock *dock = new TextViewDock("Single Ray Trace", opt_sys_.get());
    dock->createSettingDialog<SingleRayTraceDlg>();
    m_dockManager->addDockWidgetFloating(dock);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->resize(300,200);
    dock->updateText();
}

void MainWindow::showSpotDiagram()
{
    PlotViewDock *dock = new PlotViewDock("Spot Diagram", opt_sys_.get());
    dock->createSettingDialog<SpotDiagramDlg>();
    m_dockManager->addDockWidgetFloating(dock);
    ui->menuView->addAction(dock->toggleViewAction());
    //dock->resize(300,200);
    dock->updatePlot();
}

void MainWindow::showTransverseRayFan()
{
    PlotViewDock *dock = new PlotViewDock("Transverse Aberration", opt_sys_.get());
    dock->createSettingDialog<TransverseRayFanDlg>();
    m_dockManager->addDockWidgetFloating(dock);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->resize(300,200);
    dock->updatePlot();
}

void MainWindow::showLongitudinal()
{
    PlotViewDock *dock = new PlotViewDock("Longitudinal Aberration", opt_sys_.get());
    dock->createSettingDialog<LongitudinalDlg>();
    m_dockManager->addDockWidgetFloating(dock);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->resize(300,200);
    dock->updatePlot();
}

void MainWindow::showFieldCurvature()
{
    PlotViewDock *dock = new PlotViewDock("Field Curvature", opt_sys_.get());
    dock->createSettingDialog<FieldCurvatureDlg>();
    m_dockManager->addDockWidgetFloating(dock);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->resize(300,200);
    dock->updatePlot();
}

void MainWindow::showChromaticFocusShift()
{
    PlotViewDock *dock = new PlotViewDock("Chromatic Focus Shift", opt_sys_.get());
    dock->createSettingDialog<ChromaticFocusShiftDlg>();
    m_dockManager->addDockWidgetFloating(dock);
    ui->menuView->addAction(dock->toggleViewAction());
    dock->resize(300,200);
    dock->updatePlot();   
}



/*********************************************************************************************************************************
 *
 * Tool menu
 *
 * ********************************************************************************************************************************/
void MainWindow::showDebugStream()
{

}



/*********************************************************************************************************************************
 *
 * Help menu
 *
 * ********************************************************************************************************************************/
void MainWindow::showAbout()
{
    // TODO: Dialog to show description and licence notice should be implemented

    QMessageBox::information(this,tr("About"), tr("Geopter v0.1.0"));
}

int MainWindow::nsur()
{
    return opt_sys_->optical_assembly()->surface_count();
}

int MainWindow::nwav()
{
    return opt_sys_->optical_spec()->spectral_region()->wvl_count();
}

void MainWindow::insertsurface(int i)
{
    m_systemEditorDock->systemEditorWidget()->insertLineOnAssemblyTable(i);
}