#pragma once

#include <QtGlobal>

#if defined(USPRACHE_LIBRARY)
#  define USPRACHESHARED_EXPORT Q_DECL_EXPORT
#else
#  define USPRACHESHARED_EXPORT Q_DECL_IMPORT
#endif
