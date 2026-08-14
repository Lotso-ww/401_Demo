/****************************************************************************
** Meta object code from reading C++ file 'CAEDevice_HF.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../hf_tagaccess/CAEDevice_HF.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CAEDevice_HF.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CAEDevice_HF_t {
    QByteArrayData data[25];
    char stringdata0[334];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CAEDevice_HF_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CAEDevice_HF_t qt_meta_stringdata_CAEDevice_HF = {
    {
QT_MOC_LITERAL(0, 0, 12), // "CAEDevice_HF"
QT_MOC_LITERAL(1, 13, 12), // "workFinished"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 22), // "sgnl_inventory_data_hf"
QT_MOC_LITERAL(4, 50, 9), // "tag_count"
QT_MOC_LITERAL(5, 60, 15), // "vector<CTag_HF>"
QT_MOC_LITERAL(6, 76, 4), // "tags"
QT_MOC_LITERAL(7, 81, 8), // "use_time"
QT_MOC_LITERAL(8, 90, 10), // "loop_count"
QT_MOC_LITERAL(9, 101, 23), // "sgnl_inventory_end_loop"
QT_MOC_LITERAL(10, 125, 4), // "iret"
QT_MOC_LITERAL(11, 130, 17), // "sgnl_scan_data_hf"
QT_MOC_LITERAL(12, 148, 18), // "sgnl_scan_finished"
QT_MOC_LITERAL(13, 167, 28), // "sgnl_stable_scan_uid_changed"
QT_MOC_LITERAL(14, 196, 12), // "previousUids"
QT_MOC_LITERAL(15, 209, 11), // "currentUids"
QT_MOC_LITERAL(16, 221, 9), // "elapsedMs"
QT_MOC_LITERAL(17, 231, 15), // "updateConfirmed"
QT_MOC_LITERAL(18, 247, 9), // "Inventory"
QT_MOC_LITERAL(19, 257, 7), // "hreader"
QT_MOC_LITERAL(20, 265, 11), // "antennasSrc"
QT_MOC_LITERAL(21, 277, 7), // "ant_cnt"
QT_MOC_LITERAL(22, 285, 8), // "ScanOnce"
QT_MOC_LITERAL(23, 294, 21), // "ScanStableBusinessTag"
QT_MOC_LITERAL(24, 316, 17) // "onUpdateCompleted"

    },
    "CAEDevice_HF\0workFinished\0\0"
    "sgnl_inventory_data_hf\0tag_count\0"
    "vector<CTag_HF>\0tags\0use_time\0loop_count\0"
    "sgnl_inventory_end_loop\0iret\0"
    "sgnl_scan_data_hf\0sgnl_scan_finished\0"
    "sgnl_stable_scan_uid_changed\0previousUids\0"
    "currentUids\0elapsedMs\0updateConfirmed\0"
    "Inventory\0hreader\0antennasSrc\0ant_cnt\0"
    "ScanOnce\0ScanStableBusinessTag\0"
    "onUpdateCompleted"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CAEDevice_HF[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   69,    2, 0x06 /* Public */,
       3,    4,   70,    2, 0x06 /* Public */,
       9,    1,   79,    2, 0x06 /* Public */,
      11,    3,   82,    2, 0x06 /* Public */,
      12,    1,   89,    2, 0x06 /* Public */,
      13,    3,   92,    2, 0x06 /* Public */,
      17,    0,   99,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      18,    3,  100,    2, 0x0a /* Public */,
      22,    3,  107,    2, 0x0a /* Public */,
      23,    3,  114,    2, 0x0a /* Public */,
      24,    0,  121,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 5, QMetaType::Int, QMetaType::Int,    4,    6,    7,    8,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 5, QMetaType::Int,    4,    6,    7,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Int,   14,   15,   16,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::VoidStar, QMetaType::QByteArray, QMetaType::Int,   19,   20,   21,
    QMetaType::Void, QMetaType::VoidStar, QMetaType::QByteArray, QMetaType::Int,   19,   20,   21,
    QMetaType::Void, QMetaType::VoidStar, QMetaType::QByteArray, QMetaType::Int,   19,   20,   21,
    QMetaType::Void,

       0        // eod
};

void CAEDevice_HF::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CAEDevice_HF *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->workFinished(); break;
        case 1: _t->sgnl_inventory_data_hf((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< vector<CTag_HF>(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4]))); break;
        case 2: _t->sgnl_inventory_end_loop((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->sgnl_scan_data_hf((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< vector<CTag_HF>(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 4: _t->sgnl_scan_finished((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->sgnl_stable_scan_uid_changed((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 6: _t->updateConfirmed(); break;
        case 7: _t->Inventory((*reinterpret_cast< void*(*)>(_a[1])),(*reinterpret_cast< QByteArray(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 8: _t->ScanOnce((*reinterpret_cast< void*(*)>(_a[1])),(*reinterpret_cast< QByteArray(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 9: _t->ScanStableBusinessTag((*reinterpret_cast< void*(*)>(_a[1])),(*reinterpret_cast< QByteArray(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 10: _t->onUpdateCompleted(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CAEDevice_HF::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CAEDevice_HF::workFinished)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CAEDevice_HF::*)(int , vector<CTag_HF> , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CAEDevice_HF::sgnl_inventory_data_hf)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CAEDevice_HF::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CAEDevice_HF::sgnl_inventory_end_loop)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CAEDevice_HF::*)(int , vector<CTag_HF> , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CAEDevice_HF::sgnl_scan_data_hf)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CAEDevice_HF::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CAEDevice_HF::sgnl_scan_finished)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CAEDevice_HF::*)(QString , QString , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CAEDevice_HF::sgnl_stable_scan_uid_changed)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (CAEDevice_HF::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CAEDevice_HF::updateConfirmed)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CAEDevice_HF::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CAEDevice_HF.data,
    qt_meta_data_CAEDevice_HF,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CAEDevice_HF::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CAEDevice_HF::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CAEDevice_HF.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CAEDevice_HF::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void CAEDevice_HF::workFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CAEDevice_HF::sgnl_inventory_data_hf(int _t1, vector<CTag_HF> _t2, int _t3, int _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CAEDevice_HF::sgnl_inventory_end_loop(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CAEDevice_HF::sgnl_scan_data_hf(int _t1, vector<CTag_HF> _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CAEDevice_HF::sgnl_scan_finished(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CAEDevice_HF::sgnl_stable_scan_uid_changed(QString _t1, QString _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void CAEDevice_HF::updateConfirmed()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
