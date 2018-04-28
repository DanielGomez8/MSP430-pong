	.file "updateState.c"
	.text
	.balign 2
current_state:	.word 0
	.extern updateState
	.word updateState

updateState:	
	PUSH	R15
	PUSH	R14
	PUSH	R13
	PUSH	R12
	PUSH	R11
	PUSH	R10
	PUSH	R9
	PUSH	R8
	PUSH	R7
	PUSH	R6
	PUSH	R5
	PUSH	R4
	; end of prologue
	CALL	#updateState
	; start of epilogue
	POP	R4
	POP	R5
	POP	R6
	POP	R7
	POP	R8
	POP	R9
	POP	R10
	POP	R11
	POP	R12
	POP	R13
	POP	R14
	POP	R15
	cmp #0, &current_state
	jnz over
	call onPlayingState
	jmp end
over:
	call onGameoverState
end:
	pop r0
