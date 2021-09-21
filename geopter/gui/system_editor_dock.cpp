#include <QApplication>

#include "system_editor_dock.h"

SystemEditorDock::SystemEditorDock(std::shared_ptr<OpticalSystem> opt_sys, QString label, QWidget *parent) :
    ads::CDockWidget(label, parent),
    m_opticalSystem(opt_sys)
{
    m_systemEditorWidget = new SystemEditorWidget(m_opticalSystem);
    this->setWidget(m_systemEditorWidget);

    m_toolBar = new QToolBar(this);
    auto actionUpdate = m_toolBar->addAction(QApplication::style()->standardIcon( QStyle::SP_BrowserReload ),"Update");
    m_toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    this->setToolBar(m_toolBar);

    QObject::connect(actionUpdate, SIGNAL(triggered()), this, SLOT(updateOpticalSystem()));
}

SystemEditorDock::~SystemEditorDock()
{
    delete m_toolBar;
    delete m_systemEditorWidget;
    m_opticalSystem.reset();
}

void SystemEditorDock::setOpticalSystem(std::shared_ptr<OpticalSystem> sys)
{
    m_opticalSystem = sys;
}

void SystemEditorDock::syncSystemWithUi()
{
    m_systemEditorWidget->syncSystemWithUi();
}

void SystemEditorDock::syncUiWithSystem()
{
    m_systemEditorWidget->syncUiWithSystem();
}

void SystemEditorDock::updateOpticalSystem()
{
    m_systemEditorWidget->syncSystemWithUi();
    m_systemEditorWidget->syncUiWithSystem();
}
