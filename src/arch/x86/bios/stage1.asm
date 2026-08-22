[ORG 0x7C00]
[BITS 16]

jmp short _start
nop

stage2Addr equ 0x8000
stack equ 0x7C00

headCount equ (stack - 6)
secPerTrk equ (stack - 4)
drvNum equ (stack -2)

_start:
	cli
	mov ax, 0x00
	mov ds, ax
	mov ss, ax
	mov fs, ax
	mov es, ax
	mov sp, stack
	mov bp, sp
	sti

	movzx ax, dl
	push ax

	; Get (C)HS geometry
	mov ah, 0x08
	mov dl, [drvNum]
	int 0x13
	jc err_geometry

	and cx, 0x3F ; Sectors per Track
	push cx

	movzx ax, dh ; dh = max head
	inc ax	
	push ax

	call load_stage2

	mov dx, [drvNum]
	mov sp, stage2Addr
	jmp 0x0000:stage2Addr

load_stage2:
	push bp
	mov bp, sp

	sub sp, 4
	mov word [bp-2], stage2LBAs
	mov word [bp-4], stage2Addr
.read_loop:
	mov si, [bp-2]
	mov ax, [si]
	or  ax, [si+2]
	jz  .end

	movzx cx, byte [secsPerLBA]

	push word cx                ; seccount
	push word 0                 ; buffer seg
	push word [bp-4]            ; buffer off
	push word [si]              ; lba low
	push word [si+2]            ; lba high
	call disk_read
	add  sp, 10

	cmp ax, 1
	je err_read

	add word [bp-2], 4
	movzx cx, byte [secsPerLBA]
	shl cx, 9
	add word [bp-4], cx
	jmp .read_loop
.end:
	add sp, 4
	leave
	ret

err_read:
	call error
	db "Error reading disk!", 0x0A, 0x0D, 0

err_geometry:
	call error
	db "Error getting disk geometry!", 0x0A, 0x0D, 0

; int disk_read(u32 lba, seg:off buffer, u16 seccount)
disk_read:
	push bp
	mov bp, sp

	mov ax, [bp+6]       ; lba low
	mov dx, [bp+4]       ; lba high
	div word [secPerTrk] ; ax = lba / secPerTrk, dx = lba % secPerTrk

	; Depends on secPerTrk being less than 256.
	mov cl, dl           ; cl = lba % secPerTrk
	inc cl               ; cl = sector (1-based) -> bits 0-5 de CL

	xor dx, dx
	div word [headCount] ; ax = cylinder (lba / secPerTrk / headCount)
						 ; dx = head     (lba / secPerTrk % headCount)

	mov dh, dl           ; DH = Head
	mov ch, al           ; CH = 8 low bits of cylinder

	mov dl, ah
	and dl, 00000011b
	shl dl, 6
	
	and cl, 0x3F
	or cl, dl

	mov al, [bp+12]      ; seccount
	mov bx, [bp+10]      ; buffer seg
	mov es, bx
	mov bx, [bp+8]       ; buffer off
	mov dl, [drvNum]     ; drive number (ex: 0x80)
	mov ah, 0x02         ; read sectors function
	int 0x13

	sbb ax, ax
	neg ax

	leave
	ret

error:
	pop word si
	mov ah, 0x0E
.next_char:
	lodsb
	or al, al
	jz .done
	int 0x10
	jmp .next_char
.done:
	int 0x18

halt:
	hlt
	jmp halt

align 4

; Stage2 Header signature
sig: db 'CaFeBaby'
secsPerLBA: db 0
stage2LBAs: dd 0

times 0x1B8-($ - $$) db 0

; Signature 4 bytes
db "CaFe"

times 510-($ - $$) db 0
dw 0xAA55