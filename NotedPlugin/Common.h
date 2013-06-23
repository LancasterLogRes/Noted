#pragma once

#include <map>
#include <QPainter>
#include <QMetaType>
#include <Common/Time.h>
#include <Common/Trivial.h>
#include <Common/SimpleKey.h>

LIGHTBOX_STRUCT(2, DataKey, lb::SimpleKey, source, lb::SimpleKey, operation);
inline uint qHash(DataKey _k) { return (uint)lb::generateKey(_k.source, _k.operation); }

Q_DECLARE_METATYPE(lb::Time)
Q_DECLARE_METATYPE(lb::SimpleKey)
Q_DECLARE_METATYPE(DataKey)
