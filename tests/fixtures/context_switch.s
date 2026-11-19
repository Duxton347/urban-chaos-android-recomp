# Authored synthetic x86: save a checkpoint, switch stacks, restore and return 42.
.intel_syntax noprefix
.section .text
.global entry, checkpoint, restore_context, worker, _start
entry:
 push ebx
 mov ebx, 0x9876
 call body
 cmp ebx, 0x9876
 je root_done
 mov eax, 0xbad
 root_done:
 pop ebx
 ret
.balign 16
body:
 push esi
 mov esi, 0x2468
 call checkpoint
 test eax, eax
 jnz done
 mov ecx, offset worker_context
 xor edx, edx
 call restore_context
 mov eax, 0xbad
 jmp finish
 done:
 cmp esi, 0x2468
 je finish
 mov eax, 0xbad
 finish:
 pop esi
 ret
.balign 16
checkpoint:
 mov dword ptr [main_context], esp
 mov eax, dword ptr [esp]
 mov dword ptr [main_context+4], eax
 mov dword ptr [main_context+8], esi
 mov dword ptr [main_context+12], ebx
 xor eax, eax
 ret
.balign 16
restore_context:
 mov esi, dword ptr [ecx+8]
 mov ebx, dword ptr [ecx+12]
 mov esp, dword ptr [ecx]
 add esp, 4
 push dword ptr [ecx+4]
 mov eax, edx
 ret
.balign 16
worker:
 cmp esi, 0x7788
 jne failed_worker
 cmp ebx, 0x5678
 jne failed_worker
 mov ecx, offset main_context
 mov edx, 42
 call restore_context
 failed_worker:
 ud2
.balign 16
_start:
 mov esp, 0x1fff0
 call entry
 mov ebx, eax
 mov eax, 1
 int 0x80
.section .data
.global main_context, worker_context
main_context: .long 0,0,0,0
.balign 16
worker_context: .long 0x2fffc,worker,0x7788,0x5678
.section .bss
.balign 16
.space 0x20000
