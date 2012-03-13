
__asm void pre_mem_request (void) {

   IMPORT k_release_processor;  the a_c_routine is a regular c function defined somewhere else
   PUSH {r4-r11,lr} ; push registers onto the stack, lr is pushed for popping to pc later
   BL k_release_processor ;  go to the c routine to do some work
   POP {r4-r11, pc}; pop regisers off the stack

}

// pop off exception stack frame from the stack
__asm void __rte(void)
{
    PRESERVE8
	MVN  LR, #:NOT:0xFFFFFFF9  ; set EXC_RETURN value, Thread mode, MSP
	BX   LR
}

// pop off exception stack frame from the stack
__asm void __sys_rte(void)
{
    PRESERVE8
	MVN  LR, #:NOT:0xFFFFFFF1  ; set EXC_RETURN value, Thread mode, MSP
	BX   LR
}

// NOTE: assuming MSP is used
//       ideally, PSP should be used 
__asm void SVC_Handler (void) 
{
	PRESERVE8			; 8 bytes alignement of the stack
;MRS: Move the contents of a special register to a general-purpose register.
	MRS  R0, MSP		; Read MSP

;LDR Rt, [Rn {, #offset ; immediate offset  Rt is the register to load.		   
;Rn is the register on which the memory address is based.  offset is an offset from Rn. If offset is omitted, the address is the contents of Rn.
	LDR  R1, [R0, #24]	; Read Saved PC from SP
	                    ; Loads R1 from a word 24 bytes  above the address in R0
						; Note that R0 now contains the the SP value after the
						; exception stack frame is pushed onto the stack.
						 
	LDRH R1, [R1, #-2]  ; Load halfword because SVC number is encoded there
	BICS R1, R1, #0xFF00; Extract SVC Number and save it in R1.  
	                    ; R1 <= R1 & ~(0xFF00), update flags
	                 
	BNE  SVC_EXIT       ; if SVC Number !=0, exit

;LDM(IA) instructions load the registers in reglist with word values from memory addresses based on Rn.
;IA: Increment address After each access. This is the default.
	LDM  R0, {R0-R3, R12}; Read R0-R3, R12 from stack. NOTE R0 contains the sp before this instruction
;  From the manual
;The BL and BLX instructions write the address of the next instruction to LR (the link register, R14).
;The Link Register (LR) is register R14. It stores the return information for subroutines,function calls, and exceptions.
	BLX  R12             ; Call SVC Function, 
	                     ; R12 contains the corresponding 
						 ; C kernel functions entry point
						 ; R0-R3 contains the kernel function input params according to AAPCS
						;  The BX and BLX Rm instructions cause a UsageFault exception if bit[0] of Rm is 0.
		
;MRS Rd, spec_reg  Rd is the destination register.  spec_reg can be any of: APSR, IPSR, EPSR, IEPSR, IAPSR, EAPSR, PSR, MSP, PSP, PRIMASK, BASEPRI, BASEPRI_MAX, FAULTMASK, or CONTROL.				 
	MRS  R12, MSP        ; Read MSP
	; Store R0 into the address of R12
	STR  R0, [R12]       ; store C kernel function return value in R0
	                     ; to R0 on the exception stack frame  
SVC_EXIT
	;The "MVN(S)(Cond) Rd, Operand2" instruction takes the value of Operand2, performs a bitwise logical NOT operation	on the value, and places the result into Rd.	
	MVN  LR, #:NOT:0xFFFFFFF9  ; set EXC_RETURN value, Thread mode, MSP
	;BX is branch indirect (register).
	BX   LR
}
