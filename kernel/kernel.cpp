#include <stddef.h>
#include <stdint.h>

#include <kernel/multiboot.hpp>
#include <kernel/icxxabi.hpp>
#include <kernel/cpu.hpp>
#include <kernel/module.hpp>
#include <kernel/tty.hpp>
#include <kernel/gdt.hpp>
#include <kernel/idt.hpp>
#include <kernel/timer.hpp>
#include <kernel/fs.hpp>
#include <kernel/drivers/keyboard.hpp>
#include <kernel/fs/fat16.hpp>
#include <kernel/proc/scheduler.hpp>
#include <kernel/proc/process.hpp>
#include <kernel/memory/physical.hpp>
#include <kernel/user/env.hpp>
#include <kernel/gfx/surface.hpp>

extern "C"
void kernel_main(multiboot_info_t * mb_info, uint32_t k_phys_start, uint32_t k_phys_end) {
    gdt_install();
    tss_install();
    idt_install();

    // Set up the Physical Memory manager's bitmap (with pages taken up by the kernel)
    pmm_setup(mb_info, k_phys_start, k_phys_end);
    // now we can set up the kernels' Page directory (this is out VIRTUAL memory manager)
    page_directory_t kernel_pd = pg_directory_setup();

    // IRQ0
    Timer::initTimer();
    // IRQ1
    keyboard_install();

    // this loads our driver threads into the scheduler
    init_kthreads();

    start_display_driver(mb_info);

    ENV::initialise();

    // // void_fn module_one = get_module_funct(mb_info, 0); // should execute our "basic_program.s" file...
    // // terminal_printf("module_one address: %x\n", module_one);
    // // MAGIC_BREAK;
    // // module_one();

    // this line starts to run the threads and drivers, the LAST thing to enable!!!
    interrupts_enable();

    terminal_writestring("[OK] muk boot successful.\n");

    // Filesystem::initialise();

    // This does nothing apart from stop our kmain function from returning
    // Every process is now a thread (running "asynchronously")
    while(true) { }
}
