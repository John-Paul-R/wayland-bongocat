#ifndef NETWORK_H
#define NETWORK_H

#include "config/config.h"

BONGOCAT_NODISCARD bongocat_error_t network_init(config_t *config);
void network_cleanup(void);

#endif // NETWORK_H
