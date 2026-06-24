.section .text
.global demo_user_program_start
.global demo_user_program_end

.equ SYSCALL_PUTS, 0
.equ SYSCALL_GET_TICKS, 1
.equ SYSCALL_GET_PID, 2
.equ SYSCALL_GET_PROCESS_COUNT, 3
.equ SYSCALL_YIELD, 4

demo_user_program_start:
    mov $SYSCALL_PUTS, %rax
    lea user_msg(%rip), %rdi
    int $0x80

    mov $SYSCALL_GET_PID, %rax
    int $0x80

    mov $SYSCALL_GET_TICKS, %rax
    int $0x80

1:
    mov $SYSCALL_YIELD, %rax
    int $0x80
    jmp 1b

user_msg:
    .asciz "hello from user mode\n"

demo_user_program_end:
