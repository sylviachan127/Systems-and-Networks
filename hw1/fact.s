!============================================================
! CS-2200 Homework 1
! Yuen Han Chan
! Please do not change main's functionality, 
! except to change the argument for factorial or to meet your 
! calling convention
!============================================================

main:       la $sp, stack		! load address of stack label into $sp
			lw $sp, 0x00($sp)	
            la $at, factorial	! load address of factorial label into $at
            addi $a0, $zero, 5	        ! $a0 = 5, the number to factorialize
            jalr $at, $ra		! jump to factorial, set $ra to return addr
            halt			! when we return, just halt

factorial:  sw $a0, 0x00($sp) 	! put num, the first parameter, on stack
			addi $sp, $sp, -4	! allocate space for new frame
			sw	$fp, 0x01($sp)	! store old frame pointer
			addi $fp, $sp, 0	! point fp one up the store OFP, first local variable
			sw	$ra, 0x02($fp)	! store return address
			
			lw $t1, 0x04($fp)	! load num(argument) into $t1
			
			beq $t1, $zero, base	! jump to base if num == 0
			addi $a0, $a0, -1	! $a0 --
			
			jalr $at, $ra		! start recursive factorial	
			
			lw $t2, 0x04($fp)	! load num into $t2		
			add $t0, $t1, $zero	! set $t0 to have the same value as $t1(ans)
			beq $zero, $zero, loop ! jump to loop
			sw $t1, 0x03($fp)	! store mult answer to Rv
			
base:		addi $t0, $zero, 1	! $t1 = 1 since base case
			addi $t1, $zero, 1
			
return:		sw $t1, 0x03($fp)	! save return value
			lw $ra, 0x02($fp)	! restore return address
			addi $sp, $fp, 3	! make stack pointer point to return values
			sw $t1, 0x00($sp)	
			lw $fp, 0x01($fp)	! restore frame pointer
			add $v0, $t1, $zero
			jalr $ra, $zero		! jump back into halt
			
loop:		beq $t2, $zero, exit ! jump to exit if mult loop finish
			addi $t1, $zero, 0x00 ! zero out answer register

mult:		add $t1, $t0, $t1	! $t1 += fact(n-1)
			addi $t2, $t2, -1	! $t2--
			beq $t2, $zero, exit
			beq $zero, $zero, mult
			
exit:		beq $zero, $zero, return	! jump to return
			

stack:	    .word 0x4000		! the stack begins here (for example, that is)

!============================================================
!    public static long fact(int num) {
!        if (num == 0) {
!           return 1;
!        }
!        else{
!            return num*fact(num-1);
!        }
!    }
!============================================================
