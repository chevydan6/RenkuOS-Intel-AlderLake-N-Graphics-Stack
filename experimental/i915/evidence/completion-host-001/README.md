<!-- SPDX-License-Identifier: MIT -->
Complete unchanged Linux completion.c and swait.c pass ASan/UBSan with actual
pthread locks/POSIX semaphores and a thread-local model of IRQ state. The common
suite passes 112 checks, 12 phases and 24 waiter workers; an additional real SIGUSR1
interrupts a blocked POSIX wait and yields the Linux interruption result through
the original completion code. Host kill-only signal behavior is not claimed.
A mutation that drops remaining completion counts aborts as expected. Native
semaphore, IRQ and user-thread signal results are in physical-completion-001.
