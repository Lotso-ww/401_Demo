#include "app/applicationcontroller.h"
#include "storage/database.h"
#include "storage/imagefilestore.h"
#include "storage/repository.h"
#include "devices/rfidpayloadcodec.h"
#include <QTemporaryDir>
#include <QDirIterator>
#include <QSqlQuery>
#include <QtTest>

class WorkflowTest final : public QObject {
    Q_OBJECT
private slots:
    void uidBindingIsUnique();
    void captureAdvancesAndRetakes();
    void completesRoundAfterSixteenCaptures();
    void sqliteAndPngPersistence();
    void rfidPayloadDecode();
    void retakePersistsReplacement();
    void partialRoundIsPersisted();
    void captureParametersAndRoundSequencePersist();
    void restoredActiveRoundSelectsFirstPendingWell();
};

void WorkflowTest::uidBindingIsUnique() {
    ChamberSessionService sessions; sessions.selectChamber(1);
    TagProfile profile; profile.uid = QStringLiteral("uid-1"); QVERIFY(sessions.bindProfile(profile));
    sessions.selectChamber(2); QString error; QVERIFY(!sessions.bindProfile(profile, &error)); QVERIFY(!error.isEmpty());
}

void WorkflowTest::sqliteAndPngPersistence()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    Database database;
    QString error;
    QVERIFY(database.open(dir.filePath(QStringLiteral("test.sqlite")), &error));
    Repository repository(database.connection());
    ChamberSessionService sessions;
    sessions.setRepository(&repository);
    sessions.selectChamber(1);
    TagProfile profile;
    profile.uid = QStringLiteral("E004010203040506");
    profile.dishNumber = QStringLiteral("7");
    profile.inseminationTime = QDateTime::currentDateTime().addDays(-1);
    profile.femaleName = QStringLiteral("Test");
    profile.medicalRecordNumber = QStringLiteral("MR-1");
    profile.identifiedAt = QDateTime::currentDateTime();
    QVERIFY(sessions.bindProfile(profile, &error));
    ImageFileStore files(dir.path());
    CaptureWorkflowService workflow(&sessions);
    workflow.setPersistence(&repository, &files);
    QVERIFY(workflow.createRound(&error));
    QImage image(8, 8, QImage::Format_RGB32);
    image.fill(Qt::green);
    QVERIFY(workflow.acceptCapture(image, false, &error));
    QVERIFY(QDir(dir.path()).entryInfoList(QStringList() << QStringLiteral("*.sqlite"), QDir::Files).size() == 1);
    QDirIterator images(dir.path(), QStringList() << QStringLiteral("*.png"), QDir::Files, QDirIterator::Subdirectories);
    QVERIFY(images.hasNext());
}

void WorkflowTest::rfidPayloadDecode()
{
    QByteArray payload(20, '\0');
    payload[0] = 1; payload[1] = 20; payload[2] = 7;
    payload[3] = char(0x26); payload[4] = char(0x08); payload[5] = char(0x12); payload[6] = char(0x14); payload[7] = char(0x30);
    payload.replace(8, 8, QByteArray("    Test", 8));
    payload.replace(16, 4, QByteArray("MR-1"));
    DecodedRfidPayload decoded; QString error;
    QVERIFY(RfidPayloadCodec::decode(payload, &decoded, &error));
    QCOMPARE(decoded.dishNumber, 7);
    QCOMPARE(decoded.femaleName, QStringLiteral("Test"));
    QCOMPARE(decoded.medicalRecordNumber, QStringLiteral("MR-1"));
}

void WorkflowTest::retakePersistsReplacement()
{
    QTemporaryDir dir; QVERIFY(dir.isValid()); Database database; QString error; QVERIFY(database.open(dir.filePath("retake.sqlite"), &error)); Repository repository(database.connection()); ChamberSessionService sessions; sessions.setRepository(&repository); sessions.selectChamber(1); TagProfile p; p.uid="E004010203040507"; p.dishNumber="1"; p.inseminationTime=QDateTime::currentDateTime(); p.femaleName="Test"; p.medicalRecordNumber="MR-2"; p.identifiedAt=QDateTime::currentDateTime(); QVERIFY(sessions.bindProfile(p,&error)); ImageFileStore files(dir.path()); CaptureWorkflowService workflow(&sessions); workflow.setPersistence(&repository,&files); QVERIFY(workflow.createRound(&error)); QImage a(2,2,QImage::Format_RGB32); a.fill(Qt::red); QVERIFY(workflow.acceptCapture(a,false,&error)); QVERIFY(workflow.selectWell(1)); QImage b(2,2,QImage::Format_RGB32); b.fill(Qt::blue); QVERIFY(workflow.acceptCapture(b,true,&error)); QSqlQuery q(database.connection()); QVERIFY(q.exec("SELECT is_active,replaced_image_id FROM capture_image ORDER BY id")); QVERIFY(q.next()); QCOMPARE(q.value(0).toInt(), 0); QVERIFY(q.next()); QCOMPARE(q.value(0).toInt(), 1); QCOMPARE(q.value(1).toLongLong(), 1LL);
}

void WorkflowTest::partialRoundIsPersisted()
{
    QTemporaryDir dir; QVERIFY(dir.isValid()); Database database; QString error; QVERIFY(database.open(dir.filePath("partial.sqlite"), &error)); Repository repository(database.connection()); ChamberSessionService sessions; sessions.setRepository(&repository); sessions.selectChamber(1); TagProfile p; p.uid="E004010203040508"; p.dishNumber="1"; p.inseminationTime=QDateTime::currentDateTime(); p.femaleName="Test"; p.medicalRecordNumber="MR-3"; p.identifiedAt=QDateTime::currentDateTime(); QVERIFY(sessions.bindProfile(p,&error)); CaptureWorkflowService workflow(&sessions); workflow.setPersistence(&repository,nullptr); QVERIFY(workflow.createRound(&error)); QVERIFY(workflow.finishRound()); QSqlQuery q(database.connection()); QVERIFY(q.exec("SELECT status FROM capture_round")); QVERIFY(q.next()); QCOMPARE(q.value(0).toString(), QStringLiteral("partial"));
}

void WorkflowTest::captureParametersAndRoundSequencePersist()
{
    QTemporaryDir dir; QVERIFY(dir.isValid());
    Database database; QString error; QVERIFY(database.open(dir.filePath("metadata.sqlite"), &error));
    Repository repository(database.connection());
    ChamberSessionService sessions; sessions.setRepository(&repository); sessions.selectChamber(1);
    TagProfile p; p.uid="E004010203040509"; p.dishNumber="1"; p.inseminationTime=QDateTime::currentDateTime(); p.femaleName="Test"; p.medicalRecordNumber="MR-4"; p.identifiedAt=QDateTime::currentDateTime(); QVERIFY(sessions.bindProfile(p,&error));
    ImageFileStore files(dir.path()); CaptureWorkflowService workflow(&sessions); workflow.setPersistence(&repository,&files);
    QVERIFY(workflow.createRound(&error));
    QImage image(2,2,QImage::Format_RGB32); image.fill(Qt::yellow);
    QVERIFY(workflow.acceptCapture(image, false, 12345.0, 4.5, &error));
    QSqlQuery metadata(database.connection()); QVERIFY(metadata.exec("SELECT exposure_us,gain_db FROM capture_image")); QVERIFY(metadata.next()); QCOMPARE(metadata.value(0).toDouble(), 12345.0); QCOMPARE(metadata.value(1).toDouble(), 4.5);
    QVERIFY(workflow.finishRound());
    QVERIFY(workflow.createRound(&error));
    QSqlQuery rounds(database.connection()); QVERIFY(rounds.exec("SELECT round_no FROM capture_round ORDER BY id")); QVERIFY(rounds.next()); QCOMPARE(rounds.value(0).toInt(), 1); QVERIFY(rounds.next()); QCOMPARE(rounds.value(0).toInt(), 2);
}

void WorkflowTest::restoredActiveRoundSelectsFirstPendingWell()
{
    QTemporaryDir dir; QVERIFY(dir.isValid());
    Database database; QString error; QVERIFY(database.open(dir.filePath("restore.sqlite"), &error));
    Repository repository(database.connection());
    ChamberSessionService source; source.setRepository(&repository); source.selectChamber(1);
    TagProfile p; p.uid="E004010203040510"; p.dishNumber="1"; p.inseminationTime=QDateTime::currentDateTime(); p.femaleName="Test"; p.medicalRecordNumber="MR-5"; p.identifiedAt=QDateTime::currentDateTime(); QVERIFY(source.bindProfile(p,&error));
    ImageFileStore files(dir.path()); CaptureWorkflowService original(&source); original.setPersistence(&repository,&files); QVERIFY(original.createRound(&error));
    QImage image(2,2,QImage::Format_RGB32); image.fill(Qt::cyan); QVERIFY(original.acceptCapture(image, false, &error));
    ChamberSessionService restored; restored.setRepository(&repository); QVector<ChamberModel> chambers = restored.chambers(); QVERIFY(repository.loadAssignments(&chambers,&error)); QVERIFY(repository.loadRounds(&chambers,&error)); restored.restoreProfiles(chambers);
    CaptureWorkflowService recovered(&restored); recovered.setPersistence(&repository,&files); restored.selectChamber(1); QCOMPARE(recovered.currentWell(), 2);
}
void WorkflowTest::captureAdvancesAndRetakes() {
    ChamberSessionService sessions; CaptureWorkflowService workflow(&sessions); sessions.selectChamber(1); TagProfile profile; profile.uid = QStringLiteral("uid-1"); QVERIFY(sessions.bindProfile(profile)); QVERIFY(workflow.createRound());
    QImage first(20, 20, QImage::Format_RGB32); first.fill(Qt::red); QString captureError; QVERIFY2(workflow.acceptCapture(first, false, &captureError), qPrintable(captureError)); QCOMPARE(workflow.currentWell(), 2); QVERIFY(workflow.selectWell(1));
    QImage second(20, 20, QImage::Format_RGB32); second.fill(Qt::blue); QVERIFY(workflow.acceptCapture(second, true)); const auto history = workflow.historyForWell(1); QCOMPARE(history.size(), 2); QVERIFY(!history.front().active); QVERIFY(history.back().active); QVERIFY(workflow.selectWell(3)); QString error; QVERIFY(!workflow.acceptCapture(second, true, &error)); QVERIFY(!error.isEmpty());
}
void WorkflowTest::completesRoundAfterSixteenCaptures() {
    ChamberSessionService sessions; CaptureWorkflowService workflow(&sessions); sessions.selectChamber(1); TagProfile profile; profile.uid = QStringLiteral("uid-1"); QVERIFY(sessions.bindProfile(profile)); QVERIFY(workflow.createRound());
    QImage image(4, 4, QImage::Format_RGB32); image.fill(Qt::green); for (int i = 0; i < 16; ++i) QVERIFY(workflow.acceptCapture(image, false)); QVERIFY(!workflow.hasActiveRound());
}
QTEST_MAIN(WorkflowTest)
#include "tst_workflow.moc"
