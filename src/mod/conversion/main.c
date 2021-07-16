#include "config.h"

#include "srsvm/impl.h"

bool srsvm_word_size_support(const uint8_t word_size)
{
    switch(word_size) {
        case 16:
        case 32:
        case 64:
        case 128:
            return true;
        default:
            return false;
    }
}
