#include "workflowservices.h"
#include "storage/repository.h"
#include "storage/imagefilestore.h"

ChamberSessionService::ChamberSessionService(QObject *parent) : QObject(parent) {
    for (int i = 1; i <= 4; ++i) m_chambers.push_back({i, std::nullopt, {}});
}
void ChamberSessionService::restoreProfiles(const QVector<ChamberModel> &profiles) {
    for (int i = 0; i < m_chambers.size() && i < profiles.size(); ++i)
        m_chambers[i] = profiles[i];
    emit changed();
}
void ChamberSessionService::selectChamber(int chamberNo) { if (chamberNo >= 1 && chamberNo <= 4 && m_selected != chamberNo) { m_selected = chamberNo; emit selectionChanged(); emit changed(); } }
ChamberModel *ChamberSessionService::selectedModel() { return m_selected ? &m_chambers[m_selected - 1] : nullptr; }
bool ChamberSessionService::bindProfile(const TagProfile &profile, QString *error) {
    for (const auto &chamber : m_chambers) if (chamber.number != m_selected && chamber.profile && chamber.profile->uid == profile.uid) { if (error) *error = QStringLiteral("UID is already bound to chamber %1.").arg(chamber.number); return false; }
    if (auto *model = selectedModel()) {
        QVector<CaptureRound> rounds;
        if (m_repository && !m_repository->loadRoundsForUid(profile.uid, &rounds, error)) return false;
        if (m_repository && !m_repository->assignChamber(model->number, profile, profile.identifiedAt, error)) return false;
        model->profile = profile;
        model->rounds = std::move(rounds);
        emit changed();
        return true;
    }
    if (error) *error = QStringLiteral("Please select a chamber first."); return false;
}
void ChamberSessionService::clearSelected() { if (auto *model = selectedModel()) { if (m_repository) { QString error; if (!m_repository->clearChamber(model->number, &error)) return; } model->profile.reset(); model->rounds.clear(); emit changed(); } }
bool ChamberSessionService::clearAll(QString *error) { for (auto &model : m_chambers) { if (m_repository && !m_repository->clearChamber(model.number, error)) return false; model.profile.reset(); model.rounds.clear(); } m_selected = 0; emit selectionChanged(); emit changed(); return true; }

CaptureWorkflowService::CaptureWorkflowService(ChamberSessionService *sessions, QObject *parent) : QObject(parent), m_sessions(sessions) { connect(sessions, &ChamberSessionService::changed, this, [this] { selectFirstPendingWell(); emit changed(); }); connect(sessions, &ChamberSessionService::selectionChanged, this, [this] { selectFirstPendingWell(); }); }
CaptureRound *CaptureWorkflowService::activeRound() { auto *model = m_sessions->selectedModel(); if (!model || model->rounds.isEmpty() || model->rounds.last().finished) return nullptr; return &model->rounds.last(); }
bool CaptureWorkflowService::hasActiveRound() const { return const_cast<CaptureWorkflowService *>(this)->activeRound() != nullptr; }
bool CaptureWorkflowService::createRound(QString *error) { auto *model = m_sessions->selectedModel(); if (!model || !model->profile) { if (error) *error = QStringLiteral("A recognized tag is required before creating a round."); return false; } if (activeRound()) { if (error) *error = QStringLiteral("A capture round is already active."); return false; } int roundNo = static_cast<int>(model->rounds.size()) + 1; if (m_repository && !m_repository->nextRoundNumber(model->profile->uid, &roundNo, error)) return false; m_activeRoundId = 0; model->rounds.push_back(CaptureRound(roundNo, false)); m_currentWell = 1; emit changed(); return true; }
bool CaptureWorkflowService::finishRound() { auto *round = activeRound(); auto *model = m_sessions->selectedModel(); if (!round || !model || !model->profile) return false; for (const auto &well : round->history) if (well.isEmpty() || !well.last().available) return false; QString error; if (!persistCompletedRound(round, *model->profile, model->number, &error)) return false; round->finished = true; emit changed(); return true; }
void CaptureWorkflowService::discardActiveRound()
{
    auto *model = m_sessions->selectedModel();
    auto *round = activeRound();
    if (!model || !round) return;
    if (m_imageStore) {
        for (const auto &well : round->history)
            for (const auto &capture : well)
                m_imageStore->remove(capture.stagedImagePath);
    }
    model->rounds.removeLast();
    m_activeRoundId = 0;
    m_currentWell = 1;
    emit changed();
}
bool CaptureWorkflowService::selectWell(int wellNo) { if (!activeRound() || wellNo < 1 || wellNo > 16) return false; m_currentWell = wellNo; emit changed(); return true; }
bool CaptureWorkflowService::prepareCapture(const QImage &image, bool retake, double exposureUs, double gainDb,
                                            PendingCapture *pending, QString *error)
{
    auto *round = activeRound(); auto *model = m_sessions->selectedModel(); if (!round || !model || !model->profile || image.isNull() || round->history.size() != 16) { if (error) *error = QStringLiteral("No active round or image is available."); return false; }
    auto &well = round->history[m_currentWell - 1]; if (!retake && !well.isEmpty()) { if (error) *error = QStringLiteral("This well already has an image; use retake."); return false; } if (retake && well.isEmpty()) { if (error) *error = QStringLiteral("This well has no image to retake."); return false; }
    if (!pending) { if (error) *error = QStringLiteral("No capture result container is available."); return false; }
    pending->profile = *model->profile;
    pending->chamberNo = model->number;
    pending->roundNo = round->number;
    pending->wellNo = m_currentWell;
    pending->roundId = round->persistentId;
    pending->retake = retake;
    pending->capturedAt = QDateTime::currentDateTime();
    pending->exposureUs = exposureUs;
    pending->gainDb = gainDb;
    pending->image = image;
    return true;
}

bool CaptureWorkflowService::commitCapture(const PendingCapture &pending, QString *error)
{
    auto *round = activeRound();
    auto *model = m_sessions->selectedModel();
    if (!round || !model || !model->profile || model->number != pending.chamberNo
        || model->profile->uid != pending.profile.uid || round->number != pending.roundNo
        || round->persistentId != pending.roundId || m_currentWell != pending.wellNo) {
        if (error) *error = QStringLiteral("Capture context changed before the image was saved.");
        return false;
    }
    auto &well = round->history[pending.wellNo - 1];
    if ((!pending.retake && !well.isEmpty()) || (pending.retake && well.isEmpty())) {
        if (error) *error = QStringLiteral("The selected well changed before the image was saved.");
        return false;
    }
    WellCapture capture;
    capture.capturedAt = pending.capturedAt;
    capture.sourceSize = pending.image.size();
    capture.active = true;
    capture.available = true;
    capture.exposureUs = pending.exposureUs;
    capture.gainDb = pending.gainDb;
    if (m_imageStore) {
        if (!m_imageStore->stageJpeg(pending.image, pending.profile.uid, pending.roundNo, pending.wellNo,
                                     pending.capturedAt, &capture.stagedImagePath, error))
            return false;
        capture.image = pending.image.scaled(QSize(800, 600), Qt::KeepAspectRatio, Qt::FastTransformation);
    } else {
        capture.image = pending.image;
    }
    for (auto &previous : well) {
        previous.active = false;
        if (m_imageStore) m_imageStore->remove(previous.stagedImagePath);
    }
    well.push_back(std::move(capture));
    bool completed = true;
    for (const auto &item : round->history) if (item.isEmpty() || !item.last().available) completed = false;
    if (completed) {
        if (!persistCompletedRound(round, *model->profile, model->number, error)) return false;
        round->finished = true;
        emit roundCompleted();
    } else {
        advanceToNext();
    }
    emit changed();
    return true;
}

bool CaptureWorkflowService::acceptCapture(const QImage &image, bool retake, double exposureUs, double gainDb, QString *error) {
    PendingCapture pending;
    if (!prepareCapture(image, retake, exposureUs, gainDb, &pending, error)) return false;
    return commitCapture(pending, error);
}

bool CaptureWorkflowService::acceptCapture(const QImage &image, bool retake, QString *error)
{
    return acceptCapture(image, retake, 0, 0, error);
}

bool CaptureWorkflowService::persistCompletedRound(CaptureRound *round, const TagProfile &profile, int chamberNo, QString *error)
{
    if (!m_repository || !m_imageStore)
        return true;
    qint64 roundId = 0;
    if (!m_repository->createRound(profile, chamberNo, round->number, &roundId, error))
        return false;
    QVector<QString> paths;
    for (int wellNo = 1; wellNo <= 16; ++wellNo) {
        WellCapture &capture = round->history[wellNo - 1].last();
        QString path;
        const QSize imageSize = capture.sourceSize.isValid() ? capture.sourceSize : capture.image.size();
        const bool stored = capture.stagedImagePath.isEmpty()
            ? m_imageStore->savePng(capture.image, profile.uid, round->number, wellNo, capture.capturedAt, &path, error)
            : m_imageStore->finalizeStagedJpeg(capture.stagedImagePath, profile.uid, round->number, wellNo,
                                               capture.capturedAt, &path, error);
        if (!stored || !m_repository->insertImage(roundId, profile, wellNo, path, imageSize, capture.capturedAt,
                                          capture.exposureUs, capture.gainDb, 0, nullptr, error)) {
            if (!path.isEmpty()) paths.push_back(path);
            for (const auto &savedPath : paths) m_imageStore->remove(savedPath);
            m_repository->deleteRound(roundId, nullptr);
            return false;
        }
        paths.push_back(path);
    }
    if (!m_repository->finishRound(roundId, QStringLiteral("completed"), error)) {
        for (const auto &path : paths) m_imageStore->remove(path);
        m_repository->deleteRound(roundId, nullptr);
        return false;
    }
    round->persistentId = roundId;
    m_activeRoundId = roundId;
    for (auto &well : round->history)
        for (auto &capture : well) {
            m_imageStore->remove(capture.stagedImagePath);
            capture.stagedImagePath.clear();
        }
    return true;
}
void CaptureWorkflowService::advanceToNext() { auto *round = activeRound(); if (!round) return; for (int offset = 1; offset <= 16; ++offset) { const int candidate = ((m_currentWell - 1 + offset) % 16); if (round->history[candidate].isEmpty()) { m_currentWell = candidate + 1; return; } } }
void CaptureWorkflowService::selectFirstPendingWell() { auto *round = activeRound(); if (!round) { m_currentWell = 1; return; } for (int i = 0; i < round->history.size(); ++i) if (round->history[i].isEmpty() || !round->history[i].last().available) { m_currentWell = i + 1; return; } m_currentWell = 1; }
QVector<WellState> CaptureWorkflowService::wellStates() const { QVector<WellState> states(16, WellState::Empty); auto *round = const_cast<CaptureWorkflowService *>(this)->activeRound(); if (!round) return states; for (int i = 0; i < 16; ++i) if (!round->history[i].isEmpty()) states[i] = round->history[i].last().available ? WellState::Complete : WellState::RetakeRequired; if (m_currentWell >= 1 && m_currentWell <= 16 && states[m_currentWell - 1] == WellState::Empty) states[m_currentWell - 1] = WellState::Current; return states; }
QVector<WellCapture> CaptureWorkflowService::historyForWell(int wellNo) const { QVector<WellCapture> result; const auto *model = m_sessions->selectedModel(); if (!model || wellNo < 1 || wellNo > 16) return result; for (const auto &round : model->rounds) for (const auto &item : round.history[wellNo - 1]) result.push_back(item); return result; }
