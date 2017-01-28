.data
count_prompt: .asciiz "Array size? "
item_prompt:  .asciiz "Enter number: "
xs_output: .asciiz "xs =  "
ys_output: .asciiz "\nys = "

# Note: This program breaks on n = 0

.text
main:
	# x is $t0
	# ii is $t1
	# n is $t2
	# tmp is $t3
	# 4 is $t4 (size of word)
	# xs is $t5

        subi $sp, $sp, 28
        sw $ra, 0($sp)
        la $t0, 4($sp)
        la $t1, 8($sp)
        la $t2, 12($sp)
        la $t3, 16($sp)
        la $t4, 20($sp)
        la $t5, 24($sp)

	li $v0, 4
	la $a0, count_prompt
	syscall
		
	# read n (array size)
	li $v0, 5
	syscall
	move $t2, $v0

	# multiply size by 4 per word
	li $t4, 4
	mul $t3, $t2, $t4 
	
	# allocate n words on heap
	li $v0, 9
	move $a0, $t3
	syscall 
	
	# retain address to xs
	move $t5, $v0
	
	li $t1, 0
	
input_loop:
	
	# ask for number
	li $v0, 4
	la $a0, item_prompt
	syscall
	
	# read item
	li $v0, 5
	syscall
	move $t0, $v0
	
	# calculate address of $sp + 4 * ii
	mul $t3, $t1, $t4
	add $t3, $t5, $t3
	
	# xs[ii] = x
	sw $t0, 0($t3)
	
	# continue looping until at array size
	addi $t1, $t1, 1
	blt $t1, $t2, input_loop

	li $t1, 0
	move $a0, $t2
	move $a1, $t4
	move $a2, $t5
	
	addi $sp, $sp, 28
	
	jal sort
	
sort:
	# x is $t0
	# ii is $t1
	# n is $t2
	# tmp is $t3
	# 4 is $t4 (size of word)
	# xs is $t5
	# ys is $t6
	# jj is $t7

        subi $sp, $sp, 36
        sw $ra, 0($sp)
        la $t0, 4($sp)
        la $t1, 8($sp)
        la $t2, 12($sp)
        la $t3, 16($sp)
        la $t4, 20($sp)
        la $t5, 24($sp)
        la $t6, 28($sp)
        la $t7, 32($sp)
        
        li $t1, 0
        move $t2, $a0
        move $t4, $a1
        move $t5, $a2
        
	# multiply size by 4 per word
	mul $t3, $t2, $t4 
	
	# allocate n words on heap
	li $v0, 9
	move $a0, $t3
	syscall 
	
	# retain address to ys
	move $t6, $v0
	
sort_loop:
	# calculate address of $sp + 4 * ii
	mul $t3, $t1, $t4
	add $t3, $t5, $t3
	
	# load the word into the arguments for insert
	lw $a0, 0($t3)
	
	# reset jj
	li $t7, 0
	addi $t1, $t1, 1
	jal insert
	
	blt $t1, $t2, sort_loop
	
	move $a2, $t5
	move $a3, $t6
	
	addi $sp, $sp, 36
	
	jal end
	
insert: 
	# calculate address of $sp + 4 * jj
	mul $t3, $t7, $t4
	add $t3, $t6, $t3
	
	# branch if number is less than x
	lw $t0, 0($t3)
	blt $a0, $t0, insert_help
	
	
	addi $t7, $t7, 1
	blt $t7, $t1, insert
	
	# set current address to y
	sw $a0, 0($t3)
	jr $ra
	
insert_help:
	# set current address to y
	sw $a0, 0($t3)
	# set y to x
	move $a0, $t0
	
	j insert

output: 	
	# calculate address of $sp + 4 * ii
	mul $t3, $t1, $t4
	add $t3, $a1, $t3
	
	# load word for output
	li $v0, 1
	lw $a0, 0($t3)
	syscall
	
	# output space
	li $a0, 32
	li $v0, 11  
	syscall
	
	addi $t1, $t1, 1
	blt $t1, $t2, output
	
	jr $ra

end: 
	# output xs
	li $v0, 4
	la $a0, xs_output
	syscall
	
	li $t1, 0
	move $a1, $a2
	jal output
	
	# output ys
	li $v0, 4
	la $a0, ys_output
	syscall
	
	li $t1, 0
	move $a1, $a3
	jal output
	
	li $v0, 10
	syscall