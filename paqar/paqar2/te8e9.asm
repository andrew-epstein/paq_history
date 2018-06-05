	.386P
.model FLAT
PUBLIC	_te8e9
_TEXT	SEGMENT
_memheap$ = 8
_mode$ = 12
_bytesread$ = 16
_data2write$= 20
_te8e9	PROC NEAR

; 7    : {
	push	ebp
	mov	ebp, esp
	sub	esp, 64
	push	ebx
	push	esi
	push	edi
	lea	edi, DWORD PTR [ebp-64]
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
_te8e9	ENDP
_TEXT	ENDS

_DATA	SEGMENT
o equ offset
q equ qword ptr
d equ dword ptr
w equ word ptr
b equ byte ptr
s equ short

sta_:
sta0 equ o sta_-100h

cextab	dd 00000000h
c1seg	dd 00001000h
tdseg8	dd 00041200h
da2wri dd 4 dup(0)

ALIGN16 macro
	MY_ALIGN 16
	endm

MY_ALIGN macro ste
local @@curpos
@@curpos:
  xxx = o @@curpos - o sta0
  count = ((xxx+ste-1)/ste)*ste -xxx
		IF count gt 0
		db count dup (03eh)
		ENDIF
	endm

l17: mov ecx,d [bread+1]
 mov edx,[tdseg8]
 mov b [mtfb2+3],0
 	and d [da2wri],0
 	and d [da2wri+8],0

       mov ebp,ecx
       mov esi,edx
	lea edi,[esi+ebp]
        call e8e9repmovsb

	mov ecx,ebp
	lea edi,[ecx+edx]
	push edx

mtfb2:	  dec edx
	  mov b [edx],"r"
	mov ecx,d [e89tab+e16tab]
	jecxz enomake
	and d [e89tab+e16tab],0
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
 mov [da2wri+4],edx
		popad
		pop edx
		db 3ch
enomake:pop ecx
	mov ecx,edi
	sub ecx,edx
 mov [da2wri+8],ecx
 mov [da2wri+12],edx
	ret


deflat: mov edx,[tdseg8]
bread:	mov ecx,"rrrr"

	mov b [de89li],0ebh ; means: no e8e9
	mov al,[edx]
	inc edx
		mov bh,0
		neg al
		jz $+7
		call get_method
bmp7:   or d [de89n16+2],-1
	or d [de89n32+2],-1
	or bh,bh
	jz s noe8e9
		push ecx
		movzx ecx,w [edx]
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
	mov d [de89n16+2],eax
	mov d [de89n32+2],eax
	mov d [de8nxt16+1],edi
	mov d [de8nxt32+1],edi
		pop ecx
noe8e9:	mov edi,[tdseg8]
     mov eax,edx
     sub eax,edi
     sub ecx,eax

       mov ebp,ecx
       mov esi,edx
	call de8e9repmovsb
 mov [da2wri],ecx
 mov [da2wri+4],edx
get_ret:ret

get_method:cmp al,0f0h
	jnb s get_ret
		mov b [de89li],076h ; means: jbe
	neg al
	sub al,17+6
		jb s get16e8
	add al,2
	shr al,1
	jnb $+4
	mov bh,1
		shr al,1
		mov ah,03ch
		jnb $+4
		mov ah,089h
	mov b [de8d32],ah
		shr al,1
		mov ah,03ch
		jnb $+4
		mov ah,089h
	mov b [de9d32],ah
		mov b [de16o32],03ch ; means: 32
		ret
get16e8:add al,2+6
	shr al,1
	jnb $+4
	mov bh,1
		shr al,1
		mov ah,03ch
		jnb $+4
		mov ah,089h
	mov b [de8d16+1],ah
		shr al,1
		mov ah,03ch
		jnb $+4
		mov ah,089h
	mov b [de9d16+1],ah
		mov b [de16o32],0ebh ; means: 16
		ret


;---------------------------------------
fnqli16:shl edi,8
	mov al,[esi-1]
	or edi,eax
	lea ebx,[esi-2]
	sub ebx,edx
		add edi,ebx
		cmp di,bp
		jb s fnq16x
	add ebx,ebp
	sub di,bx
	jnb s fnq16r
	sub ebx,ebp
fnq16x:		inc d [e89tab+e16tab]
		mov edi,d [e89tab+e16tab+8]
	mov [edi],ebx
	mov [edi+4],al
		add edi,16
		mov d [e89tab+e16tab+8],edi
fnq16r:		ret

fnqli32:push d [esi]
	mov [esi],edi
		test ebx,80h
		jnz s fnq32m3
		test ebx,8000h
		jnz s fnq32m2
	lea ebx,[esi-2]
	jmp s fnq32l4
fnq32m2:lea ebx,[esi-3]
	jmp s fnq32l4
fnq32m3:	lea ebx,[esi-4]
fnq32l4:mov edi,[ebx+1]
	sub ebx,edx
		add edi,ebx
		cmp edi,ebp
		jb s fnq32x
	add ebx,ebp
	sub edi,ebx
	jnb s fnq32r
	sub ebx,ebp
fnq32x:		inc d [e89tab+e32tab]
		mov edi,d [e89tab+e32tab+8]
	mov al,[esi-1]
	mov [edi],ebx
	mov [edi+4],al
		add edi,16
		mov d [e89tab+e32tab+8],edi
fnq32r:	pop d [esi]
	ret
			ALIGN16
addesi_subecx macro n
	add esi,n
	sub ecx,n
	endm
getCebx macro
	lea ebx,[esi-1]
	sub ebx,edx
	endm

e89tab dd 16 dup(0)
ta16n equ 00*4
ta16u equ 02*4
ta16c equ 04*4
ta32c equ 06*4
ta32n equ 08*4
ta32u equ 10*4
e16tab equ 12*4
e32tab equ 13*4

e8t164:	inc d [e89tab+ta16u+eax*4]
	inc eax
	mov [ebx+edi],al
	dec eax
e8t160:		mov bl,b [esi-1+ebp+4-1]
		test bl,40h
		jz s $+7
	call fnqli16
		addesi_subecx 2
		jnbe s e8t161
e8t16r:	ret
e8t16m:		or b [esi-1+ebp+4],40h
e8t161:	dec ecx
	jz s e8t16r
e8e9t16:	lodsb
		sub al,0e8h
		cmp al,2
		jnb s e8t161
e8t16g:	movzx edi,w [esi]
	getCebx
e8t16h:		add di,bx
		cmp di,bp
e8t16f:		jb s e8t163	;if 0<= A+C <N
	add ebx,ebp
	sub di,bx
	jnb s e8t16m		;if N<= A+C <N+C
	inc d [e89tab+ta16c+eax*4]
	jmp s e8t160
e8t162:		cmp edi,ebp
		jb s e8t163
	cmp edi,10000h
	jb s e8t163
	sub edi,10000h
e8t163:		lea ebx,[edx-20h+ebp+24h]
	test b [ebx+edi],3
	jz s e8t164
	test b [ebx+edi],4
	jnz s e8t165
		test b [ebx+edi],1
		mov b [ebx+edi],7
		jnz s e8t166
	dec d [e89tab+ta16u+04]
	inc d [e89tab+ta16n+04]
	jmp s e8t165
e8t166:		dec d [e89tab+ta16u]
		inc d [e89tab+ta16n]
e8t165:	inc d [e89tab+ta16n+eax*4]
	jmp   e8t160
;-----------------------
e8t324:	inc d [e89tab+ta32u+eax*4]
	inc eax
	shl eax,3
	mov [ebx+edi],al
e8t320:		mov ebx,[esi-1+ebp+4-3]
		test ebx,808080h
		jz s $+7
	call fnqli32
		addesi_subecx 4
		jnbe s e8t321
e8t32r:	ret
e8t32m:		or b [esi-1+ebp+4],80h
e8t321:	dec ecx
	jz s e8t32r
e8e9t32:	lodsb
		sub al,0e8h
		cmp al,2
		jnb s e8t321
	mov edi,[esi]
	getCebx
		add edi,ebx
		cmp edi,ebp
		jb s e8t323	;if 0<= A+C <N
	add ebx,ebp
	sub edi,ebx
	jnb s e8t32m		;if N<= A+C <N+C
	inc d [e89tab+ta32c+eax*4]
	jmp s e8t320



e8t323:		lea ebx,[edx-20h+ebp+24h]
	test b [ebx+edi],24
	jz s e8t324
	test b [ebx+edi],32
	jnz s e8t325
		test b [ebx+edi],8
		mov b [ebx+edi],56
		jnz s e8t326
	dec d [e89tab+ta32u+04]
	inc d [e89tab+ta32n+04]
	jmp s e8t325
e8t326:		dec d [e89tab+ta32u]
		inc d [e89tab+ta32n]
e8t325:	inc d [e89tab+ta32n+eax*4]
	jmp   e8t320
;-----------------------
e8d161:	dec ecx
	jz s e8d16r
e8e9d16:	lodsb
		sub al,0e8h
		cmp al,2
		jnb s e8d161
	mov di,[esi]
	getCebx
		add edi,ebx
		cmp di,bp
e8d16f:		jb s e8d163	;if 0<= A+C <N
	add ebx,ebp
	sub di,bx
	jnb s e8d161
e8d163:		or al,al ;cmp al,0e8h
		jz s e8d16
e9d16:	mov [esi],di
	db 3dh,3dh
e8d16:	mov [esi],di
	addesi_subecx 2
	jnbe s e8d161
e8d16r:		ret
de8d161:dec ecx
	jz s de8d16r
de8e9d16:	lodsb
		sub al,0e8h
		cmp al,2
		jnb s de8d161
	mov di,[esi]
	getCebx
	cmp di,bp
de8d16f:jb s de89r16
		add edi,ebx
		cmp di,bx
		jnb s de8d161
	add edi,ebp
de89r16:sub edi,ebx
de89n16:cmp ebx,"rrrr"
	jz de8nxt16
		or al,al ;cmp al,0e8h
		jz s de8d16
de9d16:	mov [esi],di
	db 3dh,3dh
de8d16:	mov [esi],di
	addesi_subecx 2
	jnbe s de8d161
de8d16r:	ret
de8nxt16:mov eax,d ds:["rrrr"]
	add d [$-4],3
	and eax,0ffffffh
	add eax,ebx
	mov d [de89n16+2],eax
	jmp s de8d161
;-----------------------
e8d321:	dec ecx
	jz s e8d32r
e8e9d32:	lodsb
		sub al,0e8h
		cmp al,2
		jnb s e8d321
	mov edi,[esi]
	getCebx
		add edi,ebx
		cmp edi,ebp
		jb s e8d323	;if 0<= A+C <N
	add ebx,ebp
	sub edi,ebx
	jnb s e8d321
e8d323:		or al,al ;cmp al,0e8h
		jz s e8d32
e9d32:	mov [esi],edi
	db 3dh,3dh,3dh
e8d32:	mov [esi],edi
	addesi_subecx 4
	jnbe s e8d321
e8d32r:		ret
de8d321:dec ecx
	jz s de8d32r
de8e9d32:	lodsb
		sub al,0e8h
		cmp al,2
		jnb s de8d321
	mov edi,[esi]
	getCebx
	cmp edi,ebp
	jb s de89r32
		add edi,ebx
		cmp edi,ebx
		jnb s de8d321
	add edi,ebp
de89r32:sub edi,ebx
de89n32:cmp ebx,"rrrr"
	jz s de8nxt32
		or al,al ;cmp al,0e8h
		jz s de8d32
de9d32:	mov [esi],edi
	db 3dh,3dh,3dh
de8d32:	mov [esi],edi
	addesi_subecx 4
	jnbe s de8d321
de8d32r:	ret
de8nxt32:mov eax,d ds:["rrrr"]
	add d [$-4],3
	and eax,0ffffffh
	add eax,ebx
	mov d [de89n32+2],eax
	jmp s de8d321
;---------------------------------------
de8e9repmovsb:pushad
		add esi,20h
		sub ecx,24h
de89li:		jbe s de8e9r
	mov edx,esi
	mov ebp,ecx
de16o32:jmp $+9
		call de8e9d32
		jmp s de8e9r
	mov ecx,10000h
	sub ecx,ebp
	jbe s de8m1
		cmp ecx,ebp
		jb $+4
		mov ecx,ebp
		mov b [de8d16f],072h ; means: jb
		call de8e9d16
de8m1:	lea ecx,[edx+ebp]
	sub ecx,esi
	jbe $+7+7
		mov b [de8d16f],0ebh ; means: jmp s
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
		mov edi,o e89tab
		push edi
		mov cl,14
		rep stosd
	mov [edi],ebx
	add ebx,8
	mov [edi+4],ebx
	mov ecx,10000h
	sub ecx,ebp
	jbe s e8l1
		cmp ecx,ebp
		jb $+4
		mov ecx,ebp
		mov b [e8t16f],072h ; means: jb
		call e8e9t16
e8l1:	mov b [e8t16f],0ebh ; means: jmp s
		cmp ebp,8000h
		jb s e8l2
	lea ecx,[edx+8000h]
	sub ecx,esi
	jbe $+7
		call e8e9t16
	lea ecx,[edx+ebp]
	sub ecx,esi
	jbe s e8l2
		sub b [e8t16f+1],o e8t163-o e8t162
		mov b [e8t16g+1],0bfh ; means: movsx
		mov b [e8t16h],03eh ; means:
		call e8e9t16
		add b [e8t16f+1],o e8t163-o e8t162
		mov b [e8t16g+1],0b7h ; means: movzx
		mov b [e8t16h],066h ; means:
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
		jbe s e8do32
;-------
	mov ebx,[edi+ta16n+00]
	sub ebx,[edi+ta16c+00]
	jnb $+4
	xor ebx,ebx
	lea ebx,[ebx*4+ebx]	;5/8
	shr ebx,3
	cmp ebx,[edi+ta16u+00]
	jbe s e8u16
		mov ah,089h ; means: mov [esi],edi
e8u16:  mov ebx,[edi+ta16n+04]
	sub ebx,[edi+ta16c+04]
	jnb $+4
	xor ebx,ebx
	lea ebx,[ebx*4+ebx]	;5/8
	shr ebx,3
	cmp ebx,[edi+ta16u+04]
	jbe s e9u16
		mov al,089h ; means:
e9u16:	cmp ax,3c3ch
	jz s e8e9r
	call e8alah16
	;mov ecx,10000h
	sub ecx,ebp
	jbe s e8m1
		cmp ecx,ebp
		jb $+4
		mov ecx,ebp
		mov b [e8d16f],072h ; means: jb
		call e8e9d16
e8m1:	lea ecx,[edx+ebp]
	sub ecx,esi
	jbe $+7+7
		mov b [e8d16f],0ebh ; means: jmp s
		call e8e9d16
	mov ecx,d [e89tab+e16tab]
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
	jbe s e8u32
		mov ah,089h ; means:
e8u32:  mov ebx,[edi+ta32n+04]
	sub ebx,[edi+ta32c+04]
	jnb $+4
	xor ebx,ebx
	shr ebx,1		;4/8
	cmp ebx,[edi+ta32u+04]
	jbe s e9u32
		mov al,089h ; means:
e9u32:	cmp ax,3c3ch
	jz s e8e9r
	mov b [e8d32],ah
	mov b [e9d32],al
	mov cl,23-2
	call e8alah
		mov ecx,ebp
		call e8e9d32
	mov ecx,d [e89tab+e32tab]
	mov d [e89tab+e16tab],ecx
	jecxz e8e9r
		call e8make32
		jmp s e8e9r

;-------
e8alah16:mov b [e8d16+1],ah
	 mov b [e9d16+1],al
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
	mov b [mtfb2+3],al
		ret

e8make32:mov esi,d [e89tab+e32tab+8]
	mov dh,b [e8d32]
	mov dl,b [e9d32]
		jmp s e8m32

e8make16:mov esi,d [e89tab+e16tab+8]
	mov dh,b [e8d16+1]
	mov dl,b [e9d16+1]
e8m32:		mov edi,[c1seg]
		add edi,10008h
		xor ebp,ebp
	mov d [e89tab+e16tab],ebp
		lea eax,[ecx*8]
		add eax,eax
		sub esi,eax
e8m16:	lodsd
	mov bl,[esi]
	sub bl,0e8h
		cmp dh,03ch	;e8 disabled?
		jz s e8m16b
		cmp dl,03ch	;e9 disabled?
		jnz s e8m16y
	dec ebx
e8m16b:	or bl,bl
	jz s e8m16z
e8m16y:		sub eax,ebp
		add ebp,eax
	inc d [e89tab+e16tab]
	stosd
	dec edi
e8m16z:		add esi,12
	dec ecx
	jnz s e8m16
		ret

run_e89:mov	eax, d _bytesread$[ebp]
	mov	d [bread+1],eax
	mov eax, d _memheap$[ebp]
	mov	d [cextab+0],eax
	add eax,01000h
	mov	d [cextab+4],eax
	add eax,40200h
	mov	d [cextab+8],eax
 push ebp
 mov eax,d _mode$[ebp]
 test al,4
 jnz s deca
	call l17
	jmp s $+7

deca: call deflat
 pop ebp
 mov esi,o da2wri
 mov edi,d _data2write$[ebp]
 movsd
 movsd
 movsd
 movsd
 ret

_DATA	ENDS

END
