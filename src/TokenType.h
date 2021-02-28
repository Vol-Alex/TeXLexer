#pragma once

enum TokenType
{
    END = 0,
    COMMAND,
    START_GROUP,
    END_GROUP,
    BEGIN_ENV,
    END_ENV,
    DIGIT,
    TEXT,
    SIGN,
};
