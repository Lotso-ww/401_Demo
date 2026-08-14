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
#include <QPushButton>
#include <QPainter>
#include <QSlider>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyleOptionButton>
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

class WellThumbnailButton final : public QPushButton
{
public:
    explicit WellThumbnailButton(int wellNo, QWidget *parent = nullptr) : QPushButton(parent), m_wellNo(wellNo) {}

    void setThumbnail(const QImage &image)
    {
        if (image.isNull()) {
            m_thumbnail = {};
        } else {
            const int side = qMin(image.width(), image.height());
            const QRect crop((image.width() - side) / 2, (image.height() - side) / 2, side, side);
            m_thumbnail = QPixmap::fromImage(image.copy(crop));
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QStyleOptionButton option;
        initStyleOption(&option);
        option.text.clear();
        option.icon = {};
        QPainter painter(this);
        style()->drawControl(QStyle::CE_PushButton, &option, &painter, this);

        const QRect numberRect = rect().adjusted(3, 2, -3, 0);
        painter.setPen(QColor(QStringLiteral("#d7e8f5")));
        painter.setFont(QFont(font().family(), 10, QFont::DemiBold));
        painter.drawText(numberRect, Qt::AlignHCenter | Qt::AlignTop, QString::number(m_wellNo));
        if (m_thumbnail.isNull())
            return;
        const QRect imageArea = rect().adjusted(6, 20, -6, -6);
        const int side = qMin(imageArea.width(), imageArea.height());
        const QRect target(imageArea.center().x() - side / 2, imageArea.center().y() - side / 2, side, side);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(target, m_thumbnail);
    }

private:
    int m_wellNo;
    QPixmap m_thumbnail;
};

void setWellThumbnail(QPushButton *button, const QImage &image)
{
    if (auto *thumbnail = dynamic_cast<WellThumbnailButton *>(button))
        thumbnail->setThumbnail(image);
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
    m_title = ui->pageTitle;
    m_status = ui->statusLabel;
    m_pages = ui->pages;
    m_pages->addWidget(buildHomePage());
    m_pages->addWidget(buildDishPage());
    m_pages->addWidget(buildWellPage());
}

QWidget *MainWindow::buildHomePage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("homeDashboard"));
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    auto *cardGrid = new QGridLayout;
    cardGrid->setSpacing(14);
    for (int chamberNo = 1; chamberNo <= 4; ++chamberNo) {
        ChamberCardView card;
        card.frame = new QFrame;
        card.frame->setObjectName(QStringLiteral("dashboardChamberCard"));
        card.frame->setMinimumSize(460, 300);
        auto *cardLayout = new QVBoxLayout(card.frame);
        cardLayout->setContentsMargins(12, 10, 12, 12);
        cardLayout->setSpacing(8);
        auto *header = new QHBoxLayout;
        card.select = button(QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1").arg(chamberNo));
        card.select->setObjectName(QStringLiteral("dashboardChamberSelect"));
        card.select->setCheckable(true);
        card.state = label();
        card.state->setObjectName(QStringLiteral("dashboardChamberState"));
        header->addWidget(card.select);
        header->addStretch();
        header->addWidget(card.state);
        cardLayout->addLayout(header);
        card.details = label();
        card.details->setObjectName(QStringLiteral("dashboardChamberDetails"));
        card.details->setMinimumHeight(54);
        card.details->setAlignment(Qt::AlignCenter);
        cardLayout->addWidget(card.details);
        auto *wellGrid = new QGridLayout;
        wellGrid->setSpacing(4);
        wellGrid->setContentsMargins(0, 0, 0, 0);
        for (int well = 1; well <= 16; ++well) {
            auto *wellButton = new WellThumbnailButton(well);
            wellButton->setObjectName(QStringLiteral("homeWellThumbnail"));
            wellButton->setToolTip(QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1 %2\xE5\x8F\xB7\xE5\xAD\x94").arg(chamberNo).arg(well));
            wellButton->setMinimumSize(46, 48);
            wellButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
        for (int row = 0; row < 2; ++row)
            wellGrid->setRowStretch(row, 1);
        for (int column = 0; column < 8; ++column)
            wellGrid->setColumnStretch(column, 1);
        cardLayout->addLayout(wellGrid, 1);
        connect(card.select, &QPushButton::clicked, this, [this, chamberNo] {
            m_controller->sessions()->selectChamber(chamberNo);
        });
        m_homeCards.push_back(card);
        cardGrid->addWidget(card.frame, (chamberNo - 1) / 2, (chamberNo - 1) % 2);
    }
    cardGrid->setRowStretch(0, 1);
    cardGrid->setRowStretch(1, 1);
    cardGrid->setColumnStretch(0, 1);
    cardGrid->setColumnStretch(1, 1);
    layout->addLayout(cardGrid, 1);

    auto *actionsFrame = new QFrame;
    actionsFrame->setObjectName(QString::fromUtf8("homeActions"));
    auto *actions = new QHBoxLayout(actionsFrame);
    auto *identify = button(QString::fromUtf8("\xE5\xBC\x80\xE5\xA7\x8B\xE8\xAF\x86\xE5\x88\xAB"), true);
    auto *captureAll = button(QString::fromUtf8("\xE5\xBC\x80\xE5\xA7\x8B\xE6\x8B\x8D\xE7\x85\xA7"), true);
    connect(identify, &QPushButton::clicked, this, [this] { m_controller->identifyAll(); });
    connect(captureAll, &QPushButton::clicked, this, [this] { m_controller->startCaptureSequence(); });
    actions->addWidget(identify);
    actions->addWidget(captureAll);
    actions->addStretch();
    layout->addWidget(actionsFrame);
    return page;
}

QWidget *MainWindow::buildDishPage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("clinicalPage"));
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    auto *side = new QFrame;
    side->setObjectName(QStringLiteral("dishChamberPanel"));
    side->setFixedWidth(140);
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(6, 6, 6, 6);
    sideLayout->setSpacing(7);
    for (int i = 1; i <= 4; ++i) {
        auto *item = button(QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1").arg(i));
        item->setObjectName(QStringLiteral("chamberNavButton"));
        item->setCheckable(true);
        item->setMinimumHeight(118);
        m_chamberButtons.push_back(item);
        connect(item, &QPushButton::clicked, this, [this, i] { m_controller->sessions()->selectChamber(i); });
        sideLayout->addWidget(item);
    }
    layout->addWidget(side, 130);

    auto *infoBox = new QFrame;
    infoBox->setObjectName(QStringLiteral("patientPanel"));
    infoBox->setFixedWidth(225);
    m_dishInfo = label();
    auto *infoLayout = new QVBoxLayout(infoBox);
    infoLayout->setContentsMargins(16, 14, 14, 14);
    infoLayout->addWidget(m_dishInfo);
    infoLayout->addStretch();
    auto *back = button(QStringLiteral("<"));
    back->setObjectName(QStringLiteral("returnButton"));
    back->setToolTip(QString::fromUtf8("\xE8\xBF\x94\xE5\x9B\x9E\xE9\xA6\x96\xE9\xA1\xB5"));
    back->setFixedSize(48, 48);
    connect(back, &QPushButton::clicked, this, [this] { showPage(0); });
    infoLayout->addWidget(back, 0, Qt::AlignRight);
    layout->addWidget(infoBox);

    auto *content = new QVBoxLayout;
    content->setSpacing(8);
    auto *wellArea = new QFrame;
    wellArea->setObjectName(QStringLiteral("thumbnailArea"));
    auto *wellLayout = new QGridLayout(wellArea);
    wellLayout->setContentsMargins(8, 8, 8, 8);
    wellLayout->setSpacing(6);
    m_wellGroup = new QButtonGroup(this);
    for (int i = 1; i <= 16; ++i) {
        auto *item = new WellThumbnailButton(i);
        item->setObjectName(QStringLiteral("dishWellThumbnail"));
        item->setCheckable(true);
        item->setMinimumSize(130, 92);
        item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_wellButtons.push_back(item);
        m_wellGroup->addButton(item, i);
        wellLayout->addWidget(item, (i - 1) / 4, (i - 1) % 4);
        connect(item, &QPushButton::clicked, this, [this, i] { m_detailWell = i; m_historyIndex = 0; showPage(2); });
    }
    for (int row = 0; row < 4; ++row)
        wellLayout->setRowStretch(row, 1);
    for (int column = 0; column < 4; ++column)
        wellLayout->setColumnStretch(column, 1);
    content->addWidget(wellArea, 1);

    auto *roundControls = new QHBoxLayout;
    roundControls->setContentsMargins(8, 0, 8, 0);
    auto *previousRound = button(QStringLiteral("|<"));
    auto *playRounds = button(QStringLiteral(">"));
    auto *nextRound = button(QStringLiteral(">|"));
    for (auto *control : {previousRound, playRounds, nextRound}) {
        control->setObjectName(QStringLiteral("mediaButton"));
        control->setFixedSize(34, 30);
    }
    previousRound->setToolTip(QString::fromUtf8("\xE4\xB8\x8A\xE4\xB8\x80\xE8\xBD\xAE"));
    playRounds->setToolTip(QString::fromUtf8("\xE6\x92\xAD\xE6\x94\xBE / \xE6\x9A\x82\xE5\x81\x9C"));
    nextRound->setToolTip(QString::fromUtf8("\xE4\xB8\x8B\xE4\xB8\x80\xE8\xBD\xAE"));
    auto *speed = new QComboBox;
    speed->setObjectName(QStringLiteral("speedSelector"));
    speed->addItems({QStringLiteral("1X"), QStringLiteral("2X"), QStringLiteral("3X"), QStringLiteral("4X")});
    speed->setCurrentIndex(2);
    m_dishRoundLabel = label();
    m_dishRoundLabel->setObjectName(QStringLiteral("timelineLabel"));
    m_dishRoundSlider = new QSlider(Qt::Horizontal);
    m_dishRoundSlider->setObjectName(QStringLiteral("timelineSlider"));
    connect(previousRound, &QPushButton::clicked, this, [this] { setDishRoundIndex(m_dishRoundIndex - 1); });
    connect(nextRound, &QPushButton::clicked, this, [this] { setDishRoundIndex(m_dishRoundIndex + 1); });
    connect(playRounds, &QPushButton::clicked, this, [this, playRounds] {
        const bool playing = !m_dishPlayTimer->isActive();
        if (playing) m_dishPlayTimer->start(); else m_dishPlayTimer->stop();
        playRounds->setText(playing ? QStringLiteral("||") : QStringLiteral(">"));
    });
    connect(speed, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        m_dishPlayTimer->setInterval(index == 0 ? 500 : index == 1 ? 250 : index == 2 ? 150 : 100);
    });
    connect(m_dishRoundSlider, &QSlider::valueChanged, this, &MainWindow::setDishRoundIndex);
    roundControls->addWidget(previousRound);
    roundControls->addWidget(playRounds);
    roundControls->addWidget(nextRound);
    roundControls->addWidget(speed);
    roundControls->addWidget(m_dishRoundLabel);
    roundControls->addWidget(m_dishRoundSlider, 1);
    content->addLayout(roundControls);
    layout->addLayout(content, 1);
    return page;
}

QWidget *MainWindow::buildWellPage()
{
    auto *page = new QWidget;
    page->setObjectName(QStringLiteral("clinicalPage"));
    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    auto *wellSelector = new QFrame;
    wellSelector->setObjectName(QStringLiteral("wellSelector"));
    wellSelector->setFixedWidth(146);
    auto *selectorLayout = new QGridLayout(wellSelector);
    selectorLayout->setContentsMargins(6, 6, 6, 6);
    selectorLayout->setSpacing(7);
    for (int well = 1; well <= 16; ++well) {
        auto *item = button(QString::number(well));
        item->setObjectName(QStringLiteral("wellNavButton"));
        item->setCheckable(true);
        item->setMinimumSize(56, 56);
        connect(item, &QPushButton::clicked, this, [this, well] { m_detailWell = well; m_historyIndex = 0; refreshWell(); });
        m_detailWellButtons.push_back(item);
        selectorLayout->addWidget(item, (well - 1) / 2, (well - 1) % 2);
    }
    layout->addWidget(wellSelector, 130);
    auto *details = new QFrame;
    details->setObjectName(QStringLiteral("patientPanel"));
    details->setFixedWidth(220);
    auto *detailsLayout = new QVBoxLayout(details);
    detailsLayout->setContentsMargins(14, 14, 14, 14);
    detailsLayout->setSpacing(14);
    auto *modeBar = new QHBoxLayout;
    modeBar->setSpacing(0);
    m_browseMode = button(QString::fromUtf8("\xE8\x83\x9A\xE8\x83\x8E\xE6\xB5\x8F\xE8\xA7\x88"));
    m_calibrationMode = button(QString::fromUtf8("\xE4\xBD\x8D\xE7\xBD\xAE\xE6\xA0\xA1\xE5\x87\x86"));
    m_browseMode->setObjectName(QStringLiteral("modeTab"));
    m_calibrationMode->setObjectName(QStringLiteral("modeTab"));
    m_browseMode->setCheckable(true);
    m_calibrationMode->setCheckable(true);
    m_browseMode->setChecked(true);
    modeBar->addWidget(m_browseMode);
    modeBar->addWidget(m_calibrationMode);
    detailsLayout->addLayout(modeBar);
    m_wellModes = new QStackedWidget;
    m_wellInfo = label();
    m_wellInfo->setObjectName(QStringLiteral("wellInfo"));
    detailsLayout->addWidget(m_wellInfo);
    detailsLayout->addStretch();
    auto *back = button(QStringLiteral("<"));
    back->setObjectName(QStringLiteral("returnButton"));
    back->setToolTip(QString::fromUtf8("\xE8\xBF\x94\xE5\x9B\x9E\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBF\xE8\xAF\xA6\xE6\x83\x85"));
    back->setFixedSize(48, 48);
    connect(back, &QPushButton::clicked, this, [this] { m_playTimer->stop(); showPage(1); });
    detailsLayout->addWidget(back, 0, Qt::AlignRight);
    layout->addWidget(details);

    m_wellImage = label(QString::fromUtf8("\xE8\xAF\xA5\xE5\xAD\x94\xE6\x9A\x82\xE6\x97\xA0\xE5\x8E\x86\xE5\x8F\xB2\xE5\x9B\xBE\xE5\x83\x8F"));
    m_wellImage->setMinimumSize(560, 420);
    m_wellImage->setAlignment(Qt::AlignCenter);
    m_wellImage->setObjectName(QStringLiteral("imagePreview"));
    auto *browsePage = new QWidget;
    auto *browseLayout = new QVBoxLayout(browsePage);
    browseLayout->setContentsMargins(0, 0, 0, 0);
    browseLayout->addWidget(m_wellImage, 1);
    auto *controls = new QHBoxLayout;
    controls->setContentsMargins(4, 0, 4, 0);
    auto *previous = button(QStringLiteral("|<"));
    auto *play = button(QStringLiteral(">"));
    auto *next = button(QStringLiteral(">|"));
    for (auto *control : {previous, play, next}) {
        control->setObjectName(QStringLiteral("mediaButton"));
        control->setFixedSize(34, 30);
    }
    auto *speed = new QComboBox;
    speed->addItems({QString::fromUtf8("1x"), QString::fromUtf8("2x"), QString::fromUtf8("3x"), QString::fromUtf8("4x")});
    connect(previous, &QPushButton::clicked, this, [this] { const auto frames = playbackForWell(m_detailWell); if (!frames.isEmpty()) { m_historyIndex = (m_historyIndex - 1 + frames.size()) % frames.size(); refreshWell(); } });
    connect(play, &QPushButton::clicked, this, [this, play] { const bool playing = !m_playTimer->isActive(); if (playing) m_playTimer->start(); else m_playTimer->stop(); play->setText(playing ? QString::fromUtf8("\xE6\x9A\x82\xE5\x81\x9C") : QString::fromUtf8("\xE6\x92\xAD\xE6\x94\xBE")); });
    connect(next, &QPushButton::clicked, this, [this] { const auto frames = playbackForWell(m_detailWell); if (!frames.isEmpty()) { m_historyIndex = (m_historyIndex + 1) % frames.size(); refreshWell(); } });
    connect(speed, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) { m_playStep = index < 2 ? 1 : 2; m_playTimer->setInterval(index == 0 ? 500 : index == 1 ? 250 : index == 2 ? 150 : 100); });
    controls->addWidget(previous);
    controls->addWidget(play);
    controls->addWidget(next);
    controls->addWidget(speed);
    controls->addStretch();
    browseLayout->addLayout(controls);
    m_wellModes->addWidget(browsePage);

    auto *calibrationPage = new QWidget;
    auto *calibrationLayout = new QHBoxLayout(calibrationPage);
    calibrationLayout->setContentsMargins(0, 0, 0, 0);
    calibrationLayout->setSpacing(12);
    auto *calibrationControls = new QGridLayout;
    calibrationControls->setHorizontalSpacing(8);
    calibrationControls->setVerticalSpacing(10);
    m_calibrationPreview = label(QString::fromUtf8("\xE7\xAD\x89\xE5\xBE\x85 CCD \xE5\xAE\x9E\xE6\x97\xB6\xE7\x94\xBB\xE9\x9D\xA2"));
    m_calibrationPreview->setObjectName(QString::fromUtf8("calibrationPreview"));
    m_calibrationPreview->setMinimumSize(560, 420);
    m_calibrationPreview->setAlignment(Qt::AlignCenter);
    m_calibrationPreview->setStyleSheet(QString::fromUtf8("background: #0c0d0f; color: #d6e5e6;"));
    const QStringList axes = {QString::fromUtf8("X\xE8\xBD\xB4"), QString::fromUtf8("Y\xE8\xBD\xB4"), QString::fromUtf8("Z\xE8\xBD\xB4"), QString::fromUtf8("L\xE8\xBD\xB4"), QString::fromUtf8("\xE6\x9B\x9D\xE5\x85\x89\xE6\x97\xB6\xE9\x97\xB4")};
    for (int row = 0; row < axes.size(); ++row) {
        calibrationControls->addWidget(label(axes.at(row)), row, 0);
        auto *minus = button(QString::fromUtf8("<"));
        minus->setObjectName(QStringLiteral("adjustButton"));
        auto *value = new QSpinBox;
        value->setObjectName(QStringLiteral("axisValue"));
        value->setRange(-1000000, 1000000);
        value->setValue(row == 4 ? static_cast<int>(m_controller->camera()->exposure()) : 0);
        auto *plus = button(QString::fromUtf8(">"));
        plus->setObjectName(QStringLiteral("adjustButton"));
        calibrationControls->addWidget(minus, row, 1);
        calibrationControls->addWidget(value, row, 2);
        calibrationControls->addWidget(plus, row, 3);
        connect(minus, &QPushButton::clicked, this, [value] { value->stepDown(); });
        connect(plus, &QPushButton::clicked, this, [value] { value->stepUp(); });
    }
    auto *save = button(QStringLiteral("S"));
    save->setObjectName(QStringLiteral("saveButton"));
    save->setToolTip(QString::fromUtf8("\xE4\xBF\x9D\xE5\xAD\x98\xE6\xA0\xA1\xE5\x87\x86\xE5\x8F\x82\xE6\x95\xB0"));
    save->setFixedSize(48, 48);
    calibrationControls->addWidget(save, axes.size(), 1, 1, 2);
    auto *presetColumn = new QVBoxLayout;
    presetColumn->setSpacing(5);
    presetColumn->addWidget(label(QString::fromUtf8("\xE5\xB9\x85\xE5\xBA\xA6\n(um)")), 0, Qt::AlignHCenter);
    for (const auto value : {100, 50, 20, 10, 5, 2, 1}) {
        auto *preset = button(QString::number(value));
        preset->setObjectName(QStringLiteral("stepPreset"));
        preset->setCheckable(true);
        preset->setFixedSize(42, 42);
        if (value == 10) preset->setChecked(true);
        presetColumn->addWidget(preset, 0, Qt::AlignHCenter);
    }
    presetColumn->addStretch();
    calibrationLayout->addLayout(calibrationControls);
    calibrationLayout->addLayout(presetColumn);
    calibrationLayout->addWidget(m_calibrationPreview, 1);
    m_wellModes->addWidget(calibrationPage);
    connect(m_browseMode, &QPushButton::clicked, this, [this] {
        m_browseMode->setChecked(true);
        m_calibrationMode->setChecked(false);
        m_wellModes->setCurrentIndex(0);
        m_playTimer->stop();
        m_controller->setCaptureSequencePaused(false);
        m_controller->camera()->startPreview();
    });
    connect(m_calibrationMode, &QPushButton::clicked, this, [this] {
        m_browseMode->setChecked(false);
        m_calibrationMode->setChecked(true);
        m_wellModes->setCurrentIndex(1);
        m_controller->setCaptureSequencePaused(true);
        m_controller->camera()->startPreview();
    });
    layout->addWidget(m_wellModes, 1);
    return page;
}

void MainWindow::refreshAll()
{
    if (m_currentPage == 0)
        refreshHome();
    else if (m_currentPage == 1)
        refreshDish();
    else
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
    if (m_controller->identifying())
        return chamber.number == activeChamber ? QString::fromUtf8("\xE6\xAD\xA3\xE5\x9C\xA8\xE8\xAF\x86\xE5\x88\xAB") : QString::fromUtf8("\xE7\xAD\x89\xE5\xBE\x85\xE8\xAF\x86\xE5\x88\xAB");
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
        card.state->setProperty("activity", m_controller->identifying() && chamber.number == activeChamber ? QStringLiteral("recognizing") : QStringLiteral("idle"));
        card.state->style()->unpolish(card.state);
        card.state->style()->polish(card.state);
        card.details->setText(chamberSummary(chamber));
        const CaptureRound *round = chamber.rounds.isEmpty() ? nullptr : &chamber.rounds.last();
        for (int well = 0; well < card.wells.size(); ++well) {
            auto *item = card.wells[well];
            item->setProperty("captureState", QString::fromUtf8("empty"));
            setWellThumbnail(item, {});
            if (round && !round->history[well].isEmpty() && round->history[well].last().available) {
                item->setProperty("captureState", QString::fromUtf8("complete"));
                const auto &capture = round->history[well].last();
                setWellThumbnail(item, capture.image);
            }
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
        ? QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1%1\xE5\x8F\xB7\xE5\xAD\x94\n\n\xE6\x82\xA3\xE8\x80\x85\xE4\xBF\xA1\xE6\x81\xAF\n\xE5\xA5\xB3\xE6\x96\xB9\xE5\xA7\x93\xE5\x90\x8D\xEF\xBC\x9A\n%2\n\xE7\x94\xB7\xE6\x96\xB9\xE5\xA7\x93\xE5\x90\x8D\xEF\xBC\x9A\n%3\n\xE5\x8F\x91\xE8\x82\xB2\xE5\xA4\xA9\xE6\x95\xB0\xEF\xBC\x9A\n%4\n\xE7\x97\x85\xE5\x8E\x86\xE5\x8F\xB7\xEF\xBC\x9A\n%5\n\n\xE8\x83\x9A\xE8\x83\x8E\xE4\xBF\xA1\xE6\x81\xAF\n\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBFID\xEF\xBC\x9A\n%6\n\xE6\x8E\x88\xE7\xB2\xBE\xE6\x97\xB6\xE9\x97\xB4\xEF\xBC\x9A\n%7\n\xE5\x9F\xB9\xE5\x85\xBB\xE6\x97\xB6\xE9\x97\xB4\xEF\xBC\x9A\n%8h")
            .arg(model->number).arg(model->profile->femaleName, model->profile->maleName).arg(developmentDays(*model->profile)).arg(model->profile->medicalRecordNumber, model->profile->dishNumber, model->profile->inseminationTime.toString(QString::fromUtf8("yyyy-MM-dd  HH:mm:ss"))).arg(model->profile->inseminationTime.secsTo(QDateTime::currentDateTime()) / 3600.0, 0, 'f', 2)
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
        item->setChecked(i + 1 == m_controller->workflow()->currentWell());
        setWellThumbnail(item, {});
        if (round && !round->history[i].isEmpty() && round->history[i].last().available) {
            setWellThumbnail(item, round->history[i].last().image);
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
    const auto *model = m_controller->sessions()->selectedModel();
    const QString details = model && model->profile
        ? QString::fromUtf8("%1\xE5\x8F\xB7\xE8\x88\xB1%2\xE5\x8F\xB7\xE5\xAD\x94\n\n\xE6\x82\xA3\xE8\x80\x85\xE4\xBF\xA1\xE6\x81\xAF\n\xE5\xA5\xB3\xE6\x96\xB9\xE5\xA7\x93\xE5\x90\x8D\xEF\xBC\x9A\n%3\n\xE7\x94\xB7\xE6\x96\xB9\xE5\xA7\x93\xE5\x90\x8D\xEF\xBC\x9A\n%4\n\xE5\x8F\x91\xE8\x82\xB2\xE5\xA4\xA9\xE6\x95\xB0\xEF\xBC\x9A\n%5\n\xE7\x97\x85\xE5\x8E\x86\xE5\x8F\xB7\xEF\xBC\x9A\n%6\n\n\xE8\x83\x9A\xE8\x83\x8E\xE4\xBF\xA1\xE6\x81\xAF\n\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBFID\xEF\xBC\x9A\n%7\n\xE6\x8E\x88\xE7\xB2\xBE\xE6\x97\xB6\xE9\x97\xB4\xEF\xBC\x9A\n%8\n\xE5\x9F\xB9\xE5\x85\xBB\xE6\x97\xB6\xE9\x97\xB4\xEF\xBC\x9A\n%9h")
            .arg(model->number).arg(m_detailWell).arg(model->profile->femaleName, model->profile->maleName).arg(developmentDays(*model->profile)).arg(model->profile->medicalRecordNumber, model->profile->dishNumber, model->profile->inseminationTime.toString(QString::fromUtf8("yyyy-MM-dd  HH:mm:ss"))).arg(model->profile->inseminationTime.secsTo(QDateTime::currentDateTime()) / 3600.0, 0, 'f', 2)
        : QString::fromUtf8("%1\xE5\x8F\xB7\xE5\xAD\x94\n\n\xE6\x9A\x82\xE6\x97\xA0\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBF\xE4\xBF\xA1\xE6\x81\xAF").arg(m_detailWell);
    m_wellInfo->setText(details);
    const auto history = playbackForWell(m_detailWell);
    if (history.isEmpty()) {
        m_wellImage->setText(QString::fromUtf8("\xE8\xAF\xA5\xE5\xAD\x94\xE6\x9A\x82\xE6\x97\xA0\xE5\x8E\x86\xE5\x8F\xB2\xE5\x9B\xBE\xE5\x83\x8F"));
        m_wellImage->setPixmap({});
        return;
    }
    m_historyIndex = qBound(0, m_historyIndex, history.size() - 1);
    const auto &capture = history[m_historyIndex];
    m_wellImage->setPixmap(QPixmap::fromImage(capture.image).scaled(m_wellImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::showPage(int index)
{
    if (index != 1) m_dishPlayTimer->stop();
    if (m_currentPage == 2 && index != 2)
        m_controller->setCaptureSequencePaused(false);
    if (index == 2) {
        m_controller->setCaptureSequencePaused(true);
        m_controller->camera()->startPreview();
        if (m_wellModes) {
            m_wellModes->setCurrentIndex(0);
            m_browseMode->setChecked(true);
            m_calibrationMode->setChecked(false);
        }
    } else if (index == 0) {
        if (m_controller->sequenceActive())
            m_controller->camera()->startPreview();
        else
            m_controller->camera()->stopPreview();
    } else if (m_wellModes && m_currentPage != 2) {
        m_wellModes->setCurrentIndex(0);
        m_playTimer->stop();
    }
    m_pages->setCurrentIndex(index);
    m_currentPage = index;
    m_title->setText(index == 0 ? QString::fromUtf8("TLS401 \xE8\x83\x9A\xE8\x83\x8E\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9B\x91\xE6\x8E\xA7") : index == 1 ? QString::fromUtf8("\xE5\x9F\xB9\xE5\x85\xBB\xE7\x9A\xBF\xE8\xAF\xA6\xE6\x83\x85") : QString::fromUtf8("\xE5\x9F\xB9\xE5\x85\xBB\xE5\xAD\x94\xE8\xAF\xA6\xE6\x83\x85"));
    refreshAll();
}

void MainWindow::showMessage(const QString &text, bool error)
{
    if (!m_status || text.isEmpty())
        return;
    m_status->setText((error ? QString::fromUtf8("\xE6\x8F\x90\xE7\xA4\xBA\xEF\xBC\x9A") : QString::fromUtf8("\xE5\xAE\x8C\xE6\x88\x90\xEF\xBC\x9A")) + text);
}

void MainWindow::updatePreview(const QImage &image)
{
    if (m_currentPage == 2 && m_wellModes && m_wellModes->currentIndex() == 1 && m_calibrationPreview)
        m_calibrationPreview->setPixmap(QPixmap::fromImage(image).scaled(m_calibrationPreview->size(), Qt::KeepAspectRatio, Qt::FastTransformation));
}
