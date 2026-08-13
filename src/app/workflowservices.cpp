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
    if (auto *model = selectedModel()) { if (m_repository && !m_repository->assignChamber(model->number, profile, profile.identifiedAt, error)) return false; model->profile = profile; emit changed(); return true; }
    if (error) *error = QStringLiteral("Please select a chamber first."); return false;
}
void ChamberSessionService::clearSelected() { if (auto *model = selectedModel()) { if (m_repository) { QString error; if (!m_repository->clearChamber(model->number, &error)) return; } model->profile.reset(); emit changed(); } }

CaptureWorkflowService::CaptureWorkflowService(ChamberSessionService *sessions, QObject *parent) : QObject(parent), m_sessions(sessions) { connect(sessions, &ChamberSessionService::changed, this, &CaptureWorkflowService::changed); connect(sessions, &ChamberSessionService::selectionChanged, this, [this] { selectFirstPendingWell(); }); }
CaptureRound *CaptureWorkflowService::activeRound() { auto *model = m_sessions->selectedModel(); if (!model || model->rounds.isEmpty() || model->rounds.last().finished) return nullptr; return &model->rounds.last(); }
bool CaptureWorkflowService::hasActiveRound() const { return const_cast<CaptureWorkflowService *>(this)->activeRound() != nullptr; }
bool CaptureWorkflowService::createRound(QString *error) { auto *model = m_sessions->selectedModel(); if (!model || !model->profile) { if (error) *error = QStringLiteral("A recognized tag is required before creating a round."); return false; } if (activeRound()) { if (error) *error = QStringLiteral("A capture round is already active."); return false; } int roundNo = static_cast<int>(model->rounds.size()) + 1; if (m_repository) { if (!m_repository->nextRoundNumber(model->profile->uid, &roundNo, error) || !m_repository->createRound(*model->profile, model->number, roundNo, &m_activeRoundId, error)) return false; } CaptureRound created(roundNo, false, m_activeRoundId); model->rounds.push_back(created); m_currentWell = 1; emit changed(); return true; }
bool CaptureWorkflowService::finishRound() { if (auto *round = activeRound()) { m_activeRoundId = round->persistentId; if (round->history.size() != 16) return false; bool complete = true; for (const auto &well : round->history) if (well.isEmpty() || !well.last().available) { complete = false; break; } if (m_repository && m_activeRoundId > 0) { QString error; if (!m_repository->finishRound(m_activeRoundId, complete ? QStringLiteral("completed") : QStringLiteral("partial"), &error)) return false; } round->finished = true; emit changed(); return true; } return false; }
bool CaptureWorkflowService::selectWell(int wellNo) { if (!activeRound() || wellNo < 1 || wellNo > 16) return false; m_currentWell = wellNo; emit changed(); return true; }
bool CaptureWorkflowService::acceptCapture(const QImage &image, bool retake, double exposureUs, double gainDb, QString *error) {
    auto *round = activeRound(); auto *model = m_sessions->selectedModel(); if (!round || !model || !model->profile || image.isNull() || round->history.size() != 16) { if (error) *error = QStringLiteral("No active round or image is available."); return false; }
    auto &well = round->history[m_currentWell - 1]; if (!retake && !well.isEmpty()) { if (error) *error = QStringLiteral("This well already has an image; use retake."); return false; } if (retake && well.isEmpty()) { if (error) *error = QStringLiteral("This well has no image to retake."); return false; }
    m_activeRoundId = round->persistentId;
    const QDateTime capturedAt = QDateTime::currentDateTime(); QString relativePath;
    if (m_imageStore && !m_imageStore->savePng(image, model->profile->uid, round->number, m_currentWell, capturedAt, &relativePath, error)) return false;
    if (m_repository && !m_repository->insertImage(m_activeRoundId, *model->profile, m_currentWell, relativePath, image, capturedAt, exposureUs, gainDb, 0, nullptr, error)) { if (m_imageStore) m_imageStore->remove(relativePath); return false; }
    for (auto &capture : well) capture.active = false; well.push_back({image.copy(), capturedAt, true, true}); bool completed = true; for (const auto &item : round->history) if (item.isEmpty() || !item.last().available) completed = false; if (completed) { if (m_repository && m_activeRoundId > 0) { QString dbError; if (!m_repository->finishRound(m_activeRoundId, QStringLiteral("completed"), &dbError)) { if (error) *error = dbError; return false; } } round->finished = true; emit roundCompleted(); } else advanceToNext(); emit changed(); return true;
}

bool CaptureWorkflowService::acceptCapture(const QImage &image, bool retake, QString *error)
{
    return acceptCapture(image, retake, 0, 0, error);
}
void CaptureWorkflowService::advanceToNext() { auto *round = activeRound(); if (!round) return; for (int offset = 1; offset <= 16; ++offset) { const int candidate = ((m_currentWell - 1 + offset) % 16); if (round->history[candidate].isEmpty()) { m_currentWell = candidate + 1; return; } } }
void CaptureWorkflowService::selectFirstPendingWell() { auto *round = activeRound(); if (!round) { m_currentWell = 1; return; } for (int i = 0; i < round->history.size(); ++i) if (round->history[i].isEmpty() || !round->history[i].last().available) { m_currentWell = i + 1; return; } m_currentWell = 1; }
QVector<WellState> CaptureWorkflowService::wellStates() const { QVector<WellState> states(16, WellState::Empty); auto *round = const_cast<CaptureWorkflowService *>(this)->activeRound(); if (!round) return states; for (int i = 0; i < 16; ++i) if (!round->history[i].isEmpty()) states[i] = round->history[i].last().available ? WellState::Complete : WellState::RetakeRequired; if (m_currentWell >= 1 && m_currentWell <= 16 && states[m_currentWell - 1] == WellState::Empty) states[m_currentWell - 1] = WellState::Current; return states; }
QVector<WellCapture> CaptureWorkflowService::historyForWell(int wellNo) const { QVector<WellCapture> result; const auto *model = m_sessions->selectedModel(); if (!model || wellNo < 1 || wellNo > 16) return result; for (const auto &round : model->rounds) for (const auto &item : round.history[wellNo - 1]) result.push_back(item); return result; }
