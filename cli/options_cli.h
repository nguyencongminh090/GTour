#pragma once

#include "options.h"
#include <vector>

void options_parse(int                         argc,
                   const char                **argv,
                   Options                    &o,
                   std::vector<EngineOptions> &eo);

void options_print(const Options &o, const std::vector<EngineOptions> &eo);
