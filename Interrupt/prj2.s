!=================================================================
! General conventions:
!   1) Stack grows from high addresses to low addresses, and the
!      top of the stack points to valid data
!
!   2) Register usage is as implied by assembler names and manual
!
!   3) Function Calling Convention:
!
!       Setup)
!       * Immediately upon entering a function, push the RA on the stack.
!       * Next, push all the registers used by the function on the stack.
!
!       Teardown)
!       * Load the return value in $v0.
!       * Pop any saved registers from the stack back into the registers.
!       * Pop the RA back into $ra.
!       * Return by executing jalr $ra, $zero.
!=================================================================

!vector table
vector0:    .fill 0x00000000 !0
            .fill 0x00000000 !1
            .fill 0x00000000 !2
            .fill 0x00000000
            .fill 0x00000000 !4
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000 !8
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000
            .fill 0x00000000 !15
!end vector table

main:   lea $sp, stack                 !initialize the stack pointer
        lw $sp, 0($sp)                  !finish initialization

                                        ! Install timer interrupt handler into vector table
		lea $a1, vector0
        lea $a0, ti_inthandler
		sw $a0, 1($a1)
        ei                              ! Don't forget to enable interrupts...

        lea $a0, BASE                   !load base for pow
        lw $a0, 0($a0)
        lea $a1, EXP                    !load power for pow
        lw $a1, 0($a1)
        lea $at, POW                    !load address of pow
        jalr $at, $ra                   !run pow
        lea $a0, ANS                    !load base for pow
        sw $v0, 0($a0)

        halt

BASE:   .fill 2
EXP:    .fill 7
ANS:    .fill 0                               ! should come out to 32

POW:    addi $sp, $sp, -1                     ! allocate space for old frame pointer
        sw $fp, 0($sp)
        addi $fp, $sp, 0                      !set new frame pinter
        add $a1, $a1, $zero                   ! check if $a1 is zero
        brz RET1                              ! if the power is 0 return 1
        add $a0, $a0, $zero
        brz RET0                              ! if the base is 0 return 0
        addi $a1, $a1, -1                     ! decrement the power
        lea $at, POW                          ! load the address of POW
        addi $sp, $sp, -2                     ! push 2 slots onto the stack
        sw $ra, -1($fp)                        ! save RA to stack
        sw $a0, -2($fp)                        ! save arg 0 to stack
        jalr $at, $ra                         ! recursively call POW
        add $a1, $v0, $zero                   ! store return value in arg 1
        lw $a0, -2($fp)                       ! load the base into arg 0
        lea $at, MULT                         ! load the address of MULT
        jalr $at, $ra                         ! multiply arg 0 (base) and arg 1 (running product)
        lw $ra, -1($fp)                       ! load RA from the stack
        addi $sp, $sp, 2
        brnzp FIN                             ! return
RET1:   addi $v0, $zero, 1                    ! return a value of 1
        brnzp FIN
RET0:   add $v0, $zero, $zero                 ! return a value of 0
FIN:    lw $fp, 0($fp)                        ! restore old frame pointer
        addi $sp, $sp, 1                      ! pop off the stack
        jalr $ra, $zero


MULT:   add $v0, $zero, $zero            ! zero out return value
AGAIN:  add $v0,$v0, $a0                ! multiply loop
        nand $a2, $zero, $zero
        add $a1, $a1, $a2
        brz  DONE                  ! finished multiplying
        brnzp AGAIN                ! loop again
DONE:   jalr $ra, $zero

ti_inthandler:
		addi    $sp, $sp, -1         	! push $k0 onto stack
		sw      $k0, 0($sp)
		ei

		addi $sp, $sp, -1				! Save processor registers
		sw $at, 0($sp)
		addi $sp, $sp, -1
		sw $v0, 0($sp)
		addi $sp, $sp, -1
		sw $a0, 0($sp)
		addi $sp, $sp, -1
		sw $a1, 0($sp)
		addi $sp, $sp, -1
		sw $a2, 0($sp)
		addi $sp, $sp, -1
		sw $t0, 0($sp)
		addi $sp, $sp, -1
		sw $t1, 0($sp)
		addi $sp, $sp, -1
		sw $t2, 0($sp)
		addi $sp, $sp, -1
		sw $s0, 0($sp)
		addi $sp, $sp, -1
		sw $s1, 0($sp)
		addi $sp, $sp, -1
		sw $s2, 0($sp)
		addi $sp, $sp, -1
		sw $fp, 0($sp)
		addi $sp, $sp, -1
		sw $ra, 0($sp)

		lea $s2, seconds
		lw $s0, 0($s2)			 	! Load second counter into $s0
		addi $s0, $s0, 1			! Increment  by 1
		addi $t1, $zero, -60		! store 60 to compare
		add $t1, $t1, $s0			! Check if $t1 + $s0 == 0
		brz MIN						! if yes jump to MIN
		sw $s0, 0($s2)				! save the seconds
		brnzp FINISH

MIN:
		add $s0, $zero, $zero		! repeat the same process
		sw $s0, 0($s2)
		lw $s0, 1($s2)
		addi $s0, $s0, 1
		addi $t1, $zero, -60
		add $t1, $t1, $s0
		brz HRS
		sw $s0, 1($s2)
		brnzp FINISH

HRS:
		add $s0, $zero, $zero		! repeat the same process
		sw $s0, 1($s2)
		lw $s0, 2($s2)
		addi $s0, $s0, 1
		sw $s0, 1($s2)

FINISH:
		lw $ra, 0($sp)				! Restore processor registers
		addi $sp, $sp, 1
		lw $fp, 0($sp)
		addi $sp, $sp, 1
		lw $s2, 0($sp)
		addi $sp, $sp, 1
		lw $s1, 0($sp)
		addi $sp, $sp, 1
		lw $s0, 0($sp)
		addi $sp, $sp, 1
		lw $t2, 0($sp)
		addi $sp, $sp, 1
		lw $t1, 0($sp)
		addi $sp, $sp, 1
		lw $t0, 0($sp)
		addi $sp, $sp, 1
		lw $a2, 0($sp)
		addi $sp, $sp, 1
		lw $a1, 0($sp)
		addi $sp, $sp, 1
		lw $a0, 0($sp)
		addi $sp, $sp, 1
		lw $v0, 0($sp)
		addi $sp, $sp, 1
		lw $at, 0($sp)
		addi $sp, $sp, 1

		di

		lw $k0, 0($sp)
		addi $sp, $sp, 1

		reti


stack:      .fill 0xA00000
seconds:    .fill 0xFFFFFC
minutes:    .fill 0xFFFFFD
hours:      .fill 0xFFFFFE
