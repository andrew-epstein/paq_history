	CPU	386
GLOBAL	_te8e9
GLOBAL	te8e9
SECTION	.text
_memheap$	EQU	8
_mode$		EQU	12
_bytesread$	EQU	16
_data2write$	EQU	20
_te8e9:
te8e9:

	push	ebp
	mov	ebp, esp
	sub	esp, 64
	push	ebx
	push	esi
	push	edi
	lea	edi, [ebp-64]
	mov	ecx, 16
	mov	eax, 0ccccccccH
	rep stosd

 call run_e89

	pop	edi
	pop	esi
	pop	ebx
	mov	esp, ebp
	pop	ebp
	ret

SECTION	.data

sta_:
%DEFINE	sta0	sta_-100h

cextab	dd 00000000h
c1seg	dd 00001000h
tdseg8	dd 00041200h
da2wri times	4	dd 0

l17: mov ecx,dword [bread+1]
 mov edx,[tdseg8]
 mov byte [mtfb2+3],0
 	and dword [da2wri],0
 	and dword [da2wri+8],0

       mov ebp,ecx
       mov esi,edx
	lea edi,[esi+ebp]
	call e8e9repmovsb

	mov ecx,ebp
	lea edi,[ecx+edx]
	push edx

mtfb2:	  dec edx
	  mov byte [edx],"r"
	mov ecx,dword [e89tab+e16tab]
	jecxz enomake
	and dword [e89tab+e16tab],0
	cmp byte [edx],0
	jz short enomake
		pushad
	mov bl,[edx]
	inc ebx
		mov edx,[c1seg]
		lea edx,[edx+10006h]
		mov [edx],cx
		lea ecx,[ecx*2+ecx+3]
	dec edx
	mov [edx],bl
 mov [da2wri],ecx
 mov DWORD [da2wri+4],edx
		popad
		pop edx
		db 3ch
enomake:pop ecx
	mov ecx,edi
	sub ecx,edx
 mov DWORD [da2wri+8],ecx
 mov DWORD [da2wri+12],edx
	ret


deflat: mov edx,[tdseg8]
bread:	mov ecx,"rrrr"

	mov byte [de89li],0ebh ; means: no e8e9
	mov al,[edx]
	inc edx
		mov bh,0
		neg al
		jz $+7
		call get_method
bmp7:   or dword [de89n16+2],-1
	or dword [de89n32+2],-1
	or bh,bh
	jz short noe8e9
		push ecx
		movzx ecx,word [edx]
		lea ecx,[ecx*2+ecx]
		inc edx
		inc edx
	mov esi,edx
	mov edi,[c1seg]
	add edi,010000h
		add edx,ecx
	push edi
	rep movsb
	mov [edi],ecx
	pop edi
		mov eax,[edi]
		and eax,0ffffffh
		add edi,3
	mov dword [de89n16+2],eax
	mov dword [de89n32+2],eax
	mov dword [de8nxt16+1],edi
	mov dword [de8nxt32+1],edi
		pop ecx
noe8e9:	mov edi,[tdseg8]
     mov eax,edx
     sub eax,edi
     sub ecx,eax

       mov ebp,ecx
       mov esi,edx
	call de8e9repmovsb
 mov [da2wri],ecx
 mov DWORD [da2wri+4],edx
get_ret:ret

get_method:cmp al,0f0h
	jnb short get_ret
		mov byte [de89li],076h ; means: jbe
	neg al
	sub al,17+6
		jb short get16e8
	add al,2
	shr al,1
	jnb $+4
	mov bh,1
		shr al,1
		mov ah,03ch
		jnb $+4
		mov ah,089h
	mov byte [de8d32],ah
		shr al,1
		mov ah,03ch
		jnb $+4
		mov ah,089h
	mov byte [de9d32],ah
		mov byte [de16o32],03ch ; means: 32
		ret
get16e8:add al,2+6
	shr al,1
	jnb $+4
	mov bh,1
		shr al,1
		mov ah,03ch
		jnb $+4
		mov ah,089h
	mov byte [de8d16+1],ah
		shr al,1
		mov ah,03ch
		jnb $+4
		mov ah,089h
	mov byte [de9d16+1],ah
		mov byte [de16o32],0ebh ; means: 16
		ret


;---------------------------------------
fnqli16:shl edi,8
	mov al,[esi-1]
	or edi,eax
	lea ebx,[esi-2]
	sub ebx,edx
		add edi,ebx
		cmp di,bp
		jb short fnq16x
	add ebx,ebp
	sub di,bx
	jnb short fnq16r
	sub ebx,ebp
fnq16x:		inc dword [e89tab+e16tab]
		mov edi,dword [e89tab+e16tab+8]
	mov [edi],ebx
	mov [edi+4],al
		add edi,16
		mov dword [e89tab+e16tab+8],edi
fnq16r:		ret

fnqli32:push dword [esi]
	mov [esi],edi
		test ebx,80h
		jnz short fnq32m3
		test ebx,8000h
		jnz short fnq32m2
	lea ebx,[esi-2]
	jmp short fnq32l4
fnq32m2:lea ebx,[esi-3]
	jmp short fnq32l4
fnq32m3:	lea ebx,[esi-4]
fnq32l4:mov edi,[ebx+1]
	sub ebx,edx
		add edi,ebx
		cmp edi,ebp
		jb short fnq32x
	add ebx,ebp
	sub edi,ebx
	jnb short fnq32r
	sub ebx,ebp
fnq32x:		inc dword [e89tab+e32tab]
		mov edi,dword [e89tab+e32tab+8]
	mov al,[esi-1]
	mov [edi],ebx
	mov [edi+4],al
		add edi,16
		mov dword [e89tab+e32tab+8],edi
fnq32r:	pop dword [esi]
	ret
			ALIGN 16,db 3Eh
%MACRO	addesi_subecx	1-*
%DEFINE	%%n	%1
	add esi,%%n
	sub ecx,%%n
	%ENDMACRO
%MACRO	getCebx	0
	lea ebx,[esi-1]
	sub ebx,edx
	%ENDMACRO

e89tab times	16	dd 0
ta16n equ 00*4
ta16u equ 02*4
ta16c equ 04*4
ta32c equ 06*4
ta32n equ 08*4
ta32u equ 10*4
e16tab equ 12*4
e32tab equ 13*4

e8t164:	inc dword [e89tab+ta16u+eax*4]
	inc eax
	mov [ebx+edi],al
	dec eax
e8t160:		mov bl,byte [esi-1+ebp+4-1]
		test bl,40h
		jz short $+7
	call fnqli16
		addesi_subecx 2
		jnbe short e8t161
e8t16r:	ret
e8t16m:		or byte [esi-1+ebp+4],40h
e8t161:	dec ecx
	jz short e8t16r
e8e9t16:	lodsb
		sub al,0e8h
		cmp al,2
		jnb short e8t161
e8t16g:	movzx edi,word [esi]
	getCebx
e8t16h:		add di,bx
		cmp di,bp
e8t16f:		jb short e8t163	;if 0<= A+C <N
	add ebx,ebp
	sub di,bx
	jnb short e8t16m		;if N<= A+C <N+C
	inc dword [e89tab+ta16c+eax*4]
	jmp short e8t160
e8t162:		cmp edi,ebp
		jb short e8t163
	cmp edi,10000h
	jb short e8t163
	sub edi,10000h
e8t163:		lea ebx,[edx-20h+ebp+24h]
	test byte [ebx+edi],3
	jz short e8t164
	test byte [ebx+edi],4
	jnz short e8t165
		test byte [ebx+edi],1
		mov byte [ebx+edi],7
		jnz short e8t166
	dec dword [e89tab+ta16u+04]
	inc dword [e89tab+ta16n+04]
	jmp short e8t165
e8t166:		dec dword [e89tab+ta16u]
		inc dword [e89tab+ta16n]
e8t165:	inc dword [e89tab+ta16n+eax*4]
	jmp   e8t160
;-----------------------
e8t324:	inc dword [e89tab+ta32u+eax*4]
	inc eax
	shl eax,3
	mov [ebx+edi],al
e8t320:		mov ebx,[esi-1+ebp+4-3]
		test ebx,808080h
		jz short $+7
	call fnqli32
		addesi_subecx 4
		jnbe short e8t321
e8t32r:	ret
e8t32m:		or byte [esi-1+ebp+4],80h
e8t321:	dec ecx
	jz short e8t32r
e8e9t32:	lodsb
		sub al,0e8h
		cmp al,2
		jnb short e8t321
	mov edi,[esi]
	getCebx
		add edi,ebx
		cmp edi,ebp
		jb short e8t323	;if 0<= A+C <N
	add ebx,ebp
	sub edi,ebx
	jnb short e8t32m		;if N<= A+C <N+C
	inc dword [e89tab+ta32c+eax*4]
	jmp short e8t320



e8t323:		lea ebx,[edx-20h+ebp+24h]
	test byte [ebx+edi],24
	jz short e8t324
	test byte [ebx+edi],32
	jnz short e8t325
		test byte [ebx+edi],8
		mov byte [ebx+edi],56
		jnz short e8t326
	dec dword [e89tab+ta32u+04]
	inc dword [e89tab+ta32n+04]
	jmp short e8t325
e8t326:		dec dword [e89tab+ta32u]
		inc dword [e89tab+ta32n]
e8t325:	inc dword [e89tab+ta32n+eax*4]
	jmp   e8t320
;-----------------------
e8d161:	dec ecx
	jz short e8d16r
e8e9d16:	lodsb
		sub al,0e8h
		cmp al,2
		jnb short e8d161
	mov di,[esi]
	getCebx
		add edi,ebx
		cmp di,bp
e8d16f:		jb short e8d163	;if 0<= A+C <N
	add ebx,ebp
	sub di,bx
	jnb short e8d161
e8d163:		or al,al ;cmp al,0e8h
		jz short e8d16
e9d16:	mov [esi],di
	db 3dh,3dh
e8d16:	mov [esi],di
	addesi_subecx 2
	jnbe short e8d161
e8d16r:		ret
de8d161:dec ecx
	jz short de8d16r
de8e9d16:	lodsb
		sub al,0e8h
		cmp al,2
		jnb short de8d161
	mov di,[esi]
	getCebx
	cmp di,bp
de8d16f:jb short de89r16
		add edi,ebx
		cmp di,bx
		jnb short de8d161
	add edi,ebp
de89r16:sub edi,ebx
de89n16:cmp ebx,"rrrr"
	jz de8nxt16
		or al,al ;cmp al,0e8h
		jz short de8d16
de9d16:	mov [esi],di
	db 3dh,3dh
de8d16:	mov [esi],di
	addesi_subecx 2
	jnbe short de8d161
de8d16r:	ret
de8nxt16:mov eax,["rrrr"]
	add dword [$-4],3
	and eax,0ffffffh
	add eax,ebx
	mov dword [de89n16+2],eax
	jmp short de8d161
;-----------------------
e8d321:	dec ecx
	jz short e8d32r
e8e9d32:	lodsb
		sub al,0e8h
		cmp al,2
		jnb short e8d321
	mov edi,[esi]
	getCebx
		add edi,ebx
		cmp edi,ebp
		jb short e8d323	;if 0<= A+C <N
	add ebx,ebp
	sub edi,ebx
	jnb short e8d321
e8d323:		or al,al ;cmp al,0e8h
		jz short e8d32
e9d32:	mov [esi],edi
	db 3dh,3dh,3dh
e8d32:	mov [esi],edi
	addesi_subecx 4
	jnbe short e8d321
e8d32r:		ret
de8d321:dec ecx
	jz short de8d32r
de8e9d32:	lodsb
		sub al,0e8h
		cmp al,2
		jnb short de8d321
	mov edi,[esi]
	getCebx
	cmp edi,ebp
	jb short de89r32
		add edi,ebx
		cmp edi,ebx
		jnb short de8d321
	add edi,ebp
de89r32:sub edi,ebx
de89n32:cmp ebx,"rrrr"
	jz short de8nxt32
		or al,al ;cmp al,0e8h
		jz short de8d32
de9d32:	mov [esi],edi
	db 3dh,3dh,3dh
de8d32:	mov [esi],edi
	addesi_subecx 4
	jnbe short de8d321
de8d32r:	ret
de8nxt32:mov eax,["rrrr"]
	add dword [$-4],3
	and eax,0ffffffh
	add eax,ebx
	mov dword [de89n32+2],eax
	jmp short de8d321
;---------------------------------------
de8e9repmovsb:pushad
		add esi,20h
		sub ecx,24h
de89li:		jbe short de8e9r
	mov edx,esi
	mov ebp,ecx
de16o32:jmp $+9
		call de8e9d32
		jmp short de8e9r
	mov ecx,10000h
	sub ecx,ebp
	jbe short de8m1
		cmp ecx,ebp
		jb $+4
		mov ecx,ebp
		mov byte [de8d16f],072h ; means: jb
		call de8e9d16
de8m1:	lea ecx,[edx+ebp]
	sub ecx,esi
	jbe $+7+7
		mov byte [de8d16f],0ebh ; means: jmp s
		call de8e9d16
de8e9r:	popad
	ret;jmp amrepmovsb

e8e9repmovsb:	pushad
		add esi,20h
		sub ecx,24h
		jbe   e8e9r
	mov ebp,ecx
	mov edx,esi
	xor eax,eax
		lea ecx,[ecx+24h]
		;cmp ecx,10000h
		;jnb $+7
		;mov ecx,10000h
		rep stosb
	lea ebx,[edi+3]
	and ebx,-4
		mov edi,e89tab
		push edi
		mov cl,14
		rep stosd
	mov [edi],ebx
	add ebx,8
	mov [edi+4],ebx
	mov ecx,10000h
	sub ecx,ebp
	jbe short e8l1
		cmp ecx,ebp
		jb $+4
		mov ecx,ebp
		mov byte [e8t16f],072h ; means: jb
		call e8e9t16
e8l1:	mov byte [e8t16f],0ebh ; means: jmp s
		cmp ebp,8000h
		jb short e8l2
	lea ecx,[edx+8000h]
	sub ecx,esi
	jbe $+7
		call e8e9t16
	lea ecx,[edx+ebp]
	sub ecx,esi
	jbe short e8l2
		sub byte [e8t16f+1],e8t163-e8t162
		mov byte [e8t16g+1],0bfh ; means: movsx
		mov byte [e8t16h],03eh ; means:
		call e8e9t16
		add byte [e8t16f+1],e8t163-e8t162
		mov byte [e8t16g+1],0b7h ; means: movzx
		mov byte [e8t16h],066h ; means:
e8l2:	mov esi,edx
	mov ecx,ebp
	call e8e9t32
	mov esi,edx
		pop edi
	mov eax,[edi+ta16n+00]
	add eax,[edi+ta16n+04]
	sub eax,[edi+ta16c+00]
	sub eax,[edi+ta16c+04]
	jns $+4
	xor eax,eax
	mov ebx,[edi+ta32n+00]
	add ebx,[edi+ta32n+04]
	sub ebx,[edi+ta32c+00]
	sub ebx,[edi+ta32c+04]
	jns $+4
	xor ebx,ebx
		lea ecx,[eax+ebx]
		jecxz e8e9r
	lea ebx,[ebx*2+ebx]
	shr ebx,1
	cmp eax,ebx
		mov ax,03c3ch
		jbe short e8do32
;-------
	mov ebx,[edi+ta16n+00]
	sub ebx,[edi+ta16c+00]
	jnb $+4
	xor ebx,ebx
	lea ebx,[ebx*4+ebx]	;5/8
	shr ebx,3
	cmp ebx,[edi+ta16u+00]
	jbe short e8u16
		mov ah,089h ; means: mov [esi],edi
e8u16:  mov ebx,[edi+ta16n+04]
	sub ebx,[edi+ta16c+04]
	jnb $+4
	xor ebx,ebx
	lea ebx,[ebx*4+ebx]	;5/8
	shr ebx,3
	cmp ebx,[edi+ta16u+04]
	jbe short e9u16
		mov al,089h ; means:
e9u16:	cmp ax,3c3ch
	jz short e8e9r
	call e8alah16
	;mov ecx,10000h
	sub ecx,ebp
	jbe short e8m1
		cmp ecx,ebp
		jb $+4
		mov ecx,ebp
		mov byte [e8d16f],072h ; means: jb
		call e8e9d16
e8m1:	lea ecx,[edx+ebp]
	sub ecx,esi
	jbe $+7+7
		mov byte [e8d16f],0ebh ; means: jmp s
		call e8e9d16
	mov ecx,dword [e89tab+e16tab]
	jecxz e8e9r
		call e8make16
e8e9r:	popad
	rep movsb
	ret
;-------
e8do32:	mov ebx,[edi+ta32n+00]
	sub ebx,[edi+ta32c+00]
	jnb $+4
	xor ebx,ebx
	mov ecx,ebx
	shr ecx,3
	sub ebx,ecx		;7/8
	cmp ebx,[edi+ta32u+00]
	jbe short e8u32
		mov ah,089h ; means:
e8u32:  mov ebx,[edi+ta32n+04]
	sub ebx,[edi+ta32c+04]
	jnb $+4
	xor ebx,ebx
	shr ebx,1		;4/8
	cmp ebx,[edi+ta32u+04]
	jbe short e9u32
		mov al,089h ; means:
e9u32:	cmp ax,3c3ch
	jz short e8e9r
	mov byte [e8d32],ah
	mov byte [e9d32],al
	mov cl,23-2
	call e8alah
		mov ecx,ebp
		call e8e9d32
	mov ecx,dword [e89tab+e32tab]
	mov dword [e89tab+e16tab],ecx
	jecxz e8e9r
		call e8make32
		jmp short e8e9r

;-------
e8alah16:mov byte [e8d16+1],ah
	 mov byte [e9d16+1],al
		mov cl,17-2
e8alah:	cmp al,89h	;  x  x  x
	sbb al,al	;  ^  ^  ^exceptions
	inc al		;  |  e8
	cmp ah,89h	;  e9
	sbb ah,ah
	inc ah
	add al,al
	add al,ah
	add al,al
	add al,cl
	mov byte [mtfb2+3],al
		mov ecx,10000h
		ret

e8make32:mov esi,dword [e89tab+e32tab+8]
	mov dh,byte [e8d32]
	mov dl,byte [e9d32]
		jmp short e8m32

e8make16:mov esi,dword [e89tab+e16tab+8]
	mov dh,byte [e8d16+1]
	mov dl,byte [e9d16+1]
e8m32:		mov edi,[c1seg]
		add edi,10008h
		xor ebp,ebp
	mov dword [e89tab+e16tab],ebp
		lea eax,[ecx*8]
		add eax,eax
		sub esi,eax
e8m16:	lodsd
	mov bl,[esi]
	sub bl,0e8h
		cmp dh,03ch	;e8 disabled?
		jz short e8m16b
		cmp dl,03ch	;e9 disabled?
		jnz short e8m16y
	dec ebx
e8m16b:	or bl,bl
	jz short e8m16z
e8m16y:		sub eax,ebp
		add ebp,eax
	inc dword [e89tab+e16tab]
	stosd
	dec edi
e8m16z:		add esi,12
	dec ecx
	jnz short e8m16
		ret

run_e89:mov	eax, dword [_bytesread$+ebp]
	mov	DWORD [bread+1],eax
	mov eax, dword [_memheap$+ebp]
	mov	DWORD [cextab+0],eax
	add eax,01000h
	mov	DWORD [cextab+4],eax
	add eax,40200h
	mov	DWORD [cextab+8],eax
 push ebp
 mov eax,dword [_mode$+ebp]
 test al,4
 jnz short deca
	call l17
	jmp short $+7

deca: call deflat
 pop ebp
 mov esi,da2wri
 mov edi,dword [_data2write$+ebp]
 movsd
 movsd
 movsd
 movsd
 ret
