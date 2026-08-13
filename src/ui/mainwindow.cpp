#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QSpinBox>

namespace {
QLabel *label(const QString &text = {})
{
    auto *result = new QLabel(text);
    result->setWordWrap(true);
    return result;
}

QPushButton *button(const QString &text, bool primary = false)
{
    auto *result = new QPushButton(text);
    if (primary)
        result->setObjectName(QString::fromUtf8("primary"));
    return result;
}

QString wellStateText(WellState state)
{
    switch (state) {
    case WellState::Current: return QString::fromUtf8("\xE6\x8B\x8D\xE6\x91\x84\xE4\xB8\xAD");
    case WellState::Complete: return QString::fromUtf8("\xE5\xB7\xB2\xE5\xAE\x8C\xE6\x88\x90");
    case WellState::RetakeRequired: return QString::fromUtf8("\xE9\x9C\x80\xE9\x87\x8D\xE6\x8B\x8D");
    default: return QString::fromUtf8("\xE5\xBE\x85\xE6\x8B\x8D\xE6\x91\x84");
    }
}
}

MainWindow::MainWindow(ApplicationController *controller, QWidget *parent)
    : QMainWindow(parent), m_controller(controller), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    buildUi();
    connect(m_controller, &ApplicationController::message, this, &MainWindow::showMessage);
    connect(m_controller, &ApplicationController::stateChanged, this, &MainWindow::refreshAll);
    connect(m_controller->sessions(), &ChamberSessionService::changed, this, &MainWindow::refreshAll);
    connect(m_controller->workflow(), &CaptureWorkflowService::changed, this, &MainWindow::refreshAll);
    connect(m_controller->camera(), &ICameraService::previewFrame, this, &MainWindow::updatePreview);

    m_playTimer = new QTimer(this);
    m_playTimer->setInterval(500);
    connect(m_playTimer, &QTimer::timeout, this, [this] {
        const int count = playbackForWell(m_detailWell).size();
        if (count) {
            m_historyIndex = (m_historyIndex + m_playStep) % count;
            refreshWell();
        }
    });
    m_dishPlayTimer = new QTimer(this);
    m_dishPlayTimer->setInterval(500);
    connect(m_dishPlayTimer, &QTimer::timeout, this, [this] {
        const auto *model = m_controller->sessions()->selectedModel();
        if (model && !model->rounds.isEmpty())
            setDishRoundIndex((m_dishRoundIndex + 1) % model->rounds.size());
    });
    refreshAll();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::buildUi()
{
    auto *root = new QVBoxLayout(ui->centralWidget);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    auto *bar = new QFrame;
    bar->setObjectName(QString::fromUtf8("topBar"));
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(20, 12, 20, 12);
    m_title = label(QString::fromUtf8("TLS401 \xE8\x83\x9A\xE8\x83\x8E\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9B\x91\xE6\x8E\xA7"));
    m_title->setObjectName(QString::fromUtf8("pageTitle"));
    m_status = label();
    m_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    barLayout->addWidget(m_title);
    barLayout->addStretch();
    barLayout->addWidget(m_status);
    root->addWidget(bar);
    m_pages = new QStackedWidget;
    m_pages->addWidget(buildHomePage());
    m_pages->addWidget(buildDishPage());
    m_pages->addWidget(buildWellPage());
    root->addWidget(m_pages);
}

QWidget *MainWindow::buildHomePage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(22, 18, 22, 18);
    auto *cardGrid = new QGridLayout;
    cardGrid->setSpacing(12);
    for (int chamberNo = 1; chamberNo <= 4; ++chamberNo) {
        ChamberCardView card;
        card.frame = new QFrame;
        card.frame->setObjectName(QString::fromUtf8("chamberCard"));
        auto *cardLayout = new QVBoxLayout(card.frame);
        cardLayout->setContentsMargins(12, 10, 12, 10);
        cardLayout->setSpacing(6);
        auto *header = new QHBoxLayout;
        card.select = button(QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1").arg(chamberNo));
        card.select->setObjectName(QString::fromUtf8("chamberSelect"));
        card.select->setCheckable(true);
        card.state = label();
        card.state->setObjectName(QString::fromUtf8("chamberState"));
        header->addWidget(card.select);
        header->addStretch();
        header->addWidget(card.state);
        cardLayout->addLayout(header);
        card.details = label();
        card.details->setObjectName(QString::fromUtf8("chamberDetails"));
        card.details->setMinimumHeight(38);
        cardLayout->addWidget(card.details);
        auto *wellGrid = new QGridLayout;
        wellGrid->setSpacing(3);
        wellGrid->setContentsMargins(0, 0, 0, 0);
        for (int well = 1; well <= 16; ++well) {
            auto *wellButton = button(QString::number(well));
            wellButton->setObjectName(QString::fromUtf8("homeWell"));
            wellButton->setToolTip(QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1 %2\xE5\x8F\xB7\xE5\xAD\x94").arg(chamberNo).arg(well));
            wellButton->setFixedHeight(24);
            connect(wellButton, &QPushButton::clicked, this, [this, chamberNo] {
                m_controller->sessions()->selectChamber(chamberNo);
                const auto *model = m_controller->sessions()->selectedModel();
                if (model && model->profile)
                    showPage(1);
                else
                    showMessage(QString::fromUtf8("\xE8\xAF\xA5\xE8\x88\xB1\xE5\xAE\xA4\xE5\xB0\x9A\xE6\x9C\xAA\xE8\xAF\x86\xE5\x88\xAB\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBF"), true);
            });
            card.wells.push_back(wellButton);
            wellGrid->addWidget(wellButton, (well - 1) / 8, (well - 1) % 8);
        }
        cardLayout->addLayout(wellGrid);
        connect(card.select, &QPushButton::clicked, this, [this, chamberNo] {
            m_controller->sessions()->selectChamber(chamberNo);
        });
        m_homeCards.push_back(card);
        cardGrid->addWidget(card.frame, (chamberNo - 1) / 2, (chamberNo - 1) % 2);
    }
    layout->addLayout(cardGrid, 1);

    auto *actionsFrame = new QFrame;
    actionsFrame->setObjectName(QString::fromUtf8("homeActions"));
    auto *actions = new QHBoxLayout(actionsFrame);
    auto *identify = button(QString::fromUtf8("\xE5\xBC\x80\xE5\xA7\x8B\xE8\xAF\x86\xE5\x88\xAB"), true);
    auto *captureAll = button(QString::fromUtf8("\xE5\xBC\x80\xE5\xA7\x8B\xE6\x8B\x8D\xE7\x85\xA7"), true);
    auto *clear = button(QString::fromUtf8("\xE6\xB8\x85\xE9\x99\xA4\xE5\xBD\x93\xE5\x89\x8D\xE7\xBB\x91\xE5\xAE\x9A"));
    connect(identify, &QPushButton::clicked, this, [this] { m_controller->identifyAll(RfidScenario::Success); });
    connect(captureAll, &QPushButton::clicked, this, [this] { m_controller->startCaptureSequence(); });
    connect(clear, &QPushButton::clicked, this, [this] { m_controller->sessions()->clearSelected(); });
    actions->addWidget(identify);
    actions->addWidget(captureAll);
    actions->addWidget(clear);
    actions->addStretch();
    layout->addWidget(actionsFrame);
    return page;
}

QWidget *MainWindow::buildDishPage()
{
    auto *page = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(18, 18, 18, 18);
    auto *side = new QFrame;
    side->setObjectName(QString::fromUtf8("sidePanel"));
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->addWidget(label(QString::fromUtf8("\xE8\x88\xB1\xE5\xAE\xA4")));
    for (int i = 1; i <= 4; ++i) {
        auto *item = button(QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1").arg(i));
        item->setCheckable(true);
        m_chamberButtons.push_back(item);
        connect(item, &QPushButton::clicked, this, [this, i] { m_controller->sessions()->selectChamber(i); });
        sideLayout->addWidget(item);
    }
    sideLayout->addStretch();
    layout->addWidget(side, 130);

    auto *main = new QVBoxLayout;
    auto *infoBox = new QGroupBox(QString::fromUtf8("\xE6\x82\xA3\xE8\x80\x85 / \xE8\x83\x9A\xE8\x83\x8E\xE4\xBF\xA1\xE6\x81\xAF"));
    m_dishInfo = label();
    auto *infoLayout = new QVBoxLayout(infoBox);
    infoLayout->addWidget(m_dishInfo);
    main->addWidget(infoBox);
    auto *body = new QHBoxLayout;
    auto *wellBox = new QGroupBox(QString::fromUtf8("16 \xE5\xAD\x94\xE6\x9C\x80\xE6\x96\xB0\xE5\x9B\xBE\xE5\x83\x8F\xE9\xA2\x84\xE8\xA7\x88"));
    auto *wellLayout = new QGridLayout(wellBox);
    wellLayout->setSpacing(5);
    m_wellGroup = new QButtonGroup(this);
    for (int i = 1; i <= 16; ++i) {
        auto *item = button(QString::number(i));
        item->setCheckable(true);
        item->setMinimumSize(82, 58);
        m_wellButtons.push_back(item);
        m_wellGroup->addButton(item, i);
        wellLayout->addWidget(item, (i - 1) / 4, (i - 1) % 4);
        connect(item, &QPushButton::clicked, this, [this, i] { m_detailWell = i; m_historyIndex = 0; showPage(2); });
    }
    body->addWidget(wellBox, 1);
    main->addLayout(body, 1);

    auto *roundControls = new QHBoxLayout;
    auto *previousRound = button(QString::fromUtf8("\xE4\xB8\x8A\xE4\xB8\x80\xE8\xBD\xAE"));
    auto *playRounds = button(QString::fromUtf8("\xE6\x92\xAD\xE6\x94\xBE\xE8\xBD\xAE\xE6\xAC\xA1"), true);
    auto *nextRound = button(QString::fromUtf8("\xE4\xB8\x8B\xE4\xB8\x80\xE8\xBD\xAE"));
    m_dishRoundLabel = label();
    m_dishRoundSlider = new QSlider(Qt::Horizontal);
    connect(previousRound, &QPushButton::clicked, this, [this] { setDishRoundIndex(m_dishRoundIndex - 1); });
    connect(nextRound, &QPushButton::clicked, this, [this] { setDishRoundIndex(m_dishRoundIndex + 1); });
    connect(playRounds, &QPushButton::clicked, this, [this, playRounds] {
        const bool playing = !m_dishPlayTimer->isActive();
        if (playing) m_dishPlayTimer->start(); else m_dishPlayTimer->stop();
        playRounds->setText(playing ? QString::fromUtf8("\xE6\x9A\x82\xE5\x81\x9C\xE8\xBD\xAE\xE6\xAC\xA1") : QString::fromUtf8("\xE6\x92\xAD\xE6\x94\xBE\xE8\xBD\xAE\xE6\xAC\xA1"));
    });
    connect(m_dishRoundSlider, &QSlider::valueChanged, this, &MainWindow::setDishRoundIndex);
    roundControls->addWidget(previousRound);
    roundControls->addWidget(playRounds);
    roundControls->addWidget(nextRound);
    roundControls->addWidget(m_dishRoundLabel);
    roundControls->addWidget(m_dishRoundSlider, 1);
    main->addLayout(roundControls);

    auto *tools = new QHBoxLayout;
    auto *back = button(QString::fromUtf8("\xE8\xBF\x94\xE5\x9B\x9E\xE9\xA6\x96\xE9\xA1\xB5"));
    connect(back, &QPushButton::clicked, this, [this] { showPage(0); });
    tools->addWidget(back);
    tools->addStretch();
    main->addLayout(tools);
    layout->addLayout(main, 1);
    return page;
}

QWidget *MainWindow::buildWellPage()
{
    auto *page = new QWidget;
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(18, 18, 18, 18);
    auto *wellSelector = new QFrame;
    wellSelector->setObjectName(QString::fromUtf8("sidePanel"));
    auto *selectorLayout = new QGridLayout(wellSelector);
    selectorLayout->setSpacing(5);
    for (int well = 1; well <= 16; ++well) {
        auto *item = button(QString::number(well));
        item->setCheckable(true);
        item->setMinimumSize(48, 42);
        connect(item, &QPushButton::clicked, this, [this, well] { m_detailWell = well; m_historyIndex = 0; refreshWell(); });
        m_detailWellButtons.push_back(item);
        selectorLayout->addWidget(item, (well - 1) / 2, (well - 1) % 2);
    }
    layout->addWidget(wellSelector, 130);
    auto *main = new QVBoxLayout;
    auto *modeBar = new QHBoxLayout;
    m_browseMode = button(QString::fromUtf8("\xE8\x83\x9A\xE8\x83\x8E\xE6\xB5\x8F\xE8\xA7\x88"), true);
    m_calibrationMode = button(QString::fromUtf8("\xE4\xBD\x8D\xE7\xBD\xAE\xE6\xA0\xA1\xE5\x87\x86"));
    modeBar->addWidget(m_browseMode);
    modeBar->addWidget(m_calibrationMode);
    modeBar->addStretch();
    main->addLayout(modeBar);
    m_wellModes = new QStackedWidget;
    m_wellInfo = label();
    main->addWidget(m_wellInfo);
    m_wellImage = label(QString::fromUtf8("\xE8\xAF\xA5\xE5\xAD\x94\xE6\x9A\x82\xE6\x97\xA0\xE5\x8E\x86\xE5\x8F\xB2\xE5\x9B\xBE\xE5\x83\x8F"));
    m_wellImage->setMinimumSize(720, 440);
    m_wellImage->setAlignment(Qt::AlignCenter);
    m_wellImage->setStyleSheet(QString::fromUtf8("background: #15242c; color: #d6e5e6;"));
    auto *browsePage = new QWidget;
    auto *browseLayout = new QVBoxLayout(browsePage);
    browseLayout->setContentsMargins(0, 0, 0, 0);
    browseLayout->addWidget(m_wellImage, 1);
    auto *controls = new QHBoxLayout;
    auto *back = button(QString::fromUtf8("\xE8\xBF\x94\xE5\x9B\x9E\xE7\x9A\xBF\xE8\xAF\xA6\xE6\x83\x85"));
    auto *previous = button(QString::fromUtf8("\xE4\xB8\x8A\xE4\xB8\x80\xE8\xBD\xAE"));
    auto *play = button(QString::fromUtf8("\xE6\x92\xAD\xE6\x94\xBE"), true);
    auto *next = button(QString::fromUtf8("\xE4\xB8\x8B\xE4\xB8\x80\xE8\xBD\xAE"));
    auto *speed = new QComboBox;
    speed->addItems({QString::fromUtf8("1x"), QString::fromUtf8("2x"), QString::fromUtf8("3x"), QString::fromUtf8("4x")});
    connect(back, &QPushButton::clicked, this, [this] { m_playTimer->stop(); showPage(1); });
    connect(previous, &QPushButton::clicked, this, [this] { const auto frames = playbackForWell(m_detailWell); if (!frames.isEmpty()) { m_historyIndex = (m_historyIndex - 1 + frames.size()) % frames.size(); refreshWell(); } });
    connect(play, &QPushButton::clicked, this, [this, play] { const bool playing = !m_playTimer->isActive(); if (playing) m_playTimer->start(); else m_playTimer->stop(); play->setText(playing ? QString::fromUtf8("\xE6\x9A\x82\xE5\x81\x9C") : QString::fromUtf8("\xE6\x92\xAD\xE6\x94\xBE")); });
    connect(next, &QPushButton::clicked, this, [this] { const auto frames = playbackForWell(m_detailWell); if (!frames.isEmpty()) { m_historyIndex = (m_historyIndex + 1) % frames.size(); refreshWell(); } });
    connect(speed, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) { m_playStep = index < 2 ? 1 : 2; m_playTimer->setInterval(index == 0 ? 500 : index == 1 ? 250 : index == 2 ? 150 : 100); });
    controls->addWidget(back);
    controls->addWidget(previous);
    controls->addWidget(play);
    controls->addWidget(next);
    controls->addWidget(label(QString::fromUtf8("\xE9\x80\x9F\xE5\xBA\xA6")));
    controls->addWidget(speed);
    controls->addStretch();
    browseLayout->addLayout(controls);
    m_wellModes->addWidget(browsePage);

    auto *calibrationPage = new QWidget;
    auto *calibrationLayout = new QHBoxLayout(calibrationPage);
    m_calibrationPreview = label(QString::fromUtf8("\xE7\xAD\x89\xE5\xBE\x85 CCD \xE5\xAE\x9E\xE6\x97\xB6\xE7\x94\xBB\xE9\x9D\xA2"));
    m_calibrationPreview->setObjectName(QString::fromUtf8("calibrationPreview"));
    m_calibrationPreview->setMinimumSize(720, 440);
    m_calibrationPreview->setAlignment(Qt::AlignCenter);
    m_calibrationPreview->setStyleSheet(QString::fromUtf8("background: #15242c; color: #d6e5e6;"));
    calibrationLayout->addWidget(m_calibrationPreview, 1);
    auto *calibrationControls = new QGridLayout;
    const QStringList axes = {QString::fromUtf8("X\xE8\xBD\xB4"), QString::fromUtf8("Y\xE8\xBD\xB4"), QString::fromUtf8("Z\xE8\xBD\xB4"), QString::fromUtf8("L\xE8\xBD\xB4"), QString::fromUtf8("\xE6\x9B\x9D\xE5\x85\x89\xE6\x97\xB6\xE9\x97\xB4")};
    for (int row = 0; row < axes.size(); ++row) {
        calibrationControls->addWidget(label(axes.at(row)), row, 0);
        auto *minus = button(QString::fromUtf8("<"));
        auto *value = new QSpinBox;
        value->setRange(-1000000, 1000000);
        value->setValue(row == 4 ? static_cast<int>(m_controller->camera()->exposure()) : 0);
        auto *plus = button(QString::fromUtf8(">"));
        calibrationControls->addWidget(minus, row, 1);
        calibrationControls->addWidget(value, row, 2);
        calibrationControls->addWidget(plus, row, 3);
        connect(minus, &QPushButton::clicked, this, [value] { value->stepDown(); });
        connect(plus, &QPushButton::clicked, this, [value] { value->stepUp(); });
    }
    calibrationControls->addWidget(label(QString::fromUtf8("\xE5\xB9\x85\xE5\xBA\xA6\xEF\xBC\x9A")
        + QString::fromUtf8("1 / 2 / 5 / 10 / 20 / 50 / 100 um")), axes.size(), 0, 1, 4);
    calibrationControls->addWidget(label(QString::fromUtf8("\xE5\xBD\x93\xE5\x89\x8D\xE4\xB8\xBA\xE7\x95\x8C\xE9\x9D\xA2\xE9\xA2\x84\xE8\xA7\x88\xEF\xBC\x8CXYZL \xE8\xBF\x90\xE5\x8A\xA8\xE9\x9C\x80\xE6\x8E\xA5\xE5\x85\xA5\xE8\xBF\x90\xE5\x8A\xA8\xE5\xB9\xB3\xE5\x8F\xB0")), axes.size() + 1, 0, 1, 4);
    calibrationLayout->addLayout(calibrationControls);
    m_wellModes->addWidget(calibrationPage);
    main->addWidget(m_wellModes, 1);
    connect(m_browseMode, &QPushButton::clicked, this, [this] { m_wellModes->setCurrentIndex(0); m_playTimer->stop(); m_controller->camera()->stopPreview(); });
    connect(m_calibrationMode, &QPushButton::clicked, this, [this] { m_wellModes->setCurrentIndex(1); m_controller->camera()->startPreview(); });
    layout->addLayout(main, 1);
    return page;
}

void MainWindow::refreshAll()
{
    refreshHome();
    refreshDish();
    refreshWell();
}

QString MainWindow::stateText(DeviceState state) const
{
    switch (state) {
    case DeviceState::Ready: return QString::fromUtf8("\xE5\xB0\xB1\xE7\xBB\xAA");
    case DeviceState::Busy: return QString::fromUtf8("\xE5\xB7\xA5\xE4\xBD\x9C\xE4\xB8\xAD");
    case DeviceState::Error: return QString::fromUtf8("\xE5\xBC\x82\xE5\xB8\xB8");
    default: return QString::fromUtf8("\xE7\xA6\xBB\xE7\xBA\xBF");
    }
}

QString MainWindow::chamberSummary(const ChamberModel &chamber) const
{
    if (!chamber.profile)
        return QString::fromUtf8("\xE6\x9C\xAA\xE7\xBB\x91\xE5\xAE\x9A\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBF");
    const auto &profile = *chamber.profile;
    return QString::fromUtf8("\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBF\xEF\xBC\x9A%1  |  \xE5\xA5\xB3\xE6\x96\xB9\xEF\xBC\x9A%2  |  \xE7\x97\x85\xE5\x8E\x86\xE5\x8F\xB7\xEF\xBC\x9A%3\n\xE5\x8F\x91\xE8\x82\xB2\xEF\xBC\x9A%4 \xE5\xA4\xA9  |  \xE5\xB7\xB2\xE9\x87\x87\xE9\x9B\x86 %5 \xE8\xBD\xAE")
        .arg(profile.dishNumber, profile.femaleName, profile.medicalRecordNumber)
        .arg(developmentDays(profile))
        .arg(chamber.rounds.size());
}

QString MainWindow::chamberStateText(const ChamberModel &chamber, int activeChamber) const
{
    if (!chamber.profile)
        return QString::fromUtf8("\xE7\xA9\xBA\xE8\x88\xB1");
    if (chamber.number == activeChamber && m_controller->workflow()->hasActiveRound())
        return QString::fromUtf8("\xE6\x8B\x8D\xE7\x85\xA7\xE4\xB8\xAD");
    return QString::fromUtf8("\xE7\xAD\x89\xE5\xBE\x85\xE6\x8B\x8D\xE7\x85\xA7");
}

const CaptureRound *MainWindow::displayedDishRound() const
{
    const auto *model = m_controller->sessions()->selectedModel();
    if (!model || model->rounds.isEmpty())
        return nullptr;
    const int index = qBound(0, m_dishRoundIndex < 0 ? model->rounds.size() - 1 : m_dishRoundIndex, model->rounds.size() - 1);
    return &model->rounds[index];
}

QVector<WellCapture> MainWindow::playbackForWell(int wellNo) const
{
    QVector<WellCapture> result;
    const auto *model = m_controller->sessions()->selectedModel();
    if (!model || wellNo < 1 || wellNo > 16)
        return result;
    for (const auto &round : model->rounds) {
        const auto &captures = round.history[wellNo - 1];
        for (auto it = captures.crbegin(); it != captures.crend(); ++it) {
            if (it->active && it->available) {
                result.push_back(*it);
                break;
            }
        }
    }
    return result;
}

void MainWindow::setDishRoundIndex(int index)
{
    const auto *model = m_controller->sessions()->selectedModel();
    if (!model || model->rounds.isEmpty()) {
        m_dishRoundIndex = -1;
        return;
    }
    m_dishRoundIndex = qBound(0, index, model->rounds.size() - 1);
    if (m_dishRoundSlider && m_dishRoundSlider->value() != m_dishRoundIndex) {
        const QSignalBlocker blocker(m_dishRoundSlider);
        m_dishRoundSlider->setValue(m_dishRoundIndex);
    }
    refreshDish();
}

void MainWindow::refreshHome()
{
    const auto &chambers = m_controller->sessions()->chambers();
    const int activeChamber = m_controller->sessions()->selectedChamber();
    const bool activeRound = m_controller->workflow()->hasActiveRound();
    const int activeWell = m_controller->workflow()->currentWell();
    for (int i = 0; i < m_homeCards.size(); ++i) {
        auto &card = m_homeCards[i];
        const auto &chamber = chambers[i];
        card.select->setChecked(chamber.number == activeChamber);
        card.state->setText(chamberStateText(chamber, activeChamber));
        card.details->setText(chamberSummary(chamber));
        const CaptureRound *round = chamber.rounds.isEmpty() ? nullptr : &chamber.rounds.last();
        for (int well = 0; well < card.wells.size(); ++well) {
            auto *item = card.wells[well];
            item->setProperty("captureState", QString::fromUtf8("empty"));
            if (round && !round->history[well].isEmpty() && round->history[well].last().available)
                item->setProperty("captureState", QString::fromUtf8("complete"));
            if (activeRound && chamber.number == activeChamber && well + 1 == activeWell)
                item->setProperty("captureState", QString::fromUtf8("current"));
            item->style()->unpolish(item);
            item->style()->polish(item);
        }
    }
    QString status = QString::fromUtf8("RFID: %1   CCD: %2   \xE6\x95\xB0\xE6\x8D\xAE\xE5\xBA\x93: %3   |   \xE6\x8B\x8D\xE6\x91\x84\xE9\xA1\xBA\xE5\xBA\x8F: 4 \xE2\x86\x92 3 \xE2\x86\x92 2 \xE2\x86\x92 1")
        .arg(stateText(m_controller->rfidState()), stateText(m_controller->cameraState()), m_controller->databaseReady() ? QString::fromUtf8("\xE5\xB0\xB1\xE7\xBB\xAA") : QString::fromUtf8("\xE5\xBC\x82\xE5\xB8\xB8"));
    if (m_controller->sequenceActive() && !m_controller->sequenceStatus().isEmpty())
        status += QStringLiteral("  |  ") + m_controller->sequenceStatus();
    m_status->setText(status);
}

void MainWindow::refreshDish()
{
    const auto *model = m_controller->sessions()->selectedModel();
    if (m_dishRoundSlider) {
        const int count = model ? model->rounds.size() : 0;
        m_dishRoundSlider->setEnabled(count > 0);
        m_dishRoundSlider->setRange(0, qMax(0, count - 1));
        if (count && m_dishRoundIndex < 0) m_dishRoundIndex = count - 1;
    }
    m_dishInfo->setText(model && model->profile
        ? QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1  |  \xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBF\xE5\xBA\x8F\xE5\x8F\xB7\xEF\xBC\x9A%2  |  \xE5\xA5\xB3\xE6\x96\xB9\xEF\xBC\x9A%3  |  \xE7\x94\xB7\xE6\x96\xB9\xEF\xBC\x9A%4  |  \xE7\x97\x85\xE5\x8E\x86\xE5\x8F\xB7\xEF\xBC\x9A%5\n\xE6\x8E\x88\xE7\xB2\xBE\xE6\x97\xB6\xE9\x97\xB4\xEF\xBC\x9A%6  |  \xE5\x8F\x91\xE8\x82\xB2\xE5\xA4\xA9\xE6\x95\xB0\xEF\xBC\x9A%7  |  \xE6\xA0\x87\xE7\xAD\xBE UID\xEF\xBC\x9A%8")
            .arg(model->number).arg(model->profile->dishNumber, model->profile->femaleName, model->profile->maleName, model->profile->medicalRecordNumber, model->profile->inseminationTime.toString(QString::fromUtf8("yyyy-MM-dd HH:mm"))).arg(developmentDays(*model->profile)).arg(model->profile->uid)
        : QString::fromUtf8("\xE8\xAF\xB7\xE9\x80\x89\xE6\x8B\xA9\xE5\xB9\xB6\xE8\xAF\x86\xE5\x88\xAB\xE4\xB8\x80\xE4\xB8\xAA\xE8\x88\xB1\xE5\xAE\xA4\xE3\x80\x82"));
    for (int i = 0; i < m_chamberButtons.size(); ++i) {
        const bool bound = m_controller->sessions()->chambers()[i].profile.has_value();
        m_chamberButtons[i]->setEnabled(bound);
        m_chamberButtons[i]->setChecked(i + 1 == m_controller->sessions()->selectedChamber());
    }
    const auto states = m_controller->workflow()->wellStates();
    const auto *round = displayedDishRound();
    for (int i = 0; i < m_wellButtons.size(); ++i) {
        auto *item = m_wellButtons[i];
        item->setText(QString::number(i + 1) + QStringLiteral("\n") + wellStateText(states[i]));
        item->setChecked(i + 1 == m_controller->workflow()->currentWell());
        item->setIcon({});
        if (round && !round->history[i].isEmpty() && round->history[i].last().available) {
            item->setIcon(QIcon(QPixmap::fromImage(round->history[i].last().image).scaled(70, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            item->setIconSize(QSize(70, 40));
        }
    }
    if (m_dishRoundLabel) {
        const int count = model ? model->rounds.size() : 0;
        m_dishRoundLabel->setText(count ? QString::fromUtf8("\xE5\x9B\x9E\xE6\x94\xBE\xEF\xBC\x9A\xE7\xAC\xAC %1 / %2 \xE8\xBD\xAE").arg(m_dishRoundIndex + 1).arg(count) : QString::fromUtf8("\xE6\x9A\x82\xE6\x97\xA0\xE8\xBD\xAE\xE6\xAC\xA1"));
    }
}

void MainWindow::refreshWell()
{
    for (int i = 0; i < m_detailWellButtons.size(); ++i)
        m_detailWellButtons[i]->setChecked(i + 1 == m_detailWell);
    const auto history = playbackForWell(m_detailWell);
    if (history.isEmpty()) {
        m_wellInfo->setText(QString::fromUtf8("%1\xE5\x8F\xB7\xE5\xAD\x94 | \xE6\x9A\x82\xE6\x97\xA0\xE8\xBD\xAE\xE6\xAC\xA1\xE5\x9B\xBE\xE5\x83\x8F").arg(m_detailWell));
        m_wellImage->setText(QString::fromUtf8("\xE8\xAF\xA5\xE5\xAD\x94\xE6\x9A\x82\xE6\x97\xA0\xE5\x8E\x86\xE5\x8F\xB2\xE5\x9B\xBE\xE5\x83\x8F"));
        m_wellImage->setPixmap({});
        return;
    }
    m_historyIndex = qBound(0, m_historyIndex, history.size() - 1);
    const auto &capture = history[m_historyIndex];
    m_wellInfo->setText(QString::fromUtf8("%1\xE5\x8F\xB7\xE5\xAD\x94 | \xE7\xAC\xAC %2 / %3 \xE8\xBD\xAE | %4 | \xE5\x8D\x95\xE5\xB1\x82\xE5\x9B\xBE\xE5\x83\x8F")
        .arg(m_detailWell).arg(m_historyIndex + 1).arg(history.size()).arg(capture.capturedAt.toString(QString::fromUtf8("yyyy-MM-dd HH:mm:ss"))));
    m_wellImage->setPixmap(QPixmap::fromImage(capture.image).scaled(m_wellImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::showPage(int index)
{
    if (index != 1) m_dishPlayTimer->stop();
    if (index != 2) {
        m_controller->camera()->stopPreview();
    } else if (m_wellModes) {
        m_wellModes->setCurrentIndex(0);
        m_playTimer->stop();
        m_controller->camera()->stopPreview();
    }
    m_pages->setCurrentIndex(index);
    m_title->setText(index == 0 ? QString::fromUtf8("TLS401 \xE8\x83\x9A\xE8\x83\x8E\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9B\x91\xE6\x8E\xA7") : index == 1 ? QString::fromUtf8("\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBF\xE8\xAF\xA6\xE6\x83\x85") : QString::fromUtf8("\xE5\x9F\xB9\xE5\x85\xBB\xE5\xAD\x94\xE8\xAF\xA6\xE6\x83\x85"));
    refreshAll();
}

void MainWindow::showMessage(const QString &text, bool error)
{
    QMessageBox box(error ? QMessageBox::Warning : QMessageBox::Information, error ? QString::fromUtf8("\xE6\x93\x8D\xE4\xBD\x9C\xE6\x8F\x90\xE7\xA4\xBA") : QString::fromUtf8("\xE6\x93\x8D\xE4\xBD\x9C\xE5\xAE\x8C\xE6\x88\x90"), text, QMessageBox::Ok, this);
    box.exec();
}

void MainWindow::updatePreview(const QImage &image)
{
    if (m_calibrationPreview)
        m_calibrationPreview->setPixmap(QPixmap::fromImage(image).scaled(m_calibrationPreview->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
}
