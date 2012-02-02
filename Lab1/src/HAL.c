/* @brief: HAL.c Hardware Abstraction Layer
 * @author: Yiqing Huang
 * @date: 2012/01/08
 */

// NOTE: assuming MSP is used
//       ideally, PSP should be used 
__asm void SVC_Handler (void) 
{
	PRESERVE8			; 8 bytes alignement of the stack
	MRS  R0, MSP		; Read MSP
	LDR  R1, [R0, #24]	; Read Saved PC from SP
	LDRH R1, [R1, #-2]  ; Load halfword
	BICS R1, R1, #0xFF00; Extract SVC Number
	BNE  SVC_EXIT       ; if SVC !=0, exit

	LDM  R0, {R0-R3, R12}; Read R0-R3, R12 from stack
	BLX  R12             ; Call SVC Function, 
	                     ; R12 contains the corresponding 
						 ; C kernel functions entry point
						 
	MRS  R12, MSP        ; Read MSP
	STR  R0, [R12]       ; store C kernel function return value in R0
	                     ; to R0 on the exception stack frame  
SVC_EXIT	
	MVN  LR, #:NOT:0xFFFFFFF9  ; set EXC_RETURN value, Thread mode, MSP
	BX   LR
}
