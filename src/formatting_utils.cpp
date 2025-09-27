#include "formatting_utils.h"

char format_as_character(character_type character) {
    if(character == SHRT_MAX) {
        return 'r';
    }
    return 'a' + character;
}
