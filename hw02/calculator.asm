.globl main

.data 
select:  .asciiz "Select operation (+, -, *, or /): "
first: .asciiz "\nFirst number: "
second: .asciiz "Second number: "
answer: .asciiz "answer = "

.text
main:
	# $t0 is operator string
	# $t1 is operator byte
	# $t2 is return value

        subi $sp, $sp, 16
        sw $ra, 0($sp)
        la $t0, 4($sp)
        la $t1, 8($sp)
        la $t2, 12($sp)

	la $a0, select
	li $v0, 4
	syscall

	# get the string
	move $a0, $t0
	li $a1, 2
	li $v0, 8
	syscall

	# compare chars
	move $t0, $a0
	lb $t1, 0($t0)
	beq $t1, '+', add_link
	beq $t1, '-', subtract_link
	beq $t1, '*', multiply_link
	beq $t1, '/', divide_link

end: 
	move $t2, $v0
	
	la $a0, answer
	li $v0, 4
	syscall
	
	move $a0, $t2
	li $v0, 1
	syscall
	
	lw $ra, 0($sp)
	addi $sp, $sp, 16
	
	# exit
	li $v0, 10
	syscall
	
add_link: 
	jal add_numbers
	j end
	
subtract_link: 
	jal subtract_numbers
	j end
	
multiply_link: 
	jal multiply_numbers
	j end
	
divide_link: 
	jal divide_numbers
	j end

add_numbers: 
	# $t0 is first number
	# $t1 is second number
        subi $sp, $sp, 12
        sw $ra, 0($sp)
        la $t0, 4($sp)
        la $t1, 8($sp)

        la $a0, first
        jal read_int
        move $t0, $v0
        la $a0, second
        jal read_int
        move $t1, $v0
        
        add $v0, $t0, $t1
        j return

subtract_numbers:
	# $t0 is first number
	# $t1 is second number
        subi $sp, $sp, 12
        sw $ra, 0($sp)
        la $t0, 4($sp)
        la $t1, 8($sp)
        
        la $a0, first
        jal read_int
        move $t0, $v0
        la $a0, second
        jal read_int
        move $t1, $v0
        
        sub $v0, $t0, $t1
        j return
        
multiply_numbers:
	# $t0 is first number
	# $t1 is second number
        subi $sp, $sp, 12
        sw $ra, 0($sp)
        la $t0, 4($sp)
        la $t1, 8($sp)
        
        la $a0, first
        jal read_int
        move $t0, $v0
        la $a0, second
        jal read_int
        move $t1, $v0
        
        mul $v0, $t0, $t1
        j return

divide_numbers:
	# $t0 is first number
	# $t1 is second number
        subi $sp, $sp, 12
        sw $ra, 0($sp)
        la $t0, 4($sp)
        la $t1, 8($sp)
        
        la $a0, first
        jal read_int
        move $t0, $v0
        la $a0, second
        jal read_int
        move $t1, $v0
        
        div $v0, $t0, $t1
        j return

read_int:
	li $v0, 4
	syscall
	
	li $v0, 5
	syscall
	jr $ra

return:	
	lw $ra, 0($sp)
	addi $sp, $sp, 12
	jr $ra
