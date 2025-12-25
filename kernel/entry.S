.section .text
.global _entry
_entry:
 la sp, stack0 
 li a1, 1024*4 
 addi a0, a0, 1 
 mul a1, a1, a0 
 add sp, sp, a1 
 la a0,_bss_start
 la a1,_bss_end
_bss_clear:
 beq a0,a1,_bss_done
 sd zero,0(a0)
 addi a0,a0,8
 j _bss_clear
_bss_done:
 call start 
 spin: j spin
