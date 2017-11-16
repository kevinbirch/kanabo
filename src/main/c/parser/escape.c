#include "str.h"

#include "parser/parse.h"

char *unescape(const char *lexeme)
{
    size_t length = strlen(lexeme);
    MutableString *cooked = make_mstring(length);

    for(size_t i = 0; i < length; i++)
    {
        if(lexeme[i] != '\\')
        {
            mstring_append(&cooked, lexeme[i]);
            continue;
        }
        i++;
        switch(lexeme[i])
        {
            case '"':
            case '/':
            case ' ':
                // N.B. - unescape as the literal value
                mstring_append(&cooked, lexeme[i]);
                break;
            case '\\':
                mstring_append(&cooked, "\\\\");
                break;
            case 'b':
                mstring_append(&cooked, "\\b");
                break;
            case 'n':
                mstring_append(&cooked, "\\n");
                break;
            case 'r':
                mstring_append(&cooked, "\\r");
                break;
            case 't':
                mstring_append(&cooked, "\\t");
                break;
            case '_':
                mstring_append(&cooked, "\\u00a0");
                break;
            case '0':
                mstring_append(&cooked, "\\u0000");
                break;
            case 'a':
                mstring_append(&cooked, "\\u0007");
                break;
            case 'e':
                mstring_append(&cooked, "\\u001b");
                break;
            case 'v':
                mstring_append(&cooked, "\\u000b");
                break;
            case 'L':
                mstring_append(&cooked, "\\u2028");
                break;
            case 'N':
                mstring_append(&cooked, "\\u0085");
                break;
            case 'P':
                mstring_append(&cooked, "\\u2029");
                break;
            case 'x':
                mstring_append(&cooked, "\\x");
                mstring_append_stream(&cooked, (uint8_t *)lexeme + i, 2);
                i++;
                break;
            case 'u':
                mstring_append(&cooked, "\\u");
                mstring_append_stream(&cooked, (uint8_t *)lexeme + i, 4);
                i += 3;
                break;
            case 'U':
                mstring_append(&cooked, "\\U");
                mstring_append_stream(&cooked, (uint8_t *)lexeme + i, 8);
                i += 7;
                break;
            default:
                // N.B. - ignore this escape sequence
                break;
        }
    }


    char *result = mstring_copy(cooked);
    mstring_free(cooked);

    return result;
}
