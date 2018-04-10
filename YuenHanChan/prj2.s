! Yuen Han Chan
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

main:           la $sp, stack           ! Initialize stack pointer
                lw $sp, 0($sp)          
                
                ! Install timer interrupt handler into vector table

                la $s2, ti_inthandler   ! load interrupt handler address into $s2
                sw $s2, 0x01($zero)     ! store interrupt handler address at 0x01
                ei                      ! Dont forget to enable interrupts...

                la $at, factorial       ! load address of factorial label into $at
                addi $a0, $zero, 5      ! $a0 = 5, the number to factorialize
                jalr $at, $ra           ! jump to factorial, set $ra to return addr
                halt                    ! when we return, just halt

factorial:      addi    $sp, $sp, -1    ! push RA
                sw      $ra, 0($sp)
                addi    $sp, $sp, -1    ! push a0
                sw      $a0, 0($sp)
                addi    $sp, $sp, -1    ! push s0
                sw      $s0, 0($sp)
                addi    $sp, $sp, -1    ! push s1
                sw      $s1, 0($sp)

                beq     $a0, $zero, base_zero
                addi    $s1, $zero, 1
                beq     $a0, $s1, base_one
                beq     $zero, $zero, recurse
                
    base_zero:  addi    $v0, $zero, 1   ! 0! = 1
                beq     $zero, $zero, done

    base_one:   addi    $v0, $zero, 1   ! 1! = 1
                beq     $zero, $zero, done

    recurse:    add     $s1, $a0, $zero     ! save n in s1
                addi    $a0, $a0, -1        ! n! = n * (n-1)!
                la      $at, factorial
                jalr    $at, $ra

                add     $s0, $v0, $zero     ! use s0 to store (n-1)!
                add     $v0, $zero, $zero   ! use v0 as sum register
        mul:    beq     $s1, $zero, done    ! use s1 as counter (from n to 0)
                add     $v0, $v0, $s0
                addi    $s1, $s1, -1
                beq     $zero, $zero, mul

    done:       lw      $s1, 0($sp)     ! pop s1
                addi    $sp, $sp, 1
                lw      $s0, 0($sp)     ! pop s0
                addi    $sp, $sp, 1
                lw      $a0, 0($sp)     ! pop a0
                addi    $sp, $sp, 1
                lw      $ra, 0($sp)     ! pop RA
                addi    $sp, $sp, 1
                jalr    $ra, $zero

ti_inthandler:
                sw      $k0, -1($sp)    ! Save $k0
                addi    $sp, $sp, -1    ! Move stack pointer to the top of stack

                ei                      ! Enable interrupt

                sw      $ra, -1($sp)    ! Save processor registers
                sw      $v0, -2($sp) 
                sw      $a0, -3($sp) 
                sw      $a1, -4($sp) 
                sw      $a2, -5($sp) 
                sw      $a3, -6($sp) 
                sw      $a4, -7($sp) 
                sw      $s0, -8($sp) 
                sw      $s1, -9($sp) 
                sw      $s2, -10($sp) 
                sw      $s3, -11($sp) 
                sw      $fp, -12($sp)  
                sw      $at, -13($sp)
				addi	$sp, $sp, -13	! Point stack pointer at the top of stack

                ! Execute Device code

                la $s0, seconds ! load seconds address into $s0
                lw $s0, 0($s0)  ! load seconds value into $s2
				lw $s2, 0($s0)
                addi $s2, $s2, 1 ! $s2 = second+1
                addi $s1, $zero, 60    ! $s1=60
				
				bonr $s1, $s2, $s1	!$s1=second+1-60
                
				beq $zero, $s1, inc_min ! if seconds+1 == 60, jump to inc_min
                sw  $s2, 0($s0) ! put second+1 into $s0 as the new second
                beq $zero, $zero, restore   ! jump to restore as done 

inc_min:        sw  $zero, 0($s0)   ! put zero into seconds
                la $s0, minutes ! load minutes address into $s0
                lw $s0, 0($s0)  ! load minutes value into $s2
				lw $s2, 0($s0)
                addi $s2, $s2, 1 ! $s2 = minutes+1
                addi $s1, $zero, 60    ! $s1=60
				
				bonr $s1, $s2, $s1	!$s1=second+1-60
				
                beq $zero, $s1, inc_hour ! if minutes+1 == 60, jump to inc_hour
                sw  $s2, 0($s0) ! put minutes+1 into $s0 as the new minutes
                beq $zero, $zero, restore   ! jump to restore as done 

inc_hour:       sw  $zero, 0($s0)   ! put zero into minutes
                la $s0, hours ! load hour address into $s0
                lw $s0, 0($s0)  ! load hour value into $s2
				lw $s2, 0($s0)
                addi $s2, $s2, 1 ! $s2 = hour+1
                sw  $s2, 0($s0) ! put hour+1 into $s0 as the new hour
                beq $zero, $zero, restore   ! jump to restore as done 

restore:        addi $sp, $sp, 13
				lw $at, -13($sp)  ! restore processor registers
                lw $fp, -12($sp)
                lw $s3, -11($sp)
                lw $s2, -10($sp)
                lw $s1, -9($sp)
                lw $s0, -8($sp)
                lw $a4, -7($sp)
                lw $a3, -6($sp)
                lw $a2, -5($sp)
                lw $a1, -4($sp)
                lw $a0, -3($sp)
                lw $v0, -2($sp)
                lw $ra, -1($sp)

                ! Disable interrupt
                di

                ! restore $k0
                lw $k0, 0($sp)
                addi $sp, $sp, 1    ! restore stack pointer

                ! Return from interrupt
                reti


stack:      .fill 0xA00000
seconds:    .fill 0xFFFFFC
minutes:    .fill 0xFFFFFD
hours:      .fill 0xFFFFFE
