	.text
	.global updateState
updateState:
	cmp #0 , r12
	jnz over
	call #onPlayingState
	jmp end
over:	call #onGameoverState
end:	 pop r0
