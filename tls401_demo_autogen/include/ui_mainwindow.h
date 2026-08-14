/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QVBoxLayout *rootLayout;
    QFrame *topBar;
    QHBoxLayout *topBarLayout;
    QLabel *pageTitle;
    QSpacerItem *topBarSpacer;
    QLabel *statusLabel;
    QStackedWidget *pages;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1280, 820);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        rootLayout = new QVBoxLayout(centralWidget);
        rootLayout->setSpacing(0);
        rootLayout->setObjectName(QString::fromUtf8("rootLayout"));
        rootLayout->setContentsMargins(0, 0, 0, 0);
        topBar = new QFrame(centralWidget);
        topBar->setObjectName(QString::fromUtf8("topBar"));
        topBar->setMinimumSize(QSize(0, 56));
        topBar->setFrameShape(QFrame::NoFrame);
        topBarLayout = new QHBoxLayout(topBar);
        topBarLayout->setObjectName(QString::fromUtf8("topBarLayout"));
        topBarLayout->setContentsMargins(20, 10, 20, 10);
        pageTitle = new QLabel(topBar);
        pageTitle->setObjectName(QString::fromUtf8("pageTitle"));

        topBarLayout->addWidget(pageTitle);

        topBarSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        topBarLayout->addItem(topBarSpacer);

        statusLabel = new QLabel(topBar);
        statusLabel->setObjectName(QString::fromUtf8("statusLabel"));
        statusLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

        topBarLayout->addWidget(statusLabel);


        rootLayout->addWidget(topBar);

        pages = new QStackedWidget(centralWidget);
        pages->setObjectName(QString::fromUtf8("pages"));

        rootLayout->addWidget(pages);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "TLS401 RFID + CCD \350\201\224\345\212\250\346\274\224\347\244\272", nullptr));
        pageTitle->setText(QCoreApplication::translate("MainWindow", "TLS401 \350\203\232\350\203\216\345\237\271\345\205\273\347\233\221\346\216\247", nullptr));
        statusLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
