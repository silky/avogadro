/**********************************************************************
  MainWindow - main window, menus, main actions

  Copyright (C) 2006 by Geoffrey R. Hutchison
  Some portions Copyright (C) 2006 by Donald E. Curtis

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.sourceforge.net/>

  Some code is based on Open Babel
  For more information, see <http://openbabel.sourceforge.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#include "mainwindow.h"
#include "aboutdialog.h"
#include <avogadro/primitives.h>
#include <avogadro/toolgroup.h>

#include <fstream>

#include <QHeaderView>
#include <QFileDialog>
#include <QToolButton>
#include <QStackedLayout>
#include <QUndoStack>
#include <QSettings>
#include <QColorDialog>
#include <QMessageBox>
#include <QGLFramebufferObject>
#include <QStandardItem>

using namespace std;
using namespace OpenBabel;

namespace Avogadro {

  class MainWindowPrivate 
  {
    public:
      MainWindowPrivate() : molecule(0), 
      undo(0), toolsFlow(0), toolSettingsStacked(0), messagesText(0),
      toolGroup(0) {}

      Molecule  *molecule;

      QString    currentFile;
      bool       modified;
      QUndoStack *undo;
      
      FlowLayout *toolsFlow;
      QStackedLayout *toolSettingsStacked;

      QTextEdit *messagesText;

      ToolGroup *toolGroup;
      QAction    *actionRecentFile[MainWindow::maxRecentFiles];
  };

  MainWindow::MainWindow() : QMainWindow(0), d_ptr(new MainWindowPrivate)
  {
    constructor();
    setCurrentFile("");
  }

  MainWindow::MainWindow(const QString &fileName) : QMainWindow(0), d_ptr(new MainWindowPrivate)
  {
    constructor();
    loadFile(fileName);
  }

  void MainWindow::constructor()
  {
    Q_D(MainWindow);
    d->molecule = new Molecule(this);

    ui.setupUi(this);
    
    readSettings();
    setAttribute(Qt::WA_DeleteOnClose);

    d->undo = new QUndoStack(this);
    d->toolsFlow = new FlowLayout(ui.toolsWidget);
    d->toolsFlow->setMargin(9);
    d->toolSettingsStacked = new QStackedLayout(ui.toolSettingsWidget);

    d->toolGroup = new ToolGroup(this);
    d->toolGroup->load();
    connect(d->toolGroup, SIGNAL(toolActivated(Tool *)), this, SLOT(setTool(Tool *)));

    const QList<Tool *> tools = d->toolGroup->tools();
    int toolCount = tools.size();
    for(int i = 0; i < toolCount; i++)
    {
      QAction *action = tools.at(i)->activateAction();
      QToolButton *button = new QToolButton(ui.toolsWidget);
      button->setDefaultAction(action);
      d->toolsFlow->addWidget(button);
      d->toolSettingsStacked->addWidget(tools.at(i)->settingsWidget());
    }

//     QVBoxLayout *vbCentral = new QVBoxLayout(ui.centralWidget);
//     vbCentral->setMargin(0);
//     vbCentral->setSpacing(0);

    ui.glWidget->setMolecule(d->molecule);
    ui.glWidget->setToolGroup(d->toolGroup);
    // at least for now, try to always do multisample OpenGL (i.e., antialias)
    // graphical improvement is great and many cards do this in hardware
    // At some point, this should be a preference
//     QGLFormat format;
//     format.setSampleBuffers(true);
//     ui.glWidget = new GLWidget(d->molecule, format, ui.centralWidget);
//     vbCentral->addWidget(ui.glWidget);

//     ui.bottomFlat = new FlatTabWidget(ui.centralWidget);
//     vbCentral->addWidget(ui.bottomFlat);

    QWidget *messagesWidget = new QWidget();
    QVBoxLayout *messagesVBox = new QVBoxLayout(messagesWidget);
    d->messagesText = new QTextEdit();
    d->messagesText->setReadOnly(true);
    messagesVBox->setMargin(3);
    messagesVBox->addWidget(d->messagesText);
    ui.bottomFlat->addTab(messagesWidget, tr("Messages"));

//     QList<int> sizes = d->splitCentral->sizes();
//     sizes[0] = ui.glWidget->maximumHeight();
//     d->splitCentral->setSizes(sizes);

//     ui.projectTree->setAnimated(true);
//     ui.projectTree->setAllColumnsShowFocus(true);
//     ui.projectTree->setAlternatingRowColors(true);
    //ui.projectTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
//     ui.projectTree->setSelectionMode(QAbstractItemView::MultiSelection);
//     ui.projectTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui.projectTree->setMolecule(d->molecule);
    ui.projectTree->header()->hide();

//dc:     toolBox = new QToolBox(this);
//dc:     toolBox->addItem(new QWidget(this), "Tools");
//dc:     toolBox->addItem(new QWidget(this), "Tool Settings");
//dc: 
//dc:     layout->addWidget(gl,0,0);
//dc:     layout->addWidget(toolBox, 0,1);


    loadPlugins();

//     // add all gl engines to the dropdown
//     QList<Engine *> engines = ui.glWidget->engines();
//     for(int i=0; i< engines.size(); ++i) {
//       Engine *engine = engines.at(i);
// //       cbEngine->insertItem(i, engine->description(), QVariant(engine));
//     }
    ui.enginesList->setGLWidget(ui.glWidget);

    // set the default to whatever GL has selected as default on startup
//     cbEngine->setCurrentIndex(engines.indexOf(ui.glWidget->getDefaultEngine()));
//     cbTool->setCurrentIndex(d->tools.indexOf(d->currentTool));

//     connect(cbEngine, SIGNAL(activated(int)), ui.glWidget, SLOT(setDefaultEngine(int)));
//     connect(cbTool, SIGNAL(activated(int)), this, SLOT(setTool(int)));

    connectUi();

    statusBar()->showMessage(tr("Ready."), 10000);

    qDebug() << "MainWindow Initialized" << endl;

  }

  void MainWindow::newFile()
  {
  }

  void MainWindow::openFile()
  {
    Q_D(MainWindow);

    QString fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty()) {
      // check to see if we already have an open window
      MainWindow *existing = findMainWindow(fileName);
      if (existing) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        return;
      }

      if (d->currentFile.isEmpty())
        loadFile(fileName);
      else {
        MainWindow *other = new MainWindow;
        other->move(x() + 40, y() + 40);
        other->show();
        other->loadFile(fileName);
      }
    }
  }

  void MainWindow::openRecentFile()
  {
    Q_D(MainWindow);

    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
      MainWindow *existing = findMainWindow(action->data().toString());
      if (existing) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        return;
      }

      if (d->currentFile.isEmpty())
        loadFile(action->data().toString());
      else {
        MainWindow *other = new MainWindow;
        other->move(x() + 40, y() + 40);
        other->show();
        other->loadFile(action->data().toString());
      }
    }
  }

  void MainWindow::closeEvent(QCloseEvent *event)
  {
    if (maybeSave()) {
      writeSettings();
      event->accept();
    } else {
      event->ignore();
    }
  }

  bool MainWindow::save()
  {
    Q_D(MainWindow);

    if (d->currentFile.isEmpty())
      return saveAs();
    else
      return saveFile(d->currentFile);
  }

  bool MainWindow::saveAs()
  {
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
      return false;

    if (QFile::exists(fileName)) {
      QMessageBox::StandardButton ret;
      ret = QMessageBox::warning(this, tr("Avogadro"),
          tr("File %1 already exists.\n"
            "Do you want to overwrite it?")
          .arg(QDir::convertSeparators(fileName)),
          QMessageBox::Yes | QMessageBox::Cancel);
      if (ret == QMessageBox::Cancel)
        return false;
    }
    return saveFile(fileName);
  }

  void MainWindow::exportGraphics()
  {
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
      return;

    // render it (with alpha channel)
    if (!ui.glWidget->grabFrameBuffer(true).save(fileName))
    {
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot save file %1.").arg(fileName));
      return;
    }
  }

  void MainWindow::revert()
  {
    Q_D(MainWindow);

    // this currently leaks -- need to free the display list and render, etc.
    if (!d->currentFile.isEmpty()) {
      loadFile(d->currentFile);
    }
  }

  void MainWindow::documentWasModified()
  {
    Q_D(MainWindow);

    setWindowModified(true);
    d->modified = false;
  }

  bool MainWindow::maybeSave()
  {
    Q_D(MainWindow);

    if (d->modified) {
      QMessageBox::StandardButton ret;
      ret = QMessageBox::warning(this, tr("Avogadro"),
          tr("The document has been modified.\n"
            "Do you want to save your changes?"),
          QMessageBox::Save | QMessageBox::Discard
          | QMessageBox::Cancel);
      if (ret == QMessageBox::Save)
        return save();
      else if (ret == QMessageBox::Cancel)
        return false;
    }
    return true;
  }

  void MainWindow::clearRecentFiles()
  {
    QSettings settings; // already set up properly via main.cpp
    QStringList files;
    settings.setValue("recentFileList", files);

    updateRecentFileActions();
  }

  void MainWindow::about()
  {
      AboutDialog * about = new AboutDialog(this);
      about->show();
  }

  void MainWindow::fullScreen()
  {
    if (!this->isFullScreen()) {
      ui.actionFullScreen->setText(tr("Normal Size"));
      ui.fileToolBar->hide();
      statusBar()->hide();
      this->showFullScreen();
    } else {
      this->showNormal();
      ui.actionFullScreen->setText(tr("Full Screen"));
      ui.fileToolBar->show();
      statusBar()->show();
    }
  }

  void MainWindow::setBackgroundColor()
  {
    QColor current = ui.glWidget->background();
    ui.glWidget->setBackground(QColorDialog::getRgba(current.rgba(), NULL, this));
  }

  void MainWindow::setTool(Tool *tool)
  {
    Q_D(MainWindow);

    d->toolSettingsStacked->setCurrentWidget(tool->settingsWidget());
  }

  void MainWindow::updateEngine( QStandardItem *item )
  {
    Engine *engine = item->data().value<Engine *>();
    if(engine) {
      engine->setEnabled(item->checkState());
      ui.glWidget->updateGL();
    }
  }

  void MainWindow::connectUi()
  {
    Q_D(MainWindow);

    connect(ui.actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(ui.actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
    connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
    connect(ui.actionClose, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
    connect(ui.actionSaveAs, SIGNAL(triggered()), this, SLOT(saveAs()));
    connect(ui.actionRevert, SIGNAL(triggered()), this, SLOT(revert()));
    connect(ui.actionExportGraphics, SIGNAL(triggered()), this, SLOT(exportGraphics()));
    ui.actionExportGraphics->setEnabled(QGLFramebufferObject::hasOpenGLFramebufferObjects());

    for (int i = 0; i < maxRecentFiles; ++i) {
      d->actionRecentFile[i] = new QAction(this);
      d->actionRecentFile[i]->setVisible(false);
      ui.menuOpenRecent->addAction(d->actionRecentFile[i]);
      connect(d->actionRecentFile[i], SIGNAL(triggered()),
          this, SLOT(openRecentFile()));
    }
    
    ui.menuDocks->addAction(ui.projectDock->toggleViewAction());
    ui.menuDocks->addAction(ui.toolsDock->toggleViewAction());
    ui.menuDocks->addAction(ui.toolSettingsDock->toggleViewAction());
    ui.menuToolbars->addAction(ui.fileToolBar->toggleViewAction());
    

    connect(ui.actionClearRecent, SIGNAL(triggered()), this, SLOT(clearRecentFiles()));
    connect(ui.actionFullScreen, SIGNAL(triggered()), this, SLOT(fullScreen()));
    connect(ui.actionSetBackgroundColor, SIGNAL(triggered()), this, SLOT(setBackgroundColor()));
    connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(about()));

    connect(ui.enginesList, SIGNAL(itemChanged(QStandardItem *)), 
        this, SLOT(updateEngine(QStandardItem *)));
  }

  bool MainWindow::loadFile(const QString &fileName)
  {
    Q_D(MainWindow);

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot read file %1:\n%2.")
          .arg(fileName)
          .arg(file.errorString()));
      return false;
    }
    file.close();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    statusBar()->showMessage(tr("Reading file."), 2000);
    OBConversion conv;
    OBFormat     *inFormat = conv.FormatFromExt((fileName.toAscii()).data());
    if (!inFormat || !conv.SetInFormat(inFormat)) {
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot read file format of file %1.")
          .arg(fileName));
      return false;
    }
    ifstream     ifs;
    ifs.open((fileName.toAscii()).data());
    if (!ifs) { // shouldn't happen, already checked file above
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot read file %1.")
          .arg(fileName));
      return false;
    }

    Molecule *molecule = new Molecule;
    if (conv.Read(molecule, &ifs) && molecule->NumAtoms() != 0)
    {
      ui.projectTree->setMolecule(molecule);
      ui.glWidget->setMolecule(molecule);
      if(d->molecule)
        delete(d->molecule);
      d->molecule = molecule;

      QApplication::restoreOverrideCursor();

      QString status;
      QTextStream(&status) << "Atoms: " << d->molecule->NumAtoms() <<
        " Bonds: " << d->molecule->NumBonds();
      statusBar()->showMessage(status, 5000);

      
    }
    else {
      statusBar()->showMessage("Reading molecular file failed.", 5000);
      QApplication::restoreOverrideCursor();
      return false;
    }

    setCurrentFile(fileName);

    return true;
  }

  bool MainWindow::saveFile(const QString &fileName)
  {
    Q_D(MainWindow);

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot write to the file %1:\n%2.")
          .arg(fileName)
          .arg(file.errorString()));
      return false;
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    statusBar()->showMessage(tr("Saving file."), 2000);

    OBConversion conv;
    OBFormat     *outFormat = conv.FormatFromExt((fileName.toAscii()).data());
    if (!outFormat || !conv.SetOutFormat(outFormat)) {
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot write to file format of file %1.")
          .arg(fileName));
      return false;
    }
    ofstream     ofs;
    ofs.open((fileName.toAscii()).data());
    if (!ofs) { // shouldn't happen, already checked file above
      QMessageBox::warning(this, tr("Avogadro"),
          tr("Cannot write to the file %1.")
          .arg(fileName));
      return false;
    }

    OBMol *molecule = dynamic_cast<OBMol*>(d->molecule);
    if (conv.Write(molecule, &ofs))
      statusBar()->showMessage("Save succeeded.", 5000);
    else
      statusBar()->showMessage("Saving molecular file failed.", 5000);
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
  }

  void MainWindow::setCurrentFile(const QString &fileName)
  {
    Q_D(MainWindow);

    d->currentFile = fileName;
    if (d->currentFile.isEmpty()) {
      setWindowTitle(tr("Avogadro"));
      return;
    }
    else
      setWindowTitle(tr("%1 - %2").arg(strippedName(d->currentFile))
          .arg(tr("Avogadro")));

    QSettings settings; // already set up properly via main.cpp
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > maxRecentFiles)
      files.removeLast();

    settings.setValue("recentFileList", files);

    foreach (QWidget *widget, QApplication::topLevelWidgets()) {
      MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
      if (mainWin)
        mainWin->updateRecentFileActions();
    }
  }

  void MainWindow::updateRecentFileActions()
  {
    Q_D(MainWindow);

    QSettings settings; // set up project and program properly in main.cpp
    QStringList files = settings.value("recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)maxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
      d->actionRecentFile[i]->setText(strippedName(files[i]));
      d->actionRecentFile[i]->setData(files[i]);
      d->actionRecentFile[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < maxRecentFiles; ++j)
      d->actionRecentFile[j]->setVisible(false);

//     ui.actionSeparator->setVisible(numRecentFiles > 0);
  }

  QString MainWindow::strippedName(const QString &fullFileName)
  {
    return QFileInfo(fullFileName).fileName();
  }

  MainWindow *MainWindow::findMainWindow(const QString &fileName)
  {
    Q_D(MainWindow);

    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QWidget *widget, qApp->topLevelWidgets()) {
      MainWindow *mainWin = qobject_cast<MainWindow *>(widget);
      if (mainWin && mainWin->d_func()->currentFile == canonicalFilePath)
        return mainWin;
    }
    return 0;
  }

  void MainWindow::readSettings()
  {
    QSettings settings;
    //QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(640, 480)).toSize();
    resize(size);
    //move(pos);
  }

  void MainWindow::writeSettings()
  {
    QSettings settings;
    settings.setValue("pos", pos());
    settings.setValue("size", size());
  }
    
  void MainWindow::loadPlugins()
  {
    QStringList pluginPaths;
    pluginPaths << "/usr/lib/avogadro" << "/usr/local/lib/avogadro";

#ifdef WIN32
	pluginPaths << "./plugins";
#endif

    if(getenv("AVOGADRO_PLUGINS") != NULL)
    {
      pluginPaths += QString(getenv("AVOGADRO_PLUGINS")).split(':');
    }

    foreach (QString path, pluginPaths)
    {
      QDir dir(path); 
      qDebug() << "SearchPath:" << dir.absolutePath() << endl;
      foreach (QString fileName, dir.entryList(QDir::Files)) {
        QPluginLoader loader(dir.absoluteFilePath(fileName));
        QObject *instance = loader.instance();
        qDebug() << "File: " << fileName;
        Extension *extension = qobject_cast<Extension *>(instance);
        if(extension) {
          qDebug() << "Found Extension: " << extension->name() << " - " << extension->description();
          QList<QAction *>actions = extension->actions();
          foreach(QAction *action, actions)
          {
            ui.menuTools->addAction(action);
            connect(action, SIGNAL(triggered()), this, SLOT(actionTriggered()));
          }
        }
      }
    }
  }

  void MainWindow::actionTriggered()
  {
    Q_D(MainWindow);

    QAction *action = qobject_cast<QAction *>(sender());
    if(action) {
      Extension *extension = dynamic_cast<Extension *>(action->parent());
      extension->performAction(action, d->molecule, d->messagesText);
    }
  }

} // end namespace Avogadro

#include "mainwindow.moc"
