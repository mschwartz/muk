#include <stddef.h>
#include <stdint.h>

#include <kernel/tty.hpp>
#include <kernel/cpu.hpp>
#include <kernel/idt.hpp>
#include <kernel/timer.hpp>
#include <kernel/drivers/keyboard.hpp>
#include <kernel/gfx/surface.hpp>

#include <kernel/proc/process.hpp>
#include <kernel/proc/scheduler.hpp>
#include <kernel/proc/thread.hpp>
#include <kernel/proc/message.hpp>

#include <kernel/memory/manager.hpp>
#include <kernel/memory/heap.hpp>
#include <kernel/memory/alloc.hpp>

//std::vector<Thread *> thread_list = std::vector<Thread *>();
Thread* thread_root = nullptr;
Thread* thread_running = nullptr;

static int next_tid = 1;

/**
*  This is the hub of the process list. For the moment, this function only adds 
*  threads to the "kernel" process. Every new thread is attached to this initial
*   process for the time being.
*/
void start_thread(char* title, void_fn entry) {
	Thread* thread = new Thread();
	memcpy(thread->title, title, strlen(title)+1);
	thread->thread_id = next_tid;

	thread->entry_ptr = reinterpret_cast<uintptr_t>(entry);
	thread->stack_ptr = (uintptr_t)(&thread->stack)+4096; // 4kib of local thread stack

	PUSH(thread->stack_ptr, uintptr_t, thread->entry_ptr); // our first interrupt "ret" eip

	PUSH(thread->stack_ptr, uint32_t, 0); // seed the stack with zeroes
	PUSH(thread->stack_ptr, uint32_t, 0);
	PUSH(thread->stack_ptr, uint32_t, 0);
	PUSH(thread->stack_ptr, uint32_t, 0);

	thread->state_reg.eax = 0;
	thread->state_reg.ebx = 0;
	thread->state_reg.ecx = 0;
	thread->state_reg.edx = 0;

	thread->state_reg.esi = 0;
	thread->state_reg.edi = 0;

	thread->state_reg.useresp = 0;
	thread->state_reg.ebp = 0;
	thread->state_reg.eip = (uint32_t)thread->entry_ptr;
	thread->state_reg.esp = (uint32_t)thread->stack_ptr;

	thread->state_reg.eflags = (1 << 9) | 1;

	if(thread_root == nullptr) {
		thread_root = thread;
		thread_root->next = thread_root;
	}
	else {
		Thread* t = thread_root;
		for(; t->next != thread_root; t = t->next) { }

		thread->prev = t;
		t->next = thread;

		// stitch up the beginning node + end node into a doubly linked ring list
		thread->next = thread_root;
		thread_root->prev = thread; 
	}

	next_tid++;
}

void init_kthreads() {
	start_thread("keyboard_driver", kb_update); // adds to the ringbuffer
	start_thread("graphics_driver", surface_update); // keeps the frontbuffer synchronised
	start_thread("postbox_debug", postbox_debug);
	start_thread("tty_driver", tty_update); // updates the terminal text and parses commands
	
	

	thread_running = thread_root;
}