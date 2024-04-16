#include <asm/asm.h>
#include <env.h>
#include <kclock.h>
#include <pmap.h>
#include <printk.h>
#include <trap.h>

// When build with 'make test lab=?_?', we will replace your 'mips_init' with a generated one from
// 'tests/lab?_?'.
#ifdef MOS_INIT_OVERRIDDEN
#include <generated/init_override.h>
#else


void mips_init() {
	printk("init.c:\tmips_init() is called\n");
	// lab1_range_test
	printk("-08lR test: %-08lR\n",1,3);
	printk("inverse_string? : %-10s\n", "inverse_string?");
	printk("08ld test: %08ld\n",1);
	printk("-08ld test: %-08ld\n",3);
	// lab2:
	// mips_detect_memory();
	// mips_vm_init();
	// page_init();

	// lab3:
	// env_init();
	ENV_CREATE_PRIORITY(user_bare_loop, 1);
	ENV_CREATE_PRIORITY(user_bare_loop, 2);
	// lab3:
	// ENV_CREATE_PRIORITY(user_bare_loop, 1);
	// ENV_CREATE_PRIORITY(user_bare_loop, 2);

	// lab4:
	// ENV_CREATE(user_tltest);
	// ENV_CREATE(user_fktest);
	// ENV_CREATE(user_pingpong);

	// lab6:
	// ENV_CREATE(user_icode);  // This must be the first env!

	// lab5:
	// ENV_CREATE(user_fstest);
	// ENV_CREATE(fs_serv);  // This must be the second env!
	// ENV_CREATE(user_devtst);

	// lab3:
	// kclock_init();
	// enable_irq();
}

#endif
