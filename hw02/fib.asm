.globl main

.text
main:
	li $v0, 5 	# read int
	syscall
	move $a0, $v0
	
	jal fib
	move $a0, $v0
	
	li $v0, 1	# print int
	syscall
	
	li $v0, 10	# exit(0)
	syscall


fib:
	# fib(x):
	#    if x < 2:
	#       return x
	#    else:
	#	return fib(x-1) + fib(x-2)
	# $t0 is x
	# $t1 is fib(x-1)
	# $t2 is fib(x-2)	

	subi $sp, $sp, 16
	sw $ra, 0($sp)
	sw $t0, 4($sp)
	sw $t1, 8($sp)
	sw $t2, 12($sp)
	
	move $t0, $a0
	bgt $t0, 1, fib_else
	
	# x == 0 or 1
	move $v0, $t0
	j fib_return
	
fib_else:
	# x > 1
	subi $a0, $t0, 1
	jal fib
	move $t1, $v0
	
	subi $a0, $t0, 2
	jal fib
	move $t2, $v0
	
	add $v0, $t1, $t2	

fib_return:
	lw $t2, 12($sp)
	lw $t1, 8($sp)
	lw $t0, 4($sp)
	lw $ra, 0($sp)
	addi $sp, $sp, 16
	jr $ra