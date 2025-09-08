#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>

char* fault_to_string(seL4_Word fault_label);

bool fault_handle(size_t vcpu_id, microkit_msginfo msginfo);