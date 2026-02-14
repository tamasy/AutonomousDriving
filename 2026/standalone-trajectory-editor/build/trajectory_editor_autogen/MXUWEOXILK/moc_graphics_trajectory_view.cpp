/****************************************************************************
** Meta object code from reading C++ file 'graphics_trajectory_view.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/gui/graphics_trajectory_view.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'graphics_trajectory_view.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_trajectory_editor__GraphicsTrajectoryView_t {
    QByteArrayData data[18];
    char stringdata0[206];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_trajectory_editor__GraphicsTrajectoryView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_trajectory_editor__GraphicsTrajectoryView_t qt_meta_stringdata_trajectory_editor__GraphicsTrajectoryView = {
    {
QT_MOC_LITERAL(0, 0, 41), // "trajectory_editor::GraphicsTr..."
QT_MOC_LITERAL(1, 42, 12), // "pointClicked"
QT_MOC_LITERAL(2, 55, 0), // ""
QT_MOC_LITERAL(3, 56, 6), // "size_t"
QT_MOC_LITERAL(4, 63, 5), // "index"
QT_MOC_LITERAL(5, 69, 10), // "pointMoved"
QT_MOC_LITERAL(6, 80, 5), // "new_x"
QT_MOC_LITERAL(7, 86, 5), // "new_y"
QT_MOC_LITERAL(8, 92, 10), // "pointAdded"
QT_MOC_LITERAL(9, 103, 1), // "x"
QT_MOC_LITERAL(10, 105, 1), // "y"
QT_MOC_LITERAL(11, 107, 8), // "velocity"
QT_MOC_LITERAL(12, 116, 12), // "pointDeleted"
QT_MOC_LITERAL(13, 129, 16), // "selectionCleared"
QT_MOC_LITERAL(14, 146, 13), // "rangeSelected"
QT_MOC_LITERAL(15, 160, 11), // "start_index"
QT_MOC_LITERAL(16, 172, 9), // "end_index"
QT_MOC_LITERAL(17, 182, 23) // "onSceneSelectionChanged"

    },
    "trajectory_editor::GraphicsTrajectoryView\0"
    "pointClicked\0\0size_t\0index\0pointMoved\0"
    "new_x\0new_y\0pointAdded\0x\0y\0velocity\0"
    "pointDeleted\0selectionCleared\0"
    "rangeSelected\0start_index\0end_index\0"
    "onSceneSelectionChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_trajectory_editor__GraphicsTrajectoryView[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       5,    3,   52,    2, 0x06 /* Public */,
       8,    4,   59,    2, 0x06 /* Public */,
      12,    1,   68,    2, 0x06 /* Public */,
      13,    0,   71,    2, 0x06 /* Public */,
      14,    2,   72,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      17,    0,   77,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Double, QMetaType::Double,    4,    6,    7,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Double, QMetaType::Double, QMetaType::Double,    4,    9,   10,   11,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3,   15,   16,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void trajectory_editor::GraphicsTrajectoryView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GraphicsTrajectoryView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->pointClicked((*reinterpret_cast< size_t(*)>(_a[1]))); break;
        case 1: _t->pointMoved((*reinterpret_cast< size_t(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3]))); break;
        case 2: _t->pointAdded((*reinterpret_cast< size_t(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3])),(*reinterpret_cast< double(*)>(_a[4]))); break;
        case 3: _t->pointDeleted((*reinterpret_cast< size_t(*)>(_a[1]))); break;
        case 4: _t->selectionCleared(); break;
        case 5: _t->rangeSelected((*reinterpret_cast< size_t(*)>(_a[1])),(*reinterpret_cast< size_t(*)>(_a[2]))); break;
        case 6: _t->onSceneSelectionChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GraphicsTrajectoryView::*)(size_t );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GraphicsTrajectoryView::pointClicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (GraphicsTrajectoryView::*)(size_t , double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GraphicsTrajectoryView::pointMoved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (GraphicsTrajectoryView::*)(size_t , double , double , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GraphicsTrajectoryView::pointAdded)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (GraphicsTrajectoryView::*)(size_t );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GraphicsTrajectoryView::pointDeleted)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (GraphicsTrajectoryView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GraphicsTrajectoryView::selectionCleared)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (GraphicsTrajectoryView::*)(size_t , size_t );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GraphicsTrajectoryView::rangeSelected)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject trajectory_editor::GraphicsTrajectoryView::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_meta_stringdata_trajectory_editor__GraphicsTrajectoryView.data,
    qt_meta_data_trajectory_editor__GraphicsTrajectoryView,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *trajectory_editor::GraphicsTrajectoryView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *trajectory_editor::GraphicsTrajectoryView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_trajectory_editor__GraphicsTrajectoryView.stringdata0))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int trajectory_editor::GraphicsTrajectoryView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void trajectory_editor::GraphicsTrajectoryView::pointClicked(size_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void trajectory_editor::GraphicsTrajectoryView::pointMoved(size_t _t1, double _t2, double _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void trajectory_editor::GraphicsTrajectoryView::pointAdded(size_t _t1, double _t2, double _t3, double _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void trajectory_editor::GraphicsTrajectoryView::pointDeleted(size_t _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void trajectory_editor::GraphicsTrajectoryView::selectionCleared()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void trajectory_editor::GraphicsTrajectoryView::rangeSelected(size_t _t1, size_t _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
