#include <sys/stat.h>

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "usbd_cdc_vcp.h"

int __errno;

int _close(int file) {
	return 0;
}

int _fstat(int file, struct stat *st) {
	return 0;
}

int _isatty(int file) {
	return 1;
}

int _lseek(int file, int ptr, int dir) {
	return 0;
}

int _open(const char *name, int flags, int mode) {
	return -1;
}

int _read(int file, char *ptr, int len) {
	if (file != 0) {
		return 0;
	}

	// Use USB CDC Port for stdin
	while(!VCP_get_char((uint8_t*)ptr)){};

	// Echo typed characters
	VCP_put_char((uint8_t)*ptr);

	return 1;
}

/* Register name faking - works in collusion with the linker.  */
register char * stack_ptr asm ("sp");

caddr_t _sbrk_r (struct _reent *r, int incr) {
	extern char   end asm ("end"); /* Defined by the linker.  */
//	extern char   top_of_memory asm ("_estack");
	static char * heap_end;
	char *        prev_heap_end;

	if (heap_end == NULL)
		heap_end = & end;

	prev_heap_end = heap_end;

    /* WEP: stack_ptr is in CCM 0x10000000
            whereas heap is in regular memory 0x20000000
            So rather check if heap touches end of memory
    */
//	if (heap_end + incr > top_of_memory) {
		//errno = ENOMEM;
//		return (caddr_t) -1;
//	}

	heap_end += incr;

	return (caddr_t) prev_heap_end;
}

int _write(int file, char *ptr, int len) {
	VCP_send_buffer((uint8_t*)ptr, len);
	return len;
}

void __malloc_lock ( void * r ) {

    vTaskSuspendAll();
}

void __malloc_unlock( void * r) {

    xTaskResumeAll();
}
