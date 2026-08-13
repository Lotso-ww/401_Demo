#pragma once

#include "app/applicationcontroller.h"
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QButtonGroup;
class QLabel;
class QPushButton;
class QTimer;
class QFrame;
class QSlider;
class QStackedWidget;

struct ChamberCardView {
    QFrame *frame = nullptr;
    QPushButton *select = nullptr;
    QLabel *state = nullptr;
    QLabel *details = nullptr;
    QVector<QPushButton *> wells;
};

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(ApplicationController *controller, QWidget *parent = nullptr);
    ~MainWindow() override;
private:
    void buildUi();
    QWidget *buildHomePage();
    QWidget *buildDishPage();
    QWidget *buildWellPage();
    void refreshAll();
    void refreshHome();
    void refreshDish();
    void refreshWell();
    void showPage(int index);
    void showMessage(const QString &text, bool error);
    QString stateText(DeviceState state) const;
    QString chamberSummary(const ChamberModel &chamber) const;
    QString chamberStateText(const ChamberModel &chamber, int activeChamber) const;
    const CaptureRound *displayedDishRound() const;
    QVector<WellCapture> playbackForWell(int wellNo) const;
    void setDishRoundIndex(int index);
    void updatePreview(const QImage &image);
    ApplicationController *m_controller;
    Ui::MainWindow *ui;
    QStackedWidget *m_pages = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_wellImage = nullptr;
    QLabel *m_dishInfo = nullptr;
    QLabel *m_wellInfo = nullptr;
    QLabel *m_calibrationPreview = nullptr;
    QVector<QPushButton *> m_chamberButtons;
    QVector<ChamberCardView> m_homeCards;
    QVector<QPushButton *> m_wellButtons;
    QVector<QPushButton *> m_detailWellButtons;
    QButtonGroup *m_wellGroup = nullptr;
    QLabel *m_dishRoundLabel = nullptr;
    QSlider *m_dishRoundSlider = nullptr;
    int m_detailWell = 1;
    int m_historyIndex = 0;
    int m_dishRoundIndex = -1;
    int m_playStep = 1;
    QTimer *m_playTimer = nullptr;
    QTimer *m_dishPlayTimer = nullptr;
    QStackedWidget *m_wellModes = nullptr;
    QPushButton *m_browseMode = nullptr;
    QPushButton *m_calibrationMode = nullptr;
};
