#ifndef _MOD_CRUSH_MODULE_H
#define _MOD_CRUSH_MODULE_H

#include <light.h>

// TODO implement version fields properly
#define LIBCRUSH_MODULE_VERSION_STR           "0.1.0"

#define LIBCRUSH_MODULE_INFO_STR              "libcrush_module v" LIBCRUSH_MODULE_VERSION_STR

Light_Module_Declare(libcrush_module);

#endif