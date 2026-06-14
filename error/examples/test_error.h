#pragma once
#include "include/ef.h"

#define DB_ERRORS(X)                                                                    \
    X(1001, DBconnectionFailed, "Database connection failed", ::ef::ErrorLevel::Fatal)  \
    X(1002, DBqueryTimeout, "Database query timed out", ::ef::ErrorLevel::Warning)      \
    X(1003, DbduplicateKey, "Duplicate primary key violation", ::ef::ErrorLevel::Avoid) \
    X(1004, DBpermissionDenied, "Insufficient database permissions", ::ef::ErrorLevel::Fatal)

EF_GEN_ERROR_CODES(DB_ERRORS)

EF_GEN_ERROR_META(DB_ERRORS)

#undef DB_ERRORS

template <typename T>
using SR = Result<T, ef::Error>;