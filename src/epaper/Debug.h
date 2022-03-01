#ifndef __DEBUG_H
#define __DEBUG_H

// #include <logging/log.h>
// LOG_MODULE_REGISTER(epaper_debug, LOG_LEVEL_DBG);

#define USE_DEBUG 0
#if USE_DEBUG
	#define Debug(__info) printk(__info)
#else
	#define Debug(__info)  
#endif

#endif
