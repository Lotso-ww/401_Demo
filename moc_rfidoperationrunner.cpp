/****************************************************************************
** Meta object code from reading C++ file 'rfidoperationrunner.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../hf_tagaccess/rfidoperationrunner.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'rfidoperationrunner.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RfidOperationRunner_t {
    QByteArrayData data[10];
    char stringdata0[117];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_RfidOperationRunner_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_RfidOperationRunner_t qt_meta_stringdata_RfidOperationRunner = {
    {
QT_MOC_LITERAL(0, 0, 19), // "RfidOperationRunner"
QT_MOC_LITERAL(1, 20, 14), // "attemptStarted"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 7), // "attempt"
QT_MOC_LITERAL(4, 44, 14), // "retryScheduled"
QT_MOC_LITERAL(5, 59, 13), // "failedAttempt"
QT_MOC_LITERAL(6, 73, 7), // "delayMs"
QT_MOC_LITERAL(7, 81, 8), // "finished"
QT_MOC_LITERAL(8, 90, 19), // "RfidOperationResult"
QT_MOC_LITERAL(9, 110, 6) // "result"

    },
    "RfidOperationRunner\0attemptStarted\0\0"
    "attempt\0retryScheduled\0failedAttempt\0"
    "delayMs\0finished\0RfidOperationResult\0"
    "result"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RfidOperationRunner[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,
       4,    2,   32,    2, 0x06 /* Public */,
       7,    1,   37,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    5,    6,
    QMetaType::Void, 0x80000000 | 8,    9,

       0        // eod
};

void RfidOperationRunner::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<RfidOperationRunner *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->attemptStarted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->retryScheduled((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 2: _t->finished((*reinterpret_cast< const RfidOperationResult(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< RfidOperationResult >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (RfidOperationRunner::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RfidOperationRunner::attemptStarted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (RfidOperationRunner::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RfidOperationRunner::retryScheduled)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (RfidOperationRunner::*)(const RfidOperationResult & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&RfidOperationRunner::finished)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject RfidOperationRunner::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_RfidOperationRunner.data,
    qt_meta_data_RfidOperationRunner,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *RfidOperationRunner::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RfidOperationRunner::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RfidOperationRunner.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int RfidOperationRunner::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void RfidOperationRunner::attemptStarted(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void RfidOperationRunner::retryScheduled(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void RfidOperationRunner::finished(const RfidOperationResult & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
