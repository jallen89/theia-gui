#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "delegation.h"

int main() {
    init_lctx();
    struct delegator *del;

    instrument_indicator(4);
    instrument_delegator(10);
    instrument_indicator(5);
    instrument_indicator(6);
}
